**English** · [简体中文](05-decision-amendments.zh_CN.md)

# Decision Amendments: Transport, Codec, Gestures, Streaming and Scope

Version: 0.2. Date: 2026-10-16. Status: D-08 to D-14 accepted by the user. D-15 proposed.

This document records eight decisions, D-08 to D-15.
Several decisions replace earlier proposals in [System Architecture](02-architecture.md).
The user reviewed each change. D-15 still waits for the user.

Decision numbers continue the record in [Validation and Decisions](04-validation-and-decisions.md).
Requirement numbers stay stable. REQ-14 moves to Deferred. This document deletes no requirement.

## 1. The rule behind these decisions

The first version must take the shortest path to a working device.
The device must not do work beyond its own capability.
In the worst case, the device acts only as a microphone and a speaker for the phone.

A phone application already exists. Another team develops it.
The phone application therefore takes every task that the device does not need to own.

## 2. D-08: the device reaches the network through a BLE phone bridge

**Superseded proposal.** The device joins a phone Wi-Fi hotspot. The device opens a TLS connection to the gateway.

**Decision.** The device acts as a BLE peripheral. The phone application acts as the central.
The phone carries report traffic and audio to the gateway. The device does not use Wi-Fi.

**Evidence.** A real build measured the cost of Wi-Fi on this hardware.
The method excluded `demo_wifi.c` and the `esp_wifi` dependency, and set `CONFIG_ESP_WIFI_ENABLED=n`.

| Metric | With Wi-Fi | Without Wi-Fi | Change |
| --- | --- | --- | --- |
| Static D/IRAM used | 178,960 | 140,456 | -38,504 bytes |
| Static D/IRAM free | 142,336 | 180,840 | +27.1% |
| `.bss` | 44,352 | 31,408 | -12,944 bytes |
| Flash image | 1,527,276 | 1,062,270 | -465,006 bytes |

**Reasons.**

1. The chip has no PSRAM. Wi-Fi and TLS compete with the LVGL pool, the audio frames and the font data.
2. The phone already holds the account and the gateway connection.
3. The device needs no Wi-Fi provisioning screen. Three buttons cannot enter a password.

**Cost.** The device cannot reach the gateway without the phone. The phone adds latency.
Phone background limits become a product risk.

## 3. D-09: the device streams IMA ADPCM

**Decision.** The device uses IMA ADPCM at 16 kHz, mono, in 20 ms frames.
Both directions and both channels use this format.
The protocol keeps a raw PCM frame type as a fallback.

| Format | Bytes per second | Device cost |
| --- | --- | --- |
| Raw PCM | 32,000 | No codec work. Four times the ADPCM link load. |
| IMA ADPCM | 8,000 | A few integer steps per sample. A few bytes of state. |
| Opus | About 3,000 | A new library. An Opus reference on this board used a 24 KB encoder task stack. |

**Reasons.**

1. ADPCM costs almost nothing on the device. It meets the rule in section 1.
2. ADPCM sends one quarter of the raw PCM load over BLE.
3. One 20 ms frame carries about 164 bytes of audio data. The local BLE reference uses the same format, with an ATT MTU of at least 172.

**History.** This document first selected Opus for its low bit rate.
The user then set the rule in section 1. Opus needs a new library and a large task stack, so it risks the device limits.
ADPCM replaced Opus. Opus stays a later upgrade.
The team considers Opus only after ADPCM works and the team measures its CPU and RAM cost.

**Cost.** ADPCM sends about 2.7 times more data than Opus.
The BLE link must carry 8,000 bytes per second in one direction at a time.

## 4. D-10: cancel and back use separate gestures

**Superseded proposal.** A long press on UP cancels the current exchange, or returns to the previous page.

**Decision.** A long press on UP cancels the active audio owner. A long press on DOWN returns to the previous page.
A long press on UP does nothing when no audio owner is active.

**Reason.** The superseded proposal gives one gesture two meanings.
The meaning depends on hidden state. The user cannot predict the result before the press.
REQ-17 requires that the user knows what each button controls.

**Cost.** DOWN loses its long press for other uses.

**Timing budgets stay unchanged.** The cancel command appears when UP stays down for 500 ms.
Playback stops within 200 ms after that command. Capture stops within 100 ms after that command.
The total target from the first press on UP stays at 700 ms.

## 5. D-11: the device streams audio, and the phone decides

**Superseded proposals.** The architecture draft kept the microphone off until the server accepts.
A later amendment buffered audio on the device until the gateway accepts.

**Decision.** The device streams each microphone frame to the phone. The phone decides when to forward the audio.
The rule differs per page.

- **Coach page.** The microphone starts when the OK hold reaches 500 ms. The device streams each frame at once. The phone holds the question until the gateway accepts it.
- **Team page.** The OK hold sends a speaking request to the phone. The microphone stays closed. The phone returns a start message after it obtains speaking permission. The device then starts the microphone and plays a short cue.

**Why the Coach page starts at once.** The question needs its first words.
A measured reference reports 280 ms to 290 ms between the key press and the gateway ready signal.
A user who speaks at once loses that much speech under the superseded proposal.

**Why the Team page waits.** Early capture makes the listener hear every word late.
TC-09 allows 800 ms for a grant at the 95th percentile.
Audio captured during that wait reaches the listener at least 800 ms late. TC-09 allows 500 ms of audio delay.
Early capture also plays and records at once while another member speaks. The device has no echo cancellation.

**Cost.** The device holds no whole question. The phone must accept streamed audio.
If the user releases OK before the start message, the device ignores that message.

## 6. D-12: the first-version link has no encryption

**Decision.** The BLE link uses no encryption, no passkey, and no binding check.
The device accepts one phone at a time. The baseline configuration already sets that limit.

**Reason.** The first version needs the shortest path to a working link.

**Cost.** Any nearby BLE client can connect. A connected client can read reports and audio frames.
Pilots stay indoor and in a small group. A public trial needs a review of this decision.

## 7. D-13: the first version builds no privacy or authorization feature

**Decision.** The first version builds no access control, no data deletion flow, and no log redaction.
REQ-14, US-14 and TC-14 move to Deferred. Q-04 needs no answer for the first version.

**Reason.** The first version needs the shortest path to a working device.

**What stays.** The device still shows microphone activity whenever the microphone works.
US-06 and US-17 require that state as normal interface behavior.

**Cost.** The device cannot delete cached reports on remote request.
A later version needs a new decision before it adds these features.

## 8. D-14: the device does four tasks

**Decision.** The device does these four tasks:

1. Tell the phone about button events.
2. Stream microphone frames to the phone.
3. Play the audio frames that the phone streams.
4. Show the status, text and reports that the phone supplies.

The phone application does the rest.
That work includes speech recognition, speech synthesis, gateway requests, retries, the account, the room, and the speaking permission.

**Reason.** The phone application already exists, and another team develops it.
Device requirements therefore follow its capabilities. Q-13 tracks its BLE interface.

**Cost.** The device protocol depends on an interface that this project does not own.
The device team cannot finish the link before Q-13 has an answer.

## 9. D-15: the Rest mode gestures (proposed)

**Status.** Proposed. The user has not accepted this decision.

**Proposal.**

- The device starts in Ski mode.
- An OK hold in Ski mode enters Rest mode. Rest mode opens on the Reports page.
- A DOWN hold on the top Rest page opens a confirmation. A short OK then returns to Ski mode.

**Reason.** The journey requires manual entry and exit. The gesture table had no row for either.
A hold avoids an accidental entry, because a gloved hand can press a button while skiing.
The Reports page ignores an OK hold. The hold that enters Rest mode therefore cannot start capture.

## 10. Traceability

| Decision | Status | Affects | Open question |
| --- | --- | --- | --- |
| D-08 | Accepted by the user | REQ-09, REQ-13, REQ-16, REQ-20 | Closes Q-03 for the transport |
| D-09 | Accepted by the user | REQ-06, REQ-07, REQ-09, REQ-16 | Answers Q-10 for the frame format |
| D-10 | Accepted by the user | REQ-15, REQ-17 | None |
| D-11 | Accepted by the user | REQ-06, REQ-07, REQ-09 | Depends on Q-13 |
| D-12 | Accepted by the user | REQ-01 | None |
| D-13 | Accepted by the user | REQ-14, REQ-20 | Defers Q-04 |
| D-14 | Accepted by the user | Every device requirement | Depends on Q-13 |
| D-15 | Proposed | REQ-17 | Waits for the user |

## 11. Related documents

- [Product Requirements](01-product-requirements.md).
- [System Architecture](02-architecture.md).
- [Scrum Backlog](03-scrum-backlog.md).
- [Validation and Decisions](04-validation-and-decisions.md).
- [Sprint 0 Two-Day Plan](06-sprint-0-two-day-plan.md).
- [Folder Index](README.md).
