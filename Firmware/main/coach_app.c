// main/coach_app.c
//
// Status of the actions (Sprint 1 scope):
//   ui_*     Done. coach_ui_apply() sets the texts and icons from the flow graph.
//   audio_*  Not connected yet. The task logs each action.
//   link_*   Not connected yet. The task logs each action. The BLE link stays down,
//            so the status line shows 未连接 and a question ends with a timeout.
#include "coach_app.h"

#include "bsp_display.h"
#include "coach_bindings.h"
#include "coach_flow.h"
#include "coach_input.h"
#include "coach_screens.h"
#include "coach_ui.h"
#include "ui_theme.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "coach";

#define QUEUE_DEPTH 8
#define TASK_STACK 4096
#define TASK_PRIO 5
#define LVGL_LOCK_MS 500
#define CAPTURE_LIMIT_US (20LL * 1000 * 1000)   // Document 01: one question lasts at most 20 s.
#define AGENT_TIMEOUT_US (10LL * 1000 * 1000)   // Document 07: the first answer comes within 10 s.

typedef enum { MSG_KEY, MSG_EVENT } msg_kind_t;

typedef struct {
    msg_kind_t kind;
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
    coach_event_t event;
} msg_t;

static QueueHandle_t s_queue;
static coach_flow_t s_flow;
static coach_input_t s_input;
static coach_screen_coach_t s_coach;
static coach_screen_ski_t s_ski;
static coach_screen_exit_t s_exit;
static esp_timer_handle_t s_capture_timer;
static esp_timer_handle_t s_answer_timer;
static coach_screen_id_t s_shown = COACH_SCREEN_COUNT;   // COUNT: no screen yet
static lv_obj_t *s_blank;   // Active while the old screen is deleted. LVGL must not delete the active screen.
static bool s_mic_on;
static bool s_speaker_on;

_Static_assert((int)COACH_KEY_UP == (int)BSP_BTN_UP && (int)COACH_KEY_DOWN == (int)BSP_BTN_DOWN &&
               (int)COACH_KEY_OK == (int)BSP_BTN_OK, "coach_key_t must match bsp_btn_t");
_Static_assert((int)COACH_KEY_EV_PRESS == (int)BSP_BTN_PRESS && (int)COACH_KEY_EV_CLICK == (int)BSP_BTN_CLICK &&
               (int)COACH_KEY_EV_DOUBLE == (int)BSP_BTN_DOUBLE && (int)COACH_KEY_EV_LONG == (int)BSP_BTN_LONG &&
               (int)COACH_KEY_EV_RELEASE == (int)BSP_BTN_RELEASE, "coach_key_ev_t must match bsp_btn_ev_t");

static bool lvgl_lock(void) { return bsp_lvgl_lock(LVGL_LOCK_MS); }
static void lvgl_unlock(void) { bsp_lvgl_unlock(); }

static void post_event(coach_event_t event) {
    if (!s_queue) return;
    const msg_t m = { .kind = MSG_EVENT, .event = event };
    if (xQueueSend(s_queue, &m, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full: drop %s", coach_event_name(event));
    }
}

static void on_capture_limit(void *arg) { (void)arg; post_event(COACH_EVENT_CAPTURE_LIMIT); }
static void on_answer_timeout(void *arg) { (void)arg; post_event(COACH_EVENT_AGENT_TIMEOUT); }

// Only one screen exists at a time. All three together do not fit the 24 KB LVGL pool.
// Load the blank screen, delete the old screen, then build the new one. The peak stays
// at one screen. tools/host_render measures the same sequence.
// The subjects keep their values, so a new Coach screen shows the current texts.
// The caller holds the LVGL lock.
static void show_screen(coach_screen_id_t id) {
    if (id == s_shown) return;
    lv_screen_load(s_blank);
    switch (s_shown) {
    case COACH_SCREEN_COACH: coach_teardown(&s_coach); break;
    case COACH_SCREEN_SKI: coach_screen_ski_destroy(&s_ski); break;
    case COACH_SCREEN_EXIT: coach_screen_exit_destroy(&s_exit); break;
    default: break;
    }
    switch (id) {
    case COACH_SCREEN_COACH:
        coach_screen_coach_build(&s_coach);
        coach_bindings_attach(&s_coach);
        lv_screen_load(s_coach.screen);
        break;
    case COACH_SCREEN_SKI:
        coach_screen_ski_build(&s_ski);
        lv_screen_load(s_ski.screen);
        break;
    case COACH_SCREEN_EXIT:
        coach_screen_exit_build(&s_exit);
        lv_screen_load(s_exit.screen);
        break;
    default:
        break;
    }
    s_shown = id;
}

static void run_action(coach_action_t a) {
    if (coach_ui_apply(a)) return;
    switch (a) {
    case COACH_ACTION_AUDIO_CAPTURE_START:
        if (s_speaker_on) ESP_LOGE(TAG, "rule broken: the microphone starts during playback");
        s_mic_on = true;
        break;
    case COACH_ACTION_AUDIO_CAPTURE_STOP: s_mic_on = false; break;
    case COACH_ACTION_AUDIO_PLAY_START:
        if (s_mic_on) ESP_LOGE(TAG, "rule broken: playback starts during capture");
        s_speaker_on = true;
        break;
    case COACH_ACTION_AUDIO_PLAY_STOP: s_speaker_on = false; break;
    default: break;
    }
    // Audio and link actions are not connected yet. See the note at the top of the file.
    ESP_LOGI(TAG, "  action %s (not connected)", coach_action_name(a));
}

// Start the timer that belongs to the new state. Stop the timers of the old state.
static void update_timers(coach_state_t from, coach_state_t to) {
    if (from == COACH_STATE_CAPTURE && to != COACH_STATE_CAPTURE) esp_timer_stop(s_capture_timer);
    if (from == COACH_STATE_WAIT_ANSWER && to != COACH_STATE_WAIT_ANSWER) esp_timer_stop(s_answer_timer);
    if (to == COACH_STATE_CAPTURE && from != COACH_STATE_CAPTURE) {
        esp_timer_start_once(s_capture_timer, CAPTURE_LIMIT_US);
    }
    if (to == COACH_STATE_WAIT_ANSWER && from != COACH_STATE_WAIT_ANSWER) {
        esp_timer_start_once(s_answer_timer, AGENT_TIMEOUT_US);
    }
}

static void handle(coach_event_t event) {
    const coach_step_t step = coach_flow_step(&s_flow, event);
    if (!step.handled) {
        ESP_LOGD(TAG, "%s ignores %s", coach_state_name(step.from), coach_event_name(event));
        return;
    }
    ESP_LOGI(TAG, "%s --%s--> %s", coach_state_name(step.from), coach_event_name(event),
             coach_state_name(step.to));
    for (size_t i = 0; i < step.action_count; i++) run_action(step.actions[i]);
    update_timers(step.from, step.to);

    const coach_screen_id_t screen = coach_state_screen(step.to);
    if (screen != s_shown && lvgl_lock()) {
        show_screen(screen);
        lvgl_unlock();
    }
}

static void coach_task(void *arg) {
    (void)arg;
    msg_t m;
    for (;;) {
        if (xQueueReceive(s_queue, &m, portMAX_DELAY) != pdTRUE) continue;
        if (m.kind == MSG_EVENT) {
            handle(m.event);
            continue;
        }
        coach_event_t event;
        if (coach_input_map(&s_input, (coach_key_t)m.btn, (coach_key_ev_t)m.ev, &event)) handle(event);
    }
}

void coach_app_post_key(bsp_btn_t btn, bsp_btn_ev_t ev) {
    if (!s_queue) return;
    const msg_t m = { .kind = MSG_KEY, .btn = btn, .ev = ev };
    (void)xQueueSend(s_queue, &m, 0);
}

esp_err_t coach_app_start(void) {
    if (s_queue) return ESP_ERR_INVALID_STATE;
    coach_flow_init(&s_flow);
    coach_input_init(&s_input);

    const esp_timer_create_args_t cap = { .callback = on_capture_limit, .name = "coach_cap" };
    const esp_timer_create_args_t ans = { .callback = on_answer_timeout, .name = "coach_ans" };
    esp_err_t e = esp_timer_create(&cap, &s_capture_timer);
    if (e == ESP_OK) e = esp_timer_create(&ans, &s_answer_timer);
    if (e != ESP_OK) return e;

    coach_bindings_init();
    coach_bindings_set_lock(lvgl_lock, lvgl_unlock);
    if (!lvgl_lock()) return ESP_ERR_TIMEOUT;
    s_blank = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_blank, ui_color_bg(), 0);
    show_screen(coach_state_screen(s_flow.state));
    lvgl_unlock();

    s_queue = xQueueCreate(QUEUE_DEPTH, sizeof(msg_t));
    if (!s_queue) return ESP_ERR_NO_MEM;
    if (xTaskCreate(coach_task, "coach", TASK_STACK, NULL, TASK_PRIO, NULL) != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "start in %s", coach_state_name(s_flow.state));
    return ESP_OK;
}
