#include "ui_state_icon.h"

#include "ui_theme.h"

typedef struct {
    const char *symbol;
    lv_color_t color;
} icon_spec_t;

static icon_spec_t icon_spec(int32_t icon)
{
    switch (icon) {
    case UI_STATE_ICON_LISTENING:
        return (icon_spec_t){ LV_SYMBOL_VOLUME_MAX, ui_color_ok() };
    case UI_STATE_ICON_THINKING:
        return (icon_spec_t){ LV_SYMBOL_REFRESH, ui_color_accent() };
    case UI_STATE_ICON_ANSWERING:
        return (icon_spec_t){ LV_SYMBOL_PLAY, ui_color_ok() };
    case UI_STATE_ICON_FAILED:
        return (icon_spec_t){ LV_SYMBOL_WARNING, ui_color_danger() };
    case UI_STATE_ICON_SKI:
        return (icon_spec_t){ LV_SYMBOL_GPS, ui_color_accent() };
    case UI_STATE_ICON_EXIT:
        return (icon_spec_t){ LV_SYMBOL_CLOSE, ui_color_danger() };
    case UI_STATE_ICON_LINK_LOST:
        return (icon_spec_t){ LV_SYMBOL_BLUETOOTH, ui_color_danger() };
    case UI_STATE_ICON_READY:
    default:
        return (icon_spec_t){ LV_SYMBOL_AUDIO, ui_color_accent() };
    }
}

lv_obj_t *ui_state_icon_create(lv_obj_t *parent, ui_state_icon_t icon)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(obj, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(obj, ui_color_surface_hi(), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 2, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);

    lv_obj_t *glyph = lv_label_create(obj);
    lv_obj_set_style_text_font(glyph, &lv_font_montserrat_20, 0);
    lv_obj_center(glyph);
    ui_state_icon_set(obj, icon);
    return obj;
}

void ui_state_icon_set(lv_obj_t *obj, int32_t icon)
{
    if (!obj) return;
    icon_spec_t spec = icon_spec(icon);
    lv_obj_set_style_border_color(obj, spec.color, 0);
    lv_obj_t *glyph = lv_obj_get_child(obj, 0);
    if (!glyph) return;
    lv_label_set_text(glyph, spec.symbol);
    lv_obj_set_style_text_color(glyph, spec.color, 0);
}
