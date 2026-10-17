// main/ui/ui_theme.h
// Shared palette for the 240x320 coach interface.
// The values come from the Heygo brand book in Figma (file d1RBbdR0K2sEScrmKP1efZ):
// a black base, #1A1A1A cards, white text, and the lime accent #B1FE2B.
// Success and danger also differ in lightness, not only in hue.
#pragma once

#include "lvgl.h"
#include "heygo_fonts.h"   // Chinese fonts from assets/fonts

#ifdef __cplusplus
extern "C" {
#endif

#define UI_HEX_BG          0x000000
#define UI_HEX_SURFACE     0x1A1A1A
#define UI_HEX_SURFACE_HI  0x2A2A2A
#define UI_HEX_INK         0xFFFFFF
#define UI_HEX_MUTED       0x94A3B8
#define UI_HEX_ACCENT      0xB1FE2B
#define UI_HEX_OK          0xB1FE2B
#define UI_HEX_DANGER      0xEF4444
#define UI_HEX_RIDGE       0x3A3A3A   // The mountain ridge lines.
#define UI_HEX_ACCENT_DIM  0x4A6A12   // The lime accent at low strength, for large shapes.

lv_color_t ui_color_bg(void);
lv_color_t ui_color_surface(void);
lv_color_t ui_color_surface_hi(void);
lv_color_t ui_color_ink(void);
lv_color_t ui_color_muted(void);
lv_color_t ui_color_accent(void);
lv_color_t ui_color_ok(void);
lv_color_t ui_color_danger(void);
lv_color_t ui_color_ridge(void);
lv_color_t ui_color_accent_dim(void);

#ifdef __cplusplus
}
#endif
