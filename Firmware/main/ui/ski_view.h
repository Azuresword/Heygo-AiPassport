// main/ui/ski_view.h
// The texts of the Ski page and the Report page, from the last LIVE_STATS and the
// last RUN_REPORT. Requirements section 7 and architecture section 3 define them.
// Plain C, so host tests can link it. The Coach task copies each text into its label.
//
// The Chinese texts here also appear in assets/ui/strings.json, so that the glyph
// check covers them: msg.wait_data, msg.ski_idle, msg.stale, msg.report_wait,
// msg.no_report, msg.skiing, msg.stopped, msg.run_report, fmt.run, fmt.turns.
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "ble_proto.h"
#include "link_core.h"

#ifdef __cplusplus
extern "C" {
#endif

// The Ski page. Each field is the text of the label with the same name.
// The sizes cover the longest result of each format, so GCC sees no possible cut.
typedef struct {
    bool report;        // true: show problem and tip. false: show the four metric cells.
    char dest[16];      // "LIVE RUN", "RUN ENDED", or "REPORT"
    char sub[64];       // "第 4 趟 · 滑行中", "第 4 趟 · 数据已过期", "等待手机数据" ...
    char big[8];        // the speed, or the Heygo IQ in a report
    char unit[12];      // "KM/H" or "HEYGO IQ"
    char drop[16];      // the metric cells: 落差, 距离, 转弯, 时间
    char dist[16];
    char turns[16];
    char time[16];
    char problem[BP_PROBLEM_MAX + 1];
    char tip[BP_TIP_MAX + 1];
} ski_view_t;

// The Report page.
typedef struct {
    char title[96];     // "第 3 趟 · 1.9 km · 25 弯"
    char iq[8];
    char age[24];       // "3 MIN AGO" from 60 s after the report arrives, else ""
    char problem[BP_PROBLEM_MAX + 1];
    char tip[BP_TIP_MAX + 1];
} report_view_t;

// live and report can be NULL. stale comes from link_live(). age_ms comes from link_report().
// After a run ends, the Ski page shows the report of that run when it is here.
void ski_view_build(ski_view_t *out, const bp_live_stats_t *live, bool stale,
                    const link_report_t *report);
void report_view_build(report_view_t *out, const link_report_t *report, uint32_t age_ms);

#ifdef __cplusplus
}
#endif
