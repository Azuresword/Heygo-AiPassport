[简体中文](01-product-requirements.zh_CN.md) · **English**

# HeygoAI and AI Passport: product requirements

Version: 0.1. Date: 2026-10-16. Status: Draft for review.
The Product Owner role has no assigned person.
This document defines a concept. It does not describe a working integration.

## 1. Product goal

During a rest break, help the skier with these actions:

- Understand one run.
- Choose one practice goal.
- Discuss the result.

HeygoAI supplies available analysis and coaching through interfaces that still need source evidence.
AI Passport provides a small screen, physical controls, microphone input, and speaker output.

The smallest complete experience includes these features:

- A report.
- Metric charts.
- Turn replay.
- A spoken exchange with the Agent.
- A conversation with another person.

The project must demonstrate each feature with the same ski session context.

## 2. Users and use moments

| User | Need | Proposed use moment |
| --- | --- | --- |
| Recreational skier | Understand a result without reading a large phone report. | A rest break after a run. |
| Skier who practices a skill | Choose one achievable goal for the next run. | Before the next run. |
| Human coach or companion | Discuss a selected turn with the skier. | A shared rest break. |
| Project tester | Reproduce a failed exchange without exposing recordings. | An indoor test session. |

The pilot assumes adult users and a small private group.
The device form factor stays unchanged.
The project starts with an indoor prototype using existing boards.
The pilot does not require a new circuit board or enclosure.

## 3. Product journey

1. The user connects the device to an account through the proposed phone companion.
2. The user manually enters Rest mode.
3. The user selects a ski session.
4. The device shows a summary and the data source.
5. The user selects a metric or a turn.
6. The user enters Coach mode.
7. The user holds OK to ask a question about the selection.
8. The Agent answers through text and speech.
9. The user enters Team mode to speak with a human coach or companion.
10. The user can record a practice goal or reflection when the P1 feature is available.
11. The user ends Rest mode before the next descent.

Rest mode means that the user states they are stationary.
The prototype cannot determine motion from the board sensors whose presence the sources establish.
The pilot permits report browsing and speech exchanges only in Rest mode.
Rest mode must never claim that the surroundings are safe.

## 4. Scope and priorities

P0 means required for the proposed minimum viable product, or MVP.
P1 means a candidate after the core experience works.
These priorities are proposals. The Product Owner must order the backlog before Sprint Planning.

All six requested capabilities remain in P0:

- Reports.
- Voice input.
- Voice output.
- Button controls for the Agent.
- Human intercom.
- Data and turn visualization.

Offline reports, privacy, device limits, and distinct modes support those capabilities.

Suggested P1 features include these capabilities:

- One practice goal.
- Skill quests.
- A reflection journal.
- Ski session comparison.
- Report sharing.

These features must reuse data that passed checks and the existing ski session context.
Badges must reward practice or review. They must not reward speed, dangerous terrain, or continuous device use while skiing.

### Exclusions from the first MVP

- Automatic emergency detection or rescue dispatch.
- Certified instruction, medical advice, or guarantees about injury prevention.
- Native measurement of position, edge angle, or body posture without a data source that passed checks.
- Continuous listening, a wake word, or an open microphone by default.
- Simultaneous Agent speech and human intercom audio.
- A phone-free radio intercom or offline delivery of live speech.
- New hardware, commercial manufacturing, payments, or a public social network.

## 5. Capability boundary

| Area | Evidence or status | Product consequence |
| --- | --- | --- |
| AI Passport screen | Source evidence establishes a 240 × 320 screen. The sources do not establish a touch interface. | Use cards, selection, and small charts. |
| AI Passport buttons | UP, DOWN, and OK share an ADC input. | Use one button at a time. |
| AI Passport audio | Microphone, speaker, and PCM APIs exist. | Limit audio streams to available capacity. Do not assume echo cancellation. |
| AI Passport resources | ESP32-C3, 8 MB Flash, no PSRAM. | Keep inference and large reports outside the device. |
| AI Passport power | The specification lists a 500 mAh battery. | Measure runtime. Do not claim full-day operation. |
| Motion and location | Source evidence does not establish an IMU, GNSS, or barometer interface. | Obtain ski measurements from another source that passed checks. |
| HeygoAI services | The project lacks a repository link, API contract, and sample report. | Check report, coaching, identity, and data contracts. |
| Phone companion | Proposed component. This repository contains no phone application. | Test permissions, transport, and background behavior. |
| Network intercom | A separate workspace contains an uncommitted BLE reference. | Evaluate reuse. Do not treat it as a released service. |

The [architecture](02-architecture.md) records source references and component boundaries.
The [validation plan](04-validation-and-decisions.md) records assumptions, owners, and unresolved decisions.

## 6. Requirements matrix

All rows have status Proposed and test stage Prototype.
Owners below are roles with no assigned person.
The validation plan defines the full test conditions for each matching TC identifier.
Each row gives the user value, required behavior, acceptance target, and main dependency.

| ID | Value and required behavior | Priority and owner | Acceptance target | Dependency or risk |
| --- | --- | --- | --- | --- |
| REQ-01 | Connect the correct user and device. Support account binding. Support account unlinking. | P0. Service and phone. | TC-01: complete 9 of 10 setup attempts within 180 seconds. Reject every cross-account test. | Identity API and phone platform remain open. |
| REQ-02 | Keep the report consistent. Synchronize the ski session identifier, report version, and time. | P0. Service and firmware. | TC-02: 30 updates retain the correct ski session and newest accepted version. | HeygoAI report identifiers remain unresolved. |
| REQ-03 | Show a report without requiring the full phone view. Include summary, available metrics, and one supported finding. | P0. Firmware and design. | TC-03: cached cards open within one second at the 95th percentile across 30 trials. | Actual report fields remain open. |
| REQ-04 | Show trends with units, axes, source, and missing-data states. | P0. Firmware and data. | TC-04: all 12 chart fixtures retain units and data gaps. Missing values never become zero. | The source must supply valid units. |
| REQ-05 | Show turns. Allow turn selection. Distinguish measured paths, estimates, and illustrative curves. | P0. Data and firmware. | TC-05: all 12 turn fixtures preserve direction, coordinate meaning, gaps, and source. | Turn data and its accuracy remain open. |
| REQ-06 | Capture a voice question that the user starts. Show capture status and transcript errors. | P0. Firmware and speech service. | TC-06: release stops capture within 100 ms in 100 trials. Capture ends by 20 seconds. | Speech recognition and language support remain unresolved. |
| REQ-07 | Read the answer aloud. Allow the user to interrupt playback. | P0. Firmware and speech service. | TC-07: interruption stops playback within 200 ms in 30 trials. No canceled answer resumes. | Speech synthesis remains an integration requirement. |
| REQ-08 | Use buttons to ask the Agent about the selected ski session or turn. | P0. Agent and firmware. | TC-08: all 20 context tests use the correct ski session. The Agent identifies conclusions that lack evidence. | HeygoAI coaching tools remain unresolved. |
| REQ-09 | Hold a button to speak with a human participant in a private room. | P0. Service, phone, and firmware. | TC-09: 20 trials with competing speakers allow one sender. No expired speech plays after the connection returns. | The feature needs a transport and a second endpoint. |
| REQ-10 | Offer one practice goal based on available evidence. | P1. Product and Agent. | TC-10: all 10 examples include the goal, evidence, and limits. | A qualified reviewer must check coaching content. |
| REQ-11 | Offer a small quest or badge for practice and review. | P1. Product and service. | TC-11: repeated completion events create one reward. No reward depends on maximum speed. | Define reward conditions from events that passed checks. |
| REQ-12 | Save a short reflection linked to a ski session. Allow edits. Allow deletion. | P1. Phone and service. | TC-12: 10 save-and-retry trials create one reflection each. Cancellation creates no reflection. Edits and deletions persist. | Obtain consent before saving transcripts. |
| REQ-13 | Keep the last report available during a network failure. Show its generation time and age. | P0. Firmware and service. | TC-13: 10 outage trials retain one valid report. The device never queues live intercom audio. | Limit storage use. Limit Flash writes. |
| REQ-14 | Show microphone state and recipient. Enforce access control. Enforce data deletion. | P0. Service and firmware. | TC-14: unauthorized access always fails in the test matrix. The device deletes local private data when the user unlinks it. | Token handling and deletion APIs need design. |
| REQ-15 | Support three buttons, gloves, and readable Chinese labels. | P0. Design and firmware. | TC-15: all required glyphs render. At least 4 of 5 users complete each core task. | Test button use with gloves on hardware. Test screen readability on hardware. |
| REQ-16 | Keep audio, reports, and power use within measured device limits. | P0. Firmware. | TC-16: a 60-minute mixed workload has no reset or allocation failure. State when battery data is unavailable. | Battery runtime and low-temperature limits are unknown. |
| REQ-17 | Keep Coach mode, Team mode, and report states distinct. Require the user to enter Rest mode manually before interaction. | P0. Firmware and design. | TC-17: all 50 transition trials preserve mode identity. No gesture starts capture for both the Agent and Team mode. | The design does not assume automatic motion classification. |
| REQ-18 | Compare two ski sessions. Show differences in conditions or source. | P1. Data and design. | TC-18: all 10 comparisons disclose incompatible units, missing fields, and source differences. | Comparable ski session metadata may be absent. |
| REQ-19 | Share a selected report through the phone with explicit consent. | P1. Phone and service. | TC-19: revocation blocks all tested attempts to open the shared report again. | Share links require a service that passed checks. |
| REQ-20 | Explain connection failures. Support recovery with limited diagnostic data. | P0. Firmware and service. | TC-20: 10 reconnect trials avoid duplicate requests. Logs contain no token, audio, or full transcript. | Test transport errors and clock changes. |

## 7. Experience details

### Reports and charts

The report starts with the selected ski session, report generation time, age, and source.
The device shows at most three summary metrics on one card.
The user selects a metric to see its chart and definition.
The device hides unsupported metrics instead of inventing a value.

Candidate metrics include these values:

- Duration.
- Distance.
- Turn count.
- Rhythm.
- Left-right consistency.

Their availability depends on the actual source.
Speed, turn radius, edge angle, and posture need specific evidence and validation.
Their names in this draft are not evidence that HeygoAI supplies them.

### Turn visualization

The report can contain a measured path, a model estimate, or an illustrative curve.
Each view must identify its type.
The architecture defines coordinates and direction to prevent mirrored turns.
Missing path segments must remain visible as gaps.

The user can select one turn.
The user can replay the available samples for that turn.
A selected turn supplies context to the Agent.
An illustrative curve can explain a concept but cannot support a numerical claim about the user's technique.

### Voice and human intercom

Coach mode sends speech to the Agent.
Team mode sends speech to the named human room.
The screen must show the destination before capture starts.
The same hold gesture must never start both flows.

The proposed gesture starts with an OK hold of 500 ms.
The user releases OK to stop capture.
The application must suppress the short-click action after a hold.
Capture has a proposed 20-second limit.
Human intercom requires speaking permission before the device captures or sends audio.

The server must accept the Agent request before the device starts capture.
The device shows a waiting state until the server accepts the request.
The device rejects server acceptance after OK release or exchange cancellation.

An UP hold of 500 ms produces the cancellation command.
The device must stop playback within 200 ms after it recognizes that command.
This limit equals 700 ms from the initial UP press.
The device must stop capture within 100 ms after it recognizes the cancellation command.
The normal capture stop limit starts when the user releases OK.

The MVP uses one audio owner at a time.
It does not depend on acoustic echo cancellation.
If the network fails, the device stops live capture.
The device shows the network failure.
It must not send old human speech after the connection returns.

## 8. Proposed success targets

These are test targets, not measured results or delivery promises.
The [validation plan](04-validation-and-decisions.md) defines the network profile and sample sizes.

| Outcome | Proposed target |
| --- | --- |
| Find the report | At least 4 of 5 pilot users find a selected ski session without help. |
| Understand the turn view | At least 4 of 5 users distinguish measured data from an illustration. |
| Receive an Agent answer | The first audible response starts within 5 seconds after release at the 95th percentile. |
| Start human intercom | The device obtains speaking permission within 800 ms at the 95th percentile. |
| Hear human speech | End-to-end audio delay stays within 500 ms at the 95th percentile. |
| Preserve context | No answer refers to the wrong ski session in the defined context tests. |
| Recover from failure | The cached report remains usable. The device never resumes speech that is no longer valid. |

## 9. Hardware and pilot limits

The prototype uses existing AI Passport boards and available phones.
The team must record actual device, phone, hosting, and speech-service costs during Sprint 1.
The budget, team capacity, phone platform, and pilot date remain open.

The checked sources do not establish water resistance, low-temperature operation, wind rejection, or outdoor screen readability.
Indoor success does not authorize a claim about slope use.
Before an outdoor pilot, the team must define the operating environment.
The team must test the device in that environment.

Commercial manufacturing, certification, packaging, and support plans remain outside this concept.
If the scope changes to sales, the Product Owner must add separate requirements for those areas.

## 10. Related documents

- [System architecture](02-architecture.md).
- [Scrum backlog](03-scrum-backlog.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Document index](README.md).
