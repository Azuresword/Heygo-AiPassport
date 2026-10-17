/* assets/sprites/heygo_sprites.h
 * Declares the generated sprites. assets/tools/gen_sprite.py makes them.
 *
 *   heygo_board_sprite  The rider of the Turns page. 16 frames of 20 x 20 px.
 *                       Frame 0 points right. Each next frame turns 22.5 degrees
 *                       clockwise on the screen. */
#pragma once

#include "lvgl.h"

#define HEYGO_SPRITE_FRAMES 16
#define HEYGO_SPRITE_SIZE   20

extern const lv_image_dsc_t heygo_board_sprite[HEYGO_SPRITE_FRAMES];
