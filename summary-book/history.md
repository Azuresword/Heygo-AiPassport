<p class="lang"><strong>English</strong> · <a href="history.zh_CN.html">简体中文</a></p>

<p class="eyebrow">History</p>

# Development history

This snapshot ends at commit `284ab00`, dated 2026-10-18 12:05, UTC+8. Later development does not change these totals.

<div class="tiles">
<div class="tile accent"><div class="k">Commits on main</div><div class="v">209</div></div>
<div class="tile"><div class="k">Elapsed span</div><div class="v">42<small>h</small></div></div>
<div class="tile"><div class="k">Development phases</div><div class="v">7</div></div>
<div class="tile"><div class="k">First commit</div><div class="v" style="font-size:20px">10-16 18:02</div></div>
</div>

The span is approximately 42 elapsed hours. It does not measure development effort or Sprint duration.

## Phases and results

{% include "./_partials/commits-en.html" %}

| Phase | Period | Commits | Result |
|---|---|---|---|
| 1 Requirements | 10-16 evening | 6 | Product scope, BLE protocol draft, and P0 reduction to Coach voice. |
| 2 Voice path | 10-17 morning | 10 | State machine, audio codec, voice and BLE integration, brand screens, and playback start correction. |
| 3 Run data | 10-17 midday | 9 | LIVE_STATS, run reports, live data and Report pages, and SHOW_PAGE. |
| 4 Page loop and turns | 10-17 evening | 4 | Flat page navigation and the Turns page with group requests. |
| 5 Popups and run start | 10-17 night | 8 | Phone popups, IQ range 0–200, animated badges, and RUN_START. |
| 6 Page rail design | 10-18 morning | 18 | Page rail requirements, simulator behavior, self-tests, and preview. |
| 7 Page rail firmware and link state | 10-18 10:55–12:05 | 149 | Shared widget bindings, link status on five screens, page rail firmware, 28 KiB LVGL pool, and 25 rendered screens. |
| Report preparation | 10-18 11:00–11:23 | 5 | Initial technical report. These commits are separate from phase 7. |
| Total | | 209 | Seven development phases and report preparation. |

The phase totals count commits, including generated assets. They do not compare engineering effort.

## Interaction changes

- P0 narrowed to the Coach voice path. Data reports, turns, and popups followed as separate increments.
- D-29 replaced Ski and Rest modes with a page loop. D-30 added Turns as the fourth page.
- D-33 added the page rail. Review removed directional arrows and gave every stop the same color.
- D-34 added one link status bar across the four pages and the popup screen.

The [Sprint record](sprints.html) retains the project owner's completion confirmation and its verification boundary.
