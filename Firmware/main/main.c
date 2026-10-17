// main/main.c —— Heygo AI Passport 入口:初始化外设,然后运行 Coach 流程。
//
// The BSP demo menu of the baseline (FoloToy/ai-passport 3d685a9) is not started.
// The demo_*.c files stay in the build as BSP references. Nothing calls them, so
// the linker removes them.
//
// Keys go to the Coach app. See coach_input.h for the map from key events to flow events.
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_pins.h"      // 错误日志里要打印 BSP_LCD_* 引脚号
#include "coach_app.h"
#include "key_guard.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

static const char *TAG = "main";
static volatile bool s_input_ready;

// Waking from deep sleep by key reboots the chip while that key is still held: the
// button component starts a few hundred milliseconds later, registers a press that
// is already down, and grades it a long press once it has been down for its
// threshold (500 ms here), so an unintended action fires almost immediately. Drop
// key actions until that press comes up; the deadline is only a safety net so a
// stuck key cannot leave the pad dead. The state machine lives in key_guard.h so it
// can be unit-tested without the rest of the demo.
#define KEY_GUARD_MAX_US (10LL * 1000 * 1000)
static key_guard_t s_key_guard;

// button callbacks run on the shared esp_timer task; enqueue only and return immediately.
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    // Consume the guard before the readiness check: a release can arrive after the
    // post-init voltage sample but before input dispatch is ready, and dropping it
    // here would leave the guard armed to swallow the next real press.
    const bool was_armed = s_key_guard.armed;
    if (key_guard_consume(&s_key_guard, ev == BSP_BTN_RELEASE, esp_timer_get_time())) {
        // The release and the deadline both arrive on this task; log the disarm
        // transition once it happens.
        if (was_armed && !s_key_guard.armed) {
            ESP_LOGI(TAG, "唤醒键已松手,按键恢复响应");
        }
        return;
    }
    if (!s_input_ready) return;
    coach_app_post_key(btn, ev);
}

void app_main(void) {
    ESP_LOGI(TAG, "Heygo AI Passport 启动");
    const uint32_t wake_causes = esp_sleep_get_wakeup_causes();
    if (wake_causes & ~(1u << ESP_SLEEP_WAKEUP_UNDEFINED)) {
        ESP_LOGI(TAG, "休眠唤醒原因位图: 0x%02x", (unsigned)wake_causes);
    }
    // 按键唤醒时,那颗键在整个启动期间还按着。先挡住它的动作(见 on_key)。
    if (wake_causes & (1u << ESP_SLEEP_WAKEUP_GPIO)) {
        key_guard_arm(&s_key_guard, esp_timer_get_time(), KEY_GUARD_MAX_US);
        ESP_LOGI(TAG, "按键唤醒:先屏蔽按键动作,直到那颗键松手");
    }

    bsp_i2c_init();
    bsp_i2c_scan();

    // The screen carries the whole interface. Without it the app cannot continue.
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败。"
                      "检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    esp_err_t e = coach_app_start();
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "Coach app start failed: %s", esp_err_to_name(e));
        return;
    }

    e = bsp_button_init(on_key, NULL);
    if (e != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败: %s", esp_err_to_name(e));
    } else if (s_key_guard.armed) {
        // 若唤醒键在按键驱动启动前就已松手,组件不会产生 release 事件,守卫会一直
        // 拦到兜底。初始化完成后按真实电压同步一次:已松手立即撤防。
        const bool released = bsp_button_read_mv() >= BSP_BTN_MV_RELEASED_MIN;
        key_guard_sync_released(&s_key_guard, released);
        if (!s_key_guard.armed) {
            ESP_LOGI(TAG, "唤醒键在按键驱动启动前已松手,提前撤防");
        }
    }
    s_input_ready = (e == ESP_OK);

    // Sprint 1 connects audio. Initialize it now so that the log shows a hardware fault early.
    const bool audio_ok = bsp_audio_init() == ESP_OK;
    const bool battery_ok = bsp_battery_init() == ESP_OK;
    ESP_LOGI(TAG, "就绪:Button=%d Audio=%d Battery=%d", s_input_ready, audio_ok, battery_ok);
}
