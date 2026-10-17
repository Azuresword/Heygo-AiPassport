// main/ui/turn_view.c
#include "turn_view.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define NO_TURNS   "暂无弯型数据"
#define LOADING    "加载中"
#define LINK_DOWN  "蓝牙断开"
#define SCHEMATIC  "示意"
#define KEY_UP     "▲报告"
#define KEY_NEXT   "●下一弯"
#define KEY_SKIP   "●跳过"
#define KEY_DOWN   "▼教练"
#define HOLD_PREV  "长按▲上一弯"
#define HOLD_PAGE  "长按▼下一页"
#define HOLD_COACH "长按●问教练"
#define HOLD_BOTH  "长按 ▲上一弯 ▼下一页"
#define SEP        " · "
#define UNKNOWN    "--"

static void put_u8(char *out, size_t cap, uint8_t v) {
    if (v == BP_U8_UNKNOWN) snprintf(out, cap, UNKNOWN);
    else snprintf(out, cap, "%u", v);
}

// One Chaikin pass in place: each segment a-b gives the points at 1/4 and 3/4. The ends stay.
// n points become 2n. The loop runs from the end, so it reads each point before it writes over it.
static uint8_t smooth(turn_point_t *p, uint8_t n) {
    p[2 * n - 1] = p[n - 1];
    for (int i = n - 2; i >= 0; i--) {
        const turn_point_t a = p[i], b = p[i + 1];
        p[2 * i + 1] = (turn_point_t){ (int16_t)((3 * a.x + b.x + 2) / 4), (int16_t)((3 * a.y + b.y + 2) / 4) };
        p[2 * i + 2] = (turn_point_t){ (int16_t)((a.x + 3 * b.x + 2) / 4), (int16_t)((a.y + 3 * b.y + 2) / 4) };
    }
    return (uint8_t)(2 * n);
}

// Packet values 0 to 255 fill the box inside the margin. Downhill (y) goes right.
static turn_point_t place(const uint8_t pt[2], int16_t box_w, int16_t box_h, int16_t margin) {
    const int32_t w = box_w - 2 * margin, h = box_h - 2 * margin;
    return (turn_point_t){ (int16_t)(margin + (pt[1] * w + 127) / 255), (int16_t)(margin + (pt[0] * h + 127) / 255) };
}

uint8_t turn_view_frame(int32_t dx, int32_t dy) {
    if (dx == 0 && dy == 0) return 0;
    const float step = 6.28318531f / TURN_VIEW_FRAMES;
    const int k = (int)lroundf(atan2f((float)dy, (float)dx) / step);
    return (uint8_t)((k % TURN_VIEW_FRAMES + TURN_VIEW_FRAMES) % TURN_VIEW_FRAMES);
}

// The texts of turn k of the group, and the mark on the part of it that the line already has.
static void put_turn(turn_view_t *v, const link_turns_t *t, const link_turn_group_t *g, uint8_t k) {
    const bp_turn_t *turn = &g->turns[k];
    snprintf(v->counter, sizeof v->counter, "TURN %u/%u", g->group * BP_GROUP_TURNS + k + 1u, t->n_turns);
    snprintf(v->side, sizeof v->side, "%s",
             turn->side == BP_SIDE_TOE ? "TOE SIDE" : turn->side == BP_SIDE_HEEL ? "HEEL SIDE" : "SIDE " UNKNOWN);
    char speed[8], edge[8], radius[8];
    put_u8(speed, sizeof speed, turn->speed_kmh);
    put_u8(edge, sizeof edge, turn->edge_deg);
    put_u8(radius, sizeof radius, turn->radius_m);
    snprintf(v->metrics, sizeof v->metrics, "%s KM/H  %s°  R %s M", speed, edge, radius);
    const unsigned first = (unsigned)turn->first << TURN_VIEW_SMOOTH;
    unsigned end = ((unsigned)turn->last + 1u) << TURN_VIEW_SMOOTH;
    if (end > v->n_path) end = v->n_path;
    if (end > first + 1u) {
        v->turn_first = (uint8_t)first;
        v->n_turn = (uint8_t)(end - first);
    }
}

void turn_view_build(turn_view_t *v, const link_turns_t *t, uint8_t sel, uint8_t drawn, bool link_up,
                     int16_t box_w, int16_t box_h, int16_t margin) {
    memset(v, 0, sizeof *v);
    snprintf(v->side, sizeof v->side, "SIDE " UNKNOWN);
    // D-30. Short UP and DOWN presses change the page, as on every page. The holds and OK act
    // inside the page.
    snprintf(v->key_up, sizeof v->key_up, KEY_UP);
    snprintf(v->key_down, sizeof v->key_down, KEY_DOWN);
    snprintf(v->key_hold, sizeof v->key_hold, HOLD_COACH);
    if (!t) {
        snprintf(v->title, sizeof v->title, NO_TURNS);
        return;
    }
    char no[8];
    put_u8(no, sizeof no, t->run_no);
    snprintf(v->title, sizeof v->title, "第 %s 趟" SEP "%u 弯", no, t->n_turns);
    if (t->source == BP_TURN_SRC_SCHEMATIC) snprintf(v->source, sizeof v->source, SCHEMATIC);
    if (t->n_turns == 0) return;

    if (sel >= t->n_turns) sel = 0;
    const uint8_t group = sel / BP_GROUP_TURNS;
    // The second row names the holds that work now. With none, it names the global OK hold.
    if (sel > 0 && t->n_groups > 1) snprintf(v->key_hold, sizeof v->key_hold, HOLD_BOTH);
    else if (sel > 0) snprintf(v->key_hold, sizeof v->key_hold, HOLD_PREV);
    else if (t->n_groups > 1) snprintf(v->key_hold, sizeof v->key_hold, HOLD_PAGE);
    if (t->n_groups > 1) {
        const int32_t w = box_w - 2 * margin;
        v->has_progress = true;
        v->prog_x = (int16_t)(margin + w * group / t->n_groups);
        v->prog_w = (int16_t)(w / t->n_groups > 4 ? w / t->n_groups : 4);
    }

    snprintf(v->key_ok, sizeof v->key_ok, KEY_NEXT);
    const link_turn_group_t *g = link_turns_group(t, group);
    if (!g) {
        snprintf(v->msg, sizeof v->msg, "%s", link_up ? LOADING : LINK_DOWN);
        return;
    }
    const uint8_t n = drawn < g->n_points ? drawn : g->n_points;
    v->drawing = n < g->n_points;
    if (v->drawing) snprintf(v->key_ok, sizeof v->key_ok, KEY_SKIP);
    for (uint8_t i = 0; i < n; i++) v->path[i] = place(g->points[i], box_w, box_h, margin);
    if (n >= 1 && v->drawing) {
        v->has_head = true;
        v->head = v->path[n - 1];
        if (n >= 2) v->head_frame = turn_view_frame(v->head.x - v->path[n - 2].x, v->head.y - v->path[n - 2].y);
    }
    if (n >= 2) {
        uint8_t m = n;
        for (int pass = 0; pass < TURN_VIEW_SMOOTH; pass++) m = smooth(v->path, m);
        v->n_path = m;
    }

    if (v->drawing) {
        // The turn of the head: the last turn that starts before the last point.
        int k = -1;
        for (uint8_t i = 0; n >= 2 && i < g->n_turns; i++) {
            if (g->turns[i].first < n - 1) k = i;
        }
        if (k >= 0) put_turn(v, t, g, (uint8_t)k);
        return;
    }
    put_turn(v, t, g, (uint8_t)(sel - group * BP_GROUP_TURNS));
}

uint8_t turn_view_next(const link_turns_t *t, uint8_t sel) {
    if (!t || t->n_turns == 0) return 0;
    return (uint8_t)((sel + 1u) % t->n_turns);
}
