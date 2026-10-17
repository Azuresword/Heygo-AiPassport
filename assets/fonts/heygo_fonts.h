/* assets/fonts/heygo_fonts.h
 * Declares the generated Chinese fonts. assets/tools/gen_lv_font.py makes them.
 *
 *   lv_font_heygo_sc_16  Body text. ASCII, Chinese punctuation, GB2312 level 1,
 *                        and every vocabulary character. Use it for any text
 *                        that the phone sends.
 *   lv_font_heygo_sc_28  Large state words. ASCII and the vocabulary only.
 */
#pragma once

#include "lvgl.h"

LV_FONT_DECLARE(lv_font_heygo_sc_16)
LV_FONT_DECLARE(lv_font_heygo_sc_28)
