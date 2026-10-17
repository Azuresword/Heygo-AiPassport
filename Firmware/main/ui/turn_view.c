// main/ui/turn_view.c
#include "turn_view.h"

#include <stdio.h>
#include <string.h>

#define NO_TURNS   "暂无弯型数据"
#define SCHEMATIC  "示意"
#define SEP        " · "
#define UNKNOWN    "--"

static void put_u8(char *out, size_t cap, uint8_t v) {
    if (v == BP_U8_UNKNOWN) snprintf(out, cap, UNKNOWN);
    else snprintf(out, cap, "%u", v);
}

// One axis: map v from [lo, lo + span] into the box. num / den is the shared scale.
static int16_t place(uint8_t v, uint8_t lo, int32_t num, int32_t den, int32_t offset) {
    return (int16_t)(offset + ((int32_t)(v - lo) * num) / den);
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

void turn_view_build(turn_view_t *v, const link_turns_t *t, uint8_t sel,
                     int16_t box_w, int16_t box_h, int16_t margin) {
    memset(v, 0, sizeof *v);
    if (!t || t->n_points < BP_TURN_POINTS_MIN) {
        snprintf(v->title, sizeof v->title, NO_TURNS);
        snprintf(v->side, sizeof v->side, "SIDE " UNKNOWN);
        return;
    }

    // The page draws downhill from left to right, so the wide box holds a long run.
    // Screen x comes from the downhill axis (y), and screen y from the side axis (x).
    // The bounding box of the points, on the screen axes.
    uint8_t min_x = 255, min_y = 255, max_x = 0, max_y = 0;
    for (uint8_t i = 0; i < t->n_points; i++) {
        const uint8_t x = t->points[i][1], y = t->points[i][0];
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }
    const int32_t span_x = max_x - min_x, span_y = max_y - min_y;
    const int32_t avail_w = box_w - 2 * margin, avail_h = box_h - 2 * margin;

    // One scale for both axes keeps the aspect ratio. The tighter axis sets it.
    int32_t num = 1, den = 1;
    if (span_x == 0 && span_y == 0) {
        num = 0;
    } else if (span_y == 0 || (span_x > 0 && span_x * avail_h > span_y * avail_w)) {
        num = avail_w; den = span_x;
    } else {
        num = avail_h; den = span_y;
    }
    const int32_t used_w = span_x * num / den, used_h = span_y * num / den;
    const int32_t off_x = margin + (avail_w - used_w) / 2, off_y = margin + (avail_h - used_h) / 2;

    for (uint8_t i = 0; i < t->n_points; i++) {
        v->path[i].x = place(t->points[i][1], min_x, num, den, off_x);
        v->path[i].y = place(t->points[i][0], min_y, num, den, off_y);
    }
    uint8_t n = t->n_points;
    for (int pass = 0; pass < TURN_VIEW_SMOOTH; pass++) n = smooth(v->path, n);
    v->n_path = n;

    char no[8];
    put_u8(no, sizeof no, t->run_no);
    snprintf(v->title, sizeof v->title, "第 %s 趟" SEP "%u 弯", no, t->n_turns);
    if (t->source == BP_TURN_SRC_SCHEMATIC) snprintf(v->source, sizeof v->source, SCHEMATIC);

    if (t->n_turns == 0) {
        snprintf(v->side, sizeof v->side, "SIDE " UNKNOWN);
        return;
    }
    if (sel >= t->n_turns) sel = 0;
    const bp_turn_t *turn = &t->turns[sel];
    v->turn_first = (uint8_t)(turn->first << TURN_VIEW_SMOOTH);
    v->n_turn = (uint8_t)((turn->last + 1 - turn->first) << TURN_VIEW_SMOOTH);

    snprintf(v->counter, sizeof v->counter, "TURN %u/%u", sel + 1, t->n_turns);
    snprintf(v->side, sizeof v->side, "%s",
             turn->side == BP_SIDE_TOE ? "TOE SIDE" : turn->side == BP_SIDE_HEEL ? "HEEL SIDE" : "SIDE " UNKNOWN);
    char speed[8], edge[8], radius[8];
    put_u8(speed, sizeof speed, turn->speed_kmh);
    put_u8(edge, sizeof edge, turn->edge_deg);
    put_u8(radius, sizeof radius, turn->radius_m);
    snprintf(v->metrics, sizeof v->metrics, "%s KM/H  %s°  R %s M", speed, edge, radius);
}

uint8_t turn_view_next(const link_turns_t *t, uint8_t sel) {
    if (!t || t->n_turns == 0) return 0;
    return (uint8_t)((sel + 1u) % t->n_turns);
}
