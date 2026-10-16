[简体中文](03-scrum-backlog.zh_CN.md) · **English**

# Scrum backlog and delivery plan

Date: 2026-10-16. Version: 0.3.

Status: Proposed. This document defines a draft plan. No story, Sprint, or test is complete.

Related documents: [Requirements](01-product-requirements.md), [Architecture](02-architecture.md), [Validation and decisions](04-validation-and-decisions.md), [Decision amendments](05-decision-amendments.md), and [BLE protocol draft](07-ble-protocol-draft.md).

## 1. Product Goal

A skier can review a ski session and discuss one useful observation through AI Passport while resting.
The skier can also speak with one partner through a deliberate button action.
HeygoAI supplies coaching data or coaching services when documented interfaces permit access.

The first release combines reports, charts, turn views, voice, Agent interaction, and human push-to-talk.
Human push-to-talk means two people share a network voice channel with one speaker at a time.
A speaking permission lease is temporary speaking permission from the service.
Human push-to-talk remains a core requirement.

The project uses manual Rest mode for focused interaction.
Rest mode does not prove that the skier stopped moving.
The product does not provide emergency communication, route guidance, or medical advice.

## 2. Planning assumptions

| Item | Proposed assumption | Required check |
|---|---|---|
| Product stage | Concept prototype using existing AI Passport hardware. | Product Owner agrees scope. |
| Delivery horizon | Four Sprints, each with a two-week timebox. | Developers check capacity before each Sprint. |
| Dates | No start date or release date exists. | Product Owner agrees dates after dependency review. |
| People | Three Developers cover firmware, phone or gateway, and service integration. | Assign people. Check their actual availability. |
| Additional skills | UX, skiing expertise, and testing support remain available. | Assign these responsibilities before field tests. |
| Scrum accountabilities | One Product Owner and one Scrum Master support the Developers. | Assign accountabilities before Sprint Planning. |
| Hardware | The minimum demonstration uses two authenticated endpoints, including at least one AI Passport. A phone client can serve as the peer. | Check available hardware before Sprint 1. The team prefers two AI Passports for expanded tests. A second AI Passport remains optional. |
| Device limits | ESP32-C3, 8 MB Flash, no PSRAM, 240 × 320 screen, three buttons, and a 500 mAh battery. | Check memory, storage, rendering, and controls on hardware. |
| Connections | D-08 selects a BLE phone bridge. The device does not use Wi-Fi. | Test link stability, power use, and phone background behavior. |
| Existing software | The baseline advertises through BLE. A phone application exists, and another team develops it. | Implement the device side of the BLE protocol draft. Test it with that application. |
| HeygoAI | Interfaces, account rules, report formats, and voice services remain unknown. | Obtain permitted access and sample data. |
| Measurements | The project has no documented device IMU or GNSS. | Identify each source before promising a turn view. |
| Voice reference | A separate AI-passport workspace contains uncommitted BLE intercom reference code. | Test its frame format with the phone application. No device test evidence exists. |

Story points compare uncertainty and effort. They do not represent days.
Point ranges remain provisional until Developers refine each story.
The plan does not assume a velocity or guarantee delivery within four Sprints.
Velocity means the amount of work the team completes in a Sprint.

Large stories need smaller vertical slices before Sprint selection.
A vertical slice gives the user a complete action across the necessary components.

## 3. Scrum accountabilities and artifacts

| Accountability | Person | Responsibility |
|---|---|---|
| Product Owner | Unassigned | Own the Product Goal. Order the Product Backlog. Clarify value. |
| Scrum Master | Unassigned | Support Scrum. Resolve process barriers. Help the team improve. |
| Developers | Unassigned | Select feasible work. Maintain the Sprint Backlog. Produce a usable Increment. |

The Product Goal describes the desired future state of the product.
The Product Backlog contains all proposed product work.
The Product Owner orders this backlog as evidence changes.

The Sprint Goal is the single objective for a Sprint.
The Sprint Backlog contains the Sprint Goal, selected work, and the Developers' delivery plan.
Developers adapt the Sprint Backlog each day.

An Increment is usable product work that meets the Definition of Done.
The Definition of Done states the quality conditions that an Increment must meet.
An internal prototype or a simulated service does not prove actual integration.

## 4. Proposed Product Backlog

P0 means the proposed first release needs the capability.
P1 means a later increment may add the capability.
These priorities do not represent user approval.
Every story below has status **Proposed**, except US-14. US-14 has status **Deferred**.
Each story maps to the matching requirement and test in the related documents.

| Story | Requirement | Priority | Points | Proposed owner | First target |
|---|---|---|---|---|---|
| US-01 Device connection | REQ-01 | P0 | Re-estimate | Firmware and Phone Developers | Sprint 1 |
| US-02 Session and report sync | REQ-02 | P0 | 5–8 | Service Developer | Sprint 1 |
| US-03 Report cards | REQ-03 | P0 | 3–5 | Firmware Developer | Sprint 1 |
| US-04 Metric charts | REQ-04 | P0 | 3–5 | Firmware Developer | Sprint 2 |
| US-05 Turn view and replay | REQ-05 | P0 | 8–13 | Firmware Developer | Sprint 2 |
| US-06 Voice capture | REQ-06 | P0 | 5–8 | Firmware Developer | Sprint 3 |
| US-07 Speech playback | REQ-07 | P0 | 3–5 | Firmware Developer | Sprint 3 |
| US-08 Button Agent interaction | REQ-08 | P0 | 5–8 | Service Developer | Sprint 3 |
| US-09 Human push-to-talk | REQ-09 | P0 | 8–13 | Service Developer | Sprint 4 |
| US-10 Focused practice | REQ-10 | P1 | 3–5 | Service Developer | Later |
| US-11 Quests and badges | REQ-11 | P1 | 3–5 | Product Developer | Later |
| US-12 Reflection journal | REQ-12 | P1 | 3–5 | Product Developer | Later |
| US-13 Cached reports | REQ-13 | P0 | 5–8 | Firmware Developer | Sprint 1–4 |
| US-14 Privacy and authorization | REQ-14 | Deferred | None | None | Not in the first version |
| US-15 Gloves and readability | REQ-15 | P0 | 3–5 | Firmware Developer | Sprint 1–4 |
| US-16 Resource limits | REQ-16 | P0 | 5–8 | Firmware Developer | Sprint 1–4 |
| US-17 Explicit modes | REQ-17 | P0 | 3–5 | Firmware Developer | Sprint 1–4 |
| US-18 Historical comparison | REQ-18 | P1 | 3–5 | Product Developer | Later |
| US-19 Private share link | REQ-19 | P1 | 3–5 | Phone Developer | Later |
| US-20 Diagnostics and recovery | REQ-20 | P0 | 5–8 | Service Developer | Sprint 1–4 |

Owners describe skill responsibilities. They do not assign people or create separate Scrum teams.
The initial Sprint targets show ordering. They do not commit the whole story to that Sprint.

### US-01: Device connection

As a skier, I want my phone to find and connect to my device, so that the device can use the phone link.

- Acceptance: The device advertises its name after start-up. The existing phone application finds it and connects.
- Acceptance: The device accepts one phone at a time.
- Acceptance: The link uses no encryption, passkey, or binding check, as D-12 states.
- Dependencies: The phone application accepts the BLE protocol draft (Q-13).
- Risk: Any nearby BLE client can connect. The first version accepts this risk.
- Verification: TC-01 with repeated connection attempts.

### US-02: Session and report sync

As a skier, I want the correct report version after each sync.

- Acceptance: Each report carries a ski session identifier, version, source, and update time.
- Acceptance: Duplicate downloads do not duplicate ski sessions. Older versions do not replace newer versions.
- Dependencies: US-01 and a documented report source.
- Risk: HeygoAI may expose summaries without stable versions.
- Verification: TC-02, including duplicate, older, and interrupted transfers.

### US-03: Report cards

As a skier, I want a short report that I can read during a rest.

- Acceptance: Cards show the selected ski session, available summary, and available coaching observations.
- Acceptance: Buttons reach every card. Missing fields show an explicit state.
- Dependencies: US-02, US-15, and US-17.
- Risk: Source reports may exceed screen or memory limits.
- Verification: TC-03 on the physical screen, with complete and incomplete reports.

### US-04: Metric charts

As a skier, I want charts that explain changes without hiding missing measurements.

- Acceptance: Every chart shows its metric, unit, scale, and data source.
- Acceptance: Missing samples appear as gaps. Invalid values do not appear as zero measurements.
- Dependencies: US-02 and documented metric definitions.
- Risk: Sources may use incompatible units or sampling rates.
- Verification: TC-04 with reference values, gaps, and boundary values.

### US-05: Turn view and replay

As a skier, I want to check individual turns and understand the evidence behind each view.

- Acceptance: The view identifies the source, coordinate meaning, quality, and time basis.
- Acceptance: Buttons select turns and control replay. The selection matches the associated report section.
- Acceptance: Schematics carry a schematic label. Missing path data never produces a claimed measured path.
- Dependencies: US-02, US-04, and documented turn data or a defined schematic alternative.
- Risk: A report alone may not support a geographic path or turn radius.
- Verification: TC-05 with known geometry, a schematic case, and missing data.

### US-06: Voice capture

As a skier, I want a deliberate button action to capture my question, so that the Agent hears the first word.

- Acceptance: On the Coach page, a 500 ms OK hold starts the microphone. The screen shows microphone activity from the same moment.
- Acceptance: The device streams each 20 ms frame to the phone at once. It does not wait for the gateway.
- Acceptance: Releasing OK stops capture within 100 ms. The device tells the phone that the input ended. Each question lasts at most 20 seconds.
- Acceptance: A 500 ms UP hold cancels the question. Capture then stops within 100 ms. The device tells the phone to drop the audio.
- Acceptance: When the phone reports a recognition failure, the screen shows a short retry message.
- Dependencies: US-17, device audio, and a phone application that accepts streamed audio and runs speech recognition.
- Risk: Wind, gloves, and link loss can affect capture.
- Verification: TC-06 with capture, release, cancellation, recognition failure, and link loss.

### US-07: Speech playback

As a skier, I want to hear an answer and stop it at once, so that I control the device.

- Acceptance: The phone streams the answer frame by frame. The device plays each frame as it arrives and shows playback state.
- Acceptance: The device recognizes cancellation when the user holds UP for 500 ms. Playback then stops within 200 ms. The device tells the phone.
- Acceptance: The proposed playback budget is 700 ms from the initial UP press to silence.
- Acceptance: Cancelled playback does not resume after a delayed frame or a reconnect.
- Dependencies: US-17, device audio, and a phone application that supplies synthesized speech.
- Risk: Speaker output can enter the microphone or conflict with human push-to-talk.
- Verification: TC-07, including interruption and audio priority tests.

### US-08: Button Agent interaction

As a skier, I want to ask the Agent about the report or turn currently on screen.

- Acceptance: The device tells the phone which report and turn the user selected. The request includes the ski session, report version, and optional turn identifier.
- Acceptance: The answer identifies its source context and states when evidence is insufficient.
- Acceptance: A late answer cannot replace a newer request or speak during human push-to-talk.
- Dependencies: US-03, US-05, US-06, US-07, and a documented Agent interface.
- Risk: An unsupported context format can produce irrelevant or invented explanations.
- Verification: TC-08 with context changes, insufficient data, and delayed answers.

### US-09: Human push-to-talk

As a skier, I want a button to speak with my selected partner through a shared voice channel.
The button lets me keep my gloves on and my phone in my pocket.

- Acceptance: Two participants can join the same room and hear each other in turn.
- Acceptance: In Team mode, a 500 ms OK hold sends a speaking request to the phone. The microphone stays closed.
- Acceptance: Capture starts only after the phone returns a start message and while OK remains held. The device plays a short cue at that moment.
- Acceptance: The device ignores a start message that arrives after OK release.
- Acceptance: The phone and the room service allow one speaker at a time. Each lease lasts at most 25 seconds.
- Acceptance: Releasing OK stops capture within 100 ms. Each utterance lasts at most 20 seconds.
- Acceptance: The screen distinguishes waiting, speaking, listening, and unavailable states.
- Acceptance: Button release, timeout, or link loss ends capture.
- Acceptance: Human voice and Agent speech follow the audio priority rules in the architecture.
- Dependencies: US-01, US-17, a phone application with intercom support, a room service, and two endpoints. At least one endpoint must be an AI Passport.
- Dependencies: A phone client can serve as the peer. Two AI Passports remain the preferred expanded test.
- Risk: Each utterance crosses two BLE links. The 500 ms audio target lacks a measurement.
- Verification: TC-09 with simultaneous requests, alternating speakers, link loss, and reconnection.

### US-10: Focused practice

As a skier, I want one practice suggestion that relates to my selected report.

- Acceptance: The suggestion names its source observation and a clear completion check.
- Acceptance: Insufficient evidence produces a request for more information rather than a fabricated diagnosis.
- Dependencies: US-08 and a skiing expert's review.
- Risk: A suggestion may exceed the skier's ability or local conditions.
- Verification: TC-10 with representative reports and expert review.

### US-11: Quests and badges

As a skier, I want optional goals that reward reflection and appropriate practice.

- Acceptance: Every quest states its completion rule and required evidence.
- Acceptance: Repeated sync does not duplicate rewards. No reward encourages higher speed or dangerous terrain.
- Dependencies: US-10 and an agreed reward policy.
- Risk: Poor incentives can change behavior in unsafe ways.
- Verification: TC-11 with repeated events and a review of all reward rules.

### US-12: Reflection journal

As a skier, I want to attach a short reflection to a ski session.

- Acceptance: The journal entry retains its ski session identifier and text.
- Acceptance: The user can edit or delete the entry through the defined device or phone flow.
- Dependencies: US-02, US-06, and US-14.
- Risk: A mistaken transcript can distort the user's reflection.
- Verification: TC-12 with correction, interrupted sync, and deletion.

### US-13: Cached reports

As a skier, I want to read downloaded reports without a network connection.

- Acceptance: A complete cached report remains readable after disconnection and restart.
- Acceptance: The screen shows the report generation time and elapsed age. Interrupted downloads preserve the last complete report.
- Dependencies: US-02 and a measured storage budget.
- Risk: Flash limits and interrupted writes can damage cached data.
- Verification: TC-13 with power interruption, full storage, and reconnection.

### US-14: Privacy and authorization

Status: Deferred. D-13 excludes privacy and authorization features from the first version.

The first version still shows microphone activity, because US-06 and US-17 require it.
A later version needs a separate decision before this story returns to the backlog.

### US-15: Gloves and readability

As a skier, I want to use the three buttons and read Chinese text during a rest.

- Acceptance: Core actions require no touch input. The glove tasks meet the criteria in TC-15.
- Acceptance: Required Chinese glyphs appear on the device without missing characters or clipped controls.
- Dependencies: Button mapping, approved font assets, US-03, US-05, and US-17.
- Risk: Small text, sunlight, gloves, and button timing can cause errors.
- Verification: TC-15 on hardware, with the specified gloves and screen conditions.

### US-16: Resource limits

As a skier, I want the device to remain usable during the defined operating profile.

- Acceptance: Each profile meets the memory, storage, battery, and recovery limits in TC-16.
- Acceptance: The device rejects oversized data without a crash. Low battery produces the defined user state.
- Dependencies: All active features and measured hardware limits.
- Risk: Audio buffers, fonts, charts, and network activity compete for limited resources.
- Verification: TC-16 with measured peaks, repeated use, and stress conditions.

### US-17: Explicit modes

As a skier, I want to know whether buttons control reports, the Agent, or human voice.

- Acceptance: The screen identifies the current mode and the available button actions. Coach mode and Team mode remain distinct.
- Acceptance: The user can enter and leave Rest mode manually.
- Acceptance: Mode changes follow the architecture's audio rules and cannot leave a hidden microphone session active.
- Dependencies: Button mapping and the shared mode state machine.
- Risk: The same button can trigger different actions without clear context.
- Verification: TC-17 across every defined transition, including cancellation and connection loss.

### US-18: Historical comparison

As a skier, I want to compare compatible ski sessions and see what changed.

- Acceptance: The user selects two ski sessions and sees aligned metric definitions and units.
- Acceptance: The view identifies incompatible data and missing evidence instead of presenting a false comparison.
- Dependencies: US-02, US-04, and stable metric definitions.
- Risk: Different conditions or algorithms can invalidate comparisons.
- Verification: TC-18 with compatible, incompatible, and incomplete ski session pairs.

### US-19: Private share link

As a skier, I want to share a selected report through a private server link that I can revoke.

- Acceptance: The phone shows selected report content before the user consents to link creation.
- Acceptance: After consent, the phone requests a private server link. Private audio and location data remain excluded unless explicitly selected.
- Acceptance: The user can revoke the link. The server then rejects further access through that link.
- Dependencies: US-02, US-14, a documented phone flow, and server access controls.
- Risk: Recipients can retain copies. Link revocation cannot delete copies that recipients already obtained.
- Verification: TC-19 with preview, consent, cancellation, authorized access, revocation, and denied access after revocation.

### US-20: Diagnostics and recovery

As a skier, I want a clear recovery action when a connection or service fails.

- Acceptance: Each defined failure shows a useful state and a recovery action.
- Acceptance: Reconnection does not duplicate ski sessions, questions, or speaking permission.
- Dependencies: All network operations and a documented error model.
- Risk: Blind retries can repeat actions or consume battery.
- Verification: TC-20 with service errors, network changes, expired access, and repeated reconnects.

## 5. Proposed Sprint sequence

Each Sprint integrates a user flow on physical hardware.
The table identifies story slices, not a promise to complete every listed story.
Each Increment must meet the Definition of Done for its selected scope.

Sprint 1 tests a synthetic report path. It does not complete actual HeygoAI integration.
The control and duration targets remain proposals until device tests provide evidence.
Cancellation budgets start when the 500 ms UP hold completes. Release budgets start when the user releases OK.

| Sprint | Sprint Goal | Selected story slices | User-visible Increment | Review evidence |
|---|---|---|---|---|
| 1 | Read a synthetic ski session report. Test connection feasibility. | US-01–03 prototypes, US-13, US-15–17, and US-20 foundations, plus a US-09 link experiment. | Read clearly labeled synthetic cards on hardware. Disconnect. Reopen the cached report. | Synthetic source labels, device demonstration, cache recovery, and BLE link findings with the phone application. |
| 2 | Obtain an authorized report. Explain its available measurements. | Actual US-01–05 paths, plus applicable US-13, US-15–17, and US-20 slices. | Read a real report. Select available turns. Check values. Replay the supported representation. | Actual source and access evidence, reference geometry, data provenance, missing-data cases, and glove checks. |
| 3 | Ask the Agent about the visible report through a button. | US-06–08, applicable shared controls, and a US-09 integration check. | Record a question. Check its transcript. Hear an answer about the report. Stop playback. | Real audio and service paths, cancellation cases, latency measurements, and human voice integration findings. |
| 4 | Speak with one partner. Recover from interrupted connections. | US-09 and remaining first-release acceptance across all P0 stories. | Two authenticated endpoints alternate human voice, with at least one AI Passport. Reports, Agent controls, and recovery states remain consistent. | Two-participant tests, complete P0 evidence, resource profiles, and a controlled pilot. |

The Sprint 1 link experiment checks BLE streaming with the existing phone application.
It checks the uncommitted reference code without assuming that the code works on the device.
The Sprint 3 integration check reduces remaining risk for human push-to-talk.
An experiment alone does not complete US-09.
P1 stories remain in the Product Backlog until the Product Owner reorders them.

If actual HeygoAI access remains unavailable, the team can demonstrate a clearly labeled prototype using permitted sample data.
That prototype cannot satisfy acceptance for actual HeygoAI integration.
The team records the dependency and revises the release forecast.
The team does not silently omit human push-to-talk or replace it with Agent voice.

## 6. Initial Sprint Backlog proposal

Sprint Goal: Read a synthetic ski session report. Test connection feasibility.
Developers select the final scope during Sprint Planning.
Every task below has status **Proposed**.

| Task | Selected slice | Proposed owner | Output and completion evidence |
|---|---|---|---|
| S1-01 Discover permitted HeygoAI access. | US-01–02 | Service Developer | Record known identity and source options. Mark unknown access limits. Request a permitted sample. |
| S1-02 Define one candidate report contract. | US-02–03 | Service and Firmware Developers | Create labeled synthetic examples, version cases, missing states, and payload limits. Keep actual HeygoAI fields unknown. |
| S1-03 Test the BLE link with the phone application. | US-09, US-20 | Firmware and Phone Developers | Stream ADPCM frames in both directions. Record frame loss, latency, phone background behavior, and device evidence. |
| S1-04 Integrate one synthetic report path. | US-01–02 prototypes | Service Developer | Connect the test service to hardware through the phone. Show synthetic labels. |
| S1-05 Implement the first report cards. | US-03, US-15, US-17 | Firmware Developer | Show readable cards, Chinese glyphs, manual Rest mode, and all button actions on hardware. |
| S1-06 Preserve one cached report. | US-13 | Firmware Developer | Read the cached report after disconnection and restart. Preserve it during an interrupted transfer. |
| S1-07 Add failure states. | US-20 | Developers | Show link failure and retry states. Check that the microphone is inactive outside capture. |
| S1-08 Measure the first resource profile. | US-16 | Firmware Developer | Record firmware size, free memory, storage use, current draw, and measurement conditions. |
| S1-09 Run the complete user flow. | All selected slices | Developers | Record requirement links, actual results, known gaps, and the usable Increment. |
| S1-10 Review the Increment. Adapt the Product Backlog. | Product Backlog | Scrum Team | Record user feedback, new work, changed assumptions, and the next ordering decision. |

S1-01 determines options for actual integration after the synthetic prototype.
S1-02 supplies the synthetic contract for S1-04.
S1-02 constrains S1-05 and S1-06.
S1-09 requires the integrated outputs from the preceding implementation tasks.
Connection experiments and screen prototypes can proceed while external access remains pending.
Such parallel work does not resolve the external dependency.

## 7. Scrum events and feedback

| Event or practice | Proposed cadence | Purpose | Result |
|---|---|---|---|
| Sprint Planning | At each Sprint start, up to four hours. | Agree the Sprint Goal. Check capacity, dependencies, and acceptance. | Developers create the Sprint Backlog. |
| Daily Scrum | Each working day, 15 minutes. | Check progress toward the Sprint Goal. Adapt the plan. | Developers adjust their next work. |
| Refinement | Ongoing, with a proposed weekly session. | Split large stories. Clarify evidence, risks, and estimates. | A more precise Product Backlog. |
| Sprint Review | At each Sprint end, up to two hours. | Check the usable Increment with users and stakeholders. | Product Backlog changes and new evidence. |
| Sprint Retrospective | After the Review, up to 90 minutes. | Check teamwork, quality, tools, and recurring problems. | At least one specific improvement for the next Sprint. |

These event durations are proposals for two-week Sprints.
Refinement is an ongoing practice, not an additional formal Scrum event.
The Sprint Review checks results. It does not replace test evidence or the Definition of Done.

## 8. Definition of Ready: optional team practice

Scrum does not require a Definition of Ready.
The team can use these checks to reduce avoidable uncertainty before selection.

- State the user outcome and requirement identifier.
- Define observable acceptance and the matching test identifier.
- Identify the real source, necessary access, and unresolved dependency.
- Separate documented facts from assumptions.
- Identify privacy, device, and audio constraints.
- Split the story until the team can integrate a useful slice within one Sprint.
- Record the estimate range and required skills.

An unresolved experiment can enter a Sprint when its question, effort limit, and evidence are clear.
The experiment does not inherit completion status from the product story.

## 9. Definition of Done

Every selected product slice must meet all applicable checks below.

- The user completes the intended action on physical AI Passport hardware.
- The implemented path uses actual services and permitted source data where the slice requires them.
- Synthetic prototype slices show explicit source labels and retain outstanding actual integration work.
- Requirement and test identifiers link to recorded results.
- Required host tests, repository checks, and firmware builds pass.
- Applicable device tests pass under recorded conditions.
- The screen shows correct Chinese glyphs and necessary units, sources, and failure states.
- Button actions, mode transitions, and audio cancellation pass their tests.
- Microphone behavior matches the capture states in the slice.
- Memory, storage, power, and timing measurements meet the agreed slice limits.
- The team resolves defects that prevent the selected action or violate a P0 constraint.
- English and Chinese documents describe the same behavior.
- The team records remaining gaps without claiming that incomplete requirements passed.

Simulated services can support automated tests and early demonstrations.
Simulated services cannot satisfy acceptance that requires real HeygoAI access, actual network audio, or physical device behavior.
Report build, host test, device test, and unverified results separately.
The full first release requires every P0 acceptance test to pass.

## 10. Change and release decisions

1. Record feedback with its affected requirement, evidence, and proposed change.
2. Check the effect on user value, privacy, hardware limits, and current dependencies.
3. Ask the Product Owner to reorder the Product Backlog.
4. Let Developers update estimates and the delivery plan.
5. Preserve the Sprint Goal when adapting current Sprint scope.
6. Record each accepted decision in the shared decision log.

The Product Owner can cancel a Sprint when its goal becomes obsolete.
Incomplete work returns to the Product Backlog for further ordering.
The team does not mark a story Done because the Sprint ends.
No release decision can treat unavailable source data or an untested connection as a passed requirement.

Before field use, the team needs real integration evidence and explicit approval for the proposed device test.
Undocumented radio coverage, cold tolerance, water protection, and battery behavior remain visible limits.
The project plan does not claim outdoor certification or emergency availability.
