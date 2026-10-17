// main/link/ble_link.h
// The BLE link on the device: NimBLE, the Nordic UART Service layout of doc 07,
// and the protocol session in link_core.
//
// Lock order: the link lock comes before the LVGL lock. The app callbacks run
// inside the link lock. They can take the LVGL lock, but they must not call any
// ble_link_* function.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "link_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void (*voice_changed)(link_voice_t voice);
    void (*agent_state)(uint8_t state, uint8_t failure, const char *text);
    void (*play_start)(void);
    void (*audio_down)(const uint8_t *block, size_t len, uint8_t missing);
    void (*play_end)(void);
    // Increment 2 data. Each call runs on the NimBLE host task.
    void (*live_stats)(const bp_live_stats_t *s);
    void (*run_report)(const link_report_t *r);
    void (*show_page)(uint8_t page);
    // Increment 3 data: the turn shape of a run.
    void (*turn_shape)(const link_turns_t *t);
} ble_link_app_t;

// Start NimBLE, register the service, and advertise. Call once.
esp_err_t ble_link_start(const ble_link_app_t *app);

link_voice_t ble_link_voice(void);
uint8_t ble_link_agent_start(void);
void ble_link_audio_up(const uint8_t *block);
void ble_link_agent_end(uint8_t reason);
void ble_link_agent_cancel(void);
void ble_link_agent_done(void);
// Copy the last ski data under the link lock. live or report can be NULL.
// Each flag is false when that data never arrived.
void ble_link_get_data(bp_live_stats_t *live, bool *has_live, bool *stale,
                       link_report_t *report, bool *has_report, uint32_t *report_age_ms);
// Copy the last turn shape and its cached groups under the link lock. Returns false when none arrived.
bool ble_link_get_turns(link_turns_t *out);
// The Turns page shows this group. The link asks the phone for the groups next to it.
void ble_link_turns_focus(uint8_t group);
// The mode and page for CONTEXT and DEVICE_STATUS (doc 07 field values).
void ble_link_set_context(uint8_t mode, uint8_t page);

#ifdef __cplusplus
}
#endif
