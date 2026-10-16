[简体中文](01-product-requirements.zh_CN.md) · **English**

# HeygoAI and AI Passport: product requirements

Version: 0.4. Date: 2026-10-16. Status: Draft for review.
The Product Owner role has no assigned person.
This document defines a concept. It does not describe a working integration.

## 1. Product goal

During a rest break, the skier asks an AI coach about the last runs and hears the answer.
The skier does not take out the phone. The skier keeps the gloves on.

HeygoAI supplies the ski analysis and the coaching through interfaces that still need evidence.
The phone application supplies speech recognition, speech synthesis, and the link to HeygoAI.
AI Passport supplies the buttons, the microphone, the speaker, and a small screen for status.

The phone stays in a pocket during use. The device reaches the network through the phone.
The device therefore removes the need to take the phone out. It does not remove the need for the phone.

## 2. Users and use moments

| User | Need | Use moment |
| --- | --- | --- |
| Recreational skier | Understand a run without a long phone report. | A rest break after a run. |
| Skier who practices a skill | Get one short tip before the next run. | Before the next run. |
| Project tester | Reproduce a failed question. | An indoor test session. |

The pilot assumes adult users and a small group.
The pilot uses existing boards. It needs no new circuit board or enclosure.

## 3. Product journey

1. The user connects the device to the existing phone application.
2. The device starts in Ski mode. Ski mode shows only status.
3. The user holds OK to enter Rest mode. Rest mode opens on the Coach page.
4. The user holds OK again and asks a question.
5. The user releases OK. The device ends the question.
6. The Agent answers through speech, with short text on the screen.
7. The user can hold UP to stop the answer.
8. Before the next run, the user holds DOWN and then presses OK to return to Ski mode.

Rest mode means that the user states they stopped.
The device cannot check motion. Rest mode never claims that the surroundings are safe.

## 4. Scope and priorities

D-17 sets these priorities. [Decision amendments](05-decision-amendments.md) gives the reasons.

| Priority | Meaning | Requirements |
| --- | --- | --- |
| P0 | The first version | REQ-01, REQ-06, REQ-07, REQ-08, REQ-15, REQ-16, REQ-17, REQ-20 |
| P1 | The next increment | REQ-02, REQ-03, REQ-04, REQ-09, REQ-13 |
| P2 | Later candidates | REQ-10, REQ-11, REQ-12, REQ-18, REQ-19 |
| Deferred | No planned increment | REQ-05, REQ-14 |

P0 builds the complete voice chain once: button, BLE link, phone, Agent, phone, BLE link, speaker.
Each P1 feature reuses that chain.

### Exclusions from the first version

- Reports, charts, turn views, and human intercom. They sit in P1 or Deferred.
- Automatic emergency detection or rescue dispatch.
- Certified instruction, medical advice, or injury prevention claims.
- Native measurement of position, edge angle, or body posture.
- Continuous listening, a wake word, or an open microphone by default.
- A phone-free radio intercom.
- New hardware, manufacturing, payments, or a public social network.
- BLE encryption, a pairing passkey, or a binding check.
- Privacy and authorization features.

## 5. Capability boundary

| Area | Evidence or status | Product consequence |
| --- | --- | --- |
| Screen | 240 × 320. No evidence of a touch interface. | Show status and short text. |
| Buttons | UP, DOWN, and OK share one ADC input. | Use one button at a time. |
| Audio | Microphone, speaker, and PCM APIs exist. No echo cancellation. | Stream audio to the phone in small blocks. Never record and play together. |
| Resources | ESP32-C3, 8 MB Flash, no PSRAM. | Keep speech recognition, synthesis, and the Agent off the device. |
| Power | 500 mAh battery. | Measure runtime. Do not claim full-day use. |
| Motion and location | No evidence of an IMU, GNSS, or barometer. | Get ski data from HeygoAI through the phone. |
| HeygoAI | No API contract and no sample data yet. | The phone application handles the integration. |
| Phone application | It exists. Another team develops it. | Device requirements follow its capabilities. The [BLE protocol draft](07-ble-protocol-draft.md) proposes the link. |

## 6. Requirements matrix

Each row has test stage Prototype. Owners are roles with no assigned person.
[Validation and decisions](04-validation-and-decisions.md) defines each matching TC.

### P0

| ID | Value and required behavior | Owner | Acceptance target | Dependency |
| --- | --- | --- | --- | --- |
| REQ-01 | Let the phone find and connect to the device. The device advertises its name. The phone connects without a passkey. | Phone and firmware. | TC-01: 9 of 10 connection attempts finish within 180 seconds. | The phone application must accept the BLE protocol draft. |
| REQ-06 | Capture a question that the user starts. Stream each block to the phone while the user speaks. Show capture status and recognition failures. | Firmware and phone. | TC-06: release stops capture within 100 ms in 100 trials. Capture ends by 20 seconds. | The phone must accept streamed audio and run speech recognition. |
| REQ-07 | Play the spoken answer as it arrives. Allow the user to stop it. | Firmware and phone. | TC-07: playback stops within 200 ms of the cancel command in 30 trials. No canceled answer resumes. | The phone must supply synthesized speech. |
| REQ-08 | Ask the Agent with a button. The phone attaches the current ski session. | Agent, phone, and firmware. | TC-08: the first audible answer starts within 5 seconds of release at the 95th percentile. The Agent marks claims that lack evidence. | HeygoAI coaching tools remain open. |
| REQ-15 | Support three buttons, gloves, and readable Chinese status text. | Design and firmware. | TC-15: every required glyph shows. At least 4 of 5 users complete each core task. | Test gloves and readability on hardware. |
| REQ-16 | Keep audio, link, and power use within measured device limits. | Firmware. | TC-16: a 60-minute workload has no reset and no allocation failure. | Battery runtime and cold limits are unknown. |
| REQ-17 | Keep Ski mode and Rest mode distinct. Require manual entry before any voice use. | Firmware and design. | TC-17: 50 transition trials keep the mode clear. The press that enters Rest mode never starts capture. | No automatic motion detection exists. |
| REQ-20 | Explain link failures. Support recovery. | Firmware and phone. | TC-20: 10 reconnect trials create no duplicate question. No stale answer plays. | Test link loss and phone background behavior. |

### P1

| ID | Value | Why it waits |
| --- | --- | --- |
| REQ-02 | Keep each report consistent across updates. | Q-01 blocks real report data. |
| REQ-03 | Show a short report card without the phone. | It depends on REQ-02. |
| REQ-04 | Show metric trends with units and gaps. | It depends on REQ-02. |
| REQ-09 | Hold a button to speak with a companion in a shared room. | It needs a room service, a second endpoint, and phone intercom support. |
| REQ-13 | Keep the last report readable during a network failure. | It exists only for reports. |

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
| REQ-05 | Show and replay one turn. | It is the largest story, and Q-02 may never supply trajectory data. |
| REQ-14 | Access control and data deletion. | D-13 excludes privacy work. |

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

### Cancel and exit

An UP hold of 500 ms cancels the active capture or playback.
Playback stops within 200 ms of that command. Capture stops within 100 ms of that command.
The total target is 700 ms from the first UP press to silence.
A DOWN hold of 500 ms opens the exit question. A short OK then returns to Ski mode.
A hold never also triggers a short press.

### Audio rules

The device never records and plays at the same time.
An OK hold during playback stops the playback first. The microphone starts after that.
If the link fails, the device stops capture and shows the failure.
A canceled answer never resumes.

## 8. Success targets

These numbers are test targets, not measurements.

| Outcome | Target |
| --- | --- |
| Connect | 9 of 10 attempts finish within 180 seconds. |
| Ask and stop | At least 4 of 5 users ask a question and stop an answer with gloves, without help. |
| Hear the answer | The first audible answer starts within 5 seconds of release at the 95th percentile. |
| Stop quickly | Capture stops within 100 ms of release. Playback stops within 200 ms of the cancel command. |
| Run for an hour | A 60-minute workload has no reset. |

## 9. Hardware and pilot limits

The prototype uses existing AI Passport boards and available phones.
The sources do not show water resistance, cold operation, wind rejection, or outdoor readability.
Indoor success does not support a claim about slope use.
Manufacturing, certification, packaging, and support stay outside this concept.

## 10. Related documents

- [System architecture](02-architecture.md).
- [Scrum backlog](03-scrum-backlog.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Decision amendments](05-decision-amendments.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Document index](README.md).
