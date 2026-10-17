// main/ui/turn_view.h
// The Turns page: the shape of a run and the metrics of one turn, from the last
// TURN_SHAPE (doc 07 section 6). Plain C, so host tests can link it.
// The Coach task copies the points into two LVGL lines and each text into its label.
//
// The Chinese texts here also appear in assets/ui/strings.json, so that the glyph
// check covers them: msg.no_turns, tag.schematic, fmt.turn_title.
#pragma once

#include <stdint.h>

#include "ble_proto.h"
#include "link_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// The page rounds the corners of the line TURN_VIEW_SMOOTH times (Chaikin). Each pass
// doubles the points. The line stays inside the corners of the packet points, and the
// first and last points stay where they are.
#define TURN_VIEW_SMOOTH   2
#define TURN_VIEW_PATH_MAX (BP_TURN_POINTS_MAX << TURN_VIEW_SMOOTH)

typedef struct { int16_t x, y; } turn_point_t;

typedef struct {
    uint8_t n_path;                              // The whole run. 0 when no shape arrived.
    turn_point_t path[TURN_VIEW_PATH_MAX];
    uint8_t turn_first;                          // The selected turn: path[turn_first] and the
    uint8_t n_turn;                              // n_turn - 1 points after it. 0 with no turns.
    char title[64];       // "第 4 趟 · 12 弯", or "暂无弯型数据"
    char counter[16];     // "TURN 3/12", or ""
    char side[16];        // "TOE SIDE", "HEEL SIDE", or "SIDE --"
    char metrics[40];     // "58 KM/H  56°  R 10 M". An unknown value shows "--".
    char source[16];      // "示意" for a schematic shape, else ""
} turn_view_t;

// The page draws downhill from left to right: screen x is the downhill axis (y in the
// packet), and screen y is the side axis (x in the packet).
// Fit the points into a box of box_w x box_h pixels, with margin pixels on each side.
// The aspect ratio stays. The shape is centered. Points are relative to the box.
// Turn k of the packet covers path[first << TURN_VIEW_SMOOTH] to path[((last + 1) << TURN_VIEW_SMOOTH) - 1].
// t can be NULL. sel is the index of the selected turn. A value out of range selects turn 0.
void turn_view_build(turn_view_t *v, const link_turns_t *t, uint8_t sel,
                     int16_t box_w, int16_t box_h, int16_t margin);

// The next turn index, with a wrap to the first turn. Returns 0 when the run has no turns.
uint8_t turn_view_next(const link_turns_t *t, uint8_t sel);

#ifdef __cplusplus
}
#endif
