[简体中文](08-sprint-roadmap.zh_CN.md) · **English**

# Four-hour Sprint roadmap

Version: 0.4. Date: 2026-10-17. Status: In progress. The code of Sprints 2 to 4 is in the repository and merged into `main`. Device tests are NOT RUN.

This document divides the remaining work of Increments 1 to 4 in the [backlog](03-scrum-backlog.md) into four-hour Sprints.
Each Sprint has one Sprint Goal.
The minutes are planning limits, not measurements. The team has no velocity data yet.

## 1. Fixed Sprint structure

Each Sprint lasts four hours and has four events:

| Event | Time | Output |
| --- | --- | --- |
| Sprint Planning | 15 min | The Sprint Goal and the tasks. A check of the inputs. |
| Development | 200 min | A usable increment that passes the exit check of the Sprint. |
| Sprint Review | 15 min | A demonstration of the increment. Evidence in the format of section 5. |
| Sprint Retrospective | 10 min | One improvement for the next Sprint. |

Rules:

- If an input is missing, the Sprint does not start. The team does the next Sprint whose inputs exist.
- If development passes the cut line, the team drops tasks as the cut line of that Sprint states. Each dropped task goes back to the backlog.
- If the team misses the Sprint Goal, the Product Owner decides the next step. The team does not extend the Sprint.
- At the end of each Sprint, `./tools/validate.sh` must give exit code 0.

## 2. Roadmap

| Sprint | Increment | Sprint Goal | Inputs | Status |
| --- | --- | --- | --- | --- |
| 0 | 1 | Build the device logic for one Coach question. | None | Done, commit `a917aaa` |
| 1 | 1 | Connect voice and the BLE link. | None | Code done, commits `941f4e7` and `11b10cc`. The planned hardware acceptance moves to Sprints 5 to 8 |
| 2 | 2 | The device receives LIVE_STATS and RUN_REPORT and parses each field as doc 07 defines. | None | Code committed. Section 3 gives the result |
| 3 | 2 | The live data page shows live data and passes the host screen check. | Sprint 2 | Code committed. Section 3 gives the result |
| 4 | 2 | Rest mode switches between the Coach page and the Report page. The Report page passes the host screen check. | Sprint 3 | Code committed. Section 3 gives the result |
| 5 | 1, 2 | On the board, run a question and one run of ski data with `phone_sim`, and record the measurements. | The board at hand | Not started |
| 6 | 1 | Pass TC-01, TC-06, TC-07, and TC-20 with the real phone application. | Sprint 5. A phone application with protocol 0.2 | Not started |
| 7 | 2 | Pass TC-02, TC-03, and the timing part of TC-21 with the real phone application. | Sprint 6 | Not started |
| 8 | 1 | Pass TC-08 with the real service. | Sprint 6. HeygoAI coaching tools (Q-01) | Not started |
| 9 | 1, 2 | Pass TC-15 and the reading part of TC-21 with the U1 usability test. | Sprint 7. Five test users | Not started |
| 10 | 1, 2 | Fix defects, and review whether the two increments meet the Definition of Done. | Sprint 9 | Not started |
| 11 | 3 | The device asks for and receives the turn groups, and the Turns page passes the host screen check. | Sprint 4 | Code on the branch `feat/turn-view`. Section 4 gives the result |
| 12 | 3 | Pass TC-05 on the board with `phone_sim`, and then with the real phone application. | Sprint 11. The board. A phone application that answers TURN_REQUEST | Not started |
| 13 | 4 | The device shows the popups from the phone, and the popup page passes the host screen check. | Sprint 11 | Code on the branch `feat/app-popups`. Section 4 gives the result |
| 14 | 4 | Run the device tests of the phone popups (D-31) on the board with `phone_sim --popups`. | Sprint 13. The board | Not started |

Sprints 2 to 4 run on the host only. They can start now.
Sprint 5 is the first Sprint that needs the board.
Sprints 6 to 8 need a phone application build with protocol 0.2.

## 3. Increment 2 Sprints (host)

### Sprint 2: data layer

**Sprint Goal.** The device receives LIVE_STATS and RUN_REPORT, and host tests show that it parses each field as doc 07 defines.

**Story.** US-02.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | `ble_proto` parses LIVE_STATS and RUN_REPORT. It gives ERROR codes 2, 4, and 7 as doc 07 defines. | 60 min |
| 2 | `link_core` keeps the last LIVE_STATS, the last report, and their receive times. It checks the 5-second age limit and the 60-second report time. HELLO changes to protocol 0.2. | 60 min |
| 3 | Number formats in plain C: speed, time, m or km, and "--" for an unknown value. | 50 min |
| 4 | `phone_sim` gets `--runs`: one packet each second, a stop mid-run, the end packet, the report, the lift silence, and a missing speed. | 30 min |

**Exit check.**

- Host tests cover each field of both messages, each unknown value, a short payload, and a long text.
- Host tests show that an unknown value never formats as 0.
- The gate gives exit code 0.

**Cut line.** If tasks 1 and 2 together take more than 150 minutes, move task 4 to Sprint 3.

**Result.** The team finished all four tasks. The cut line did not apply. Nobody recorded the actual time.
Section 8 of the [firmware change notes](../Firmware/docs/heygo-changes.md) lists the firmware changes. The commit message gives the gate result.

### Sprint 3: live data page

**Sprint Goal.** The live data page shows live data and passes the host screen check.

**Story.** US-21.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Make the 48-pixel digit font subset. Record its Flash size. | 40 min |
| 2 | Add the live data page bindings and texts to `coach.flow.json` and `strings.json`. | 60 min |
| 3 | Add the "Waiting for phone data" and "Data out of date" states. | 40 min |
| 4 | Render the five live data page states on the host, and run the glyph check. | 40 min |
| 5 | Update the flow simulator. | 20 min |

**Exit check.**

- The host renders five live data page images: skiing, the report after a run, waiting for data, out of date, and a missing field.
- The screen check passes: no missing glyph, no overlap, and no pool growth in the second pass.
- The report states the LVGL pool peak and the Flash size of the font.

**Cut line.** If task 1 takes more than 60 minutes, use the current 28-pixel font. Move the large font to Sprint 4.

**Result.** The 48-pixel font exists, with a bitmap of 26,115 bytes. The host renders 4 live data page images: 01, 10, 11, and 12. A missing field and out-of-date data both show "--". `tests/test_ski_view.c` covers both. The LVGL pool peak is 17,584 of 21,656 bytes (81%). Nobody recorded the actual time.

### Sprint 4: Report page

**Sprint Goal.** Rest mode switches between the Coach page and the Report page. The Report page passes the host screen check.

**Story.** US-03.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | The state machine adds the page changes on a short DOWN press and a short UP press. CONTEXT sends page 3. | 60 min |
| 2 | The Report page layout: Heygo IQ, distance and turns, core problem, and tip. | 60 min |
| 3 | The report age line: "N MIN AGO". | 30 min |
| 4 | Host tests: one test for each new transition. A short press during audio changes no page. | 30 min |
| 5 | The flow simulator adds the Report page and its self-test items. | 20 min |

**Exit check.**

- Each new transition has a host test.
- The host renders three Report page images: with a report, with no report yet, and with a report older than 60 seconds. Texts at the limit show in full.
- The gate gives exit code 0.

**Result.** Images 13, 14, and 15 show the Report page. Image 15 holds the longest texts. The flow simulator self-test passes 53 of 53. Nobody recorded the actual time.

**After Sprint 4.** D-24 merges the branch `feat/report-live-data` into `main`. At the same time, the team tells the phone application team to change to protocol 0.2.

## 4. Device and integration Sprints

### Sprint 5: board smoke test

**Sprint Goal.** On the board, run a question and one run of ski data with `phone_sim`, and record the measurements.

**Input.** The board at hand.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Flash the firmware. Record the free heap after start. | 30 min |
| 2 | Ask 30 questions in the `phone_sim` echo mode. Use `analyze_log.py` to count the capture stop and playback stop times. | 80 min |
| 3 | Do the 50 page changes and Coach page openings of TC-17 (D-29). | 40 min |
| 4 | Send one run of ski data with a `phone_sim` script. Count the screen update times from the log. Check the glyphs on the board. | 50 min |

**Exit check.**

- The measurement table states the firmware version, the `phone_sim` version, and the test conditions.
- TC-17 can pass in this Sprint, because it needs no phone application.
- The other TCs record measurements only. Per the Definition of Done, a stub phone cannot pass them.

### Sprint 6: Increment 1 with the phone application

**Sprint Goal.** Pass TC-01, TC-06, TC-07, and TC-20 with the real phone application.

**Inputs.** Sprint 5 ended. The phone application supports protocol 0.2.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | TC-01: connect 10 times. | 40 min |
| 2 | TC-06: 100 capture gestures. | 60 min |
| 3 | TC-07: cancel 30 answers. | 40 min |
| 4 | TC-20: reconnect 10 times under N2 conditions. | 60 min |

**Cut line.** If TC-20 does not fit, move it to the start of Sprint 7.

### Sprint 7: Increment 2 with the phone application

**Sprint Goal.** Pass TC-02, TC-03, and the timing part of TC-21 with the real phone application.

**Input.** Sprint 6 ended.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | TC-02: the phone application sends 300 packets, some with missing fields. | 60 min |
| 2 | TC-21 timing: count the time from packet receipt to screen update. | 50 min |
| 3 | TC-03: press DOWN 30 times on the Coach page. Compare the Report page values with the phone log. | 50 min |
| 4 | Check that the run detection of the phone application follows doc 07. | 40 min |

### Sprint 8: Agent and the real service

**Sprint Goal.** Pass TC-08 with the real service.

**Inputs.** Sprint 6 ended. HeygoAI coaching tools are available (Q-01).

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Time 30 questions under N1 conditions. | 90 min |
| 2 | Run 20 context tests. Compare the session that the phone attaches. | 80 min |
| 3 | Sort the latency data and find the 95th percentile. | 30 min |

**If an input is missing.** If Q-01 has no answer, skip this Sprint and do Sprint 9 first.

### Sprint 9: usability test

**Sprint Goal.** Pass TC-15 and the reading part of TC-21 with the U1 usability test.

**Inputs.** Sprint 7 ended. Five adult test users.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Prepare the task script and the gloves. | 20 min |
| 2 | Each of the 5 users does U1 twice: once bare-handed and once with gloves. | 150 min |
| 3 | Each user reads the speed within 2 seconds. Record each result. | 30 min |

### Sprint 10: fixes and increment review

**Sprint Goal.** Fix defects, and review whether the two increments meet the Definition of Done.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Fix the defects from Sprints 5 to 9 in order of severity. | 140 min |
| 2 | Run the affected TCs again. | 40 min |
| 3 | Update the status and the evidence in the documents. | 20 min |

### Sprint 11: Turns page (host)

**Sprint Goal.** The device asks for and receives the turn groups, and the Turns page passes the host screen check.

**Story.** US-05. D-30 sets the scope.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | TURN_SHAPE, TURN_GROUP, and TURN_REQUEST in the packet layer. The cache of 3 groups in the link session. Host tests for each error code. | 50 min |
| 2 | The view: draw one group, round the corners, follow the head while it draws, and make the texts and the key hints. Host tests. | 50 min |
| 3 | The state machine: REPORT to TURNS on a short DOWN press, the next turn on a short OK press. | 30 min |
| 4 | The page on the device: two lines, the rider sprite of 16 frames, and the texts. | 40 min |
| 5 | `phone_sim` and the flow simulator send a demo shape, and the points when the page opens. The host renders the page. | 30 min |

**Exit check.**

- The packet layer and the view have host tests. The Python packet matches the C packet byte for byte.
- The host renders four Turns page images. They show no shape, a line that is still drawing, a heel side turn, and a schematic shape with 12 turns.
- The layout check passes.
- The gate gives exit code 0.

**Result.** Images 16 to 21 show the Turns page. The flow simulator self-test passes 88 of 88. The protocol version stays 0.2. Nobody recorded the actual time.

### Sprint 12: Increment 3 on the board

**Sprint Goal.** Pass TC-05 on the board with `phone_sim`, and then with the real phone application.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Flash the board. Run three runs with `phone_sim`. Run 3 sends a schematic shape. | 60 min |
| 2 | Check the page on the screen: the live line, the rider, the turn values, and "示意". | 60 min |
| 3 | Run the same check with the real phone application. | 80 min |

### Sprint 13: popups (host)

**Sprint Goal.** The device shows the popups from the phone, and the popup page passes the host screen check.

**Story.** US-22. D-31 sets the scope.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | POPUP and POPUP_END in the packet layer. Channel 2 and the replace rule in the link core. Host tests for each error code. | 50 min |
| 2 | The states POPUP_WAIT, POPUP_PLAY, and POPUP_HOLD. A POPUP from every state. The return to the saved page. | 50 min |
| 3 | The popup page and its texts in `popup_view.c`. The 5-second and 10-second timers. Host tests. | 50 min |
| 4 | `phone_sim --popups` and the popup buttons of the flow simulator. The host renders the page. | 50 min |

**Exit check.**

- The packet layer, the link core, and the view have host tests. The Python packet matches the C packet byte for byte.
- The host renders the popup page for each kind, and for the longest texts.
- The layout check passes.
- The gate gives exit code 0.

**Result.** Images 22 to 26 show the popup page. The flow simulator self-test passes 102 of 102. The protocol version stays 0.2. Nobody recorded the actual time. Device tests are NOT RUN.

### Sprint 14: phone popups on the board

**Sprint Goal.** Run the device tests of the phone popups (D-31) on the board with `phone_sim --popups`.

**Input.** The board at hand. The firmware of Sprint 13.

| Order | Task | Time box |
| --- | --- | --- |
| 1 | Flash the board. Start `phone_sim.py --popups 8`. Stay on the live data page. Check the name, the color, the audio, the countdown, and POPUP_END reason 0 of each kind. | 40 min |
| 2 | Receive at least 3 popups in each state: LIVE, REPORT, TURNS, READY, CAPTURE, WAIT_ANSWER, and PLAYBACK. Use `--mode silent` for WAIT_ANSWER. Check the cancel and the return page. | 70 min |
| 3 | Close popups with each key, as a short press and as a hold. Check reason 1, no page change, and no capture. Hold OK before a popup, and release OK during it. The popup must stay. | 40 min |
| 4 | Start `phone_sim.py --popups 6 --popup-gap 5`. Check reason 2 and the saved page. Stop `phone_sim.py` during a popup. Check the countdown and the page after it. | 30 min |
| 5 | Send a popup with audio during an answer. Record whether the first blocks of the popup audio play. | 20 min |

**Exit check.**

- The measurement table states the firmware version, the `phone_sim` version, and the result of each step.
- The `phone_sim` log shows each POPUP_END with its reason and the time after the POPUP.
- TC-22 records measurements only. Per the Definition of Done, a stub phone cannot pass it. The real phone application must run it again.

**Cut line.** If time runs out, drop task 5 first, and then task 4. Each dropped task goes back to the backlog.

## 5. Evidence format

Each Sprint Review reports in this format:

```text
Sprint: N
Sprint Goal: MET / NOT MET
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / PARTIAL / NOT RUN (state the real phone application or phone_sim)
Unverified: each item that has no evidence
```

## 6. Related documents

- [Scrum backlog](03-scrum-backlog.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Document index](README.md).
