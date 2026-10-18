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
//
// Popups (D-31): a POPUP from the phone stops everything at once and shows the popup page.
// The popup audio plays, then a 10-second countdown runs. Any key, or the end of the
// countdown, sends POPUP_END and goes back to the page before the popup. The Coach page
// states go back to READY. A new popup replaces the old one and keeps the saved page.
#include "coach_app.h"

#include <stdio.h>
#include <string.h>

#include "ble_link.h"
#include "boot_screen.h"
#include "bsp_display.h"
#include "coach_bindings.h"
#include "coach_flow.h"
#include "coach_input.h"
#include "coach_screens.h"
#include "coach_ui.h"
#include "heygo_boot_sound.h"
#include "page_rail.h"
#include "popup_anim.h"
#include "popup_view.h"
#include "ski_view.h"
#include "turn_page.h"
#include "turn_view.h"
#include "ui_theme.h"
#include "voice.h"
#include "esp_heap_caps.h"
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
#define TURN_POINT_MS    40                      // The Turns page draws one point each 40 ms.

typedef enum { MSG_KEY, MSG_EVENT, MSG_LINK, MSG_AGENT_STATE, MSG_DATA, MSG_BOOT_DONE } msg_kind_t;

typedef struct {
    msg_kind_t kind;
    bsp_btn_t btn;
    bsp_btn_ev_t ev;
    coach_event_t event;
    link_voice_t voice;
    uint8_t state;      // MSG_AGENT_STATE: the Agent state. A popup timer event: the popup number.
    uint8_t failure;
} msg_t;

static QueueHandle_t s_queue;
static coach_flow_t s_flow;
static coach_input_t s_input;
static coach_screen_coach_t s_coach;
static coach_screen_ski_t s_ski;
static coach_screen_report_t s_report;
static coach_screen_turns_t s_turns;
static coach_screen_popup_t s_popup_scr;
static esp_timer_handle_t s_capture_timer;
static esp_timer_handle_t s_answer_timer;
static esp_timer_handle_t s_play_watch;
static esp_timer_handle_t s_data_tick;
static esp_timer_handle_t s_popup_wait_timer;   // 5 s for the popup audio to start.
static esp_timer_handle_t s_popup_hold_timer;   // The 10-second countdown.
static esp_timer_handle_t s_boot_timer;         // D-35: the end of the boot screen.
static bool s_booting;                          // The boot screen is on. Screen changes wait.
static int s_boot_key = -1;                     // The key that ended the boot screen. Its events stop until release.
static ski_view_t s_ski_view;          // The texts on the Ski page now.
static report_view_t s_report_view;    // The texts on the Report page now.
static turn_view_t s_turn_view;        // The shape and the texts on the Turns page now.
static uint8_t s_turn_sel;             // The selected turn in the run. The page shows group s_turn_sel / 8.
static uint8_t s_turn_drawn;           // Points of that group on the screen, or TURN_VIEW_DRAWN_ALL.
static uint8_t s_turn_run;             // The run on the page, and its turn count. A new shape starts
static uint8_t s_turn_count;           // again at turn 1.
static TickType_t s_turn_tick;         // While the group draws: the time of the next point.
static link_turns_t s_turn_data;       // A copy of the turn cache. Static, so it stays off the task stack.
static coach_screen_id_t s_shown = COACH_SCREEN_COUNT;   // COUNT: no screen yet
static lv_obj_t *s_blank;   // Active while the old screen is deleted. LVGL must not delete the active screen.
static link_voice_t s_voice = LINK_VOICE_NO_LINK;
static coach_event_t s_event;          // The event of the step that runs now.
static bool s_start_capture;           // Set by audio_capture_start. Runs after the step.
static portMUX_TYPE s_popup_mux = portMUX_INITIALIZER_UNLOCKED;
static link_popup_t s_popup_in;        // The last POPUP from the link. Guarded by s_popup_mux.
static link_popup_t s_popup;           // The popup on the screen.
static popup_view_t s_popup_view;      // The texts on the popup page now.
static volatile uint8_t s_popup_no;    // Counts the popups. A timer event of an old popup is dropped.
static coach_event_t s_popup_return = COACH_EVENT_RETURN_READY;   // The page before the popup.
static int64_t s_popup_deadline_us;    // The end of the countdown.
static bool s_popup_close;             // Set by popup_close. The return runs after the step.
static bool s_run_pending;             // D-32: a run started during a talk with the coach.

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
    case COACH_SCREEN_TURNS: return "turns";
    case COACH_SCREEN_POPUP: return "popup";
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
// The popup timers carry the popup number, so that the event of a replaced popup is dropped.
static void post_popup_timer(coach_event_t event) {
    const msg_t m = { .kind = MSG_EVENT, .event = event, .state = s_popup_no };
    post_msg(&m);
}
static void on_popup_wait(void *arg) { (void)arg; post_popup_timer(COACH_EVENT_POPUP_NO_AUDIO); }
static void on_boot_done(void *arg) {
    (void)arg;
    const msg_t m = { .kind = MSG_BOOT_DONE };
    (void)xQueueSend(s_queue, &m, 0);
}

static void on_popup_hold(void *arg) { (void)arg; post_popup_timer(COACH_EVENT_POPUP_TIMEOUT); }

static bool is_popup(coach_state_t st) {
    return st == COACH_STATE_POPUP_WAIT || st == COACH_STATE_POPUP_PLAY || st == COACH_STATE_POPUP_HOLD;
}

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

// The log name is "ski.<field>" or "report.<field>". tools/phone_sim/analyze_log.py reads it.
#define PUT(page, screen, view, field, want, force) \
    put_label(screen.field, page "." #field, view.field, sizeof view.field, want.field, force)

// Show group `group` of the Turns page from its first turn, and ask the link for the groups next to it.
static void turn_group_show(uint8_t group) {
    s_turn_sel = (uint8_t)(group * BP_GROUP_TURNS);
    s_turn_drawn = 0;
    ble_link_turns_focus(group);
}

// The Turns page. replay: move the rider along the marked turn again.
// When the page opens, the first page of 8 turns draws point by point. The rider sits at
// the head of the line, and nothing replays. Then the rider replays the first turn.
// After that, the keys move the cursor, the page follows it, and the rider replays each turn.
// The caller holds the LVGL lock.
static void refresh_turns(bool force, bool replay) {
    const bool has = ble_link_get_turns(&s_turn_data);
    if (has && (s_turn_data.run_no != s_turn_run || s_turn_data.n_turns != s_turn_count)) {
        // A new shape: start again at turn 1.
        s_turn_run = s_turn_data.run_no;
        s_turn_count = s_turn_data.n_turns;
        turn_group_show(0);
    }
    static turn_view_t v;   // About 800 B. Static, so it stays off the 4 KB task stack.
    turn_view_build(&v, has ? &s_turn_data : NULL, s_turn_sel, s_turn_drawn, s_voice != LINK_VOICE_NO_LINK,
                    TURN_PAGE_BOX_W, TURN_PAGE_BOX_H, TURN_PAGE_MARGIN);
    const bool finished = s_turn_view.drawing && !v.drawing && v.n_path;
    if (v.drawing && !s_turn_view.drawing) s_turn_tick = xTaskGetTickCount() + pdMS_TO_TICKS(TURN_POINT_MS);
    const bool changed = memcmp(&v, &s_turn_view, sizeof v) != 0;
    if (!force && !changed && !replay) return;
    const bool new_turn = strcmp(v.counter, s_turn_view.counter) != 0;
    s_turn_view = v;
    if (force || changed) {
        turn_page_apply(&s_turns, &v);
        // While a group draws, log only when the head reaches another turn.
        if (!v.drawing || new_turn || force) {
            ESP_LOGI("screen", "turns = \"%s\" %s %s \"%s\" %s %s keys [%s|%s|%s] [%s] page %u/%u", v.title, v.counter,
                     v.side, v.metrics, v.source, v.msg, v.key_up, v.key_ok, v.key_down, v.key_hold,
                     has ? s_turn_sel / BP_GROUP_TURNS + 1u : 0u, has ? s_turn_data.n_groups : 0u);
        }
    }
    if (!v.drawing && (finished || replay || force || changed)) turn_page_replay(&s_turns);
}

// One more point of the group that draws. The Coach task calls it each TURN_POINT_MS.
static void turn_draw_step(void) {
    if (s_shown != COACH_SCREEN_TURNS || !s_turn_view.drawing) return;
    s_turn_tick += pdMS_TO_TICKS(TURN_POINT_MS);
    if (s_turn_drawn < TURN_VIEW_DRAWN_ALL - 1) s_turn_drawn++;
    if (lvgl_lock()) {
        refresh_turns(false, false);
        lvgl_unlock();
    }
}

// The popup page. The countdown text changes each second with the data tick.
// The caller holds the LVGL lock.
static void refresh_popup(bool force) {
    const coach_state_t st = s_flow.state;
    const popup_phase_t phase = st == COACH_STATE_POPUP_PLAY ? POPUP_PHASE_PLAY
                              : st == COACH_STATE_POPUP_HOLD ? POPUP_PHASE_HOLD
                                                             : POPUP_PHASE_WAIT;
    const int64_t left_us = s_popup_deadline_us - esp_timer_get_time();
    popup_view_t v;
    popup_view_build(&v, &s_popup, phase, left_us > 0 ? (uint32_t)(left_us / 1000) : 0);
    PUT("popup", s_popup_scr, s_popup_view, mode, v, force);
    PUT("popup", s_popup_scr, s_popup_view, dest, v, force);
    PUT("popup", s_popup_scr, s_popup_view, title, v, force);
    PUT("popup", s_popup_scr, s_popup_view, body, v, force);
    PUT("popup", s_popup_scr, s_popup_view, foot, v, force);
    PUT("popup", s_popup_scr, s_popup_view, hint, v, force);
    if (force) {
        // A new popup: the kind color, the icon, and the entry animation.
        s_popup_view.tone = v.tone;
        s_popup_view.icon = v.icon;
        const lv_color_t c = v.tone == POPUP_TONE_DANGER ? ui_color_danger()
                           : v.tone == POPUP_TONE_WARN   ? ui_color_warn()
                           : v.tone == POPUP_TONE_GOLD   ? ui_color_gold()
                                                         : ui_color_accent();
        if (s_popup_scr.mode_dot) lv_obj_set_style_bg_color(s_popup_scr.mode_dot, c, 0);
        if (s_popup_scr.hint_panel) lv_obj_set_style_bg_color(s_popup_scr.hint_panel, c, 0);
        if (s_popup_scr.dest) lv_obj_set_style_text_color(s_popup_scr.dest, c, 0);
        if (s_popup_scr.title) lv_obj_set_style_text_color(s_popup_scr.title, c, 0);
        popup_anim_start(&s_popup_scr, v.icon);
        ESP_LOGI("screen", "popup.icon = %u, entry animation", v.icon);
    }
}

// Copy the ski data texts into the screen that is on now. force: the screen is new.
// The caller holds the LVGL lock.
static void refresh_data(bool force) {
    if (s_shown == COACH_SCREEN_TURNS) {
        refresh_turns(force, false);
        return;
    }
    if (s_shown == COACH_SCREEN_POPUP) {
        refresh_popup(force);
        return;
    }
    if (s_shown != COACH_SCREEN_SKI && s_shown != COACH_SCREEN_REPORT) return;
    bp_live_stats_t live;
    link_report_t report;
    bool has_live, stale, has_report;
    uint32_t age;
    ble_link_get_data(&live, &has_live, &stale, &report, &has_report, &age);
    if (s_shown == COACH_SCREEN_SKI) {
        ski_view_t v;
        ski_view_build(&v, has_live ? &live : NULL, stale, has_report ? &report : NULL);
        PUT("ski", s_ski, s_ski_view, dest, v, force);
        PUT("ski", s_ski, s_ski_view, sub, v, force);
        PUT("ski", s_ski, s_ski_view, big, v, force);
        PUT("ski", s_ski, s_ski_view, unit, v, force);
        PUT("ski", s_ski, s_ski_view, drop, v, force);
        PUT("ski", s_ski, s_ski_view, dist, v, force);
        PUT("ski", s_ski, s_ski_view, turns, v, force);
        PUT("ski", s_ski, s_ski_view, time, v, force);
        PUT("ski", s_ski, s_ski_view, problem, v, force);
        PUT("ski", s_ski, s_ski_view, tip, v, force);
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
        PUT("report", s_report, s_report_view, title, v, force);
        PUT("report", s_report, s_report_view, iq, v, force);
        PUT("report", s_report, s_report_view, age, v, force);
        PUT("report", s_report, s_report_view, problem, v, force);
        PUT("report", s_report, s_report_view, tip, v, force);
    }
}

// D-33: one line for each page change, for the device memory test. The LVGL pool and the
// system heap are separate. A pool with a small biggest free block can hang a draw.
static void log_memory(const char *page) {
    lv_mem_monitor_t m;
    lv_mem_monitor(&m);
    ESP_LOGI("mem", "%s: lvgl used %u of %u B, biggest free %u B, peak %u B, heap free %u B, min %u B",
             page, (unsigned)(m.total_size - m.free_size), (unsigned)m.total_size,
             (unsigned)m.free_biggest_size, (unsigned)m.max_used,
             (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
             (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
}

// Only one screen exists at a time. All of them together do not fit the LVGL pool.
// Load the blank screen, delete the old screen, then build the new one. The peak stays
// at one screen. tools/host_render measures the same sequence.
// The subjects keep their values, so a new screen shows the current texts.
// Every screen binds the link state in its status bar (D-34).
// The caller holds the LVGL lock.
static void show_screen(coach_screen_id_t id) {
    if (id == s_shown || s_booting) return;    // D-35: boot_end() shows the screen of the state then.
    ESP_LOGI("screen", "show %s screen", screen_name(id));
    lv_screen_load(s_blank);
    switch (s_shown) {
    case COACH_SCREEN_COACH: coach_teardown(&s_coach); break;
    case COACH_SCREEN_SKI: coach_teardown_ski(&s_ski); break;
    case COACH_SCREEN_REPORT: coach_teardown_report(&s_report); break;
    case COACH_SCREEN_TURNS: coach_teardown_turns(&s_turns); break;
    case COACH_SCREEN_POPUP: coach_teardown_popup(&s_popup_scr); break;
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
        coach_bindings_attach_ski(&s_ski);
        lv_screen_load(s_ski.screen);
        break;
    case COACH_SCREEN_REPORT:
        coach_screen_report_build(&s_report);
        coach_bindings_attach_report(&s_report);
        lv_screen_load(s_report.screen);
        break;
    case COACH_SCREEN_TURNS:
        coach_screen_turns_build(&s_turns);
        coach_bindings_attach_turns(&s_turns);
        lv_screen_load(s_turns.screen);
        memset(&s_turn_view, 0, sizeof s_turn_view);
        turn_group_show(0);
        break;
    case COACH_SCREEN_POPUP:
        coach_screen_popup_build(&s_popup_scr);
        coach_bindings_attach_popup(&s_popup_scr);
        lv_screen_load(s_popup_scr.screen);
        break;
    default:
        break;
    }
    s_shown = id;
    refresh_data(true);
    log_memory(screen_name(id));
}

static void run_action(coach_action_t a) {
    if (a == COACH_ACTION_UI_SHOW_POPUP) {
        // A new popup. On the popup page already, write the new texts at once.
        taskENTER_CRITICAL(&s_popup_mux);
        s_popup = s_popup_in;
        taskEXIT_CRITICAL(&s_popup_mux);
        ESP_LOGI(TAG, "popup xid=0x%02x kind=%u audio=%u", s_popup.xid, s_popup.kind, s_popup.audio);
        if (s_shown == COACH_SCREEN_POPUP && lvgl_lock()) {
            refresh_popup(true);
            lvgl_unlock();
        }
        return;
    }
    if (coach_ui_apply(a)) return;
    switch (a) {
    case COACH_ACTION_POPUP_COUNTDOWN:
        s_popup_deadline_us = esp_timer_get_time() + POPUP_HOLD_MS * 1000LL;
        esp_timer_stop(s_popup_hold_timer);
        esp_timer_start_once(s_popup_hold_timer, POPUP_HOLD_MS * 1000LL);
        break;
    case COACH_ACTION_POPUP_CLOSE:
        ble_link_popup_end(s_event == COACH_EVENT_POPUP_TIMEOUT ? BP_POPUP_END_TIMEOUT : BP_POPUP_END_KEY);
        s_popup_close = true;
        break;
    case COACH_ACTION_AUDIO_PLAY_CUT: voice_play_cut(); break;
    case COACH_ACTION_RUN_AFTER_COACH: s_run_pending = true; break;
    case COACH_ACTION_POPUP_BACK_LIVE: s_popup_return = COACH_EVENT_RETURN_LIVE; break;
    case COACH_ACTION_AUDIO_CAPTURE_START: s_start_capture = true; break;
    case COACH_ACTION_TURN_NEXT:
    case COACH_ACTION_TURN_PREV:
    case COACH_ACTION_TURN_PAGE: {
        // D-30. Inside the Turns page: a short OK goes one turn on, an UP hold one turn back,
        // and a DOWN hold one page of 8 turns on. While the first page draws, OK only ends the drawing.
        const bool drawing = s_turn_view.drawing;
        s_turn_drawn = TURN_VIEW_DRAWN_ALL;
        const bool has = ble_link_get_turns(&s_turn_data);
        const uint8_t n = has ? s_turn_data.n_turns : 0;
        uint8_t sel = s_turn_sel;
        if (a == COACH_ACTION_TURN_NEXT && !drawing && n) sel = (uint8_t)((sel + 1u) % n);
        else if (a == COACH_ACTION_TURN_PREV && sel > 0) sel--;
        else if (a == COACH_ACTION_TURN_PAGE && !drawing && n > BP_GROUP_TURNS) {
            sel = (uint8_t)((sel / BP_GROUP_TURNS + 1u) * BP_GROUP_TURNS);
            if (sel >= n) sel = 0;
        }
        if (sel / BP_GROUP_TURNS != s_turn_sel / BP_GROUP_TURNS) ble_link_turns_focus(sel / BP_GROUP_TURNS);
        s_turn_sel = sel;
        if (s_shown == COACH_SCREEN_TURNS && lvgl_lock()) {
            refresh_turns(false, true);   // A replay of the turn.
            lvgl_unlock();
        }
        break;
    }
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
    // D-29 has no Ski or Rest mode. The mode field tells the phone whether the Coach page is on.
    const bool coach = st != COACH_STATE_LIVE && st != COACH_STATE_REPORT && st != COACH_STATE_TURNS && !is_popup(st);
    const uint8_t mode = coach ? BP_MODE_REST : BP_MODE_SKI;
    const uint8_t page = st == COACH_STATE_LIVE   ? BP_PAGE_SKI
                       : st == COACH_STATE_REPORT ? BP_PAGE_REPORT
                       : st == COACH_STATE_TURNS  ? BP_PAGE_TURNS
                       : is_popup(st)             ? BP_PAGE_POPUP
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
    // The watchdog covers the answer and the popup audio.
    const bool play_from = from == COACH_STATE_PLAYBACK || from == COACH_STATE_POPUP_PLAY;
    const bool play_to = to == COACH_STATE_PLAYBACK || to == COACH_STATE_POPUP_PLAY;
    if (play_from && (!play_to || from != to)) esp_timer_stop(s_play_watch);
    if (play_to && (!play_from || from != to)) esp_timer_start_periodic(s_play_watch, PLAY_WATCH_US);
    if (from == COACH_STATE_POPUP_WAIT) esp_timer_stop(s_popup_wait_timer);
    if (to == COACH_STATE_POPUP_WAIT && s_popup.audio) esp_timer_start_once(s_popup_wait_timer, POPUP_AUDIO_WAIT_MS * 1000LL);
    if (!is_popup(to)) esp_timer_stop(s_popup_hold_timer);
    if (to == COACH_STATE_CAPTURE && from != COACH_STATE_CAPTURE) {
        esp_timer_start_once(s_capture_timer, CAPTURE_LIMIT_US);
    }
    if (to == COACH_STATE_WAIT_ANSWER && from != COACH_STATE_WAIT_ANSWER) {
        esp_timer_start_once(s_answer_timer, AGENT_TIMEOUT_US);
    }
}

static void handle(coach_event_t event) {
    const coach_state_t before = s_flow.state;
    // The answer end and the play watchdog post ANSWER_END. In a popup, that is the end of the popup audio.
    if (event == COACH_EVENT_ANSWER_END && before == COACH_STATE_POPUP_PLAY) event = COACH_EVENT_POPUP_AUDIO_END;
    if (event == COACH_EVENT_POPUP_SHOW) {
        // Keep the page before the first popup. A new popup replaces the old one and keeps it.
        if (!is_popup(before)) {
            s_popup_return = before == COACH_STATE_LIVE   ? COACH_EVENT_RETURN_LIVE
                           : before == COACH_STATE_REPORT ? COACH_EVENT_RETURN_REPORT
                           : before == COACH_STATE_TURNS  ? COACH_EVENT_RETURN_TURNS
                                                          : COACH_EVENT_RETURN_READY;
            // D-32: the popup ends the talk with the coach, so a kept run start goes to the popup.
            if (s_run_pending) s_popup_return = COACH_EVENT_RETURN_LIVE;
            s_run_pending = false;
        }
        esp_timer_stop(s_popup_wait_timer);
        esp_timer_stop(s_popup_hold_timer);
        s_popup_no++;
    }
    // The guard: without voice, a question cannot start. The screen names the cause.
    // Voice needs two parts: the BLE link and the voice task.
    if (event == COACH_EVENT_OK_HOLD && s_flow.state == COACH_STATE_READY &&
        (s_voice != LINK_VOICE_OK || !voice_ready())) {
        coach_ui_apply(!voice_ready() ? COACH_ACTION_UI_WHY_AUDIO : why_action(s_voice));
        event = COACH_EVENT_ASK_BLOCKED;
    }
    s_event = event;
    s_start_capture = false;
    s_popup_close = false;
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
    if (step.state_changed && !s_booting && lvgl_lock()) {
        page_rail_follow(step.to, true);    // D-33: the rail moves with the page change.
        lvgl_unlock();
    }
    if (screen != s_shown && lvgl_lock()) {
        show_screen(screen);
        lvgl_unlock();
    } else if (screen == COACH_SCREEN_POPUP && step.state_changed && lvgl_lock()) {
        refresh_popup(false);   // The footer follows the popup phase.
        lvgl_unlock();
    }

    // The steps below run in this call, so that no queued event comes between them.
    // D-32: the talk with the coach ended, so the kept run start opens the live data page now.
    const bool talk_ended = step.to == COACH_STATE_READY || step.to == COACH_STATE_REPORT ||
                            step.to == COACH_STATE_TURNS || step.to == COACH_STATE_LIVE;
    const bool run_now = s_run_pending && talk_ended && step.to != COACH_STATE_LIVE;
    if (talk_ended) s_run_pending = false;
    if (s_popup_close) {
        handle(s_popup_return);
    } else if (event == COACH_EVENT_POPUP_SHOW && !s_popup.audio) {
        handle(COACH_EVENT_POPUP_NO_AUDIO);
    } else if (run_now) {
        handle(COACH_EVENT_RUN_START);
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

// D-35: leave the boot screen and show the page of the current state. The flow ran
// during the boot screen, so a popup or a run start that came then shows now.
static void boot_end(void) {
    if (!s_booting) return;
    esp_timer_stop(s_boot_timer);
    if (!lvgl_lock()) return;               // Try again with the next key or message.
    s_booting = false;
    lv_screen_load(s_blank);
    boot_screen_delete();
    show_screen(coach_state_screen(s_flow.state));
    page_rail_follow(s_flow.state, false);
    lvgl_unlock();
    ESP_LOGI(TAG, "boot screen ends in %s", coach_state_name(s_flow.state));
}

static void coach_task(void *arg) {
    (void)arg;
    msg_t m;
    for (;;) {
        // While a group of the Turns page draws, wake up for its next point.
        TickType_t wait = portMAX_DELAY;
        if (s_shown == COACH_SCREEN_TURNS && s_turn_view.drawing) {
            const TickType_t now = xTaskGetTickCount();
            wait = (TickType_t)(s_turn_tick - now) <= pdMS_TO_TICKS(TURN_POINT_MS) ? (TickType_t)(s_turn_tick - now) : 0;
        }
        if (xQueueReceive(s_queue, &m, wait) != pdTRUE) {
            turn_draw_step();
            continue;
        }
        switch (m.kind) {
        case MSG_EVENT:
            // A timer event of a replaced popup.
            if ((m.event == COACH_EVENT_POPUP_NO_AUDIO || m.event == COACH_EVENT_POPUP_TIMEOUT) && m.state != s_popup_no) break;
            handle(m.event);
            break;
        case MSG_LINK: on_link_msg(m.voice); break;
        case MSG_AGENT_STATE: on_agent_state_msg(m.state, m.failure); break;
        case MSG_DATA:
            if (lvgl_lock()) {
                refresh_data(false);
                lvgl_unlock();
            }
            break;
        case MSG_BOOT_DONE: boot_end(); break;
        case MSG_KEY: {
            // D-35: a key on the boot screen only skips it. The events of that key stop until it is released.
            if (s_booting && m.ev != BSP_BTN_RELEASE) {
                boot_end();
                s_boot_key = m.btn;
                break;
            }
            if ((int)m.btn == s_boot_key) {
                if (m.ev == BSP_BTN_RELEASE) s_boot_key = -1;
                break;
            }
            coach_event_t event;
            if (coach_input_map(&s_input, (coach_key_t)m.btn, (coach_key_ev_t)m.ev, &event)) handle(event);
            break;
        }
        }
        // Many messages in a row must not stop the drawing.
        if (s_shown == COACH_SCREEN_TURNS && s_turn_view.drawing && (int32_t)(xTaskGetTickCount() - s_turn_tick) >= 0) {
            turn_draw_step();
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
static void link_turn_shape(const link_turns_t *t) { (void)t; link_data(); }
// D-28: the phone opens the Report page. The flow accepts it in LIVE and READY only.
static void link_show_page(uint8_t page) {
    if (page == BP_PAGE_REPORT) post_event(COACH_EVENT_SHOW_REPORT);
}

// D-31: a popup. Copy it now: the link keeps only the last one.
static void link_popup(const link_popup_t *p) {
    taskENTER_CRITICAL(&s_popup_mux);
    s_popup_in = *p;
    taskEXIT_CRITICAL(&s_popup_mux);
    post_event(COACH_EVENT_POPUP_SHOW);
}

static void link_popup_play_start(void) {
    voice_down_open();
    post_event(COACH_EVENT_POPUP_AUDIO_START);
}

// D-32: the phone started a run. The link already holds it in the live data.
static void link_run_start(uint8_t run_no) {
    (void)run_no;
    post_event(COACH_EVENT_RUN_START);
    link_data();
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
    const esp_timer_create_args_t pwait = { .callback = on_popup_wait, .name = "popup_wait" };
    const esp_timer_create_args_t phold = { .callback = on_popup_hold, .name = "popup_hold" };
    const esp_timer_create_args_t boot = { .callback = on_boot_done, .name = "boot" };
    esp_err_t e = esp_timer_create(&cap, &s_capture_timer);
    if (e == ESP_OK) e = esp_timer_create(&ans, &s_answer_timer);
    if (e == ESP_OK) e = esp_timer_create(&watch, &s_play_watch);
    if (e == ESP_OK) e = esp_timer_create(&data, &s_data_tick);
    if (e == ESP_OK) e = esp_timer_create(&pwait, &s_popup_wait_timer);
    if (e == ESP_OK) e = esp_timer_create(&phold, &s_popup_hold_timer);
    if (e == ESP_OK) e = esp_timer_create(&boot, &s_boot_timer);
    if (e != ESP_OK) return e;

    coach_bindings_init();
    coach_bindings_set_lock(lvgl_lock, lvgl_unlock);
    coach_bindings_set_trace(trace_screen);
    if (!lvgl_lock()) return ESP_ERR_TIMEOUT;
    s_blank = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_blank, ui_color_bg(), 0);
    // D-33: the rail lives on the top layer, so a screen change does not delete it.
    // It stays hidden until the boot screen ends.
    page_rail_create(lv_layer_top());
    // D-35: the boot screen comes first. boot_end() shows the first page.
    s_booting = true;
    lv_screen_load(boot_screen_create());
    lvgl_unlock();

    s_queue = xQueueCreate(QUEUE_DEPTH, sizeof(msg_t));
    if (!s_queue) return ESP_ERR_NO_MEM;
    esp_timer_start_once(s_boot_timer, BOOT_SCREEN_MS * 1000LL);

    // Voice and link failures leave the screens and keys working. The guard then blocks questions.
    const voice_cb_t vcb = { .up_block = voice_up_block, .play_done = voice_play_done };
    esp_err_t ve = voice_start(&vcb);
    if (ve != ESP_OK) ESP_LOGE(TAG, "voice start failed: %s", esp_err_to_name(ve));
    // D-36: the boot sound plays with the boot animation. Its time line matches boot_screen.c.
    else voice_play_cue(heygo_boot_sound, HEYGO_BOOT_SOUND_SAMPLES);
    const ble_link_app_t app = {
        .voice_changed = link_voice_changed, .agent_state = link_agent_state,
        .play_start = link_play_start, .audio_down = link_audio_down, .play_end = link_play_end,
        .live_stats = link_live_stats, .run_report = link_run_report, .show_page = link_show_page,
        .turn_shape = link_turn_shape, .popup = link_popup, .popup_play_start = link_popup_play_start,
        .run_start = link_run_start,
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
