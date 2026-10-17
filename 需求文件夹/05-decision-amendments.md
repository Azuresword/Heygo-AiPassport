[简体中文](05-decision-amendments.zh_CN.md) · **English**

# Decision amendments: D-08 to D-27

Version: 0.6. Date: 2026-10-17. Status: D-08 to D-15, D-17 to D-23, and D-26 accepted by the user. D-16, D-24, D-25, and D-27 proposed.

This document gives the reasons behind D-08 to D-27.
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

## 15. D-21: live data on the Ski page, and a Report page in Rest mode

**Problem.** In Increment 1, the Ski page shows one fixed sentence. During a run, the skier wants the speed and the progress on this run.

**Decision.** The Ski page shows five live numbers. Rest mode adds a Report page with the report of the last run.

**Reason.**

- The "LIVE RUN" title and the body area of the Ski page hold these numbers. No new page is necessary.
- The skier reads the summary during a rest break, so it belongs in Rest mode. During a run, the screen gives only content that one look can read.

**Cost.** A look at the screen during a run takes attention from the slope. The risk log records the mitigations.

## 16. D-22: Increment 2 in parallel with Increment 1

**Decision.** Live data and reports form Increment 2. The team develops it in parallel with the device acceptance of Increment 1.

**Reason.**

- The acceptance of Increment 1 waits for the board and the phone application. The first three Increment 2 Sprints need only the host.
- Increment 2 uses only the direction from the phone to the device. Its code stays mostly apart from the voice chain.

**Cost.** The team starts P1 work before P0 closes. Each increment has its own acceptance. Increment 2 cannot replace the acceptance of Increment 1.

## 17. D-23: the device team defines the fields

**Decision.** The device team defines the data fields, and the phone application team adapts to them.
The live fields cover vertical drop, distance, turn count, and run time. The speed waits for Q-17. After a run starts, the phone sends one packet each second.
On 2026-10-17 the user changed the fields and added the turn count. D-26 gives the fields of the run report.

**Reason.**

- The device sets what the screen can show. Fixed fields give the phone application team a clear target.
- The phone can calculate these fields from its location and pressure data.
- Q-01 no longer blocks Increment 2.

**Turn count accuracy.** The turn count needs the detection of each turn. Its accuracy depends on the phone algorithm. The device only shows the value from the phone. The first version leaves out rhythm.

**Cost.** A speed value can encourage high speed. The first version shows the number only, with no ranking and no reward. The rule of REQ-11 also applies.

## 18. D-24: protocol 0.2 and the merge time

**Decision.** The protocol changes from 0.1 to 0.2 and adds LIVE_STATS.
After Sprint 4, the branch `feat/report-live-data` merges into `main`. At the same time, the team tells the phone application team to change to 0.2.

**Reason.**

- While the major version is 0, both sides must use the same minor. A new message type therefore needs a new minor.
- One change is simpler than two versions side by side for a long time.
- After Sprint 4, all Increment 2 device code exists. One merge is enough.

**Cost.** Before the change, a phone application with 0.1 only gets no voice with the new firmware. Both sides must therefore change to 0.2 before Sprint 6.

**Status.** Proposed. It waits for the user.

## 19. D-25: the change between the Coach page and the Report page

**Decision.** On the idle Coach page, a short DOWN press opens the Report page. On the Report page, a short UP press returns to the Coach page. An OK hold on the Report page does nothing.

**Reason.**

- In Increment 1, a short press has no action. These two short presses therefore conflict with no current gesture.
- A DOWN hold still exits, and an UP hold still cancels. The rule of D-10 stays.
- An OK hold that does nothing on the Report page stops the user from thinking that a question started.

**Cost.** A gloved hand gives an accidental short press more easily. An accidental press only changes the page. It never starts capture.

**Status.** Proposed. It waits for the user.

## 20. D-26: a report for each run

**Decision.** After each run, the phone sends the report of that run. It holds the distance, turn count, Heygo IQ, core problem, and a tip of at most 20 characters.

**Reason.**

- After a run, the user most wants to know what went wrong and what to change on the next run.
- A short tip fits on two lines of the small screen.
- The device only shows the report. The phone and the Agent make the score and the tip.

**Cost.** The report arrives later than the end packet. The device must handle the state "run ended, report not yet here".

## 21. D-27: three default values for the report

**Decision.** The user did not answer these three items. The device team uses these values until the user answers:

- The Heygo IQ is an integer from 0 to 100, in 1 byte.
- The phone sends the core problem as text of at most 12 Chinese characters.
- The report shows on the Ski page on the lift and on the Report page in Rest mode.

**Reason.** The three items change only the screen. They do not change the rest of the protocol. A later change costs little.

**Status.** Proposed. It waits for the user.

## 22. Related documents

- [Product requirements](01-product-requirements.md).
- [System architecture](02-architecture.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Four-hour Sprint roadmap](08-sprint-roadmap.md).
- [Document index](README.md).
