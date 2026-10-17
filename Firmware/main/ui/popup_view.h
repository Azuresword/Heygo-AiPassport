// main/ui/popup_view.h
// D-31: the texts of the popup page, from the last POPUP and the popup phase.
// Plain C, so host tests can link it. The Coach task copies each text into its label.
//
// The Chinese texts here also appear in assets/ui/strings.json, so that the glyph
// check covers them: popup.*, hint.popup.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "link_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define POPUP_AUDIO_WAIT_MS 5000    // The popup audio must start within 5 s.
#define POPUP_HOLD_MS       10000   // The popup stays 10 s after its audio, or 10 s with no audio.

typedef enum {
    POPUP_PHASE_WAIT = 0,   // The popup audio did not start yet.
    POPUP_PHASE_PLAY,       // The popup audio plays.
    POPUP_PHASE_HOLD,       // The countdown runs.
} popup_phase_t;

// The color of the kind. The badge, the dot, the English word, the title, and the key bar use it.
typedef enum { POPUP_TONE_DANGER = 0, POPUP_TONE_ACCENT, POPUP_TONE_WARN, POPUP_TONE_GOLD } popup_tone_t;

typedef struct {
    popup_tone_t tone;                   // fall red, voice lime, problem orange, achievement gold
    uint8_t icon;                        // Row of heygo_popup_icon: the kind minus 1.
    char mode[16];                       // "摔倒提醒", "语音播报", "问题报告", "成就报告"
    char dest[16];                       // "FALL ALERT", "BROADCAST", "RUN ISSUE", "ACHIEVED"
    char title[BP_POPUP_TITLE_MAX + 1];  // The phone title, or a default title for the kind.
    char body[BP_POPUP_BODY_MAX + 1];
    char foot[32];                       // "等待语音", "正在播报", "8 秒后返回"
    char hint[24];                       // "按任意键关闭"
} popup_view_t;

// left_ms: the time left on the countdown. Only POPUP_PHASE_HOLD uses it.
void popup_view_build(popup_view_t *out, const link_popup_t *p, popup_phase_t phase, uint32_t left_ms);

#ifdef __cplusplus
}
#endif
