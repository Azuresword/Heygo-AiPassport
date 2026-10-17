[简体中文](01-product-requirements.zh_CN.md) · **English**

# HeygoAI and AI Passport: product requirements

Version: 0.9. Date: 2026-10-17. Status: Draft for review.
The Product Owner role has no assigned person.
This document defines a concept. It does not describe a working integration.

## 1. Product goal

The skier reads live data on the device during a run. During a rest break, the skier asks an AI coach and hears the answer.
The skier does not take out the phone. The skier keeps the gloves on.

HeygoAI supplies the ski analysis and the coaching through interfaces that still need evidence.
The phone application supplies speech recognition, speech synthesis, the ski data, and the link to HeygoAI.
AI Passport supplies the buttons, the microphone, the speaker, and a small screen.

The phone stays in a pocket during use. The device reaches the network through the phone.
The device therefore removes the need to take the phone out. It does not remove the need for the phone.

## 2. Users and use moments

| User | Need | Use moment |
| --- | --- | --- |
| Recreational skier | Know the progress of this run without the phone. | During a run. |
| Recreational skier | Know the problem of this run and the fix for the next run, without a long phone report. | After a run, on the lift or during a rest break. |
| Skier who practices a skill | Get one short tip before the next run. | Before the next run. |
| Project tester | Reproduce a failed question or a wrong data value. | An indoor test session. |

The pilot assumes adult users and a small group.
The pilot uses existing boards. It needs no new circuit board or enclosure.

## 3. Product journey

1. The user connects the device to the existing phone application.
2. The device starts on the live data page. During a run, this page shows the live data from the phone.
3. Three pages sit at one level: live data, run report, and AI coach. A short DOWN press goes forward. A short UP press goes back. The last page leads to the first page.
4. On every page, an OK hold opens the Coach page. That press never starts capture.
5. On the Coach page, the user holds OK to ask a question, and releases OK to send it.
6. The Agent answers through speech, with short text on the screen.
7. During the wait or the answer, a short UP or DOWN press cancels the question and changes the page.
8. After a run ends, the live data page shows the report of that run. The Report page shows the full report.
9. The Turns page, between the Report page and the Coach page, shows the turns of the last run, 8 at a time (D-30).
10. The phone can show a popup on the device at any time: a fall alert, a voice message, a problem report, or an achievement (D-31). Any key closes it, and the device goes back to the page before the popup.
11. When the phone detects the start of a run, the device opens the live data page (D-32). During a talk with the coach or a popup, the device opens the page after the talk or the popup ends.

D-29 removes Ski mode and Rest mode. The device cannot check motion, so voice works only on the Coach page.
After the Coach page opens, the user must hold OK again to start capture. An accidental OK press during a run can only open the Coach page.

## 4. Scope and priorities

D-17, D-18, and D-22 set these priorities. [Decision amendments](05-decision-amendments.md) gives the reasons.

| Priority | Increment | Requirements |
| --- | --- | --- |
| P0 | Increment 1: the Coach voice chain | REQ-01, REQ-06, REQ-07, REQ-08, REQ-15, REQ-17, REQ-20 |
| P1 | Increment 2: live data and reports | REQ-02, REQ-03, REQ-21, REQ-23 |
| P1 | Increment 3: the turn shape (D-30) | REQ-05 |
| P1 | Increment 4: phone popups (D-31) | REQ-22 |
| P1 | After Increment 2 | REQ-04, REQ-09, REQ-13 |
| P2 | Later candidates | REQ-10, REQ-11, REQ-12, REQ-18, REQ-19 |
| Deferred | No planned increment | REQ-14, REQ-16 |

Increment 1 builds the complete voice chain once: button, BLE link, phone, Agent, phone, BLE link, speaker.
Increment 2 uses the same BLE link in one direction only, from the phone to the device.
D-22 lets the team develop Increment 2 in parallel with the device acceptance of Increment 1. Each increment has its own acceptance.

### Exclusions from the first version

- Metric charts, the offline report cache, and human intercom. They come after Increment 2. The turn view is Increment 3.
- Automatic emergency detection or rescue dispatch. The fall popup (D-31) only shows an alert from the phone. The phone owns the escalation.
- Certified instruction, medical advice, or injury prevention claims.
- Native measurement of speed, position, edge angle, or body posture.
- Speed rankings, speed badges, or any content that rewards high speed.
- Continuous listening, a wake word, or an open microphone by default.
- A phone-free radio intercom.
- New hardware, manufacturing, payments, or a public social network.
- BLE encryption, a pairing passkey, or a binding check.
- Privacy and authorization features.
- Power management, including sleep modes and low-power link settings.

## 5. Capability boundary

| Area | Evidence or status | Product consequence |
| --- | --- | --- |
| Screen | 240 × 320. No evidence of a touch interface. | Show status, short text, and a small set of numbers. |
| Buttons | UP, DOWN, and OK share one ADC input. | Use one button at a time. |
| Audio | Microphone, speaker, and PCM APIs exist. No echo cancellation. | Stream audio to the phone in small blocks. Never record and play together. |
| Resources | ESP32-C3, 8 MB Flash, no PSRAM. | Keep speech recognition, synthesis, the Agent, and data calculation off the device. |
| Power | 500 mAh battery. | D-18 defers power management. Claim no runtime. |
| Motion and location | No evidence of an IMU, GNSS, or barometer on the device. | The phone calculates all ski data. The device only shows it. |
| HeygoAI | No API contract and no sample data yet. | The phone application handles the integration. |
| Phone application | It exists. Another team develops it. | D-23: the device team defines the data fields, and the phone application team adapts to them. The [BLE protocol draft](07-ble-protocol-draft.md) gives the packet format. |

## 6. Requirements matrix

Each row has test stage Prototype. Owners are roles with no assigned person.
[Validation and decisions](04-validation-and-decisions.md) defines each matching TC.

### P0, Increment 1

| ID | Value and required behavior | Owner | Acceptance target | Dependency |
| --- | --- | --- | --- | --- |
| REQ-01 | Let the phone find and connect to the device. The device advertises its name. The phone connects without a passkey. | Phone and firmware. | TC-01: 9 of 10 connection attempts finish within 180 seconds. | The phone application must accept the BLE protocol draft. |
| REQ-06 | Capture a question that the user starts. Stream each block to the phone while the user speaks. Show capture status and recognition failures. | Firmware and phone. | TC-06: release stops capture within 100 ms in 100 trials. Capture ends by 20 seconds. | The phone must accept streamed audio and run speech recognition. |
| REQ-07 | Play the spoken answer as it arrives. Allow the user to stop it. | Firmware and phone. | TC-07: playback stops within 200 ms of the cancel command in 30 trials. No canceled answer resumes. | The phone must supply synthesized speech. |
| REQ-08 | Ask the Agent with a button. The phone attaches the current ski session. | Agent, phone, and firmware. | TC-08: the first audible answer starts within 5 seconds of release at the 95th percentile. The Agent marks claims that lack evidence. | HeygoAI coaching tools remain open. |
| REQ-15 | Support three buttons, gloves, and readable Chinese status text. | Design and firmware. | TC-15: every required glyph shows. At least 4 of 5 users complete each core task. | Test gloves and readability on hardware. |
| REQ-17 | Three pages at one level, with UP and DOWN to change the page. An OK hold on any page opens the Coach page. Capture runs only while the user holds OK on the Coach page. | Firmware and design. | TC-17: in 50 page changes and Coach page openings, each page is as expected. The hold that opens the Coach page never starts capture. | No automatic motion detection exists. D-28: the phone can change the device to the Report page. That never starts capture. |
| REQ-20 | Explain link failures. Support recovery. | Firmware and phone. | TC-20: 10 reconnect trials create no duplicate question. No stale answer plays. | Test link loss and phone background behavior. |

### P1, Increments 2 to 4

| ID | Value and required behavior | Owner | Acceptance target | Dependency |
| --- | --- | --- | --- | --- |
| REQ-02 | Show the latest ski data packet from the phone. A missing field shows "--" and never 0. Mark data that is out of date. | Phone and firmware. | TC-02: in 300 data packets, the device shows the latest packet each time. Each missing field shows "--". | The phone application sends LIVE_STATS as doc 07 defines. |
| REQ-21 | During a run, show live data on the live data page: speed, vertical drop, distance, turn count, and run time. The phone sends one packet each second. | Firmware and design. | TC-21: the screen updates within 300 ms of the packet at the 95th percentile. At least 4 of 5 users read the speed correctly within 2 seconds. | REQ-02. |
| REQ-23 | When the phone starts a run, open the live data page at once. Show the run number and "--" for each value until the first packet. During a talk with the coach, open the page after the talk ends. During a popup, open the page after the popup closes. | Phone and firmware. | TC-23: in 20 runs with phone_sim, the screen changes to the live data page within 300 ms of RUN_START at the 95th percentile. A run start during a talk or a popup opens the live data page after the talk or the popup ends, each time. | REQ-21. RUN_START as doc 07 defines (D-32). |
| REQ-03 | After each run, show the report of that run. It holds the distance, the turn count, the Heygo IQ, the core problem, and a tip of at most 20 characters. | Firmware and design. | TC-03: in 30 short DOWN presses on the Coach page, the Report page shows within 500 ms each time. Every value matches the phone log. | REQ-02. |
| REQ-05 | After each run, show the shape of that run in groups of 8 turns, and replay one turn. The user moves from turn to turn with the keys. The page shows the action of each key. Each turn shows its side, speed, edge angle, and radius. A schematic shape shows "示意". | Firmware and design. | TC-05: in 20 runs with phone_sim and up to 255 turns, each key press shows its turn within 500 milliseconds. Each turn value matches the phone log. A schematic shape shows "示意" each time. | REQ-03. TURN_SHAPE, TURN_GROUP, and TURN_REQUEST as doc 07 defines. |
| REQ-22 | Show a popup from the phone at once, on every page and in every state. Four kinds: fall alert, voice message, problem report, and achievement. Play the popup audio, then count down 10 seconds. Any key closes the popup. Then go back to the page before the popup. | Phone and firmware. | TC-22: in 40 popups with phone_sim, each popup shows within 500 ms on every page and in every Coach state. Each popup closes after its audio and 10 seconds, or at a key press. The device then shows the page before the popup each time. | POPUP and POPUP_END as doc 07 defines. The phone owns the escalation of a fall alert. |

### P1, after Increment 2

| ID | Value | Why it is not in Increment 2 |
| --- | --- | --- |
| REQ-04 | Show metric trends with units and gaps. | Increment 2 checks the numbers on the screen first. Charts need more data points and more screen work. |
| REQ-09 | Hold a button to speak with a companion in a shared room. | It needs a room service, a second endpoint, and phone intercom support. |
| REQ-13 | Keep the last report readable after a network failure, a link loss, or a device restart. | Increment 2 keeps the last data packet in RAM only. A Flash copy is separate work. |

### P2

| ID | Value |
| --- | --- |
| REQ-10 | Offer one practice goal based on evidence. |
| REQ-11 | Offer quests or badges for practice and review. They never reward speed or dangerous terrain. |
| REQ-12 | Save a short reflection for a ski session. |
| REQ-18 | Compare two ski sessions, and show differences in conditions. |
| REQ-19 | Share a selected report with consent. |

### Deferred

| ID | Value | Reason |
| --- | --- | --- |
| REQ-14 | Access control and data deletion. | D-13 excludes privacy work. |
| REQ-16 | Keep audio, link, and power use within measured device limits. | D-18: the first version builds no power management. |

## 7. Experience details

### Coach page

The Coach page always shows the destination: the AI coach.
The page shows one state at a time: ready, listening, thinking, answering, or failed.
Each state uses text and an icon. Color never carries a state alone.

An OK hold of 500 ms starts capture. The device streams each block to the phone at once.
The device does not wait for the gateway. The phone holds the question until the gateway accepts it.
When the user releases OK, the device stops capture and tells the phone that the question ended.
One question lasts at most 20 seconds.

The phone streams the answer back block by block. The device plays each block as it arrives.
The screen can show a short transcript or a short answer.
If recognition fails, the screen asks the user to repeat the question.
The device offers no transcript editor. Three buttons cannot edit text.

### Page change and cancel

A short UP or DOWN press changes the page. The three pages form a loop.
During capture, the wait, or the playback, a short UP or DOWN press cancels the question first, and then changes the page (D-29).
Playback stops within 200 ms of that command. Capture stops within 100 ms of that command.
An UP hold and a DOWN hold have no action.
Actions inside a page will use a short OK press. No page uses it yet.
A hold never also triggers a short press.

### Audio rules

The device never records and plays at the same time.
An OK hold during playback stops the playback first. The microphone starts after that.
If the link fails, the device stops capture and shows the failure.
A canceled answer never resumes.

### Live data page

D-21 puts live data on the live data page. During a run, the eyes belong on the slope. So the speed is the largest number, and each other number has a name.

| Area | Content | Example |
| --- | --- | --- |
| Title | The page state | LIVE RUN |
| Status line | The run number for today and the run state | 第 4 趟 · 滑行中 |
| Main number | The current speed | 38 KM/H |
| Data panel | Four cells, each with a name and a value | 落差 312 m, 距离 1.9 km, 转弯 24, 时间 02:31 |
| Button hint | No change | Hold OK to rest |

- After the phone detects the start of a run, it sends RUN_START (D-32). Then it sends one packet each second. The device updates the screen for each packet.
- At RUN_START, the status line shows the new run at once, for example "第 5 趟 · 滑行中". Each number shows "--" until the first packet. A RUN_START for the run on the screen keeps the data.
- RUN_START on the Report page, the Turns page, or the idle Coach page opens the live data page. On the live data page, only the data changes.
- During a talk with the coach, the device keeps the run start. When the talk ends, or the user changes the page, the device opens the live data page. The run start cancels nothing.
- During a popup, the popup stays. When the popup closes, the device opens the live data page, not the page before the popup.
- The status line shows "skiing" or "stopped". After 5 seconds with no new packet, it shows "data out of date", the speed shows "--", and the panel keeps its numbers.
- Before any data arrives, the status line shows "waiting for phone data", and each number shows "--".
- After a run ends, the status line shows "report on the way". When the report arrives, the main number changes to the Heygo IQ, and the panel shows the core problem and the tip.
- The speed shows as a number only. No color, animation, or sound marks a high speed.
- If the phone has no speed, it sends the unknown value, and the screen shows "--".

### Run report

D-26: after each run, the phone sends the report of that run. D-27 sets two places for the report:

- **The live data page.** After a run ends, this page shows the Heygo IQ, the core problem, and the tip of that run until the next run starts.
- **The Report page.** Use UP and DOWN to go to the Report page and read the full report.

| Field | Format | Example |
| --- | --- | --- |
| Run number | Integer | Run 3 |
| Distance | As in the live data | 1.9 km |
| Turn count | Integer | 25 turns |
| Heygo IQ | Integer from 0 to 200 | 82 |
| Core problem | At most 12 Chinese characters | Weight too far back |
| Short tip | At most 20 Chinese characters | Next run, keep both hands in front |

- On the Report page, an OK hold opens the Coach page, as on every page. The button hint shows "Hold OK for the coach".
- D-28: when the phone sends SHOW_PAGE, the device changes to the Report page from the live data page or the idle Coach page. During capture, the wait, and the playback, the device ignores the message, so it never stops the user.
- Before the first report arrives, the Report page shows "The report shows after a run".
- 60 seconds after a report arrives, the Report page shows "N MIN AGO".
- After a link loss, the device keeps the last report from RAM on the screen.

### Turns page

D-30 adds the Turns page to the page loop of D-29, between the Report page and the Coach page. The phone sends the turn count of each run after its report. The device asks the phone for groups of 8 turns.

- The page shows 8 turns at a time. When the page opens, it draws the first 8 turns from left to right, one point each 40 milliseconds. A rider sprite sits at the head of the line.
- Then the page marks the selected turn in the accent color. The rider moves along it once after each key press.
- Short UP and DOWN presses change the page, as on every page: UP to the Report page, DOWN to the Coach page. An OK hold opens the Coach page.
- Inside the page, a short OK press selects the next turn. An UP hold selects the previous turn. A DOWN hold goes to turn 1 of the next page of 8 turns. The page follows the selected turn.
- A bar at the bottom of the accent color has two rows. Row 1 shows the short presses: "▲报告", "●下一弯" or "●跳过", "▼教练".
- Row 2 shows the holds that work now: "长按▲上一弯", "长按▼下一页", or both as "长按 ▲上一弯 ▼下一页". With no hold, it shows "长按●问教练".
- A thin bar under the line shows where the page is in the whole run.
- Below the shape, the page shows "TOE SIDE" or "HEEL SIDE", the turn number, the speed, the edge angle, and the radius.
- A shape that does not come from a measured track shows "示意" (schematic).
- Before the first shape arrives, the page shows "暂无弯型数据" (no turn data yet).

### Popups

D-31 lets the phone show a popup on the device at any time. The popup page is not in the page loop.

| Kind | Screen text | Color | Badge | Audio in the phone_sim example |
| --- | --- | --- | --- | --- |
| 1, fall alert | 摔倒提醒, FALL ALERT | Red | A rider on the back, with stars above | Yes |
| 2, voice message | 语音播报, BROADCAST | Lime accent | A megaphone with sound waves | Yes |
| 3, problem report | 问题报告, RUN ISSUE | Orange | A magnifier with "!" | No |
| 4, achievement | 成就报告, ACHIEVED | Gold | A cup with flashing stars | No |

- Each badge has 4 frames that play in a loop. When a popup comes, the badge falls in and bounces, and the text box rises. A fall alert also shakes its badge.

- A popup interrupts at once, in every state. Capture stops, and the device cancels the question. The wait stops. An answer that plays stops.
- The popup shows a title of at most 12 Chinese characters and a body of at most 40 Chinese characters. With no title, the device shows a default title for the kind.
- If the popup has audio, the device plays it, then counts down 10 seconds. With no audio, the countdown starts at once.
- If the audio does not start within 5 seconds, the countdown starts.
- The footer shows "等待语音" (waiting for audio), "正在播报" (playing), or "N 秒后返回" (back in N seconds). The bar at the bottom shows "按任意键关闭" (any key closes).
- Any key only closes the popup. It does not change the page and does not start capture. The release of an OK hold that started before the popup does not close it.
- After the countdown, or at a key press, the device goes back to the page before the popup. That is the live data page, the Report page, or the Turns page. From the Coach page, the device goes back to the ready state.
- A new popup replaces the old one. The device keeps the page from before the first popup.
- The phone can start a run during the popup, or during the talk that the popup stopped. Then the popup closes to the live data page (D-32).
- A fall popup also closes after 10 seconds with no user action. The phone must start the escalation, for example a call to an emergency contact. The device only shows the alert.

### Data fields

D-23 lets the device team define the fields. The phone application team calculates and sends them. Section 6 of doc 07 gives the format of each field.

| Field | Unit and format | Live data | Run report |
| --- | --- | --- | --- |
| Speed | km/h, integer | Yes | No |
| Vertical drop | m, integer | Yes | No |
| Distance | Below 1,000 m in m, else in km with one decimal | Yes | Yes |
| Turn count | Integer | Yes | Yes |
| Run time | min:s. From 1 hour, h:min | Yes | No |
| Heygo IQ | Integer from 0 to 200 | No | Yes |
| Core problem | At most 12 Chinese characters | No | Yes |
| Short tip | At most 20 Chinese characters | No | Yes |

## 8. Success targets

These numbers are test targets, not measurements.

| Outcome | Target |
| --- | --- |
| Connect | 9 of 10 attempts finish within 180 seconds. |
| Ask and stop | At least 4 of 5 users ask a question and stop an answer with gloves, without help. |
| Hear the answer | The first audible answer starts within 5 seconds of release at the 95th percentile. |
| Stop quickly | Capture stops within 100 ms of release. Playback stops within 200 ms of the cancel command. |
| Read live data | At least 4 of 5 users read the speed correctly within 2 seconds. |
| Trust the data | A missing value never shows as 0. |

## 9. Hardware and pilot limits

The prototype uses existing AI Passport boards and available phones.
The sources do not show water resistance, cold operation, wind rejection, or outdoor readability.
Indoor success does not support a claim about slope use.
The phone sets the accuracy of speed and vertical drop. The device does not check it.
Manufacturing, certification, packaging, and support stay outside this concept.

## 10. Related documents

- [System architecture](02-architecture.md).
- [Scrum backlog](03-scrum-backlog.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Decision amendments](05-decision-amendments.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Four-hour Sprint roadmap](08-sprint-roadmap.md).
- [Document index](README.md).
