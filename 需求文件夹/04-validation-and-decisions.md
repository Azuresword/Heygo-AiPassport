[简体中文](04-validation-and-decisions.zh_CN.md) · **English**

# Validation, open questions, and decisions

Version: 0.8. Date: 2026-10-17. Status: Draft for review.
Every product test below has status NOT RUN.
Document checks do not count as firmware or device tests.

## 1. Evidence rules

Record these items for each test: the firmware version, the phone application version, the phone model and system, the data source, and the date.
Record measurements apart from targets.
A stub phone proves protocol behavior only. It does not prove the real phone application.
A local audio loop does not prove a real Agent answer.

## 2. Test conditions

### N1: indoor reference link

Use one AI Passport, one supported phone, and the real phone application.
For each latency test, measure the phone round trip to the gateway, the jitter, and the loss.
The proposed limits are a 150 ms round trip and 30 ms of jitter at the 95th percentile, with 1% loss.
For 30 sorted samples, the 29th sample is the 95th percentile.
Measure audio delay with a synchronized recording. Never subtract unsynchronized clocks.

### N2: failures

Test each case separately:

- A 30-second link loss.
- A phone application in the background.
- A phone permission failure.
- A BLE MTU below 172.
- A device clock offset of five minutes.

In each case the device must stop invalid audio and show a specific state.

### U1: usability at rest

Five adults run the same task script indoors, with bare hands and with gloves.
Nobody helps during an attempt.

1. Enter Rest mode.
2. Ask a question.
3. Stop an answer.
4. Return to Ski mode.
5. On the Ski page, read the speed within 2 seconds. Increment 2 adds this task.

## 3. Acceptance matrix for P0

| Test | Conditions | Pass rule | Method and role |
| --- | --- | --- | --- |
| TC-01 | 10 connection attempts on N1 with the phone application. | At least 9 finish within 180 seconds. The device accepts one phone at a time. | Device demonstration. Phone and firmware. |
| TC-06 | 100 capture gestures on the Coach page. Include early release, cancel, the 20-second limit, recognition failure, and link loss. | Capture stops within 100 ms of release or cancel. No capture exceeds 20 seconds. The first block leaves the device without a wait for the gateway. The device sends an end or cancel message every time. | Audio measurement and state tests. Firmware and phone. |
| TC-07 | Cancel 30 answers at different stages. Include blocks that arrive after the cancel. | Playback stops within 200 ms of the cancel command. Old audio never resumes. No capture starts during playback. | Audio timing and protocol tests. Firmware. |
| TC-08 | 30 timed questions through the real service on N1, and 20 context cases. | The first audible answer starts within 5 seconds of release at the 95th percentile. Each answer uses the session that the phone attached. Claims without evidence carry a mark. | Device timing and service records. Agent, phone, and firmware. |
| TC-15 | U1, and every Chinese status string. | Every required glyph shows without clipping. At least 4 of 5 users complete each task. | Usability study and glyph inspection. Design and firmware. |
| TC-17 | 50 transitions, including holds, cancels, entry, and exit. | The press that enters Rest mode never starts capture. A hold never also triggers a short press. Exit stops all audio. | State tests and device demonstration. Firmware and design. |
| TC-20 | 10 reconnects under N2. | No duplicate question. No stale answer plays. | Fault injection. Firmware and phone. |

For TC-06 and TC-07, a 500 ms UP hold produces the cancel command.
The target is 700 ms from the first UP press to silence.
Measure the release latency from the release itself.

### Increment 2 acceptance matrix

| Test | Condition | Pass rule | Method and owner |
| --- | --- | --- | --- |
| TC-02 | The real phone application sends 300 LIVE_STATS packets. 30 packets hold unknown values. 10 packets have a short payload. | The device always shows the latest valid packet. Each unknown value shows "--", never 0. Each short packet gets ERROR code 2, and the screen keeps the old values. | Protocol test and screen log. Firmware and phone. |
| TC-21 | Under N1, send 300 packets. Read the receive time and the screen update time from the log. Also run task 5 of U1. | The screen update stays within 300 ms at the 95th percentile. At least 4 of 5 users read the speed correctly within 2 seconds. | Log timing and usability test. Firmware and design. |
| TC-03 | The phone sends 30 RUN_REPORT packets. 5 hold texts at the limit, and 3 hold texts above the limit. On the idle Coach page, press DOWN 30 times. Cover three cases: no report yet, link loss, and a report older than 60 seconds. | The Report page shows within 500 ms each time. The values and texts match the phone log. Texts at the limit show in full. A report above the limit gets ERROR code 4, and the screen keeps the old report. While audio runs, a short press changes no page. | Screen log and hardware demonstration. Firmware. |

`phone_sim` can check these three tests in Sprint 5, but it cannot pass them.

## 4. Tests outside Increment 1 and Increment 2

TC-04, TC-09, and TC-13 belong to P1, after Increment 2.
TC-10, TC-11, TC-12, TC-18, and TC-19 belong to P2.
TC-05, TC-14, and TC-16 sit in Deferred.
None of them gates the first version. Commit `970bb8e` holds their version 0.3 text.

## 5. Open questions

The user parked these questions. The user will share answers after a review with the phone application team.

| ID | Question | Needed for | Status |
| --- | --- | --- | --- |
| Q-01 | Which HeygoAI APIs and coaching tools exist? | P0 coaching | Open. After D-23, the Increment 2 fields do not depend on HeygoAI. |
| Q-02 | Which source supplies metrics and turns, with which units and frame? | Increment 2 metrics, Deferred turns | D-23 closes it for Increment 2: the phone calculates the metrics, and doc 07 gives the units. Turn data stays open. |
| Q-03 | Which phone platform runs the application? | P0 | Open. D-08 selected the transport. |
| Q-04 | Which consent and deletion rules apply? | Deferred | Deferred by D-13 |
| Q-05 | Which speech recognition and synthesis services apply? | P0 | Open. The phone application can supply them. |
| Q-06 | Who supplies the room service and the second endpoint? | P1 | Open |
| Q-07 | Which outdoor conditions define a field pilot? | Any field pilot | Open. Indoor only until then. |
| Q-08 | Which capacity, device count, and budget apply? | P0 | Open |
| Q-09 | Who reviews the coaching content? | P0 | Open |
| Q-10 | Which firmware baseline and license apply? | P0 | Open. D-09 selected the audio format. |
| Q-11 | Which final memory and queue limits apply? | Deferred | Deferred by D-18 |
| Q-12 | Which conditions make two sessions comparable? | P2 | Open |
| Q-13 | Does the phone application accept the BLE protocol draft? | P0 | Closed. The phone application team accepted protocol 0.2 on 2026-10-17. |
| Q-14 | What does the device do on a long DOWN press during capture, wait, or playback? | P0 | Closed by D-19. |
| Q-15 | What does the device do with a PLAY_START that arrives during capture? | P0 | Closed by D-20. The phone application team must still accept it. |
| Q-16 | Can the phone application calculate each LIVE_STATS and RUN_REPORT field? Which rules detect the start and the end of a run? | Increment 2 | Open. The phone application team answers. |
| Q-17 | Is the live field "structure" from the user the speed? | Increment 2 | Closed. The user stated that "structure" means the speed, on 2026-10-17. |

## 6. Decision log

| ID | Decision | Status |
| --- | --- | --- |
| D-01 | Keep all six requested capabilities in the core scope. | Superseded by D-17. |
| D-02 | AI Passport handles interaction. Analysis and inference run elsewhere. | Proposed. |
| D-03 | Voice works only in Rest mode. | Proposed. |
| D-04 | One audio owner at a time. | Proposed. |
| D-05 | Start with existing hardware and indoor trials. | Proposed. |
| D-06 | Write paired English and Chinese documents with the asd-ste100 skill. | User instruction. |
| D-07 | Keep latency, gesture, and capacity values as targets. | Proposed. |
| D-08 | Use a BLE phone bridge. The device does not use Wi-Fi. | Accepted by the user. |
| D-09 | Stream IMA ADPCM at 16 kHz in 20 ms blocks. Keep raw PCM as a fallback. | Accepted by the user. |
| D-10 | Cancel and exit use separate holds. | Accepted by the user. |
| D-11 | Stream microphone blocks to the phone at once. The phone holds the question. | Accepted by the user. |
| D-12 | The first-version link has no encryption, passkey, or binding check. | Accepted by the user. |
| D-13 | Build no privacy or authorization feature. | Accepted by the user. |
| D-14 | The device does four tasks. The phone application does the rest. | Accepted by the user. |
| D-15 | Two modes. An OK hold enters Rest mode on the Coach page. A DOWN hold and a short OK exit. | Accepted by the user. Version 0.4 changed the landing page. |
| D-16 | The device team proposes the BLE protocol draft. Document version 0.4 defines protocol version 0.1. | The phone application team accepted protocol 0.2 on 2026-10-17. |
| D-17 | P0 keeps the Coach voice chain. Reports, charts, the cache, and intercom move to P1. Turn views move to Deferred. | Accepted by the user. |
| D-18 | The first version builds no power management, and requests the best-performance link settings. REQ-16 moves to Deferred. | Accepted by the user. |
| D-19 | During capture, wait, or playback, the device ignores a long DOWN press. The user holds UP to cancel, then holds DOWN to exit. | Accepted by the user. |
| D-20 | The phone sends the PLAY_START of an exchange only after it receives AGENT_END. During capture, the device drops a PLAY_START. | Accepted by the user. Waits for the phone application team. See doc 07, section 14. |
| D-21 | The Ski page shows live data. Rest mode adds a Report page. | Accepted by the user. |
| D-22 | Live data and reports form Increment 2. The team develops it in parallel with the device acceptance of Increment 1. Each increment has its own acceptance. | Accepted by the user. |
| D-23 | The device team defines the data fields, and the phone application team adapts to them. The live fields cover vertical drop, distance, turn count, and run time. The speed waits for Q-17. After a run starts, the phone sends one packet each second. | Accepted by the user. Changed on 2026-10-17. |
| D-24 | The protocol changes to 0.2 and adds LIVE_STATS. After Sprint 4, the branch merges into `main`, and the team tells the phone application team to change to 0.2. | Accepted by the user on 2026-10-17. The phone application team accepted protocol 0.2. `d371c1d` merged into `main`. |
| D-25 | On the idle Coach page, a short DOWN press opens the Report page. On the Report page, a short UP press returns to the Coach page. An OK hold on the Report page does nothing. | Accepted by the user on 2026-10-17. |
| D-26 | After each run, the phone sends the report of that run. It holds the distance, turn count, Heygo IQ, core problem, and a tip of at most 20 characters. | Accepted by the user. |
| D-27 | The Heygo IQ is an integer from 0 to 100. The phone sends the core problem as text of at most 12 Chinese characters. The report shows on the Ski page on the lift and on the Report page in Rest mode. | Accepted by the user on 2026-10-17. |

## 7. Risks

| Risk | Effect | Mitigation | Role |
| --- | --- | --- | --- |
| The phone application cannot stream audio. | No P0 feature works. | Review doc 07 with that team first. Keep the raw PCM fallback. | Phone team and firmware. |
| Phone background limits break the link. | Questions fail. | Test background behavior on the target phone. | Phone team. |
| HeygoAI lacks coaching tools. | Answers lack ski context. | Resolve Q-01. | Phone team. |
| The BLE stack, fonts, and audio exhaust RAM. | The device resets. | Keep fixed queues. Watch for resets in the Sprint 1 device tests. | Firmware. |
| The P0 screen shows little value. | Pilot users see only status and short text. | Collect U1 feedback. Bring reports back first in P1. | Product Owner. |
| The open BLE link lets a nearby client connect. | A stranger can hear audio blocks. | Keep pilots indoor and small. Review D-12 before a public trial. | Product Owner. |
| Cold, wind, or gloves impair use. | Indoor results do not transfer. | Define Q-07 before field tests. | Product and hardware. |
| The first version has no power management. | The battery drains faster than expected, and a pilot session ends early. | Charge the device before each pilot session. Review D-18 before a field pilot. | Product Owner. |
| A look at the screen during a run takes attention from the slope. | A collision or a fall. | The page shows five numbers only. No animation, color, or sound marks the speed. Nothing rewards high speed. U1 tests a 2-second reading. | Product Owner and design. |
| The phone gives speed and vertical drop with large errors. | Users stop trusting the data. | The phone owns the accuracy. TC-02 checks only that the device matches the phone. Assess the accuracy before a field pilot. | Phone application team. |
| The change to protocol 0.2 is not at the same time on both sides. | A phone application with 0.1 only gets no voice with the new firmware. | Change both sides together per D-24. Tell the phone application team at the end of Sprint 4. | Firmware and phone application team. |

## 8. Delivery evidence

Report Build, Host tests, and Device tests separately. Record every unverified item.
The firmware is in the `Firmware` folder of the repository.
This status applies to commit `11b10cc`. It comes from `./tools/validate.sh` on a macOS host on 2026-10-17:

```text
Build: PASS (application image 1,617,856 bytes, partition 8,323,072 bytes)
Host tests: PASS (Coach, protocol, ADPCM, playback start rules, link_core, simulator self-test 37/37, glyph check)
Device tests: NOT RUN
Unverified: the real phone application, every device behavior, runtime heap use, external interfaces, and field conditions.
```

Every product test in the tables above is still NOT RUN. Section 4.1 of the [backlog](03-scrum-backlog.md) gives the progress of each story.
