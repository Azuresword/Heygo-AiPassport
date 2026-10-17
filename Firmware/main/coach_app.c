// main/coach_app.c
//
// The Coach task connects three parts:
//   the flow     generated from flows/coach.flow.json
//   the voice    main/audio/voice.c, microphone and speaker
//   the link     main/link/ble_link.c, BLE and the doc 07 session
//
// Order rules:
//   - Capture starts after every action of the step. The flow lists audio_capture_start
//     before link_send_agent_start, and the first block needs the new xid.
//   - A capture stop waits for the last block, so AGENT_END follows it.
//   - A playback stop waits for the speaker, so the microphone never runs during playback.
//
// Ski data (Increment 2): the link keeps the last LIVE_STATS and RUN_REPORT. A data
// message, or the 1-second tick, asks this task to copy the texts from main/ui/ski_view.c
// into the labels of the Ski page or the Report page. Data never changes the state.
#include "coach_app.h"

#include <stdio.h>
#include <string.h>

#include "ble_link.h"
#include "bsp_display.h"
#include "coach_bindings.h"
#include "coach_flow.h"
#include "coach_input.h"
#include "coach_screens.h"
#include "coach_ui.h"
#include "ski_view.h"
#include "ui_theme.h"
#include "voice.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"

static const char *TAG = "coach";

#define QUEUE_DEPTH 16
#define TASK_STACK 4096
#define TASK_PRIO 5
#define LVGL_LOCK_MS 500
#define CAPTURE_LIMIT_US (20LL * 1000 * 1000)   // Document 01: one question lasts at most 20 s.
#define AGENT_TIMEOUT_US (10LL * 1000 * 1000)   // Document 07: the first answer comes within 10 s.
#define PLAY_WATCH_US    (1000LL * 1000)         // Check the playback once a second.
#define PLAY_STUCK_MS    3000                    // No progress for 3 s ends the answer.
#define DATA_TICK_US     (1000LL * 1000)         // The out-of-date mark and the report age.

typedef enum { MSG_KEY, MSG_EVENT, MSG_LINK, MSG_AGENT_STATE, MSG_DATA } msg_kind_t;

typedef struct {
    msg_kind_t kind;
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
    coach_event_t event;
    link_voice_t voice;
    uint8_t state;
    uint8_t failure;
} msg_t;

static QueueHandle_t s_queue;
static coach_flow_t s_flow;
static coach_input_t s_input;
static coach_screen_coach_t s_coach;
static coach_screen_ski_t s_ski;
static coach_screen_report_t s_report;
static coach_screen_exit_t s_exit;
static esp_timer_handle_t s_capture_timer;
static esp_timer_handle_t s_answer_timer;
static esp_timer_handle_t s_play_watch;
static esp_timer_handle_t s_data_tick;
static ski_view_t s_ski_view;          // The texts on the Ski page now.
static report_view_t s_report_view;    // The texts on the Report page now.
static coach_screen_id_t s_shown = COACH_SCREEN_COUNT;   // COUNT: no screen yet
static lv_obj_t *s_blank;   // Active while the old screen is deleted. LVGL must not delete the active screen.
static link_voice_t s_voice = LINK_VOICE_NO_LINK;
static coach_event_t s_event;          // The event of the step that runs now.
static bool s_start_capture;           // Set by audio_capture_start. Runs after the step.

_Static_assert((int)COACH_KEY_UP == (int)BSP_BTN_UP && (int)COACH_KEY_DOWN == (int)BSP_BTN_DOWN &&
               (int)COACH_KEY_OK == (int)BSP_BTN_OK, "coach_key_t must match bsp_btn_t");
_Static_assert((int)COACH_KEY_EV_PRESS == (int)BSP_BTN_PRESS && (int)COACH_KEY_EV_CLICK == (int)BSP_BTN_CLICK &&
               (int)COACH_KEY_EV_DOUBLE == (int)BSP_BTN_DOUBLE && (int)COACH_KEY_EV_LONG == (int)BSP_BTN_LONG &&
               (int)COACH_KEY_EV_RELEASE == (int)BSP_BTN_RELEASE, "coach_key_ev_t must match bsp_btn_ev_t");

// Debug log of the screen. Every text on the Coach screen goes through a generated
// setter, so this one hook shows what the user sees, whatever its source.
static void trace_screen(const char *subject, const char *text, int32_t value) {
    if (text) ESP_LOGI("screen", "%s = \"%s\"", subject, text);
    else ESP_LOGI("screen", "%s = %ld", subject, (long)value);
}

static const char *screen_name(coach_screen_id_t id) {
    switch (id) {
    case COACH_SCREEN_COACH: return "coach";
    case COACH_SCREEN_SKI: return "ski";
    case COACH_SCREEN_REPORT: return "report";
    case COACH_SCREEN_EXIT: return "exit";
    default: return "?";
    }
}

static bool lvgl_lock(void) { return bsp_lvgl_lock(LVGL_LOCK_MS); }
static void lvgl_unlock(void) { bsp_lvgl_unlock(); }

static void post_event(coach_event_t event) {
    if (!s_queue) return;
    const msg_t m = { .kind = MSG_EVENT, .event = event };
    if (xQueueSend(s_queue, &m, 0) != pdTRUE) {
        ESP_LOGW(TAG, "queue full: drop %s", coach_event_name(event));
    }
}

static void post_msg(const msg_t *m) {
    if (s_queue && xQueueSend(s_queue, m, 0) != pdTRUE) ESP_LOGW(TAG, "queue full: drop message %d", m->kind);
}

static void on_capture_limit(void *arg) { (void)arg; post_event(COACH_EVENT_CAPTURE_LIMIT); }
static void on_data_tick(void *arg) {
    (void)arg;
    const msg_t m = { .kind = MSG_DATA };
    post_msg(&m);
}
static void on_answer_timeout(void *arg) { (void)arg; post_event(COACH_EVENT_AGENT_TIMEOUT); }

// The voice task ends a stalled stream after 2 s. This watchdog covers a voice task
// that stops itself, for example in a blocked speaker write. The answer then ends,
// and the user can ask again.
static void on_play_watch(void *arg) {
    (void)arg;
    if (voice_play_idle_ms() >= PLAY_STUCK_MS) {
        ESP_LOGW(TAG, "playback made no progress for %d ms: end the answer", PLAY_STUCK_MS);
        post_event(COACH_EVENT_ANSWER_END);
    }
}

// Write one label when its text changes. The log line shows what the user sees.
static void put_label(lv_obj_t *label, const char *name, char *now, size_t cap, const char *want, bool force) {
    if (!label || (!force && strcmp(now, want) == 0)) return;
    snprintf(now, cap, "%s", want);
    lv_label_set_text(label, want);
    ESP_LOGI("screen", "%s = \"%s\"", name, want);
}

static void set_hidden(lv_obj_t *obj, bool hidden) {
    if (!obj) return;
    if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

#define PUT(screen, view, field, want, force) \
    put_label(screen.field, #view "." #field, view.field, sizeof view.field, want.field, force)

// Copy the ski data texts into the screen that is on now. force: the screen is new.
// The caller holds the LVGL lock.
static void refresh_data(bool force) {
    if (s_shown != COACH_SCREEN_SKI && s_shown != COACH_SCREEN_REPORT) return;
    bp_live_stats_t live;
    link_report_t report;
    bool has_live, stale, has_report;
    uint32_t age;
    ble_link_get_data(&live, &has_live, &stale, &report, &has_report, &age);
    if (s_shown == COACH_SCREEN_SKI) {
        ski_view_t v;
        ski_view_build(&v, has_live ? &live : NULL, stale, has_report ? &report : NULL);
        PUT(s_ski, s_ski_view, dest, v, force);
        PUT(s_ski, s_ski_view, sub, v, force);
        PUT(s_ski, s_ski_view, big, v, force);
        PUT(s_ski, s_ski_view, unit, v, force);
        PUT(s_ski, s_ski_view, drop, v, force);
        PUT(s_ski, s_ski_view, dist, v, force);
        PUT(s_ski, s_ski_view, turns, v, force);
        PUT(s_ski, s_ski_view, time, v, force);
        PUT(s_ski, s_ski_view, problem, v, force);
        PUT(s_ski, s_ski_view, tip, v, force);
        // The panel shows the four metric cells, or the problem and the tip of the report.
        if (force || v.report != s_ski_view.report) {
            s_ski_view.report = v.report;
            lv_obj_t *cells[] = { s_ski.k_drop, s_ski.drop, s_ski.k_dist, s_ski.dist,
                                  s_ski.k_turns, s_ski.turns, s_ski.k_time, s_ski.time };
            for (size_t i = 0; i < sizeof cells / sizeof cells[0]; i++) set_hidden(cells[i], v.report);
            set_hidden(s_ski.problem, !v.report);
            set_hidden(s_ski.tip, !v.report);
            ESP_LOGI("screen", "ski panel = %s", v.report ? "report" : "metrics");
        }
    } else {
        report_view_t v;
        report_view_build(&v, has_report ? &report : NULL, age);
        PUT(s_report, s_report_view, title, v, force);
        PUT(s_report, s_report_view, iq, v, force);
        PUT(s_report, s_report_view, age, v, force);
        PUT(s_report, s_report_view, problem, v, force);
        PUT(s_report, s_report_view, tip, v, force);
    }
}

// Only one screen exists at a time. All three together do not fit the 24 KB LVGL pool.
// Load the blank screen, delete the old screen, then build the new one. The peak stays
// at one screen. tools/host_render measures the same sequence.
// The subjects keep their values, so a new Coach screen shows the current texts.
// The caller holds the LVGL lock.
static void show_screen(coach_screen_id_t id) {
    if (id == s_shown) return;
    ESP_LOGI("screen", "show %s screen", screen_name(id));
    lv_screen_load(s_blank);
    switch (s_shown) {
    case COACH_SCREEN_COACH: coach_teardown(&s_coach); break;
    case COACH_SCREEN_SKI: coach_screen_ski_destroy(&s_ski); break;
    case COACH_SCREEN_REPORT: coach_screen_report_destroy(&s_report); break;
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
    case COACH_SCREEN_REPORT:
        coach_screen_report_build(&s_report);
        lv_screen_load(s_report.screen);
        break;
    case COACH_SCREEN_EXIT:
        coach_screen_exit_build(&s_exit);
        lv_screen_load(s_exit.screen);
        break;
    default:
        break;
    }
    s_shown = id;
    refresh_data(true);
}

static void run_action(coach_action_t a) {
    if (coach_ui_apply(a)) return;
    switch (a) {
    case COACH_ACTION_AUDIO_CAPTURE_START: s_start_capture = true; break;
    case COACH_ACTION_AUDIO_CAPTURE_STOP: voice_capture_stop(); break;
    case COACH_ACTION_AUDIO_PLAY_START: voice_play_enable(); break;
    case COACH_ACTION_AUDIO_PLAY_STOP: voice_play_stop(); break;
    case COACH_ACTION_LINK_SEND_AGENT_START:
        if (!ble_link_agent_start()) ESP_LOGW(TAG, "AGENT_START not sent: voice %s", link_voice_name(s_voice));
        break;
    case COACH_ACTION_LINK_SEND_AGENT_END:
        ble_link_agent_end(s_event == COACH_EVENT_CAPTURE_LIMIT ? BP_END_LIMIT : BP_END_RELEASE);
        break;
    case COACH_ACTION_LINK_SEND_AGENT_CANCEL: ble_link_agent_cancel(); break;
    default: ESP_LOGW(TAG, "no handler for %s", coach_action_name(a)); break;
    }
}

// Doc 07 field values for CONTEXT and DEVICE_STATUS.
static void send_context(coach_state_t st) {
    const uint8_t mode = st == COACH_STATE_SKI ? BP_MODE_SKI : BP_MODE_REST;
    const uint8_t page = st == COACH_STATE_SKI      ? BP_PAGE_SKI
                       : st == COACH_STATE_EXIT_Q   ? BP_PAGE_EXIT
                       : st == COACH_STATE_REPORT   ? BP_PAGE_REPORT
                                                    : BP_PAGE_COACH;
    ble_link_set_context(mode, page);
}

static coach_action_t why_action(link_voice_t v) {
    switch (v) {
    case LINK_VOICE_VERSION: return COACH_ACTION_UI_WHY_VERSION;
    case LINK_VOICE_MTU: return COACH_ACTION_UI_WHY_MTU;
    case LINK_VOICE_WAIT_HELLO:
    case LINK_VOICE_NO_ACK:
    case LINK_VOICE_SILENT: return COACH_ACTION_UI_WHY_PHONE_SILENT;
    default: return COACH_ACTION_UI_WHY_LINK_DOWN;
    }
}

static coach_action_t status_action(link_voice_t v) {
    switch (v) {
    case LINK_VOICE_OK: return COACH_ACTION_UI_LINK_READY;
    case LINK_VOICE_NO_LINK:
    case LINK_VOICE_WAIT_HELLO: return COACH_ACTION_UI_LINK_DOWN;
    default: return COACH_ACTION_UI_LINK_NO_VOICE;
    }
}

// Start the timer that belongs to the new state. Stop the timers of the old state.
static void update_timers(coach_state_t from, coach_state_t to) {
    if (from == COACH_STATE_CAPTURE && to != COACH_STATE_CAPTURE) esp_timer_stop(s_capture_timer);
    if (from == COACH_STATE_WAIT_ANSWER && to != COACH_STATE_WAIT_ANSWER) esp_timer_stop(s_answer_timer);
    if (from == COACH_STATE_PLAYBACK && to != COACH_STATE_PLAYBACK) esp_timer_stop(s_play_watch);
    if (to == COACH_STATE_PLAYBACK && from != COACH_STATE_PLAYBACK) esp_timer_start_periodic(s_play_watch, PLAY_WATCH_US);
    if (to == COACH_STATE_CAPTURE && from != COACH_STATE_CAPTURE) {
        esp_timer_start_once(s_capture_timer, CAPTURE_LIMIT_US);
    }
    if (to == COACH_STATE_WAIT_ANSWER && from != COACH_STATE_WAIT_ANSWER) {
        esp_timer_start_once(s_answer_timer, AGENT_TIMEOUT_US);
    }
}

static void handle(coach_event_t event) {
    // The guard: without voice, a question cannot start. The screen names the cause.
    // Voice needs two parts: the BLE link and the voice task.
    if (event == COACH_EVENT_OK_HOLD && s_flow.state == COACH_STATE_READY &&
        (s_voice != LINK_VOICE_OK || !voice_ready())) {
        coach_ui_apply(!voice_ready() ? COACH_ACTION_UI_WHY_AUDIO : why_action(s_voice));
        event = COACH_EVENT_ASK_BLOCKED;
    }
    s_event = event;
    s_start_capture = false;
    const coach_step_t step = coach_flow_step(&s_flow, event);
    if (!step.handled) {
        ESP_LOGD(TAG, "%s ignores %s", coach_state_name(step.from), coach_event_name(event));
        return;
    }
    ESP_LOGI(TAG, "%s --%s--> %s", coach_state_name(step.from), coach_event_name(event),
             coach_state_name(step.to));
    for (size_t i = 0; i < step.action_count; i++) run_action(step.actions[i]);
    if (s_start_capture) voice_capture_start();
    update_timers(step.from, step.to);

    // The exchange ends with the answer. A timeout also tells the phone to stop.
    if (event == COACH_EVENT_ANSWER_END || event == COACH_EVENT_ANSWER_FAILED) ble_link_agent_done();
    if (event == COACH_EVENT_AGENT_TIMEOUT) ble_link_agent_cancel();
    if (step.state_changed) send_context(step.to);

    const coach_screen_id_t screen = coach_state_screen(step.to);
    if (screen != s_shown && lvgl_lock()) {
        show_screen(screen);
        lvgl_unlock();
    }
}

static void on_link_msg(link_voice_t v) {
    const link_voice_t before = s_voice;
    s_voice = v;
    coach_ui_apply(status_action(v));
    if (before == LINK_VOICE_OK && v != LINK_VOICE_OK) handle(COACH_EVENT_LINK_LOST);
}

static void on_agent_state_msg(uint8_t state, uint8_t failure) {
    if (state == BP_AGENT_FAILED) {
        handle(failure == BP_FAIL_TIMEOUT ? COACH_EVENT_AGENT_TIMEOUT : COACH_EVENT_ANSWER_FAILED);
        return;
    }
    // Each state from the phone shows progress, so the 10-second wait starts again.
    if (s_flow.state == COACH_STATE_WAIT_ANSWER) {
        esp_timer_stop(s_answer_timer);
        esp_timer_start_once(s_answer_timer, AGENT_TIMEOUT_US);
    }
}

static void coach_task(void *arg) {
    (void)arg;
    msg_t m;
    for (;;) {
        if (xQueueReceive(s_queue, &m, portMAX_DELAY) != pdTRUE) continue;
        switch (m.kind) {
        case MSG_EVENT: handle(m.event); break;
        case MSG_LINK: on_link_msg(m.voice); break;
        case MSG_AGENT_STATE: on_agent_state_msg(m.state, m.failure); break;
        case MSG_DATA:
            if (lvgl_lock()) {
                refresh_data(false);
                lvgl_unlock();
            }
            break;
        case MSG_KEY: {
            coach_event_t event;
            if (coach_input_map(&s_input, (coach_key_t)m.btn, (coach_key_ev_t)m.ev, &event)) handle(event);
            break;
        }
        }
    }
}

// ---------------------------------------------------------------- link and voice callbacks
// The link callbacks run on the NimBLE task inside the link lock. They post to the
// Coach queue, or they call the voice queue. They never call ble_link_*.
static void link_voice_changed(link_voice_t v) {
    const msg_t m = { .kind = MSG_LINK, .voice = v };
    post_msg(&m);
}

static void link_agent_state(uint8_t state, uint8_t failure, const char *text) {
    // The setter takes the LVGL lock. The lock order is link, then LVGL.
    if (text && text[0]) coach_set_body_text(text);
    const msg_t m = { .kind = MSG_AGENT_STATE, .state = state, .failure = failure };
    post_msg(&m);
}

static void link_play_start(void) {
    voice_down_open();
    post_event(COACH_EVENT_ANSWER_START);
}

static void link_audio_down(const uint8_t *block, size_t len, uint8_t missing) {
    (void)missing;   // Doc 07: play nothing for a missing block.
    voice_down_push(block, len);
}

static void link_play_end(void) { voice_down_end(); }

// The link keeps the data. The Coach task reads it with ble_link_get_data().
static void link_data(void) {
    const msg_t m = { .kind = MSG_DATA };
    post_msg(&m);
}
static void link_live_stats(const bp_live_stats_t *s) { (void)s; link_data(); }
static void link_run_report(const link_report_t *r) { (void)r; link_data(); }
// D-28: the phone opens the Report page. The flow accepts it in SKI and READY only.
static void link_show_page(uint8_t page) {
    if (page == BP_PAGE_REPORT) post_event(COACH_EVENT_SHOW_REPORT);
}

static void voice_up_block(const uint8_t *block) { ble_link_audio_up(block); }
static void voice_play_done(void) { post_event(COACH_EVENT_ANSWER_END); }

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
    const esp_timer_create_args_t watch = { .callback = on_play_watch, .name = "coach_play" };
    const esp_timer_create_args_t data = { .callback = on_data_tick, .name = "coach_data" };
    esp_err_t e = esp_timer_create(&cap, &s_capture_timer);
    if (e == ESP_OK) e = esp_timer_create(&ans, &s_answer_timer);
    if (e == ESP_OK) e = esp_timer_create(&watch, &s_play_watch);
    if (e == ESP_OK) e = esp_timer_create(&data, &s_data_tick);
    if (e != ESP_OK) return e;

    coach_bindings_init();
    coach_bindings_set_lock(lvgl_lock, lvgl_unlock);
    coach_bindings_set_trace(trace_screen);
    if (!lvgl_lock()) return ESP_ERR_TIMEOUT;
    s_blank = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_blank, ui_color_bg(), 0);
    show_screen(coach_state_screen(s_flow.state));
    lvgl_unlock();

    s_queue = xQueueCreate(QUEUE_DEPTH, sizeof(msg_t));
    if (!s_queue) return ESP_ERR_NO_MEM;

    // Voice and link failures leave the screens and keys working. The guard then blocks questions.
    const voice_cb_t vcb = { .up_block = voice_up_block, .play_done = voice_play_done };
    esp_err_t ve = voice_start(&vcb);
    if (ve != ESP_OK) ESP_LOGE(TAG, "voice start failed: %s", esp_err_to_name(ve));
    const ble_link_app_t app = {
        .voice_changed = link_voice_changed, .agent_state = link_agent_state,
        .play_start = link_play_start, .audio_down = link_audio_down, .play_end = link_play_end,
        .live_stats = link_live_stats, .run_report = link_run_report, .show_page = link_show_page,
    };
    esp_err_t le = ble_link_start(&app);
    if (le != ESP_OK) ESP_LOGE(TAG, "BLE link start failed: %s", esp_err_to_name(le));
    if (xTaskCreate(coach_task, "coach", TASK_STACK, NULL, TASK_PRIO, NULL) != pdPASS) {
        vQueueDelete(s_queue);
        s_queue = NULL;
        return ESP_ERR_NO_MEM;
    }
    esp_timer_start_periodic(s_data_tick, DATA_TICK_US);
    ESP_LOGI(TAG, "start in %s", coach_state_name(s_flow.state));
    return ESP_OK;
}
