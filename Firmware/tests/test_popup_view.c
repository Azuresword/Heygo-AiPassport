// tests/test_popup_view.c
// Host test for main/ui/popup_view.c.
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "popup_view.h"

static link_popup_t make(uint8_t kind, const char *title, const char *body) {
    link_popup_t p;
    memset(&p, 0, sizeof p);
    p.xid = 0x80;
    p.kind = kind;
    snprintf(p.title, sizeof p.title, "%s", title);
    snprintf(p.body, sizeof p.body, "%s", body);
    return p;
}

int main(void) {
    popup_view_t v;
    link_popup_t p = make(BP_POPUP_FALL, "", "需要帮助请长按手机按钮");
    popup_view_build(&v, &p, POPUP_PHASE_WAIT, 0);
    assert(v.danger && strcmp(v.mode, "摔倒提醒") == 0 && strcmp(v.dest, "FALL ALERT") == 0);
    assert(strcmp(v.title, "检测到摔倒") == 0 && strcmp(v.body, "需要帮助请长按手机按钮") == 0);
    assert(strcmp(v.foot, "等待语音") == 0 && strcmp(v.hint, "按任意键关闭") == 0);

    p = make(BP_POPUP_BADGE, "今天完成 10 趟", "换刃比上午更稳，继续保持。");
    popup_view_build(&v, &p, POPUP_PHASE_PLAY, 0);
    assert(!v.danger && strcmp(v.mode, "成就报告") == 0 && strcmp(v.dest, "ACHIEVED") == 0);
    assert(strcmp(v.title, "今天完成 10 趟") == 0 && strcmp(v.foot, "正在播报") == 0);

    p = make(BP_POPUP_ISSUE, "", "");
    popup_view_build(&v, &p, POPUP_PHASE_HOLD, POPUP_HOLD_MS);
    assert(strcmp(v.dest, "RUN ISSUE") == 0 && strcmp(v.title, "发现问题") == 0 && strcmp(v.foot, "10 秒后返回") == 0);
    popup_view_build(&v, &p, POPUP_PHASE_HOLD, 9001);
    assert(strcmp(v.foot, "10 秒后返回") == 0);
    popup_view_build(&v, &p, POPUP_PHASE_HOLD, 9000);
    assert(strcmp(v.foot, "9 秒后返回") == 0);
    popup_view_build(&v, &p, POPUP_PHASE_HOLD, 0);
    assert(strcmp(v.foot, "1 秒后返回") == 0);

    p = make(BP_POPUP_VOICE, "", "");
    popup_view_build(&v, &p, POPUP_PHASE_WAIT, 0);
    assert(strcmp(v.mode, "语音播报") == 0 && strcmp(v.dest, "BROADCAST") == 0 && strcmp(v.title, "手机播报") == 0);
    p.kind = 9;   // An unknown kind never reaches the view. It still shows as a voice message.
    popup_view_build(&v, &p, POPUP_PHASE_WAIT, 0);
    assert(strcmp(v.mode, "语音播报") == 0);
    popup_view_build(&v, NULL, POPUP_PHASE_WAIT, 0);
    assert(strcmp(v.title, "手机播报") == 0 && v.body[0] == 0);

    printf("popup view tests: PASS\n");
    return 0;
}
