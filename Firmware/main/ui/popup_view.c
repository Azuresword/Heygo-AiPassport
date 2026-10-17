// main/ui/popup_view.c
#include "popup_view.h"

#include <stdio.h>
#include <string.h>

static void copy(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src ? src : ""); }

typedef struct { const char *mode, *dest, *title; popup_tone_t tone; } kind_text_t;

static const kind_text_t KINDS[] = {
    [BP_POPUP_FALL]  = { "摔倒提醒", "FALL ALERT", "检测到摔倒", POPUP_TONE_DANGER },
    [BP_POPUP_VOICE] = { "语音播报", "BROADCAST", "手机播报", POPUP_TONE_ACCENT },
    [BP_POPUP_ISSUE] = { "问题报告", "RUN ISSUE", "发现问题", POPUP_TONE_WARN },
    [BP_POPUP_BADGE] = { "成就报告", "ACHIEVED", "获得成就", POPUP_TONE_GOLD },
};

void popup_view_build(popup_view_t *out, const link_popup_t *p, popup_phase_t phase, uint32_t left_ms) {
    memset(out, 0, sizeof *out);
    const uint8_t kind = p && p->kind >= BP_POPUP_FALL && p->kind <= BP_POPUP_BADGE ? p->kind : BP_POPUP_VOICE;
    const kind_text_t *k = &KINDS[kind];
    out->tone = k->tone;
    out->icon = (uint8_t)(kind - BP_POPUP_FALL);
    copy(out->mode, sizeof out->mode, k->mode);
    copy(out->dest, sizeof out->dest, k->dest);
    copy(out->title, sizeof out->title, p && p->title[0] ? p->title : k->title);
    copy(out->body, sizeof out->body, p ? p->body : "");
    copy(out->hint, sizeof out->hint, "按任意键关闭");
    if (phase == POPUP_PHASE_WAIT) {
        copy(out->foot, sizeof out->foot, "等待语音");
    } else if (phase == POPUP_PHASE_PLAY) {
        copy(out->foot, sizeof out->foot, "正在播报");
    } else {
        // Round up: the screen shows "10" for the whole first second, and "1" for the last one.
        const unsigned s = (unsigned)((left_ms + 999u) / 1000u);
        snprintf(out->foot, sizeof out->foot, "%u 秒后返回", s ? s : 1u);
    }
}
