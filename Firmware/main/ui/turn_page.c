// main/ui/turn_page.c
#include "turn_page.h"

#include <string.h>

#include "heygo_sprites.h"
#include "ui_theme.h"

#define RIDER_HALF (HEYGO_SPRITE_SIZE / 2)

// lv_line keeps a pointer to its points, so they must outlive the call.
// The turn line points into the path, so the page needs one buffer.
static lv_point_precise_t s_path[TURN_VIEW_PATH_MAX];
static const lv_point_precise_t *s_turn;
static uint8_t s_n_turn;
static int32_t s_cum[TURN_VIEW_PATH_MAX];   // Length along the turn up to each point, in pixels.
static coach_screen_turns_t *s_screen;     // The screen that the animation moves the rider on.

static void set_hidden(lv_obj_t *o, bool hidden) {
    if (!o) return;
    if (hidden) lv_obj_add_flag(o, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(o, LV_OBJ_FLAG_HIDDEN);
}

// An integer square root is enough for the segment lengths on a 240 x 320 screen.
static int32_t isqrt(int32_t v) {
    int32_t r = 0;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}

// Put the rider with its center on x, y of the box, in frame `frame`.
// The line points are relative to the line object. The rider is a child of the screen.
static void put_rider(coach_screen_turns_t *s, int32_t x, int32_t y, uint8_t frame) {
    lv_image_set_src(s->rider, &heygo_board_sprite[frame % HEYGO_SPRITE_FRAMES]);
    lv_obj_set_pos(s->rider, lv_obj_get_x(s->turn) + x - RIDER_HALF, lv_obj_get_y(s->turn) + y - RIDER_HALF);
    set_hidden(s->rider, false);
}

void turn_page_apply(coach_screen_turns_t *s, const turn_view_t *v) {
    if (!s || !s->screen) return;
    s_screen = s;
    // A new view ends the old animation, so it cannot move the rider to the old turn.
    lv_anim_delete(s->rider, NULL);
    for (uint8_t i = 0; i < v->n_path; i++) s_path[i] = (lv_point_precise_t){ v->path[i].x, v->path[i].y };
    s_turn = &s_path[v->turn_first];
    s_n_turn = v->n_turn;
    s_cum[0] = 0;
    for (uint8_t i = 1; i < s_n_turn; i++) {
        const int32_t dx = s_turn[i].x - s_turn[i - 1].x, dy = s_turn[i].y - s_turn[i - 1].y;
        s_cum[i] = s_cum[i - 1] + isqrt(dx * dx + dy * dy);
    }
    lv_line_set_points(s->path, s_path, v->n_path);
    lv_line_set_points(s->turn, s_turn, v->n_turn);
    set_hidden(s->path, v->n_path == 0);
    set_hidden(s->turn, v->n_turn == 0);

    lv_label_set_text(s->title, v->title);
    lv_obj_set_style_text_color(s->title, v->n_path || v->drawing ? ui_color_ink() : ui_color_muted(), 0);
    lv_label_set_text(s->source, v->source);
    lv_label_set_text(s->side, v->side);
    lv_label_set_text(s->counter, v->counter);
    lv_label_set_text(s->metrics, v->metrics);
    lv_label_set_text(s->msg, v->msg);
    lv_label_set_text(s->key_up, v->key_up);
    lv_label_set_text(s->key_ok, v->key_ok);
    lv_label_set_text(s->key_down, v->key_down);
    lv_label_set_text(s->key_hold, v->key_hold);
    // The bar under the line: the whole run, and this group on it.
    set_hidden(s->track, !v->has_progress);
    set_hidden(s->thumb, !v->has_progress);
    if (v->has_progress) {
        lv_obj_set_x(s->thumb, lv_obj_get_x(s->turn) + v->prog_x);
        lv_obj_set_width(s->thumb, v->prog_w);
    }

    if (v->drawing) {
        if (v->has_head) put_rider(s, v->head.x, v->head.y, v->head_frame);
        else set_hidden(s->rider, true);
    } else if (s_n_turn) {
        turn_page_place_rider(s, 0);
    } else {
        set_hidden(s->rider, true);
    }
}

void turn_page_place_rider(coach_screen_turns_t *s, int32_t permille) {
    if (!s || !s->rider || s_n_turn == 0) return;
    if (permille < 0) permille = 0;
    if (permille > 1000) permille = 1000;
    const int32_t total = s_cum[s_n_turn - 1];
    const int32_t want = total * permille / 1000;
    uint8_t i = 1;
    while (i < s_n_turn - 1 && s_cum[i] < want) i++;
    int32_t x = s_turn[0].x, y = s_turn[0].y;
    uint8_t frame = 0;
    if (s_n_turn > 1) {
        const int32_t sx = s_turn[i].x - s_turn[i - 1].x, sy = s_turn[i].y - s_turn[i - 1].y;
        const int32_t seg = s_cum[i] - s_cum[i - 1];
        const int32_t into = want - s_cum[i - 1];
        x = s_turn[i - 1].x + (seg ? sx * into / seg : 0);
        y = s_turn[i - 1].y + (seg ? sy * into / seg : 0);
        frame = turn_view_frame(sx, sy);
    }
    put_rider(s, x, y, frame);
}

static void anim_cb(void *var, int32_t v) {
    (void)var;
    turn_page_place_rider(s_screen, v);
}

void turn_page_replay(coach_screen_turns_t *s) {
    if (!s || !s->rider || s_n_turn < 2) return;
    s_screen = s;
    // LVGL deletes the animation of an object when it deletes the object.
    lv_anim_delete(s->rider, anim_cb);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s->rider);
    lv_anim_set_exec_cb(&a, anim_cb);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_duration(&a, TURN_PAGE_REPLAY_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}
