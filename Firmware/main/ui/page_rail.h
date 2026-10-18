// D-33: the page rail on the right edge. It shows the current page, and the pages that a
// short UP press and a short DOWN press open. One LVGL object on the top layer draws the
// four stops and the thumb, so the rail costs one object in the pool on every page.
#pragma once

#include <stdbool.h>

#include "coach_flow.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PAGE_RAIL_PAGES 4
#define PAGE_RAIL_X     231
#define PAGE_RAIL_Y     64
#define PAGE_RAIL_W     8
#define PAGE_RAIL_STEP  48      // One stop is 44 px high, with a 4 px gap.
#define PAGE_RAIL_STOP  44
#define PAGE_RAIL_MS    180     // The slide to the next stop. A wrap uses half out and half in.

// Make the rail object on parent, normally lv_layer_top(). It starts hidden.
void page_rail_create(lv_obj_t *parent);

// The rail index of a screen, top to bottom: live data 0, report 1, turns 2, coach 3.
// -1 for a screen that is not in the loop (the popup page).
int page_rail_index(coach_screen_id_t screen);

// Show the rail for this state. The targets come from the flow graph: the screens of the
// short UP and short DOWN transitions. anim false puts the thumb in place at once.
// The caller holds the LVGL lock.
void page_rail_follow(coach_state_t state, bool anim);

#ifdef __cplusplus
}
#endif
