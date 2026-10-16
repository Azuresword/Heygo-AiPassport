[简体中文](03-scrum-backlog.zh_CN.md) · **English**

# Scrum backlog and delivery plan

Version: 0.5. Date: 2026-10-16. Status: Proposed. No story, Sprint, or test is complete.

Related documents: [Requirements](01-product-requirements.md), [Architecture](02-architecture.md), [Validation and decisions](04-validation-and-decisions.md), [Decision amendments](05-decision-amendments.md), [Sprint 0 plan](06-sprint-0-plan.md), and [BLE protocol draft](07-ble-protocol-draft.md).

## 1. Product Goal

During a rest break, the skier asks the AI coach a question by voice and hears the answer.
The skier keeps the phone in a pocket and the gloves on.

## 2. Planning assumptions

| Item | Assumption | Required check |
|---|---|---|
| Stage | Concept prototype on existing AI Passport boards. | The Product Owner agrees the scope. |
| People | One firmware Developer. The phone application team works in parallel. | Check capacity before each Sprint. |
| Hardware | One AI Passport and one phone. | The board is not at hand. Sprint 0 therefore runs on the host only. |
| Connection | D-08 selects a BLE phone bridge. | Test link stability and phone background behavior. |
| Phone application | It exists. Another team develops it. | That team reviews doc 07. |
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

| Story | Requirement | Priority | Points | Owner | First target |
|---|---|---|---|---|---|
| US-01 Device connection | REQ-01 | P0 | Re-estimate | Firmware and phone | Sprint 1 |
| US-06 Voice capture | REQ-06 | P0 | 5–8 | Firmware | Sprint 0 logic, Sprint 1 hardware |
| US-07 Speech playback | REQ-07 | P0 | 3–5 | Firmware | Sprint 0 logic, Sprint 1 hardware |
| US-08 Button Agent exchange | REQ-08 | P0 | Re-estimate | Phone and firmware | Sprint 1 |
| US-15 Gloves and Chinese text | REQ-15 | P0 | Re-estimate | Firmware | Sprint 1 |
| US-17 Explicit modes | REQ-17 | P0 | Re-estimate | Firmware | Sprint 0 logic, Sprint 1 hardware |
| US-20 Recovery | REQ-20 | P0 | Re-estimate | Firmware and phone | Sprint 1 |
| US-02 Report sync | REQ-02 | P1 | 5–8 | Phone | Later |
| US-03 Report cards | REQ-03 | P1 | 3–5 | Firmware | Later |
| US-04 Metric charts | REQ-04 | P1 | 3–5 | Firmware | Later |
| US-09 Human push-to-talk | REQ-09 | P1 | 8–13 | Phone and firmware | Later |
| US-13 Cached reports | REQ-13 | P1 | 5–8 | Firmware | Later |
| US-10 Focused practice | REQ-10 | P2 | 3–5 | Agent | Later |
| US-11 Quests and badges | REQ-11 | P2 | 3–5 | Product | Later |
| US-12 Reflection journal | REQ-12 | P2 | 3–5 | Phone | Later |
| US-18 Session comparison | REQ-18 | P2 | 3–5 | Data | Later |
| US-19 Share link | REQ-19 | P2 | 3–5 | Phone | Later |
| US-05 Turn view and replay | REQ-05 | Deferred | None | None | None |
| US-14 Privacy and authorization | REQ-14 | Deferred | None | None | None |
| US-16 Resource limits | REQ-16 | Deferred | None | None | None |

"Re-estimate" marks a P0 story whose scope shrank in version 0.4. The Developers set new points.

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
- Acceptance: A 500 ms UP hold cancels the question. Capture then stops within 100 ms. The device tells the phone to drop the audio.
- Acceptance: A recognition failure from the phone produces a short retry message.
- Dependency: US-17, and a phone application that accepts streamed audio.
- Verification: TC-06.

### US-07: Speech playback

As a skier, I want to hear the answer and stop it at once, so that I control the device.

- Acceptance: The device plays each block as it arrives, and shows that playback runs.
- Acceptance: A 500 ms UP hold stops playback within 200 ms. The device tells the phone.
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

### US-17: Explicit modes

As a skier, I want to know whether voice works now, so that I never talk to the device by accident while I ski.

- Acceptance: The device starts in Ski mode. Voice stays off in Ski mode.
- Acceptance: An OK hold in Ski mode enters Rest mode on the Coach page. That press never starts capture.
- Acceptance: A DOWN hold and then a short OK return to Ski mode. The return stops all audio.
- Verification: TC-17.

### US-20: Recovery

As a skier, I want a clear message when the link fails, so that I know what to do.

- Acceptance: Link loss stops audio and shows the link state.
- Acceptance: A phone that stays silent for 15 seconds produces a clear message.
- Acceptance: A reconnect creates no duplicate question and plays no stale answer.
- Verification: TC-20.

## 6. P1, P2, and Deferred stories

These stories keep their numbers. Their full text returns when a story enters a Sprint.
Commit `970bb8e` holds the version 0.3 text.

## 7. Sprint order

| Sprint | Goal | Scope | Evidence |
|---|---|---|---|
| 0 | Build the device logic for one Coach question. | The [Sprint 0 plan](06-sprint-0-plan.md). Host only. Four hours. | Host tests and a firmware build. |
| 1 | Ask one real question on hardware. | The P0 stories with the board and the phone application. | Device tests for every P0 TC. |
| 2 and later | Add P1 items. | Reports first, then intercom. | The P1 tests. |

Sprint 1 starts after two inputs exist: the board at hand, and a phone application build that follows doc 07.

## 8. Definition of Done

Each selected P0 slice meets every applicable check:

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

## 9. Changes

1. Record the feedback and the affected requirement.
2. The Product Owner orders the backlog again.
3. The Developers update the estimates.
4. Record each accepted change in the decision log.
