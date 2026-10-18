<p class="lang"><strong>English</strong> · <a href="decisions.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Decisions</p>

# Decisions and scope

The report covers D-01–D-34. Decision approval, implementation, and device acceptance are separate statuses.

<div class="tiles">
<div class="tile accent"><div class="k">Approved</div><div class="v">18</div></div>
<div class="tile"><div class="k">App approval pending</div><div class="v">3</div></div>
<div class="tile accent"><div class="k">Added on 10-18</div><div class="v">2</div></div>
<div class="tile"><div class="k">Replaced</div><div class="v">6</div></div>
<div class="tile"><div class="k">Proposed</div><div class="v">4</div></div>
<div class="tile"><div class="k">Document rule</div><div class="v">1</div></div>
</div>

The three pending decisions have user approval. They still need phone app approval.
The two additions cover the page rail and link status bar. Their implementation is complete.

## Approved decisions

| ID | Decision | Approval |
|---|---|---|
| D-08 | BLE connection through the phone. No device Wi-Fi. | User |
| D-09 | IMA ADPCM audio at 16 kHz, in 20 ms blocks. | User |
| D-11 | Stream microphone blocks to the phone immediately. | User |
| D-12 | No link encryption, passkey, or binding check. | User |
| D-13 | No privacy or authorization functions. | User |
| D-14 | The device handles keys, microphone, speaker, and screen. The phone handles the other functions. | User |
| D-16 | BLE protocol 0.2. | Phone app team |
| D-17 | P0 contains only the Coach voice path. | User |
| D-18 | No power management in the first version. | User |
| D-21 | Live data page and Report page. | User |
| D-22 | Develop increment 2 during increment 1 acceptance. Each increment has separate acceptance. | User |
| D-23 | The device team defines the live data fields. The phone sends one frame each second during a run. | User |
| D-24 | Protocol 0.2 adds LIVE_STATS. | User and phone app team |
| D-26 | The phone sends one report for each run. | User |
| D-27 | Heygo IQ ranges from 0 to 200. | User |
| D-28 | SHOW_PAGE 3 opens the Report page from live data or the idle Coach page. Active audio states ignore it. | User |
| D-29 | Replace Ski and Rest modes with a page loop. Short UP and DOWN presses change pages and cancel any active Coach exchange. | User |
| D-30 | Add Turns as the fourth page. Show eight turns per page and request groups through TURN_REQUEST. | User |

D-29 introduced a three-page loop. D-30 added the Turns page between Report and Coach.
The Turns page shows the run of the last RUN_REPORT, following the phone app team's change request.

## App approval pending

| ID | Device contract |
|---|---|
| D-20 | The phone sends PLAY_START only after it receives AGENT_END. PLAY_START does not switch capture to playback. |
| D-31 | POPUP 0x66 interrupts the current interaction. POPUP_END 0x67 reports closure after a key press, replacement, or the 10 s countdown. |
| D-32 | RUN_START 0x68 opens live data. During Coach audio, the page change waits. A popup closes to live data. |

## Additions on 2026-10-18

| ID | Behavior | Implementation |
|---|---|---|
| D-33 | A right-edge rail shows four stops in page order. A lime thumb marks the current page. All stops share one color. The rail has no arrows or key targets. The thumb slides in 180 ms and wraps across the ends. Popups hide the rail. Key behavior stays unchanged. | Simulator and firmware complete. LVGL pool: 28 KiB. Sprint 16, REQ-24, TC-24. |
| D-34 | Five screens share a status bar with page and link indicators. Link text shows connected, disconnected, or no voice. A link change updates the current screen without a page change. One flow binding can address matching widgets across screens. | Code complete. Sprint 17, REQ-25, US-25. |

The [Sprint completion record](sprints.html) states the delivery and verification boundary for these additions.

## Proposals and document rule

| ID | Content | Status |
|---|---|---|
| D-02 | The device handles interaction. Analysis runs elsewhere. | Proposed |
| D-04 | One audio owner at a time. | Proposed |
| D-05 | Initial trials use existing hardware indoors. | Proposed |
| D-07 | Latency, gesture, and capacity values remain targets. | Proposed |
| D-06 | Paired English and Chinese documents, with asd-ste100 writing guidance. | User instruction |

## Replaced decisions

| ID | Previous scope | Replacement |
|---|---|---|
| D-01 | All six requested capabilities in the core scope | D-17 |
| D-03 | Voice only in Rest mode | D-29 |
| D-10 | Separate long presses for cancel and exit | D-29 |
| D-15 | Two operating modes | D-29 |
| D-19 | Ignore a long DOWN press during audio | D-29 |
| D-25 | Short DOWN opens Report from the idle Coach page | D-29 |

## Requirement allocation

| Priority | Increment | Requirements |
|---|---|---|
| P0 | 1 Coach voice | REQ-01, 06, 07, 08, 15, 17, 20 |
| P1 | 2 Live data and report | REQ-02, 03, 21, 23, 24, 25 |
| P1 | 3 Turns | REQ-05 |
| P1 | 4 Phone popups | REQ-22 |
| P1 | Later | REQ-04, 09, 13 |
| P2 | Later | REQ-10, 11, 12, 18, 19 |
| Deferred | — | REQ-14, 16 |
