// main/coach_app.h
// The Coach app: it runs the generated flow on the device.
//
// One task owns the flow. Key events and device timers reach it through one queue,
// so the flow sees one event at a time.
#pragma once

#include "bsp_button.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Build the screens, show the initial screen, and start the Coach task.
// Call after bsp_lvgl_init(). Do not hold the LVGL lock.
esp_err_t coach_app_start(void);

// Queue one key event. The button callback can call this. It never blocks.
void coach_app_post_key(bsp_btn_t btn, bsp_btn_ev_t ev);

#ifdef __cplusplus
}
#endif
