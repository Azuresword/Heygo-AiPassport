<p class="lang"><strong>English</strong> · <a href="history.zh_CN.html">简体中文</a></p>

<p class="eyebrow">History</p>

# Project history

<div class="tiles">
<div class="tile accent"><div class="k">Commits on main</div><div class="v">55</div></div>
<div class="tile"><div class="k">Duration</div><div class="v">41<small>h</small></div></div>
<div class="tile"><div class="k">Phases</div><div class="v">6</div></div>
<div class="tile"><div class="k">First commit</div><div class="v" style="font-size:20px">10-16 18:02</div></div>
</div>

## Commits in each phase

{% include "./_partials/commits-en.html" %}

**Table view**

| Phase | Time | Commits | Range |
|---|---|---|---|
| 1 Requirements | 10-16 evening | 6 | `f431fe6` … `cfa30fe` |
| 2 Voice path | 10-17 morning | 10 | … `344c8e5` |
| 3 Run data | 10-17 midday | 9 | … `c996e14` |
| 4 Page loop, turns | 10-17 evening | 4 | … `49573f9` |
| 5 Popups, run start | 10-17 night | 8 | … `5658911` |
| 6 Page rail | 10-18 morning | 18 | … `c865ba8` |
| Total | | 55 | |

Source: `git rev-list --count <range>`, with the commit limits in the table.

## Timeline

<div class="timeline">
<div class="ev"><div class="when">10-16 · 18:02</div><div class="what">Requirements</div><div class="how">Start <code>f431fe6</code>. Eight decisions <code>6f5b6af</code>. BLE protocol draft <code>970bb8e</code>. Smaller P0 <code>48926ff</code>.</div></div>
<div class="ev"><div class="when">10-17 · morning</div><div class="what">Sprints 0 and 1: voice path</div><div class="how">Firmware added, 406 files <code>a917aaa</code>. Voice and BLE connected <code>941f4e7</code>. Heygo brand screens <code>4ff7f13</code>. Audio start fix <code>344c8e5</code>.</div></div>
<div class="ev"><div class="when">10-17 · midday to afternoon</div><div class="what">Increment 2: run data and report</div><div class="how">Sprint 2 data layer <code>d371c1d</code>. Sprints 3 and 4: live data page and report page. D-28 SHOW_PAGE <code>9238eaf</code>.</div></div>
<div class="ev"><div class="when">10-17 · evening</div><div class="what">Page loop and Turns page</div><div class="how">D-29 four-page loop. D-30 Turns page <code>49573f9</code>.</div></div>
<div class="ev"><div class="when">10-17 · night</div><div class="what">Popups and run start</div><div class="how">D-31 phone popups <code>93b6498</code>. IQ range 0 to 200 <code>816f043</code>. Animated popup badges <code>2234275</code>. D-32 run start <code>5658911</code>.</div></div>
<div class="ev"><div class="when">10-18 · morning</div><div class="what">Page rail D-33</div><div class="how">Documents 01 to 05 add REQ-24, US-24, and TC-24, from <code>eef9c65</code>. The simulator shows the rail <code>30ba3c7</code>. Self-test coverage <code>258f6fa</code>. Preview image <code>c865ba8</code>.</div></div>
<div class="ev"><div class="when">10-18</div><div class="what">All 17 sprints complete</div><div class="how">The project owner confirmed it. The team did the board and device work on physical devices.</div></div>
</div>

Authors: Dylan Li made 52 commits. A local account made 3 commits (`git log --format=%an`). The repository has no tags.
