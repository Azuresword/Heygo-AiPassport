// D-35: the boot screen. See boot_screen.h.
//
// Time line, in ms from power-on:
//     0 -  420  The snowboard carves in from the lower left.
//   180 -  500  "g" rises into place.  260 - 580  "o" rises into place.
//   420 -  740  "H" slides in from the left.  500 - 820 "e".  580 - 900 "y".
//  1000 - 1300  The tagline "MAKE EVERY RUN COUNT" rises. "COUNT" is lime.
//  1150 - 1550  A lime line grows under the tagline from its middle.
//  2000         The first page opens (coach_app.c).
#include "boot_screen.h"

#include "heygo_fonts.h"
#include "heygo_logo.h"
#include "ui_theme.h"

#define LOGO_X      ((240 - HEYGO_LOGO_WIDTH) / 2)
#define LOGO_Y      108
#define TAG_Y       214                 // The top of the tagline.
#define LINE_Y      242                 // The lime line under the tagline.
#define LINE_H      3
#define FADE_MS     320

// The tagline. assets/ui/strings.json has the same texts as boot.tagline (here with a space
// at the end) and boot.tagline_accent.
static const char TAG_TEXT[] = "MAKE EVERY RUN ";
static const char TAG_ACCENT[] = "COUNT";

// The entry of one part: where it starts, relative to its place, and when it moves.
typedef struct {
    int16_t dx, dy;
    uint16_t delay, ms;
} entry_t;

static const entry_t ENTRY[HEYGO_LOGO_PARTS] = {
    [HEYGO_LOGO_H] = { -16, 0, 420, FADE_MS },
    [HEYGO_LOGO_E] = { -16, 0, 500, FADE_MS },
    [HEYGO_LOGO_Y] = { -16, 0, 580, FADE_MS },
    [HEYGO_LOGO_G] = { 0, 12, 180, FADE_MS },
    [HEYGO_LOGO_O] = { 0, 12, 260, FADE_MS },
    [HEYGO_LOGO_BOARD] = { -60, 40, 0, 420 },
};
static const entry_t TAG_ENTRY = { 0, 8, 1000, 300 };

static lv_obj_t *s_screen;
static lv_obj_t *s_line;
static int32_t s_line_w;

// One animation for each part. v goes from 0 to 256: the offset goes to 0, and the
// opacity goes to full. The entry of the part is in the object user data.
static void step(void *obj, int32_t v) {
    lv_obj_t *o = obj;
    const entry_t *e = lv_obj_get_user_data(o);
    lv_obj_set_style_translate_x(o, e->dx * (256 - v) / 256, 0);
    lv_obj_set_style_translate_y(o, e->dy * (256 - v) / 256, 0);
    const lv_opa_t opa = (lv_opa_t)(v > 255 ? 255 : v);
    if (lv_obj_check_type(o, &lv_image_class)) lv_obj_set_style_image_opa(o, opa, 0);
    else lv_obj_set_style_text_opa(o, opa, 0);
}

static void enter(lv_obj_t *o, const entry_t *e) {
    lv_obj_set_user_data(o, (void *)e);
    step(o, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_exec_cb(&a, step);
    lv_anim_set_values(&a, 0, 256);
    lv_anim_set_delay(&a, e->delay);
    lv_anim_set_duration(&a, e->ms);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

// The line grows from the middle, so its x moves with its width.
static void grow(void *obj, int32_t w) {
    lv_obj_set_width(obj, w);
    lv_obj_set_x(obj, (240 - w) / 2);
}

static lv_obj_t *tag_label(lv_obj_t *parent, const char *text, lv_color_t color, int32_t x) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text_static(l, text);
    lv_obj_set_style_text_font(l, &lv_font_heygo_display_20, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_set_pos(l, x, TAG_Y);
    enter(l, &TAG_ENTRY);
    return l;
}

lv_obj_t *boot_screen_create(void) {
    if (s_screen) return s_screen;
    s_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(s_screen, lv_color_black(), 0);
    lv_obj_remove_flag(s_screen, LV_OBJ_FLAG_SCROLLABLE);

    // The board is the top layer, as in Figma. Its animation starts first all the same.
    static const int order[HEYGO_LOGO_PARTS] = { HEYGO_LOGO_H, HEYGO_LOGO_E, HEYGO_LOGO_Y,
                                                 HEYGO_LOGO_G, HEYGO_LOGO_O, HEYGO_LOGO_BOARD };
    for (int i = 0; i < HEYGO_LOGO_PARTS; i++) {
        const heygo_logo_part_t *p = &heygo_logo_parts[order[i]];
        lv_obj_t *img = lv_image_create(s_screen);
        lv_image_set_src(img, &p->img);
        lv_obj_set_style_image_recolor(img, lv_color_white(), 0);
        lv_obj_set_style_image_recolor_opa(img, LV_OPA_COVER, 0);
        lv_obj_set_pos(img, LOGO_X + p->x, LOGO_Y + p->y);
        enter(img, &ENTRY[order[i]]);
    }

    // The tagline is two labels, so "COUNT" can be lime. Together they are centered.
    const lv_font_t *f = &lv_font_heygo_display_20;
    lv_point_t s1, s2;
    lv_text_get_size(&s1, TAG_TEXT, f, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_text_get_size(&s2, TAG_ACCENT, f, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const int32_t w1 = s1.x, w2 = s2.x;
    const int32_t x = (240 - w1 - w2) / 2;
    tag_label(s_screen, TAG_TEXT, ui_color_ink(), x);
    tag_label(s_screen, TAG_ACCENT, ui_color_accent(), x + w1);

    s_line_w = w1 + w2;
    s_line = lv_obj_create(s_screen);
    lv_obj_remove_style_all(s_line);
    lv_obj_set_style_bg_color(s_line, ui_color_accent(), 0);
    lv_obj_set_style_bg_opa(s_line, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(s_line, LINE_H / 2, 0);
    lv_obj_set_size(s_line, 0, LINE_H);
    lv_obj_set_pos(s_line, 120, LINE_Y);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_line);
    lv_anim_set_exec_cb(&a, grow);
    lv_anim_set_values(&a, 0, s_line_w);
    lv_anim_set_delay(&a, 1150);
    lv_anim_set_duration(&a, 400);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    return s_screen;
}

void boot_screen_delete(void) {
    if (!s_screen) return;
    // Each animation belongs to its object. Delete them first, so no step runs on a freed object.
    for (uint32_t i = 0; i < lv_obj_get_child_count(s_screen); i++) {
        lv_anim_delete(lv_obj_get_child(s_screen, (int32_t)i), NULL);
    }
    lv_obj_delete(s_screen);
    s_screen = NULL;
    s_line = NULL;
}
