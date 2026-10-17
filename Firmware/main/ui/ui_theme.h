// main/ui/ui_theme.h
// Shared palette for the 240x320 coach interface.
// The dark neutral stack keeps glare low. Cyan is the only brand accent.
// Success and danger also differ in lightness, not only in hue.
#pragma once

#include "lvgl.h"
#include "heygo_fonts.h"   // Chinese fonts from assets/fonts

#ifdef __cplusplus
extern "C" {
#endif

#define UI_HEX_BG          0x071118
#define UI_HEX_SURFACE     0x10212B
#define UI_HEX_SURFACE_HI  0x17333E
#define UI_HEX_INK         0xF3F7F4
#define UI_HEX_MUTED       0x9AAEB8
#define UI_HEX_ACCENT      0x65DDE5
#define UI_HEX_OK          0x7BE2A8
#define UI_HEX_DANGER      0xFF7272

lv_color_t ui_color_bg(void);
lv_color_t ui_color_surface(void);
lv_color_t ui_color_surface_hi(void);
lv_color_t ui_color_ink(void);
lv_color_t ui_color_muted(void);
lv_color_t ui_color_accent(void);
lv_color_t ui_color_ok(void);
lv_color_t ui_color_danger(void);

#ifdef __cplusplus
}
#endif
