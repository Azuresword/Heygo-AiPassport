/* assets/fonts/heygo_fonts.h
 * Declares the generated Chinese fonts. assets/tools/gen_lv_font.py makes them.
 *
 *   lv_font_heygo_sc_16  Body text. ASCII, Chinese punctuation, GB2312 level 1,
 *                        and every vocabulary character. Use it for any text
 *                        that the phone sends.
 *   lv_font_heygo_sc_28  Large state words, Noto Sans SC Bold. ASCII and the vocabulary only.
 *   lv_font_heygo_display_20, lv_font_heygo_display_34, lv_font_heygo_display_48
 *                        Staatliches, the condensed display face of the brand book.
 *                        ASCII only: labels such as "AI COACH" and numbers.
 *                        The 48 px font shows the speed on the Ski page.
 */
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(lv_font_heygo_sc_16)
LV_FONT_DECLARE(lv_font_heygo_sc_28)
LV_FONT_DECLARE(lv_font_heygo_display_20)
LV_FONT_DECLARE(lv_font_heygo_display_34)
LV_FONT_DECLARE(lv_font_heygo_display_48)
