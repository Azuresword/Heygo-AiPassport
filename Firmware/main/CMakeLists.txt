# main.c runs the Coach app. The baseline demo files stay as BSP references.
# The Coach logic, the packet layer, and the codec are plain C and also run in host tests.
# The Chinese fonts come from ../../assets/fonts, which assets/tools/gen_lv_font.py generates.
set(HEYGO_FONTS "${CMAKE_CURRENT_LIST_DIR}/../../assets/fonts")
# The sprites come from ../../assets/sprites: the rider of the Turns page (assets/tools/gen_sprite.py)
# and the popup icons (assets/tools/gen_popup_icons.py).
set(HEYGO_SPRITES "${CMAKE_CURRENT_LIST_DIR}/../../assets/sprites")

idf_component_register(
    SRCS "main.c"
         "coach_app.c"
         "coach_input.c"
         "link/link_core.c"
         "link/ble_link.c"
         "audio/voice.c"
         "demo_navigation.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_display.c"
         "demo_button.c"
         "demo_audio.c"
         "demo_battery.c"
         "demo_radio.c"
         "demo_wifi.c"
         "demo_ble.c"
         "demo_low_power.c"
         "ui/ui_theme.c"
         "ui/ui_state_icon.c"
         "ui/stats_fmt.c"
         "ui/ski_view.c"
         "ui/turn_view.c"
         "ui/turn_page.c"
         "ui/popup_view.c"
         "ui/popup_anim.c"
         "generated/coach_flow.c"
         "generated/coach_screens.c"
         "generated/coach_bindings.c"
         "generated/coach_ui.c"
         "audio/play_gate.c"
         "audio/play_envelope.c"
         "audio/ima_adpcm.c"
         "protocol/ble_proto.c"
         "${HEYGO_FONTS}/heygo_sc_16.c"
         "${HEYGO_FONTS}/heygo_sc_28.c"
         "${HEYGO_FONTS}/heygo_display_20.c"
         "${HEYGO_FONTS}/heygo_display_34.c"
         "${HEYGO_FONTS}/heygo_display_48.c"
         "${HEYGO_SPRITES}/heygo_board_sprite.c"
         "${HEYGO_SPRITES}/heygo_popup_icons.c"
    INCLUDE_DIRS "." "ui" "generated" "audio" "protocol" "link" "${HEYGO_FONTS}" "${HEYGO_SPRITES}"
    REQUIRES bsp bt esp_app_format esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
)
