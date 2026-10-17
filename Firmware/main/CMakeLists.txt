# main.c runs the Coach app. The baseline demo files stay as BSP references.
# The Coach logic, the packet layer, and the codec are plain C and also run in host tests.
# The Chinese fonts come from ../../assets/fonts, which assets/tools/gen_lv_font.py generates.
set(HEYGO_FONTS "${CMAKE_CURRENT_LIST_DIR}/../../assets/fonts")

idf_component_register(
    SRCS "main.c"
         "coach_app.c"
         "coach_input.c"
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
         "generated/coach_flow.c"
         "generated/coach_screens.c"
         "generated/coach_bindings.c"
         "generated/coach_ui.c"
         "audio/play_gate.c"
         "audio/ima_adpcm.c"
         "protocol/ble_proto.c"
         "${HEYGO_FONTS}/heygo_sc_16.c"
         "${HEYGO_FONTS}/heygo_sc_28.c"
    INCLUDE_DIRS "." "ui" "generated" "audio" "protocol" "${HEYGO_FONTS}"
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
)
