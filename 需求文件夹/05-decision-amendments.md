[简体中文](05-decision-amendments.zh_CN.md) · **English**

# Decision amendments: D-08 to D-30

Version: 0.9. Date: 2026-10-17. Status: D-08 to D-15 and D-17 to D-30 accepted by the user. The phone application team accepted D-16. It must accept D-29 and the three turn messages of D-30.

This document gives the reasons behind D-08 to D-30.
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

**Status.** Replaced by D-29 on 2026-10-17. The text below keeps the old decision as a record.

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

**Status.** Replaced by D-29 on 2026-10-17. The text below keeps the old decision as a record.

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

**Status.** The phone application team accepted protocol 0.2 on 2026-10-17.

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

**Status.** Replaced by D-29 on 2026-10-17. The text below keeps the old decision as a record.

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

## 15. D-21: live data on the live data page, and a Report page in Rest mode

**Problem.** In Increment 1, the live data page shows one fixed sentence. During a run, the skier wants the speed and the progress on this run.

**Decision.** The live data page shows five live numbers. Rest mode adds a Report page with the report of the last run.

**Reason.**

- The "LIVE RUN" title and the body area of the live data page hold these numbers. No new page is necessary.
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
The live fields cover vertical drop, distance, turn count, and run time. The user stated that "structure" means the speed (Q-17). After a run starts, the phone sends one packet each second.
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

**Status.** Accepted by the user on 2026-10-17.

## 19. D-25: the change between the Coach page and the Report page

**Status.** Replaced by D-29 on 2026-10-17. The text below keeps the old decision as a record.

**Decision.** On the idle Coach page, a short DOWN press opens the Report page. On the Report page, a short UP press returns to the Coach page. An OK hold on the Report page does nothing.

**Reason.**

- In Increment 1, a short press has no action. These two short presses therefore conflict with no current gesture.
- A DOWN hold still exits, and an UP hold still cancels. The rule of D-10 stays.
- An OK hold that does nothing on the Report page stops the user from thinking that a question started.

**Cost.** A gloved hand gives an accidental short press more easily. An accidental press only changes the page. It never starts capture.

**Status.** Accepted by the user on 2026-10-17.

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
- The report shows on the live data page on the lift and on the Report page in Rest mode.

**Reason.** The three items change only the screen. They do not change the rest of the protocol. A later change costs little.

**Status.** Accepted by the user on 2026-10-17.

## 22. D-28: the phone opens the Report page

**Decision.** When the phone sends SHOW_PAGE 3, the device changes to the Report page from the live data page (LIVE) or the idle Coach page (READY). During capture, the wait, and the playback, the device ignores the message.

**Reason.** The user wants the phone to trigger the change only in these two states. The phone can then show the report after a run, and it never stops a question or an answer of the user.

**Cost.**

- After a request, the device can stay on its page. The phone application trusts the page in CONTEXT.
- SHOW_PAGE is a new message. Firmware at commit `31e511c` or older answers with ERROR code 1.

**Status.** Accepted by the user on 2026-10-17. After D-29, the old rule "in Ski mode, also enter Rest mode" no longer applies. The phone application team must accept this message.

## 23. D-29: three pages at one level

**Problem.** With two modes, the user must remember the current mode and the action of each button in each mode. The Report page sat below Rest mode, one more level down.

**Decision.**

- Ski mode, Rest mode, and the exit question go away.
- Three pages sit at one level: live data, run report, and AI coach. A short DOWN press goes forward. A short UP press goes back. The last page leads to the first page. The device starts on the live data page.
- On every page, an OK hold opens the Coach page. That press never starts capture.
- On the Coach page, the user holds OK to ask a question, and releases OK to send it.
- During capture, the wait, or the playback, a short UP or DOWN press cancels the question first, and then changes the page.
- Actions inside a page will use a short OK press. No page uses it yet.
- An UP hold and a DOWN hold have no action.

**Reason.**

- Only two rules stay: UP and DOWN change the page, and an OK hold finds the coach. Users with gloves can remember them.
- The device cannot check whether the user stopped. Capture needs one more OK hold after the Coach page opens, so an accidental OK press during a run never opens the microphone.
- The user selected "cancel the audio and change the page at once". So one press can leave the Coach page at any time.

**Cost.**

- D-29 replaces D-10, D-15, D-19, and D-25. TC-17, US-17, and the U1 tasks need new text.
- With no Rest mode as a gate, an accidental press changes the page more easily. The risk log asks U1 to record each accidental press.
- The three buttons share one ADC. While the user holds OK to record, the device cannot read UP or DOWN. The user must release OK first, and that sends the question.
- The mode and page fields of the protocol change their meaning. The phone application team must accept them (doc 07, section 16).

**Status.** Accepted by the user on 2026-10-17.

## 24. D-30: a Turns page in the page loop

**Decision.** REQ-05 leaves Deferred and becomes Increment 3.
The page loop of D-29 gets a fourth page: the Turns page, between the Report page and the Coach page.
The phone sends the turn count of the last run in TURN_SHAPE (0x63).
The page shows 8 turns at a time. The device keeps 3 groups of 8 turns: the group on the screen and the groups next to it.
The device asks the phone for each missing group in TURN_REQUEST (0x65). The phone answers in TURN_GROUP (0x64).

- Short UP and DOWN presses change the page, as on every page. An OK hold opens the Coach page, as on every page.
- Inside the page, a short OK press selects the next turn. An UP hold selects the previous turn. A DOWN hold goes to turn 1 of the next page of 8 turns.
- The page follows the selected turn. When the page opens, the device draws the first page, one point each 40 milliseconds.
- A rider sprite sits at the head of the line while it draws. After each key press, the rider moves along the selected turn in 1.6 seconds.
- A bar of two rows names the keys. Row 1 shows the short presses: "▲报告", "●下一弯" or "●跳过", and "▼教练".
- Row 2 shows the holds that work now: "长按▲上一弯", "长按▼下一页", or both as "长按 ▲上一弯 ▼下一页". With no hold, it shows "长按●问教练".
- Below the shape, the page shows the side of the turn, its speed, its edge angle, and its radius.
- The phone sets a source flag in each shape: measured or schematic. For a schematic shape, the page shows "示意".

**Reason.**

- On 2026-10-17, the user asked for a turn shape effect on top of the reports.
- The App design (Figma, "Segment details") already shows a turn shape and the metrics of each turn. The device page uses the same terms.
- The device only draws what the phone sends. The phone and the Agent keep all analysis, as in D-26.
- D-17 deferred the turn view because a schematic curve cannot support a claim about technique. The source flag answers this risk. The phone must mark each shape that does not come from a measured track.
- A run can have dozens of turns. If one screen shows the whole run, a turn of a 50-turn run gets only 4 px. A page of 8 turns gives each turn 25 px.
- The user chose a fourth page in the loop. D-29 keeps the short UP and DOWN presses for the loop, so the turns use OK and the two holds that D-29 left free.
- The phone decides each point. The device sets only the pace, so it can show a whole page at once and draw it again with no new data.
- The rider is a sprite sheet of 16 frames, so the device rotates no image at run time.
- The three turn messages add messages and change no current packet. The protocol version stays 0.2, as for SHOW_PAGE (D-28).

**Cost.**

- Q-02 still has no source of real track data. Until it has one, the phone can send only schematic shapes.
- One step back costs a hold of 500 ms. One step on costs a short press.
- The loop has 4 pages. From the live data page, the Coach page is now three DOWN presses away. An OK hold still opens it in one step.
- A run has at most 255 turns. The page and the data use about 4.0 KB of static RAM. The page uses 16,824 bytes of the LVGL pool. The rider sprite uses 25.6 KB of Flash.
- TURN_REQUEST is the first request from the device to the phone. The phone must keep every group of the last run and answer within 300 ms.
- Without the link, the page can show only the cached groups.
- Firmware at commit `bedcf1a` or older answers TURN_SHAPE and TURN_GROUP with ERROR code 1. The phone application can ignore that error.

**Status.** The user accepted the page, the pages of 8 turns, the requests from the device, and the key hints on 2026-10-17. On the same day, the user chose the place of the page: the fourth page in the loop of D-29. The phone application team must accept the three turn messages.

## 25. Related documents

- [Product requirements](01-product-requirements.md).
- [System architecture](02-architecture.md).
- [Validation and decisions](04-validation-and-decisions.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Four-hour Sprint roadmap](08-sprint-roadmap.md).
- [Document index](README.md).
