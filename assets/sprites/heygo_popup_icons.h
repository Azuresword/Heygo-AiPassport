/* assets/sprites/heygo_popup_icons.h
 * Declares the icons of the popup page. assets/tools/gen_popup_icons.py makes them.
 *
 *   heygo_popup_icon[kind - 1][frame]  4 kinds (fall, voice, issue, badge), 4 frames each,
 *                                     56 x 56 px. Show the frames in a loop. */
#pragma once

#include "lvgl.h"

#define HEYGO_POPUP_KINDS  4
#define HEYGO_POPUP_FRAMES 4
#define HEYGO_POPUP_ICON   56

extern const lv_image_dsc_t heygo_popup_icon[HEYGO_POPUP_KINDS][HEYGO_POPUP_FRAMES];
