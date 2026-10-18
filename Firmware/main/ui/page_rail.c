// D-33: the page rail. See page_rail.h.
#include "page_rail.h"

#include "ui_theme.h"

#define STOP_X     3            // The stops are 2 px wide, in the middle of the rail.
#define STOP_W     2
#define THUMB_X    1            // The thumb is 6 px wide.
#define THUMB_W    6
#define HALO       1            // A dim lime edge around the thumb. A real shadow needs a pool buffer.
#define RAIL_H     (PAGE_RAIL_STEP * (PAGE_RAIL_PAGES - 1) + PAGE_RAIL_STOP)

static lv_obj_t *s_rail;
static int s_page = -1, s_up = -1, s_down = -1;
static int32_t s_thumb_y;       // The top of the thumb, from the top of the rail.
static int32_t s_wrap_to;       // During a wrap: the stop after the jump. Else -1.

static void rect(lv_layer_t *layer, const lv_area_t *base, int32_t x, int32_t y, int32_t w, int32_t h,
                 lv_color_t color, lv_opa_t opa, int32_t radius) {
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.bg_color = color;
    d.bg_opa = opa;
    d.radius = radius;
    const lv_area_t a = { base->x1 + x, base->y1 + y, base->x1 + x + w - 1, base->y1 + y + h - 1 };
    lv_draw_rect(layer, &d, &a);
}

static void on_draw(lv_event_t *e) {
    lv_layer_t *layer = lv_event_get_layer(e);
    lv_area_t base;
    lv_obj_get_coords(lv_event_get_target_obj(e), &base);
    for (int i = 0; i < PAGE_RAIL_PAGES; i++) {
        const bool target = i == s_up || i == s_down;
        rect(layer, &base, STOP_X, i * PAGE_RAIL_STEP, STOP_W, PAGE_RAIL_STOP,
             target ? ui_color_muted() : lv_color_hex(0x3A3A3A), LV_OPA_COVER, 1);
    }
    rect(layer, &base, THUMB_X - HALO, s_thumb_y - HALO, THUMB_W + 2 * HALO, PAGE_RAIL_STOP + 2 * HALO,
         ui_color_accent(), LV_OPA_40, 4);
    rect(layer, &base, THUMB_X, s_thumb_y, THUMB_W, PAGE_RAIL_STOP, ui_color_accent(), LV_OPA_COVER, 3);
}

static void set_thumb(void *obj, int32_t y) {
    s_thumb_y = y;
    lv_obj_invalidate((lv_obj_t *)obj);
}

static void slide(int32_t from, int32_t to, uint32_t ms, lv_anim_completed_cb_t done) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_rail);
    lv_anim_set_exec_cb(&a, set_thumb);
    lv_anim_set_values(&a, from, to);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    if (done) lv_anim_set_completed_cb(&a, done);
    lv_anim_start(&a);
}

// The second half of a wrap: come in from the other end of the rail.
static void wrap_in(lv_anim_t *a) {
    (void)a;
    const int32_t to = s_wrap_to * PAGE_RAIL_STEP;
    const int32_t from = s_wrap_to == 0 ? -PAGE_RAIL_STEP : RAIL_H + HALO;
    s_wrap_to = -1;
    slide(from, to, PAGE_RAIL_MS / 2, NULL);
}

void page_rail_create(lv_obj_t *parent) {
    if (s_rail) return;
    s_rail = lv_obj_create(parent);
    lv_obj_remove_style_all(s_rail);
    lv_obj_set_pos(s_rail, PAGE_RAIL_X, PAGE_RAIL_Y);
    lv_obj_set_size(s_rail, PAGE_RAIL_W, RAIL_H);
    lv_obj_remove_flag(s_rail, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_rail, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(s_rail, on_draw, LV_EVENT_DRAW_MAIN, NULL);
    s_page = s_up = s_down = s_wrap_to = -1;
}

int page_rail_index(coach_screen_id_t screen) {
    switch (screen) {
    case COACH_SCREEN_SKI: return 0;
    case COACH_SCREEN_REPORT: return 1;
    case COACH_SCREEN_TURNS: return 2;
    case COACH_SCREEN_COACH: return 3;
    default: return -1;
    }
}

// The rail index of the page that this short press opens. -1: the press changes no page.
static int target(coach_state_t state, coach_event_t event) {
    coach_flow_t probe = { .state = state };
    const coach_step_t step = coach_flow_step(&probe, event);
    return step.handled ? page_rail_index(coach_state_screen(step.to)) : -1;
}

void page_rail_follow(coach_state_t state, bool anim) {
    if (!s_rail) return;
    const int page = page_rail_index(coach_state_screen(state));
    if (page < 0) {
        lv_obj_add_flag(s_rail, LV_OBJ_FLAG_HIDDEN);
        s_page = -1;                // After the popup, the thumb appears in place.
        return;
    }
    const int up = target(state, COACH_EVENT_UP_SHORT), down = target(state, COACH_EVENT_DOWN_SHORT);
    const bool moved = page != s_page;
    if (!moved && up == s_up && down == s_down) return;
    s_up = up;
    s_down = down;
    lv_obj_remove_flag(s_rail, LV_OBJ_FLAG_HIDDEN);
    if (moved) {
        lv_anim_delete(s_rail, set_thumb);
        s_wrap_to = -1;
        const int last = PAGE_RAIL_PAGES - 1;
        const bool wrap_down = s_page == last && page == 0, wrap_up = s_page == 0 && page == last;
        if (!anim || s_page < 0) {
            s_thumb_y = page * PAGE_RAIL_STEP;
        } else if (wrap_down || wrap_up) {
            // Across the end of the loop: slide out past one end, and come in from the other end.
            s_wrap_to = page;
            slide(s_thumb_y, wrap_down ? RAIL_H + HALO : -PAGE_RAIL_STEP, PAGE_RAIL_MS / 2, wrap_in);
        } else {
            slide(s_thumb_y, page * PAGE_RAIL_STEP, PAGE_RAIL_MS, NULL);
        }
        s_page = page;
    }
    lv_obj_invalidate(s_rail);
}
