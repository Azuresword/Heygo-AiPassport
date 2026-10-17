// main/ui/turn_view.h
// The Turns page: one group of 8 turns of the last run, and the metrics of one turn.
// The data comes from TURN_SHAPE and the cached TURN_GROUP packets (doc 07 section 6).
// Plain C, so host tests can link it. The Coach task copies the points into two LVGL
// lines and each text into its label.
//
// When the Turns page opens, the device draws the first page point by point: the Coach
// task raises `drawn` each 40 ms. Until the last point, the rider sprite sits at the head
// of the line, and the texts show the turn of the head. Later pages show at once.
//
// The Chinese texts here also appear in assets/ui/strings.json, so that the glyph
// check covers them: msg.no_turns, msg.loading, msg.turns_link_down, tag.schematic,
// fmt.turn_title, the key.* hints, and the hint.hold_* hints.
//
// D-30. Short UP and DOWN presses change the page, as on every page. Inside the page,
// OK goes one turn on, an UP hold one turn back, and a DOWN hold one page of 8 turns on.
// The page of 8 turns follows the selected turn.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ble_proto.h"
#include "link_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// The page rounds the corners of the line TURN_VIEW_SMOOTH times (Chaikin). Each pass
// doubles the points. The line stays inside the corners of the packet points, and the
// first and last points stay where they are.
#define TURN_VIEW_SMOOTH    2
#define TURN_VIEW_PATH_MAX  (BP_GROUP_POINTS_MAX << TURN_VIEW_SMOOTH)
// The rider sprite has this many frames. Frame 0 points right. Each next frame turns
// 360 / TURN_VIEW_FRAMES degrees clockwise on the screen.
#define TURN_VIEW_FRAMES    16
// drawn: every point of the group.
#define TURN_VIEW_DRAWN_ALL 255

typedef struct { int16_t x, y; } turn_point_t;

typedef struct {
    uint8_t n_path;                              // The line so far. 0 below two points.
    turn_point_t path[TURN_VIEW_PATH_MAX];
    uint8_t turn_first;                          // The marked turn: path[turn_first] and the
    uint8_t n_turn;                              // n_turn - 1 points after it. 0 with no turn.
    bool drawing;                                // The group still draws.
    bool has_head;                               // While drawing: the rider sits at head and
    turn_point_t head;                           // uses frame head_frame.
    uint8_t head_frame;
    bool has_progress;                           // More than one group: the bar under the line
    int16_t prog_x, prog_w;                      // marks this group, relative to the box.
    char title[64];       // "第 4 趟 · 44 弯", or "暂无弯型数据"
    char counter[16];     // "TURN 12/44", or ""
    char side[16];        // "TOE SIDE", "HEEL SIDE", or "SIDE --"
    char metrics[40];     // "58 KM/H  56°  R 10 M". An unknown value shows "--".
    char source[16];      // "示意" for a schematic shape, else ""
    char msg[48];         // In the box: "加载中" or "蓝牙断开" while the group is missing, else ""
    char key_up[24];      // The short presses: "▲报告"
    char key_ok[24];      // "●下一弯", "●跳过", or ""
    char key_down[24];    // "▼教练"
    char key_hold[64];    // The holds that work now: "长按 ▲上一弯 ▼下一页", or "长按●问教练"
} turn_view_t;

// The page draws downhill from left to right: screen x is the downhill axis (y in the
// packet), and screen y is the side axis (x in the packet). Packet values 0 to 255 fill the
// box of box_w x box_h pixels inside margin pixels on each side. The phone sets the scale.
// Points are relative to the box.
//
// t can be NULL. sel is the turn index in the run. The page shows group sel / 8.
// drawn is the number of points of that group on the screen, or TURN_VIEW_DRAWN_ALL.
// While drawing, the marked turn is the turn of the head, and sel only sets the group.
// link_up: the BLE link is up, so a missing group can still arrive.
void turn_view_build(turn_view_t *v, const link_turns_t *t, uint8_t sel, uint8_t drawn, bool link_up,
                     int16_t box_w, int16_t box_h, int16_t margin);

// The next turn index, with a wrap to the first turn. Returns 0 when the run has no turns.
uint8_t turn_view_next(const link_turns_t *t, uint8_t sel);

// The sprite frame for a move of dx, dy on the screen. A move of 0, 0 gives frame 0.
uint8_t turn_view_frame(int32_t dx, int32_t dy);

#ifdef __cplusplus
}
#endif
