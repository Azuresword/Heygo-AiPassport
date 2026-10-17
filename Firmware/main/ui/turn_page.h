// main/ui/turn_page.h
// Puts a turn_view_t on the Turns screen, and moves the rider sprite along the line.
// The Coach app and tools/host_render use it, so the mockups show the device drawing.
// Every call needs the LVGL lock. Only one Turns screen exists at a time.
#pragma once

#include <stdint.h>

#include "coach_screens.h"
#include "turn_view.h"

#ifdef __cplusplus
extern "C" {
#endif

// The shape box. It matches shape_panel in flows/coach.flow.json.
#define TURN_PAGE_BOX_W   220
#define TURN_PAGE_BOX_H   96
#define TURN_PAGE_MARGIN  10
#define TURN_PAGE_REPLAY_MS 1600

// Copy the lines and the texts. Hide a line with no points.
// While points arrive, put the rider at the head of the line. Else put it at the start of the turn.
void turn_page_apply(coach_screen_turns_t *s, const turn_view_t *v);
// Move the rider from the start to the end of the marked turn once.
void turn_page_replay(coach_screen_turns_t *s);
// Put the rider at permille (0 to 1000) of the marked turn, with no animation.
// The rider faces along the line at that place.
void turn_page_place_rider(coach_screen_turns_t *s, int32_t permille);

#ifdef __cplusplus
}
#endif
