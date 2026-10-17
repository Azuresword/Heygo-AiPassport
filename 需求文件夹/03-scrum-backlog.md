[简体中文](03-scrum-backlog.zh_CN.md) · **English**

# Scrum backlog and delivery plan

Version: 0.10. Date: 2026-10-17. Status: In progress. Sprint 0 ended. The Sprint 1 device code is in the repository. Device acceptance has not started. Increment 2 is in the backlog. The team closed no story.

Related documents: [Requirements](01-product-requirements.md), [Architecture](02-architecture.md), [Validation and decisions](04-validation-and-decisions.md), [Decision amendments](05-decision-amendments.md), [Sprint 0 plan](06-sprint-0-plan.md), [BLE protocol draft](07-ble-protocol-draft.md), and [Four-hour Sprint roadmap](08-sprint-roadmap.md).

## 1. Product Goal

The skier reads live data on the device during a run. During a rest break, the skier asks the AI coach a question by voice and hears the answer.
The skier keeps the phone in a pocket and the gloves on.

Two increments move toward this goal:

- **Increment 1, the Coach voice chain.** P0 stories US-01, US-06, US-07, US-08, US-15, US-17, and US-20.
- **Increment 2, live data and reports.** P1 stories US-02, US-21, and US-03. D-22 lets the team develop it in parallel with the device acceptance of Increment 1.

## 2. Planning assumptions

| Item | Assumption | Required check |
|---|---|---|
| Stage | Concept prototype on existing AI Passport boards. | The Product Owner agrees the scope. |
| People | One firmware Developer. The phone application team works in parallel. | Check capacity before each Sprint. |
| Hardware | One AI Passport and one phone. | The board is not at hand. Sprint 0 therefore runs on the host only. |
| Connection | D-08 selects a BLE phone bridge. | Test link stability and phone background behavior. |
| Phone application | It exists. Another team develops it. Per D-23, the device team defines the Increment 2 fields, and the phone application team adapts to them. | That team reviews doc 07 and checks that it can calculate each field (Q-16). |
| HeygoAI | Interfaces and coaching tools remain unknown. | The phone application team gets access. |

Story points compare uncertainty and effort. They do not represent hours.
The plan assumes no velocity.

## 3. Scrum roles

| Role | Person | Responsibility |
|---|---|---|
| Product Owner | Not assigned | Owns the Product Goal and orders the backlog. |
| Scrum Master | Not assigned | Supports Scrum and clears blockers. |
| Developers | Not assigned | Select work and deliver usable increments. |

The user made every scope decision so far, including D-17.

## 4. Product Backlog

| Story | Requirement | Priority | Points | Owner | Sprint |
|---|---|---|---|---|---|
| US-01 Device connection | REQ-01 | P0 | Re-estimate | Firmware and phone | Sprint 1 code, Sprint 6 acceptance |
| US-06 Voice capture | REQ-06 | P0 | 5–8 | Firmware | Sprints 0–1 code, Sprint 6 acceptance |
| US-07 Speech playback | REQ-07 | P0 | 3–5 | Firmware | Sprints 0–1 code, Sprint 6 acceptance |
| US-08 Button Agent exchange | REQ-08 | P0 | Re-estimate | Phone and firmware | Sprint 1 code, Sprint 8 acceptance |
| US-15 Gloves and Chinese text | REQ-15 | P0 | Re-estimate | Firmware | Sprint 1 code, Sprint 9 acceptance |
| US-17 Page navigation | REQ-17 | P0 | Re-estimate | Firmware | Sprint 0 code, D-29 rewrite, Sprint 5 acceptance |
| US-20 Recovery | REQ-20 | P0 | Re-estimate | Firmware and phone | Sprint 1 code, Sprint 6 acceptance |
| US-02 Data sync | REQ-02 | P1, Increment 2 | 3–5 | Phone and firmware | Sprint 2 code, Sprint 7 acceptance |
| US-21 Live data on the live data page | REQ-21 | P1, Increment 2 | 5–8 | Firmware and design | Sprint 3 code, Sprints 7 and 9 acceptance |
| US-03 Run report | REQ-03 | P1, Increment 2 | 3–5 | Firmware and design | Sprint 4 code, Sprint 7 acceptance |
| US-05 Turn shape and replay | REQ-05 | P1, Increment 3 (D-30) | 5–8 | Phone, firmware, and design | Sprint 11 code, Sprint 12 acceptance |
| US-22 Phone popups | REQ-22 | P1, Increment 4 (D-31) | 5–8 | Phone and firmware | Sprint 13 code, Sprint 14 acceptance |
| US-04 Metric charts | REQ-04 | P1 | 3–5 | Firmware | Later |
| US-09 Human push-to-talk | REQ-09 | P1 | 8–13 | Phone and firmware | Later |
| US-13 Cached reports | REQ-13 | P1 | 5–8 | Firmware | Later |
| US-10 Focused practice | REQ-10 | P2 | 3–5 | Agent | Later |
| US-11 Quests and badges | REQ-11 | P2 | 3–5 | Product | Later |
| US-12 Reflection journal | REQ-12 | P2 | 3–5 | Phone | Later |
| US-18 Session comparison | REQ-18 | P2 | 3–5 | Data | Later |
| US-19 Share link | REQ-19 | P2 | 3–5 | Phone | Later |
| US-14 Privacy and authorization | REQ-14 | Deferred | None | None | None |
| US-16 Resource limits | REQ-16 | Deferred | None | None | None |

"Re-estimate" marks a P0 story whose scope shrank in version 0.4. The Developers set new points.

### 4.1 P0 progress

This table applies to commit `11b10cc`, dated 2026-10-17. Section 8 gives the progress of Increment 2.
"Device code" means that the code is in the repository. It does not mean that the story meets the Definition of Done.
Per section 9, each story needs device tests. Some stories also need the real phone application.

| Story | Device code | Host evidence | Acceptance test | Still needed to close |
|---|---|---|---|---|
| US-01 | Committed. `ble_link.c` advertises as `HeygoPP-XXXX` and accepts one phone at a time. | `test_link_core.c`, `test_phone_sim.py` | TC-01 NOT RUN | The board, and a phone application that follows doc 07. |
| US-06 | Committed. `voice.c` encodes the capture to ADPCM blocks and sends each block at once. 20 s limit. A short UP or DOWN press cancels and changes the page (D-29). | `test_coach_paths.c`, `test_ima_adpcm.c` | TC-06 NOT RUN | The board, and a phone application that accepts streamed audio. |
| US-07 | Committed. Playback follows the playback start rules. A playback stop waits for the speaker. | `test_play_gate.c`, `test_coach_paths.c` | TC-07 NOT RUN | The board, and a phone application that supplies speech. |
| US-08 | Device part only: the 10 s wait and the AGENT_STATE text on the screen. The phone and the Agent own the session context and the evidence labels. | `test_link_core.c` | TC-08 NOT RUN | HeygoAI coaching tools (Q-01). |
| US-15 | Committed. `coach_input.c` maps the three buttons. The Chinese fonts and screens are in place. | `test_coach_input.c`, glyph check, host screen check | TC-15 NOT RUN | A glyph check on the board, and the U1 usability test. |
| US-17 | Committed. `coach.flow.json` generates the state machine. | `test_coach_flow.c`, `test_coach_paths.c`, simulator self-test | TC-17 NOT RUN | The board. |
| US-20 | Committed. A link loss stops audio. The screen shows a message after 15 s with no phone packet. The device drops packets with an old `xid`. | `test_link_core.c`, `test_coach_paths.c` | TC-20 NOT RUN | The board, and the phone application. |

The simulator and `phone_sim` show protocol behavior only. They cannot satisfy a device test.

Known gaps, from section 13 of the [firmware change notes](../Firmware/docs/heygo-changes.md):

- A PLAY_START that arrives during capture is lost. The device then shows a timeout. D-20 forbids the phone to send it at that time. The code needs no change. The phone application team must still accept this rule.
- The battery label on the screen stays empty. D-18 keeps the battery level on the screen.
- The body label shows four lines. Longer text ends with three dots. The device speaks the full answer.

## 5. P0 stories

### US-01: Device connection

As a skier, I want my phone to find and connect to my device, so that the device can use the phone link.

- Acceptance: The device advertises its name after start-up. The phone application finds it and connects.
- Acceptance: The device accepts one phone at a time.
- Acceptance: The link uses no encryption, passkey, or binding check, as D-12 states.
- Dependency: The phone application accepts doc 07 (Q-13).
- Risk: Any nearby BLE client can connect. The first version accepts this risk.
- Verification: TC-01.

### US-06: Voice capture

As a skier, I want to hold one button and ask a question, so that the Agent hears my first word.

- Acceptance: On the Coach page, a 500 ms OK hold starts the microphone. The screen shows that the microphone works.
- Acceptance: The device streams each 20 ms block to the phone at once. It does not wait for the gateway.
- Acceptance: Release stops capture within 100 ms. The device tells the phone that the question ended. One question lasts at most 20 seconds.
- Acceptance: A short UP or DOWN press cancels the question and changes the page. Capture then stops within 100 ms. The device tells the phone to drop the audio.
- Acceptance: A recognition failure from the phone produces a short retry message.
- Dependency: US-17, and a phone application that accepts streamed audio.
- Verification: TC-06.

### US-07: Speech playback

As a skier, I want to hear the answer and stop it at once, so that I control the device.

- Acceptance: The device plays each block as it arrives, and shows that playback runs.
- Acceptance: A short UP or DOWN press stops playback within 200 ms, and then changes the page. The device tells the phone.
- Acceptance: A canceled answer never resumes, even after a late block or a reconnect.
- Dependency: US-17, and a phone application that supplies synthesized speech.
- Verification: TC-07.

### US-08: Button Agent exchange

As a skier, I want to ask the AI coach about my skiing, so that I get one useful answer during the break.

- Acceptance: The phone attaches the current ski session to the question.
- Acceptance: The answer marks claims that lack evidence.
- Acceptance: A late answer never replaces a newer question.
- Dependency: US-06, US-07, and the HeygoAI coaching tools.
- Verification: TC-08.

### US-15: Gloves and Chinese text

As a skier, I want to use the device with gloves and read every status in Chinese.

- Acceptance: Every core task uses a hold of one button. No task needs touch.
- Acceptance: Every Chinese status string shows without a missing glyph or clipping.
- Verification: TC-15.

### US-17: Page navigation

As a skier, I want to move between three pages with UP and DOWN. I also want to reach the coach with an OK hold at any time. Then I never talk to the device by accident while I ski.

- Acceptance: The device starts on the live data page. The pages loop in the order live data, report, and coach. A short DOWN goes forward. A short UP goes back.
- Acceptance: An OK hold on the live data page or the Report page opens the Coach page. That press never starts capture.
- Acceptance: A page change during audio stops all audio first (D-29).
- Verification: TC-17.

### US-20: Recovery

As a skier, I want a clear message when the link fails, so that I know what to do.

- Acceptance: Link loss stops audio and shows the link state.
- Acceptance: A phone that stays silent for 15 seconds produces a clear message.
- Acceptance: A reconnect creates no duplicate question and plays no stale answer.
- Verification: TC-20.

## 6. Increment 2 stories

The phone calculates all Increment 2 data. The device receives it, keeps the last packet, and shows it.
Section 6 of the [BLE protocol draft](07-ble-protocol-draft.md) gives the fields and the packet format.

### US-02: data sync

As a skier, I want the numbers on the device to match the numbers on the phone, so that I can trust them.

- Acceptance: after a LIVE_STATS packet arrives, the device always shows the latest packet.
- Acceptance: a field with an unknown value from the phone shows "--", never 0.
- Acceptance: if a payload is too short, the device sends ERROR code 2 and keeps the old packet on the screen.
- Acceptance: during a run, after 5 seconds with no new packet, the speed shows "--" with an out-of-date mark. 60 seconds after a report arrives, the Report page shows its receive time.
- Dependency: the phone application supports protocol 0.2 and accepts the LIVE_STATS and RUN_REPORT fields (Q-16).
- Verification: TC-02.

### US-21: live data on the live data page

As a skier, I want to read my speed and this run at a glance. Then I need not take out the phone.

- Acceptance: during a run, the live data page shows the speed, vertical drop, distance, turn count, run time, and run number. The phone sends one packet each second.
- Acceptance: the speed is the largest number on the page.
- Acceptance: after a run ends, the live data page shows the Heygo IQ, the core problem, and the tip of that run. They stay until the next run starts (D-27).
- Acceptance: before any data arrives, the page shows "Waiting for phone data".
- Acceptance: no color, animation, or sound marks a high speed.
- Dependency: US-02.
- Verification: TC-21.

### US-03: run report

As a skier, I want the problem of each run and one tip after the run. Then I know what to change on the next run.

- Acceptance: a short DOWN on the live data page, or a short UP on the Coach page, opens the Report page (D-29).
- Acceptance: the Report page shows the distance, turn count, Heygo IQ, core problem, and tip of the last run.
- Acceptance: the core problem holds at most 12 Chinese characters, and the tip at most 20. Both show in full, with no cut.
- Acceptance: before the first report arrives, the Report page shows "The report shows after a run".
- Acceptance: after a link loss, the Report page keeps the last report and shows its receive time.
- Acceptance: an OK hold on the Report page opens the Coach page, as on every page.
- Acceptance: when the phone sends SHOW_PAGE, the device opens the Report page from the live data page or the idle Coach page. During capture, the wait, and the playback, the page does not change (D-28).
- Dependency: US-02 and US-17.
- Verification: TC-03.

### US-05: turn shape

As a skier, I want to see the shape of my last run and each turn in it. Then I can connect the tip of the report to a turn.

- Acceptance: the Turns page sits in the page loop between the Report page and the Coach page. Short UP and DOWN presses change the page, as on every page.
- Acceptance: the page shows 8 turns at a time, and follows the selected turn. It draws the first page point by point. The rider sits at the head of the line.
- Acceptance: inside the page, an UP hold selects the previous turn, and a DOWN hold goes to the next page of 8 turns.
- Acceptance: the key bar shows the short presses in row 1, and the holds that work now in row 2.
- Acceptance: a run of 255 turns works. The device keeps at most 3 groups and asks the phone for the others.
- Acceptance: the Turns page shows the run of the last RUN_REPORT. It shows "加载中" until the TURN_SHAPE of that run arrives, and "暂无弯型数据" for a run with no turn data.
- Acceptance: a short OK press selects the next turn. After the last turn, it selects the first turn.
- Acceptance: each turn shows its side, speed, edge angle, and radius. An unknown value shows "--" and never 0.
- Acceptance: a shape with source 1 shows "示意". Before the first shape arrives, the page shows "暂无弯型数据".
- Acceptance: an OK hold on the Turns page opens the Coach page and starts no capture.
- Dependency: US-03. Q-02 for a measured track.
- Verification: TC-05.

### US-22: phone popups

As a skier, I want the phone to show me an important message on the device at once. Then I do not take out the phone for a fall alert, a voice message, a problem, or an achievement.

- Acceptance: a popup shows at once on every page and in every Coach state. Capture stops and the device cancels the question. The wait stops. An answer that plays stops.
- Acceptance: the four kinds show their own name and color. The fall alert uses red.
- Acceptance: a popup with audio plays it, then counts down 10 seconds. A popup with no audio counts down at once. If the audio does not start within 5 seconds, the countdown starts.
- Acceptance: any key closes the popup. It changes no page and starts no capture. The release of an OK hold that started before the popup does not close it.
- Acceptance: after the countdown or a key press, the device sends POPUP_END with reason 0 or 1. Then it shows the page before the popup. The Coach page shows its ready state.
- Acceptance: a new popup replaces the old one. The device sends POPUP_END with reason 2 for the old one and keeps the saved page.
- Acceptance: a POPUP with a bad field gets ERROR code 7, 4, or 2, and shows nothing.
- Acceptance: during a popup, CONTEXT and DEVICE_STATUS send mode 0 and page 6.
- Dependency: US-17 and US-20. The phone owns the escalation of a fall alert.
- Verification: TC-22.

## 7. Other P1 stories, P2 stories, and Deferred stories

These stories keep their numbers. Their full text returns when a story enters a Sprint.
Commit `970bb8e` holds the version 0.3 text.

## 8. Sprint order

Each Sprint lasts four hours. The [Four-hour Sprint roadmap](08-sprint-roadmap.md) gives the goal, the tasks, the inputs, and the cut line of each Sprint.

| Sprint | Sprint Goal | Status |
|---|---|---|
| 0 | Build the device logic for one Coach question. | Done, commit `a917aaa`. |
| 1 | Connect voice and the BLE link. | Code done, commits `941f4e7` and `11b10cc`. The planned hardware acceptance moves to Sprints 5 to 8. |
| 2 to 4 | Increment 2: data layer, live data page, Report page. Host only. | Code committed and merged into `main`. Device tests NOT RUN. |
| 5 | Board smoke test, with `phone_sim` as the phone. | Not started. Needs the board. |
| 6 to 9 | Accept both increments with the real phone application, the real service, and test users. | Not started. Needs a phone application with protocol 0.2. |
| 10 | Fix defects, and review both increments. | Not started. |
| 11 | Increment 3: the Turns page and the three turn messages. Host only. | Code on the branch `feat/turn-view`. Device tests NOT RUN. |
| 12 | Accept Increment 3 on the board with `phone_sim`, then with the real phone application. | Not started. Needs the board and a phone application that answers TURN_REQUEST. |
| 13 | Increment 4: the popup page and the two popup messages. Host only. | Code on the branch `feat/app-popups`. Host tests pass. Device tests NOT RUN. |
| 14 | Accept Increment 4 on the board with `phone_sim --popups`. | Not started. Needs the board. |

## 9. Definition of Done

Each selected slice of Increment 1 or Increment 2 meets every applicable check:

- The user completes the action on a physical AI Passport.
- The path uses the real phone application where the slice needs it.
- Host tests, repository checks, and the firmware build pass.
- Device tests pass under recorded conditions.
- Chinese text, button actions, mode changes, and cancel behavior pass their tests.
- Timing measurements meet the slice limits.
- English and Chinese documents describe the same behavior.
- The team records every remaining gap.

Report Build, Host tests, Device tests, and unverified items separately.
A simulator or a stub cannot satisfy a device test or a real-service test.

## 10. Changes

1. Record the feedback and the affected requirement.
2. The Product Owner orders the backlog again.
3. The Developers update the estimates.
4. Record each accepted change in the decision log.
