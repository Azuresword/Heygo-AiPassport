// main/audio/voice.c
#include "voice.h"

#include <string.h>

#include "bsp_audio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "ima_adpcm.h"
#include "play_gate.h"
#include "play_envelope.h"

static const char *TAG = "voice";

#define SAMPLE_RATE      16000
#define DOWN_QUEUE_DEPTH 8        // Doc 07 section 7: 8 blocks, drop the oldest.
#define STOP_WAIT_MS     200
#define TASK_STACK       4096
#define TASK_PRIO        7        // Above the Coach task, so a block is never late.
// The codec curve maps 90 to -5 dB, before the board's hardware compensation.
#define VOLUME           90

typedef enum { MODE_IDLE = 0, MODE_CAPTURE, MODE_PLAY } voice_mode_t;

static voice_cb_t s_cb;
static TaskHandle_t s_task;
static QueueHandle_t s_down_q;      // Items: IMA_BLOCK_BYTES.
static SemaphoreHandle_t s_gate_mu;
static SemaphoreHandle_t s_stopped;
static play_gate_t s_gate;
static unsigned s_down_dropped;   // Protected by s_gate_mu.
static volatile voice_mode_t s_mode;
static volatile bool s_stop;
static volatile uint32_t s_progress_ms;   // Last speaker write, or the playback start.

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }

static void flush_down(void) {
    xSemaphoreTake(s_gate_mu, portMAX_DELAY);
    xQueueReset(s_down_q);
    play_gate_reset(&s_gate);
    s_down_dropped = 0;
    xSemaphoreGive(s_gate_mu);
}

static void capture_loop(void) {
    static int16_t pcm[IMA_BLOCK_SAMPLES];
    static uint8_t block[IMA_BLOCK_BYTES];
    ima_state_t enc;
    ima_state_init(&enc);
    unsigned blocks = 0;
    unsigned near_full_scale = 0;
    int peak = 0;
    while (!s_stop) {
        // One block takes 20 ms, so the stop check runs every 20 ms.
        if (bsp_audio_read(pcm, sizeof pcm) != ESP_OK) {
            ESP_LOGE(TAG, "microphone read failed");
            break;
        }
        for (size_t i = 0; i < IMA_BLOCK_SAMPLES; i++) {
            const int a = pcm[i] < 0 ? -pcm[i] : pcm[i];
            if (a > peak) peak = a;
            if (a >= 32700) near_full_scale++;
        }
        ima_encode_block(&enc, pcm, IMA_BLOCK_SAMPLES, block);
        if (s_cb.up_block) s_cb.up_block(block);
        blocks++;
    }
    // A peak near 0 means that the microphone path gives silence.
    ESP_LOGI(TAG, "capture: %u blocks (%u ms), peak %d of 32767", blocks, blocks * 20, peak);
    ESP_LOGI(TAG, "capture level: %u samples near full scale", near_full_scale);
}

static void play_loop(void) {
    static int16_t pcm[IMA_BLOCK_SAMPLES];
    static uint8_t block[IMA_BLOCK_BYTES];
    // Count decoded input and attempted writes, not measured speaker output.
    unsigned blocks = 0, underruns = 0, invalid = 0, near_full_scale = 0;
    unsigned fades = 0, write_errors = 0;
    uint32_t max_write_ms = 0, first_silence_ms = 0;
    const uint32_t feed_start_ms = now_ms();
    play_envelope_t envelope;
    play_envelope_reset(&envelope);
    int peak = 0;
    bool completed = false;
    while (!s_stop) {
        xSemaphoreTake(s_gate_mu, portMAX_DELAY);
        const play_gate_action_t act = play_gate_poll(&s_gate, now_ms());
        bool have = false;
        if (act == PLAY_GATE_PLAY && xQueueReceive(s_down_q, block, 0) == pdTRUE) {
            play_gate_on_played(&s_gate);
            have = true;
        }
        xSemaphoreGive(s_gate_mu);

        if (act == PLAY_GATE_DONE) {
            completed = true;
            break;
        }
        if (act == PLAY_GATE_WAIT) {
            s_progress_ms = now_ms();   // The gate bounds the wait for more data.
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        bool valid_pcm = false;
        if (have) {
            valid_pcm = ima_decode_block(block, pcm) != 0;
            if (!valid_pcm) {
                memset(pcm, 0, sizeof pcm);   // A bad header plays silence.
                invalid++;
            }
            blocks++;
            for (size_t i = 0; i < IMA_BLOCK_SAMPLES; i++) {
                const int a = pcm[i] < 0 ? -pcm[i] : pcm[i];
                if (a > peak) peak = a;
                if (a >= 32700) near_full_scale++;
            }
        } else {
            if (underruns == 0) first_silence_ms = now_ms() - feed_start_ms;
            underruns++;
        }
        if (play_envelope_apply(&envelope, pcm, IMA_BLOCK_SAMPLES, valid_pcm)) fades++;
        const uint32_t write_start_ms = now_ms();
        const esp_err_t write_result = bsp_audio_write(pcm, sizeof pcm);
        const uint32_t write_ms = now_ms() - write_start_ms;
        if (write_ms > max_write_ms) max_write_ms = write_ms;
        if (write_result != ESP_OK) {
            write_errors++;
            ESP_LOGE(TAG, "speaker write failed");
            completed = true;
            break;
        }
        xSemaphoreTake(s_gate_mu, portMAX_DELAY);
        play_gate_on_written(&s_gate);
        xSemaphoreGive(s_gate_mu);
        s_progress_ms = now_ms();
    }
    xSemaphoreTake(s_gate_mu, portMAX_DELAY);
    const unsigned dropped = s_down_dropped;
    xSemaphoreGive(s_gate_mu);
    // One summary per answer avoids logs in the real-time speaker feed.
    ESP_LOGI(TAG, "playback: %u blocks, peak %d of 32767, %u samples near full scale, "
             "%u underruns, %u dropped, %u invalid", blocks, peak, near_full_scale,
             underruns, dropped, invalid);
    // These times measure software writes. They do not measure the DMA output.
    ESP_LOGI(TAG, "playback feed: %lu ms, first silence %ld ms, max write %lu ms, "
             "%u write errors, %u fades",
             (unsigned long)(now_ms() - feed_start_ms),
             underruns ? (long)first_silence_ms : -1L,
             (unsigned long)max_write_ms, write_errors, fades);
    if (completed && s_cb.play_done) s_cb.play_done();
}

static void voice_task(void *arg) {
    (void)arg;
    for (;;) {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        const voice_mode_t m = s_mode;
        if (m == MODE_CAPTURE) capture_loop();
        else if (m == MODE_PLAY) play_loop();
        s_mode = MODE_IDLE;
        if (s_stop) {
            s_stop = false;
            xSemaphoreGive(s_stopped);
        }
    }
}

esp_err_t voice_start(const voice_cb_t *cb) {
    if (s_task) return ESP_ERR_INVALID_STATE;
    if (cb) s_cb = *cb;
    // bsp_audio_init() allows a repeated call. Call it here, so the start order of the app
    // cannot leave the codec closed.
    esp_err_t e = bsp_audio_init();
    if (e == ESP_OK) e = bsp_audio_set_format(SAMPLE_RATE, 16, 1);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "codec start failed: %s. Voice stays off.", esp_err_to_name(e));
        return e;
    }
    bsp_audio_set_volume(VOLUME);
    ESP_LOGI(TAG, "output volume: %u", (unsigned)VOLUME);
    s_down_q = xQueueCreate(DOWN_QUEUE_DEPTH, IMA_BLOCK_BYTES);
    s_gate_mu = xSemaphoreCreateMutex();
    s_stopped = xSemaphoreCreateBinary();
    if (!s_down_q || !s_gate_mu || !s_stopped) return ESP_ERR_NO_MEM;
    play_gate_reset(&s_gate);
    if (xTaskCreate(voice_task, "voice", TASK_STACK, NULL, TASK_PRIO, &s_task) != pdPASS) return ESP_ERR_NO_MEM;
    return ESP_OK;
}

static void start_mode(voice_mode_t m) {
    if (!s_task || s_mode != MODE_IDLE) return;
    s_stop = false;
    s_mode = m;
    xTaskNotifyGive(s_task);
}

// Ask the running loop to end, and wait for the task to confirm.
static bool stop_mode(voice_mode_t m) {
    if (!s_task || s_mode != m) return true;
    xSemaphoreTake(s_stopped, 0);   // Clear an old confirmation.
    s_stop = true;
    // The loop can end by itself just before s_stop is set. Then no confirmation comes.
    if (s_mode == MODE_IDLE) {
        s_stop = false;
        return true;
    }
    const int64_t t0 = esp_timer_get_time();
    if (xSemaphoreTake(s_stopped, pdMS_TO_TICKS(STOP_WAIT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "stop took more than %d ms", STOP_WAIT_MS);
        return false;
    }
    // TC-06 and TC-07 measure this time. For playback, the I2S DMA still holds up to
    // 90 ms of sound after the task stops, so the speaker goes quiet up to 90 ms later.
    ESP_LOGI(TAG, "%s stop: %d ms", m == MODE_CAPTURE ? "capture" : "playback",
             (int)((esp_timer_get_time() - t0) / 1000));
    return true;
}

void voice_capture_start(void) { start_mode(MODE_CAPTURE); }
bool voice_capture_stop(void) { return stop_mode(MODE_CAPTURE); }

void voice_down_open(void) { if (s_down_q) flush_down(); }

void voice_down_push(const uint8_t *block, size_t len) {
    if (!s_down_q || len != IMA_BLOCK_BYTES) return;
    xSemaphoreTake(s_gate_mu, portMAX_DELAY);
    if (xQueueSend(s_down_q, block, 0) != pdTRUE) {
        uint8_t old[IMA_BLOCK_BYTES];
        (void)xQueueReceive(s_down_q, old, 0);   // Drop the oldest block.
        s_down_dropped++;
        play_gate_on_played(&s_gate);
        (void)xQueueSend(s_down_q, block, 0);
    }
    play_gate_on_block(&s_gate, now_ms());
    xSemaphoreGive(s_gate_mu);
}

void voice_down_end(void) {
    if (!s_gate_mu) return;
    xSemaphoreTake(s_gate_mu, portMAX_DELAY);
    play_gate_on_end(&s_gate, now_ms());
    xSemaphoreGive(s_gate_mu);
}

bool voice_ready(void) { return s_task != NULL; }

void voice_play_enable(void) {
    if (!s_task) {
        // Without the voice task, no block can play. End the answer at once, so the
        // flow does not wait in PLAYBACK.
        if (s_cb.play_done) s_cb.play_done();
        return;
    }
    s_progress_ms = now_ms();
    start_mode(MODE_PLAY);
}

uint32_t voice_play_idle_ms(void) { return now_ms() - s_progress_ms; }

bool voice_play_stop(void) {
    const bool ok = stop_mode(MODE_PLAY);
    if (s_down_q) flush_down();
    return ok;
}
