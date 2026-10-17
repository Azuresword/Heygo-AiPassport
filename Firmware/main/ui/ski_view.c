// main/ui/ski_view.c
#include "ski_view.h"

#include <stdio.h>
#include <string.h>

#include "stats_fmt.h"

#define WAIT_DATA   "等待手机数据"
#define SKI_IDLE    "专注雪道，教练随时待命"
#define STALE       "数据已过期"
#define REPORT_WAIT "报告生成中"
#define NO_REPORT   "滑完一趟后显示报告"
#define SEP         " · "

static void copy(char *dst, size_t cap, const char *src) { snprintf(dst, cap, "%s", src ? src : ""); }

// "RUN 4  02:31". The display font has ASCII only, so the separator is two spaces.
static void run_line(char *out, size_t cap, uint8_t run_no, const char *tail) {
    char no[STATS_TEXT_MAX];
    stats_fmt_count8(no, sizeof no, run_no);
    snprintf(out, cap, "RUN %s  %s", no, tail);
}

static void show_report(ski_view_t *v, const link_report_t *r) {
    // The Ski page is narrow next to the icon. The Report page shows the turn count.
    char dist[STATS_TEXT_MAX];
    stats_fmt_distance(dist, sizeof dist, r->distance_m);
    copy(v->dest, sizeof v->dest, "REPORT");
    run_line(v->run, sizeof v->run, r->run_no, dist);
    stats_fmt_iq(v->speed, sizeof v->speed, r->iq);
    copy(v->unit, sizeof v->unit, "HEYGO IQ");
    copy(v->line1, sizeof v->line1, r->problem);
    copy(v->line2, sizeof v->line2, r->tip);
}

void ski_view_build(ski_view_t *v, const bp_live_stats_t *live, bool stale, const link_report_t *report) {
    memset(v, 0, sizeof *v);
    copy(v->unit, sizeof v->unit, "KM/H");
    if (!live) {
        if (report) {               // A reconnect brings the report back before the next run.
            show_report(v, report);
            return;
        }
        copy(v->dest, sizeof v->dest, "LIVE RUN");
        copy(v->run, sizeof v->run, "RUN --");
        copy(v->speed, sizeof v->speed, STATS_UNKNOWN);
        copy(v->line1, sizeof v->line1, WAIT_DATA);
        copy(v->line2, sizeof v->line2, SKI_IDLE);
        return;
    }
    const bool ended = live->run_state == BP_RUN_ENDED;
    if (ended && report && report->run_no == live->run_no) {
        show_report(v, report);
        return;
    }
    char time[STATS_TEXT_MAX], drop[STATS_TEXT_MAX], dist[STATS_TEXT_MAX], turns[STATS_TEXT_MAX];
    stats_fmt_time(time, sizeof time, live->time_s);
    stats_fmt_drop(drop, sizeof drop, live->drop_m);
    stats_fmt_distance(dist, sizeof dist, live->distance_m);
    stats_fmt_count(turns, sizeof turns, live->turns);
    // After the end packet the phone stops sending, so silence is not out of date then.
    const bool old = stale && !ended;

    copy(v->dest, sizeof v->dest, ended ? "RUN ENDED" : "LIVE RUN");
    run_line(v->run, sizeof v->run, live->run_no, time);
    if (ended || old) copy(v->speed, sizeof v->speed, STATS_UNKNOWN);
    else stats_fmt_speed(v->speed, sizeof v->speed, live->speed_dkmh);
    snprintf(v->line1, sizeof v->line1, "落差 %s" SEP "%s", drop, dist);
    const char *note = ended ? SEP REPORT_WAIT : old ? SEP STALE : "";
    snprintf(v->line2, sizeof v->line2, "%s 弯%s", turns, note);
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
