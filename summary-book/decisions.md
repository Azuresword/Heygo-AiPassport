<p class="lang"><strong>English</strong> · <a href="decisions.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Decisions</p>

# Decisions D-01 to D-34

Source: `需求文件夹/04-validation-and-decisions.md`, decision table, at `main` commit `c865ba8`. The status below is the text of that document.

<div class="tiles">
<div class="tile accent"><div class="k">Confirmed</div><div class="v">21</div></div>
<div class="tile accent"><div class="k">New from the user</div><div class="v">2</div></div>
<div class="tile"><div class="k">Replaced</div><div class="v">6</div></div>
<div class="tile"><div class="k">Proposed</div><div class="v">4</div></div>
<div class="tile"><div class="k">Required</div><div class="v">1</div></div>
</div>

"Confirmed" includes 7 decisions with the phone app team, 11 decisions by the user, and 3 decisions that wait for the phone app team. "New from the user" is D-33 and D-34, from 10-18.

## Confirmed on 2026-10-17, with the phone app team

| ID | Content |
|---|---|
| D-16 | Protocol draft. The phone app team confirmed protocol 0.2. |
| D-23 | The device team sets the live data fields. One frame each second. |
| D-24 | Protocol 0.2 and LIVE_STATS. |
| D-27 | Heygo IQ is 0 to 200. |
| D-28 | SHOW_PAGE 3 works only on the live data page or an idle Coach page. |
| D-29 | Four pages at the same level in a loop. |
| D-30 | Turns page: 8 turns on each page, groups through TURN_REQUEST. |

## Confirmed by the user

| ID | Content |
|---|---|
| D-08 | BLE, with the phone as the bridge. |
| D-09 | IMA ADPCM audio. |
| D-11 | Stream the recording at once. |
| D-12 | No encryption and no pairing. |
| D-13 | No privacy functions. |
| D-14 | The device controls only keys, microphone, speaker, and screen. |
| D-17 | P0 keeps only the coach voice path. |
| D-18 | No power management. |
| D-21 | Live data page and report page. |
| D-22 | Acceptance of increment 2 and increment 1 runs in parallel. |
| D-26 | One report for each run. |

## Confirmed by the user, waiting for the phone app team

| ID | Content |
|---|---|
| D-20 | The phone sends PLAY_START only after it gets AGENT_END. |
| D-31 | Phone popups: POPUP 0x66 and POPUP_END 0x67, with a 10 s countdown. |
| D-32 | RUN_START 0x68 opens the live data page at run start. |

## New from the user (2026-10-18)

| ID | Content | Status |
|---|---|---|
| D-33 | Page rail on the right side. It has four stops in the order of the short DOWN press. A lime thumb shows the current page. The UP and DOWN targets are light gray stops, with no arrows. The thumb slides in 180 ms. The popup page hides the rail. The rail changes no key. | The simulator and the firmware show the rail. The LVGL pool is now 28 KiB. The project owner confirmed Sprint 16 as complete. Related items: REQ-24 and TC-24. |
| D-34 | All five screens use one status bar for the link state: a page dot, the page name, a link dot, and the link text. One binding in the flow graph can drive the same widget on more screens. | Document text: code complete, device tests not run. Related items: REQ-25, US-25, and Sprint 17. |

## Proposed, not confirmed

- D-02: The device controls the interaction. Analysis is in a different place.
- D-04: Only one audio user at a time.
- D-05: First, test indoors with the current hardware.
- D-07: All numbers are targets.

## Required by the user

D-06: Documents in English and Chinese pairs, with asd-ste100.

## Replaced

| ID | Replaced by |
|---|---|
| D-01 | D-17 |
| D-03 Voice only in rest mode | D-29 |
| D-10 Different long presses for cancel and exit | D-29 |
| D-15 Two modes | D-29 |
| D-19 Ignore a long press of DOWN | D-29 |
| D-25 A short press of DOWN opens the report page | D-29 |

D-29 replaced five decisions. It is the largest simplification of the interaction.

## Requirements

Source: `需求文件夹/01-product-requirements.md`, section 4.

| Priority | Increment | Items |
|---|---|---|
| P0 | 1 Coach voice | REQ-01, 06, 07, 08, 15, 17, 20 |
| P1 | 2 Live data and report | REQ-02, 03, 21, 23, 24, 25 |
| P1 | 3 Turns | REQ-05 |
| P1 | 4 Phone popups | REQ-22 |
| P1 | After increment 2 | REQ-04, 09, 13 |
| P2 | Later candidates | REQ-10, 11, 12, 18, 19 |
| Deferred | — | REQ-14, 16 |
