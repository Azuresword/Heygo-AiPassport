[简体中文](05-decision-amendments.zh_CN.md) · **English**

# Decision amendments: D-08 to D-20

Version: 0.6. Date: 2026-10-17. Status: D-08 to D-15 and D-17 to D-20 accepted by the user. D-16 proposed.

This document gives the reasons behind D-08 to D-20.
[Validation and decisions](04-validation-and-decisions.md) holds the short log.
Requirement numbers stay stable. This document deletes no requirement.

## 1. The rule behind these decisions

The first version takes the shortest path to a working device.
The device does no work beyond its own capability.
In the worst case, the device acts only as a microphone and a speaker for the phone.
The phone application already exists, so it takes every task that the device does not need.

## 2. D-17: P0 keeps only the Coach voice chain

**Decision.** P0 drops reports, human intercom, and turn visualization.
The user asked for this cut, and asked the device team to place each item.

| Item | Was | Now | Reason |
| --- | --- | --- | --- |
| Reports (REQ-02, REQ-03) | P0 | P1 | They carry the most product value, but Q-01 blocks real data. The device cost stays small once data exists. |
| Metric charts (REQ-04) | P0 | P1 | They depend on reports. |
| Offline report cache (REQ-13) | P0 | P1 | It exists only for reports. |
| Human intercom (REQ-09) | P0 | P1 | It needs a room service, a second endpoint, and phone intercom support. The device side reuses the Coach audio path, so its return cost stays small. |
| Turn visualization (REQ-05) | P0 | Deferred | It is the largest story, at 8 to 13 points. It needs trajectory data with a coordinate frame, and Q-02 may never supply that. A schematic curve cannot support a claim about technique. |
| Practice goals, quests, journal, comparison, and sharing | P1 | P2 | Most of them build on reports, and reports now sit in P1. |

**Why turn visualization does not go to P1.** P1 holds items that can return when one blocker clears.
Turn visualization needs a data source that may not exist. Deferred states that fact.
It returns only if Q-02 shows real trajectory data.

**What P0 still proves.** P0 builds the complete voice chain once: button, BLE link, phone, Agent, phone, BLE link, speaker.
Reports reuse the link. Intercom reuses the audio path.

**Cost.** In P0 the screen shows only status and short text. The main visual feature waits for data.
Pilot users can therefore see less value. The risk register in document 04 tracks this cost.

## 3. D-08: BLE phone bridge

**Superseded proposal.** The device joins a phone Wi-Fi hotspot and opens TLS to the gateway.

**Decision.** The device acts as a BLE peripheral. The phone application acts as the central. The device does not use Wi-Fi.

**Evidence.** A real build excluded `demo_wifi.c` and `esp_wifi`, and set `CONFIG_ESP_WIFI_ENABLED=n`.

| Metric | With Wi-Fi | Without Wi-Fi | Change |
| --- | --- | --- | --- |
| Static D/IRAM free | 142,336 | 180,840 | +27.1% |
| Static D/IRAM used | 178,960 | 140,456 | -38,504 bytes |
| Flash image | 1,527,276 | 1,062,270 | -465,006 bytes |

**Reasons.** The chip has no PSRAM. The phone already holds the account and the gateway link. Three buttons cannot enter a Wi-Fi password.

**Cost.** The device needs the phone. Phone background limits become a risk.

## 4. D-09: IMA ADPCM

**Decision.** The device streams IMA ADPCM at 16 kHz, mono, in 20 ms blocks of 164 bytes. Raw PCM stays a fallback.

| Format | Bytes per second | Device cost |
| --- | --- | --- |
| Raw PCM | 32,000 | No codec work. Four times the ADPCM load. |
| IMA ADPCM | 8,000 | A few integer steps per sample. |
| Opus | About 3,000 | A new library. An Opus reference on this board used a 24 KB encoder task stack. |

**History.** Version 0.2 first selected Opus. The rule in section 1 then replaced it.
Opus returns only after ADPCM works and the team measures its cost.

**Cost.** ADPCM sends about 2.7 times more data than Opus.

## 5. D-10: separate cancel and exit holds

**Superseded proposal.** One UP hold cancels or goes back, and the result depends on hidden state.

**Decision.** An UP hold cancels the active audio. A DOWN hold goes back. In P0 the DOWN hold opens the exit question for Rest mode.

**Reason.** REQ-17 requires that the user knows what each button does.

## 6. D-11: the device streams, and the phone decides

**Decision.** The device streams each microphone block to the phone at once. The phone holds the question until the gateway accepts it.

**Reason.** A measured reference shows 280 ms to 290 ms from the key press to the gateway ready signal.
The device keeps those first words because it streams at once.

**P1 note.** The Team page waits for a start message before capture.
Early capture would add the grant delay to every word. That delay can reach 800 ms, and the audio target is 500 ms.

## 7. D-12: an open link in the first version

**Decision.** The link uses no encryption, no passkey, and no binding check. The device accepts one phone at a time.

**Cost.** Any nearby BLE client can connect and read audio blocks. Pilots stay indoor and small.

## 8. D-13: no privacy or authorization feature

**Decision.** The device builds no access control, no data deletion flow, and no log redaction. REQ-14 moves to Deferred.

**What stays.** The device always shows when the microphone works. US-06 and US-17 require it.

## 9. D-14: four device tasks

**Decision.** The device does these four tasks:

1. Tell the phone about button events.
2. Stream microphone blocks to the phone.
3. Play the audio blocks that the phone streams.
4. Show the status and the short text that the phone supplies.

**Reason.** The phone application exists. Device requirements follow its capabilities.

**Cost.** The device protocol depends on an interface that this project does not own.

## 10. D-15: two modes

**Decision.**

- The device starts in Ski mode.
- An OK hold in Ski mode enters Rest mode on the Coach page.
- The press that enters Rest mode never starts capture. Capture needs a new press.
- A DOWN hold on the Coach page opens the exit question. A short OK returns to Ski mode.

**Change in version 0.4.** Rest mode opened on the Reports page before.
D-17 moved reports to P1, so Rest mode now opens on the Coach page.
The new-press rule replaces the protection that the Reports page gave.

**Reason.** A hold avoids an accidental entry with gloves.

## 11. D-16: BLE protocol draft

**Status.** Proposed. The phone application team must review it.

**Decision.** The device side follows the [BLE protocol draft](07-ble-protocol-draft.md) until Q-13 has an answer.
Version 0.2 of the draft marks the Team and Report messages as P1.
Document version 0.3 defines protocol version 0.1, and fixes every field value.

**Cost.** A change from the phone team can force rework.
The packet layer stays apart from the codec and the state machine. This split limits the rework.

## 12. D-18: no power management in the first version

**Decision.** The first version builds no power management.
REQ-16, US-16, TC-16, and Q-11 move to Deferred.
The device uses no sleep mode. It requests the best-performance BLE link settings, and keeps one fixed advertising interval.
Doc 07, section 4, lists the link settings: the shortest interval, no peripheral latency, LE 2M, and the longest data length.

**Reason.** The user wants the shortest path to a working MVP.

**What stays.**

- The screen still shows the battery level, or an unknown state when the gauge gives no value.
- The queues keep their fixed sizes. That rule belongs to the design, not to power management.

**Cost.**

- The team makes no runtime claim. The battery can drain faster than it would with power management.
- The 60-minute stability test leaves the first-version gate. A reset shows up only in the Sprint 1 device tests.
- A field pilot needs a review of this decision.

## 13. D-19: ignore a long DOWN press while audio runs

**Problem.** The requirements define a long DOWN press only on the idle Coach page. They do not define it during capture, wait, or playback.

**Decision.** In these three states, the device ignores a long DOWN press.
The user holds UP to cancel first. Then the user holds DOWN to exit.

**Reason.**

- REQ-17 requires clear modes and no accidental input. While audio runs, each button does one thing.
- This follows D-10: cancel and exit use different holds.
- The firmware already behaves this way. The code needs no change.

**Cost.** An exit during audio needs one more press.

## 14. D-20: the phone answers only after the question ends

**Problem.** The phone can start an answer while the user still holds OK.
The device still records at that time. It drops the PLAY_START and the audio blocks after it.
After the 10-second wait, the screen shows a timeout. The user hears no answer.

**Decision.** The phone sends the PLAY_START of an exchange only after it receives AGENT_END.
During capture, the device still drops a PLAY_START.

**Reason.**

- Push-to-talk ends the question when the user releases OK. The device must not stop the microphone while the user holds OK.
- The phone keeps the complexity. This follows the principle in section 1.
- No packet format changes, so the protocol version stays 0.1.

**Cost.** The phone application team must accept this rule. Doc 07, section 14, gives the rule.

## 15. Related documents

- [Product requirements](01-product-requirements.md).
- [System architecture](02-architecture.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Document index](README.md).
