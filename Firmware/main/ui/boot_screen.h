// D-35: the boot screen. The Heygo logo comes in part by part, and then the tagline.
// It plays before the first page, after each power-on. Any key skips it.
// The power key cuts the power in hardware, and the firmware cannot read it. So the
// device has no shutdown animation.
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_SCREEN_MS 2000     // The time from power-on to the first page.

// Build the boot screen and start its animations. Load the returned screen.
// The animations use only position offsets and image or text opacity, so LVGL needs
// no layer. The caller holds the LVGL lock.
lv_obj_t *boot_screen_create(void);

// Delete the boot screen and its animations. Load another screen first.
// The caller holds the LVGL lock.
void boot_screen_delete(void);

#ifdef __cplusplus
}
#endif
