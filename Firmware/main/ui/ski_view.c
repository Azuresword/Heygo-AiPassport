// main/ui/ski_view.c
#include "ski_view.h"

#include <stdio.h>
#include <string.h>

#include "stats_fmt.h"

#define WAIT_DATA   "等待手机数据"
#define SKIING      "滑行中"
#define STOPPED     "静止"
#define STALE       "数据已过期"
#define REPORT_WAIT "报告生成中"
#define RUN_REPORT  "本趟报告"
#define NO_REPORT   "滑完一趟后显示报告"
#define SEP         " · "

static void copy(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src ? src : ""); }

// "第 4 趟 · 滑行中". With no note: "第 4 趟".
static void sub_line(char *out, size_t cap, uint8_t run_no, const char *note) {
    char no[STATS_TEXT_MAX];
    stats_fmt_count8(no, sizeof no, run_no);
    if (note && note[0]) snprintf(out, cap, "第 %s 趟" SEP "%s", no, note);
    else snprintf(out, cap, "第 %s 趟", no);
}

static void metrics(ski_view_t *v, const bp_live_stats_t *s) {
    stats_fmt_drop(v->drop, sizeof v->drop, s ? s->drop_m : BP_U16_UNKNOWN);
    stats_fmt_distance(v->dist, sizeof v->dist, s ? s->distance_m : BP_U16_UNKNOWN);
    stats_fmt_count(v->turns, sizeof v->turns, s ? s->turns : BP_U16_UNKNOWN);
    stats_fmt_time(v->time, sizeof v->time, s ? s->time_s : BP_U16_UNKNOWN);
}

static void show_report(ski_view_t *v, const link_report_t *r) {
    v->report = true;
    copy(v->dest, sizeof v->dest, "REPORT");
    sub_line(v->sub, sizeof v->sub, r->run_no, RUN_REPORT);
    stats_fmt_iq(v->big, sizeof v->big, r->iq);
    copy(v->unit, sizeof v->unit, "HEYGO IQ");
    copy(v->problem, sizeof v->problem, r->problem);
    copy(v->tip, sizeof v->tip, r->tip);
}

void ski_view_build(ski_view_t *v, const bp_live_stats_t *live, bool stale, const link_report_t *report) {
    memset(v, 0, sizeof *v);
    copy(v->unit, sizeof v->unit, "KM/H");
    metrics(v, live);
    const bool ended = live && live->run_state == BP_RUN_ENDED;
    // A report shows after its run ends, and also after a reconnect with no live data.
    if (report && (!live || (ended && report->run_no == live->run_no))) {
        show_report(v, report);
        return;
    }
    if (!live) {
        copy(v->dest, sizeof v->dest, "LIVE RUN");
        copy(v->sub, sizeof v->sub, WAIT_DATA);
        copy(v->big, sizeof v->big, STATS_UNKNOWN);
        return;
    }
    // After the end packet the phone stops sending, so silence is not out of date then.
    const bool old = stale && !ended;
    const char *note = ended ? REPORT_WAIT
                     : old ? STALE
                     : live->run_state == BP_RUN_SKIING ? SKIING
                     : live->run_state == BP_RUN_STOPPED ? STOPPED : "";
    copy(v->dest, sizeof v->dest, ended ? "RUN ENDED" : "LIVE RUN");
    sub_line(v->sub, sizeof v->sub, live->run_no, note);
    if (ended || old) copy(v->big, sizeof v->big, STATS_UNKNOWN);
    else stats_fmt_speed(v->big, sizeof v->big, live->speed_dkmh);
}

void report_view_build(report_view_t *v, const link_report_t *r, uint32_t age_ms) {
    memset(v, 0, sizeof *v);
    if (!r) {
        copy(v->title, sizeof v->title, NO_REPORT);
        copy(v->iq, sizeof v->iq, STATS_UNKNOWN);
        return;
    }
    char no[STATS_TEXT_MAX], dist[STATS_TEXT_MAX], turns[STATS_TEXT_MAX];
    stats_fmt_count8(no, sizeof no, r->run_no);
    stats_fmt_distance(dist, sizeof dist, r->distance_m);
    stats_fmt_count(turns, sizeof turns, r->turns);
    snprintf(v->title, sizeof v->title, "第 %s 趟" SEP "%s" SEP "%s 弯", no, dist, turns);
    if (age_ms >= LINK_DATA_AGE_MS) snprintf(v->age, sizeof v->age, "%lu MIN AGO", (unsigned long)(age_ms / 60000u));
    stats_fmt_iq(v->iq, sizeof v->iq, r->iq);
    copy(v->problem, sizeof v->problem, r->problem);
    copy(v->tip, sizeof v->tip, r->tip);
}
