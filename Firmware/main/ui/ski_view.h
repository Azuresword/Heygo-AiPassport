// main/ui/ski_view.h
// The texts of the Ski page and the Report page, from the last LIVE_STATS and the
// last RUN_REPORT. Requirements section 7 and architecture section 3 define them.
// Plain C, so host tests can link it. The Coach task copies each text into its label.
//
// The Chinese texts here also appear in assets/ui/strings.json, so that the glyph
// check covers them: msg.wait_data, msg.ski_idle, msg.stale, msg.report_wait,
// msg.no_report, fmt.drop, fmt.turns, fmt.run.
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
    char dest[16];      // "LIVE RUN", "RUN ENDED", or "REPORT"
    char run[48];       // "RUN 4  02:31", or "RUN 4  1.9 km" with the report
    char speed[8];      // the speed, or the Heygo IQ after the report arrives
    char unit[12];      // "KM/H" or "HEYGO IQ"
    char line1[64];
    char line2[96];
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
