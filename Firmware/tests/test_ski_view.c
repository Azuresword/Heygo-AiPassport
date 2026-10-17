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
    SAME(v.run, "RUN --");
    SAME(v.speed, "--");
    SAME(v.unit, "KM/H");
    SAME(v.line1, "等待手机数据");
    SAME(v.line2, "专注雪道，教练随时待命");
}

static void skiing(void) {
    ski_view_t v;
    ski_view_build(&v, &SKIING, false, NULL);
    SAME(v.dest, "LIVE RUN");
    SAME(v.run, "RUN 4  02:31");
    SAME(v.speed, "38");
    SAME(v.line1, "落差 312 m · 1.9 km");
    SAME(v.line2, "24 弯");

    // Out of date: the speed shows "--". The other numbers stay.
    ski_view_build(&v, &SKIING, true, NULL);
    SAME(v.speed, "--");
    SAME(v.line1, "落差 312 m · 1.9 km");
    SAME(v.line2, "24 弯 · 数据已过期");

    // A missing speed shows "--", never 0.
    bp_live_stats_t s = SKIING;
    s.speed_dkmh = BP_U16_UNKNOWN;
    s.turns = BP_U16_UNKNOWN;
    ski_view_build(&v, &s, false, NULL);
    SAME(v.speed, "--");
    SAME(v.line2, "-- 弯");

    // An old report of an earlier run does not hide the live data.
    const link_report_t old = report(3);
    ski_view_build(&v, &SKIING, false, &old);
    SAME(v.dest, "LIVE RUN");
    SAME(v.speed, "38");
}

static void ended(void) {
    ski_view_t v;
    bp_live_stats_t s = SKIING;
    s.run_state = BP_RUN_ENDED;
    s.time_s = 192;
    // The report is not here yet. Silence after the end packet is not out of date.
    ski_view_build(&v, &s, true, NULL);
    SAME(v.dest, "RUN ENDED");
    SAME(v.run, "RUN 4  03:12");
    SAME(v.speed, "--");
    SAME(v.line2, "24 弯 · 报告生成中");

    // The report of this run replaces the live data.
    const link_report_t r = report(4);
    ski_view_build(&v, &s, true, &r);
    SAME(v.dest, "REPORT");
    SAME(v.run, "RUN 4  1.9 km");
    SAME(v.speed, "82");
    SAME(v.unit, "HEYGO IQ");
    SAME(v.line1, "重心偏后");
    SAME(v.line2, "下一趟把双手放在身体前方");

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
    if (strlen(s.line1) != 36 || strlen(s.line2) != 60) {
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
