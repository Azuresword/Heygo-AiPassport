// main/ui/popup_anim.h
// D-31: the entry animation and the icon loop of the popup page.
//
// When a popup comes, the parts of the page move in:
//   - The icon falls from above and bounces (520 ms). Then its 4 frames play in a loop.
//   - A fall popup also shakes the icon from side to side after the bounce.
//   - The English word slides in from the left. The text card rises with a small overshoot.
//   - The countdown line and the key bar rise last.
// The animations use only position offsets, so LVGL needs no extra layer memory.
// Each animation belongs to its widget, so deleting the screen also stops it.
// The caller holds the LVGL lock.
#pragma once

#include <stdint.h>

#include "coach_screens.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POPUP_ANIM_MS      900   // All entry animations end within this time, the fall shake too.
#define POPUP_ICON_FRAME_MS 160  // Each icon frame shows this long.

// Show frame `frame` of icon row `icon` (0 fall, 1 voice, 2 problem, 3 achievement), with no animation.
void popup_anim_icon(coach_screen_popup_t *s, uint8_t icon, uint8_t frame);
// Start the entry animation and the icon loop. A new popup on the same page starts them again.
void popup_anim_start(coach_screen_popup_t *s, uint8_t icon);

#ifdef __cplusplus
}
#endif
