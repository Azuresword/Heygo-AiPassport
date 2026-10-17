#pragma once

#include <stdint.h>

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UI_STATE_ICON_READY = 0,
    UI_STATE_ICON_LISTENING,
    UI_STATE_ICON_THINKING,
    UI_STATE_ICON_ANSWERING,
    UI_STATE_ICON_FAILED,
    UI_STATE_ICON_SKI,
    UI_STATE_ICON_EXIT,
    UI_STATE_ICON_LINK_LOST,
} ui_state_icon_t;

lv_obj_t *ui_state_icon_create(lv_obj_t *parent, ui_state_icon_t icon);
void ui_state_icon_set(lv_obj_t *obj, int32_t icon);

#ifdef __cplusplus
}
#endif
