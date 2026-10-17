// tests/test_ski_view.c
// Host test for main/ui/ski_view.c: the texts of the Ski page and the Report page.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ski_view.h"

static int s_fail;

#define SAME(got, want) do {                                                              \
    if (strcmp((got), (want)) != 0) {                                                     \
        printf("FAIL %s:%d %s is \"%s\", want \"%s\"\n", __FILE__, __LINE__, #got, (got), (want)); \
        s_fail++;                                                                         \
    }                                                                                     \
} while (0)

static const bp_live_stats_t SKIING = { BP_RUN_SKIING, 4, 383, 312, 1850, 24, 151 };

static link_report_t report(uint8_t run_no) {
    link_report_t r = { run_no, 1900, 25, 82, "重心偏后", "下一趟把双手放在身体前方" };
    return r;
}

static void waiting(void) {
    ski_view_t v;
    ski_view_build(&v, NULL, true, NULL);
    SAME(v.dest, "LIVE RUN");
    SAME(v.sub, "等待手机数据");
    SAME(v.big, "--");
    SAME(v.unit, "KM/H");
    SAME(v.drop, "--");
    SAME(v.dist, "--");
    SAME(v.turns, "--");
    SAME(v.time, "--");
    if (v.report) { printf("FAIL waiting shows the metric cells\n"); s_fail++; }
}

static void skiing(void) {
    ski_view_t v;
    ski_view_build(&v, &SKIING, false, NULL);
    SAME(v.dest, "LIVE RUN");
    SAME(v.sub, "第 4 趟 · 滑行中");
    SAME(v.big, "38");
    // Each number sits in its own labeled cell.
    SAME(v.drop, "312 m");
    SAME(v.dist, "1.9 km");
    SAME(v.turns, "24");
    SAME(v.time, "02:31");

    bp_live_stats_t s = SKIING;
    s.run_state = BP_RUN_STOPPED;
    ski_view_build(&v, &s, false, NULL);
    SAME(v.sub, "第 4 趟 · 静止");

    // Out of date: the speed shows "--". The other numbers stay.
    ski_view_build(&v, &SKIING, true, NULL);
    SAME(v.big, "--");
    SAME(v.sub, "第 4 趟 · 数据已过期");
    SAME(v.drop, "312 m");

    // A missing value shows "--", never 0.
    s = SKIING;
    s.speed_dkmh = BP_U16_UNKNOWN;
    s.turns = BP_U16_UNKNOWN;
    ski_view_build(&v, &s, false, NULL);
    SAME(v.big, "--");
    SAME(v.turns, "--");

    // An old report of an earlier run does not hide the live data.
    const link_report_t old = report(3);
    ski_view_build(&v, &SKIING, false, &old);
    SAME(v.dest, "LIVE RUN");
    SAME(v.big, "38");
    if (v.report) { printf("FAIL an old report hides the live data\n"); s_fail++; }
}

static void ended(void) {
    ski_view_t v;
    bp_live_stats_t s = SKIING;
    s.run_state = BP_RUN_ENDED;
    s.time_s = 192;
    // The report is not here yet. Silence after the end packet is not out of date.
    ski_view_build(&v, &s, true, NULL);
    SAME(v.dest, "RUN ENDED");
    SAME(v.sub, "第 4 趟 · 报告生成中");
    SAME(v.big, "--");
    SAME(v.time, "03:12");

    // The report of this run replaces the metric cells.
    const link_report_t r = report(4);
    ski_view_build(&v, &s, true, &r);
    if (!v.report) { printf("FAIL the report must replace the cells\n"); s_fail++; }
    SAME(v.dest, "REPORT");
    SAME(v.sub, "第 4 趟 · 本趟报告");
    SAME(v.big, "82");
    SAME(v.unit, "HEYGO IQ");
    SAME(v.problem, "重心偏后");
    SAME(v.tip, "下一趟把双手放在身体前方");

    // After a reconnect, a report with no live data still shows.
    ski_view_build(&v, NULL, true, &r);
    SAME(v.dest, "REPORT");
}

static void report_page(void) {
    report_view_t v;
    report_view_build(&v, NULL, 0);
    SAME(v.title, "滑完一趟后显示报告");
    SAME(v.iq, "--");
    SAME(v.problem, "");
    SAME(v.tip, "");

    link_report_t r = report(3);
    report_view_build(&v, &r, LINK_DATA_AGE_MS - 1);
    SAME(v.title, "第 3 趟 · 1.9 km · 25 弯");
    SAME(v.iq, "82");
    SAME(v.problem, "重心偏后");
    SAME(v.tip, "下一趟把双手放在身体前方");
    SAME(v.age, "");

    report_view_build(&v, &r, 3u * 60000u + 5000u);
    SAME(v.title, "第 3 趟 · 1.9 km · 25 弯");
    SAME(v.age, "3 MIN AGO");

    r.iq = BP_U8_UNKNOWN;
    r.distance_m = BP_U16_UNKNOWN;
    report_view_build(&v, &r, 0);
    SAME(v.iq, "--");
    SAME(v.title, "第 3 趟 · -- · 25 弯");
}

static void longest_texts_fit(void) {
    // 12 and 20 Chinese characters: the limits of doc 07.
    link_report_t r = report(9);
    r.problem[0] = r.tip[0] = '\0';
    for (int i = 0; i < 12; i++) strcat(r.problem, "滑");
    for (int i = 0; i < 20; i++) strcat(r.tip, "雪");
    report_view_t v;
    report_view_build(&v, &r, 0);
    if (strlen(v.problem) != 36 || strlen(v.tip) != 60) {
        printf("FAIL the longest texts must arrive whole\n");
        s_fail++;
    }
    ski_view_t s;
    bp_live_stats_t live = SKIING;
    live.run_no = 9;
    live.run_state = BP_RUN_ENDED;
    ski_view_build(&s, &live, false, &r);
    if (strlen(s.problem) != 36 || strlen(s.tip) != 60) {
        printf("FAIL the Ski page must hold the longest texts\n");
        s_fail++;
    }
}

int main(void) {
    waiting();
    skiing();
    ended();
    report_page();
    longest_texts_fit();
    if (s_fail) return EXIT_FAILURE;
    printf("ski view tests: PASS\n");
    return EXIT_SUCCESS;
}
