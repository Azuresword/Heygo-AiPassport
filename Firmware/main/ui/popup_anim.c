// main/ui/popup_anim.c
#include "popup_anim.h"

#include <math.h>

#include "heygo_popup_icons.h"

static uint8_t s_icon;   // The icon row that the frame loop shows.

void popup_anim_icon(coach_screen_popup_t *s, uint8_t icon, uint8_t frame) {
    if (!s->icon) return;
    if (icon >= HEYGO_POPUP_KINDS) icon = 1;
    s_icon = icon;
    lv_image_set_src(s->icon, &heygo_popup_icon[icon][frame % HEYGO_POPUP_FRAMES]);
}

static void set_frame(void *obj, int32_t v) {
    lv_image_set_src((lv_obj_t *)obj, &heygo_popup_icon[s_icon][(uint32_t)v % HEYGO_POPUP_FRAMES]);
}

static void set_y(void *obj, int32_t v) { lv_obj_set_style_translate_y((lv_obj_t *)obj, v, 0); }
static void set_x(void *obj, int32_t v) { lv_obj_set_style_translate_x((lv_obj_t *)obj, v, 0); }

// A damped shake: v goes from 0 to 1000. The offset ends at 0.
static void set_shake(void *obj, int32_t v) {
    const float t = (float)v / 1000.0f;
    const int32_t x = (int32_t)lroundf(6.0f * sinf(t * 6.0f * 3.14159265f) * (1.0f - t));
    lv_obj_set_style_translate_x((lv_obj_t *)obj, x, 0);
}

static void move(lv_obj_t *obj, lv_anim_exec_xcb_t cb, int32_t from, uint32_t ms, uint32_t delay, lv_anim_path_cb_t path) {
    if (!obj) return;
    lv_anim_delete(obj, cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_exec_cb(&a, cb);
    lv_anim_set_values(&a, from, 0);
    lv_anim_set_duration(&a, ms);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, path);
    lv_anim_set_early_apply(&a, true);   // The widget waits at the start offset during the delay.
    lv_anim_start(&a);
}

void popup_anim_start(coach_screen_popup_t *s, uint8_t icon) {
    popup_anim_icon(s, icon, 0);
    move(s->icon, set_y, -80, 520, 0, lv_anim_path_bounce);
    move(s->dest, set_x, -40, 300, 60, lv_anim_path_ease_out);
    move(s->body_panel, set_y, 48, 360, 120, lv_anim_path_overshoot);
    move(s->title, set_y, 48, 360, 120, lv_anim_path_overshoot);
    move(s->body, set_y, 48, 360, 120, lv_anim_path_overshoot);
    move(s->foot, set_y, 40, 300, 240, lv_anim_path_ease_out);
    move(s->hint_panel, set_y, 50, 300, 240, lv_anim_path_ease_out);
    move(s->hint, set_y, 50, 300, 240, lv_anim_path_ease_out);
    if (!s->icon) return;
    // The frame loop.
    lv_anim_delete(s->icon, set_frame);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s->icon);
    lv_anim_set_exec_cb(&a, set_frame);
    lv_anim_set_values(&a, 0, HEYGO_POPUP_FRAMES);
    lv_anim_set_duration(&a, POPUP_ICON_FRAME_MS * HEYGO_POPUP_FRAMES);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_path_cb(&a, lv_anim_path_linear);
    lv_anim_start(&a);
    // A fall shakes the icon after the bounce.
    lv_anim_delete(s->icon, set_shake);
    lv_obj_set_style_translate_x(s->icon, 0, 0);
    if (icon == 0) {
        lv_anim_t b;
        lv_anim_init(&b);
        lv_anim_set_var(&b, s->icon);
        lv_anim_set_exec_cb(&b, set_shake);
        lv_anim_set_values(&b, 0, 1000);
        lv_anim_set_duration(&b, 380);
        lv_anim_set_delay(&b, 520);
        lv_anim_set_early_apply(&b, false);
        lv_anim_start(&b);
    }
}
