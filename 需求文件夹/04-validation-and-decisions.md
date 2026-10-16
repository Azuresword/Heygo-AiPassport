[简体中文](04-validation-and-decisions.zh_CN.md) · **English**

# Validation, assumptions, and decisions

Version: 0.2. Date: 2026-10-16. Status: Draft for review.
All product tests below have status NOT RUN.
Document checks do not count as firmware or device tests.

## 1. Evidence rules

Record these details for each test run:

- Firmware revision.
- Service revision.
- Phone model and operating system.
- Transport.
- Data source.
- Test date.

Record the board revision when it is available.
Separate measured results from proposed targets.

Synthetic data can test rendering and protocol behavior.
It cannot prove ski-analysis accuracy or actual HeygoAI integration.
A mock Agent cannot prove that the real coaching service works.
A local audio loop cannot prove that two people can use the intercom.

All TC identifiers map to the matching REQ identifiers in the [requirements](01-product-requirements.md).
The [Scrum backlog](03-scrum-backlog.md) assigns stories and dependencies.
The [architecture](02-architecture.md) defines the proposed component contracts.

## 2. Proposed test conditions

### N1: indoor reference network

Use the selected transport, one AI Passport, a supported phone, and the actual regional services.
Add a second authenticated endpoint for human intercom.
That endpoint can be a phone client or another AI Passport with its required bridge.

Measure these network values during each latency test:

- Gateway round-trip time.
- Jitter.
- Packet loss.

The proposed reference limits are:

- Round-trip time: 150 ms.
- Jitter: 30 ms.
- Packet loss: 1%.

Use the 95th percentile for round-trip time and jitter.
If a run exceeds these limits, report its results separately as a degraded-network run.

For 30 ordered latency samples, use sample 29 as the 95th percentile.
Use monotonic clocks for durations on one endpoint.
Measure speech delay with synchronized recordings or an external audio measurement.
Do not subtract unsynchronized device and server clocks.

### N2: failures and interruptions

Inject 5% packet loss and a 30-second outage as separate cases.
Also test these cases:

- A phone permission failure.
- A phone background transition.

Test a device clock change of five minutes.
Test unsupported BLE frame capacity and link loss.

These cases test recovery. They do not inherit the N1 latency target.
In each case, the device must stop invalid capture or playback.
The device must show a specific status.

### U1: usability at rest

Recruit five adult participants for a supervised indoor test.
Record previous skiing experience and previous device use.
Test bare hands and the selected glove type.
Use the same task script for every participant.
Do not coach the participant during an attempt.

Each participant must complete these tasks:

- Find a report.
- Select a turn.
- Ask the Agent a question.
- Start human intercom.
- Cancel speech.

Record errors, completion, and confusion between Coach and Team modes.
The sample can reveal usability problems. It cannot establish population-wide usability.

## 3. Acceptance matrix

Each responsible role remains unassigned.
Each test must retain a result record and a link to its evidence.
P1 tests become release gates only when the corresponding feature enters a release scope.

| Test | Conditions and sample | Pass rule | Method and responsible role |
| --- | --- | --- | --- |
| TC-01 | Run 10 connection attempts on N1 with the phone application. | At least 9 attempts finish within 180 seconds. The device accepts one phone at a time. | Device demonstration. Phone and firmware. |
| TC-02 | Apply 30 report updates. Include duplicates, reordered versions, interrupted transfers, and changes between ski sessions. | Show the correct ski session and latest valid report version. Never show a partially updated report. | Contract tests and device demonstration. Service and firmware. |
| TC-03 | Open cached report cards 30 times on the device. | The 95th percentile is at most one second. The card shows the source, generation time, age, and supported fields. | Screen recording with event timing. Firmware and design. |
| TC-04 | Use 12 fixtures covering values, units, absent fields, gaps, outliers, and long labels. | Every fixture retains its meaning. Unknown values stay unknown. No chart silently changes units. | Host layout tests and device screenshots. Data and firmware. |
| TC-05 | Use 12 turn fixtures. Include left, right, straight, partial, mirrored, and invalid-coordinate cases. | Valid curves preserve direction and gaps. Invalid curves produce a clear error. Every view identifies its evidence type. | Geometry tests and device review. Data and design. |
| TC-06 | Run 100 capture gestures on the Coach page. Include early release, cancellation, the 20-second limit, recognition failure, and link loss. | Capture stops within 100 ms after release or cancellation. No capture exceeds 20 seconds. The first frame leaves the device without a wait for the gateway. The device sends an end or cancel message every time. | Audio measurements and state tests. Firmware and phone. |
| TC-07 | Interrupt 30 spoken replies at different stages. Include packets that arrive after cancellation. | Playback stops within 200 ms. Old audio never resumes. No capture starts during playback without an explicit transition. | Audio timing and protocol tests. Firmware. |
| TC-08 | Use N1. Run 20 context cases. Time 30 questions through the actual service. | All context cases use the correct ski session. The 95th percentile for first audible response is at most five seconds. | Service transcripts with consent and device timing. Agent and service. |
| TC-09 | Use N1. Run 20 cases with competing speaking requests. Run 30 timing trials. Add room isolation and reconnect cases. | Only one sender holds speaking permission. Grant latency is at most 800 ms at the 95th percentile. Audio delay is at most 500 ms. A start message after release never opens the microphone. | Integration test with two endpoints. Service, phone, and firmware. |
| TC-10 | Review 10 suggested practice goals against their source reports. | Each goal cites evidence and states relevant limits. The content reviewer accepts each goal for the defined user scenario. | Expert content review. Product and Agent. |
| TC-11 | Repeat 10 quest completion events with retries and reordered delivery. | Each eligible event grants one reward. No reward encourages maximum speed or device use during descent. | Service tests and content review. Product and service. |
| TC-12 | Run 10 note-save trials with retries, cancellation, edits, and deletion. | A save with confirmation creates one note in the correct ski session. Cancellation creates no note. Edits and deletions persist after reload. | Service and phone tests. Phone and service. |
| TC-13 | Run 10 outage and restart trials. Interrupt a report update in five trials. | One complete cached report remains available. Generation time, age, and offline status remain visible. No live human speech enters a retry queue. | Power and network interruption tests. Firmware. |
| TC-14 | Deferred. D-13 excludes this test from the first version. | None. | None. |
| TC-15 | Run U1. Render every Chinese string in the product glossary. | At least four users complete each core task. Every required glyph renders without clipping. No participant sends speech to the wrong destination. | Device usability study and glyph inspection. Design and firmware. |
| TC-16 | Run a 60-minute workload with reports, speech, and reconnects. Test valid and failed battery readings. | No reset or allocation failure occurs. Queues stay bounded. Invalid readings show unknown status. Measure runtime without extrapolating a full day. | Heap, power, and device logs. Firmware. |
| TC-17 | Run 50 state transitions, including holds, cancellation, mode changes, and Rest exit. | Coach and Team never own audio together. A hold never also triggers a short-click action. Rest exit stops audio. | State-machine tests and device demonstration. Firmware and design. |
| TC-18 | Compare 10 session pairs with differing sources, units, terrain metadata, or missing fields. | Incompatible comparisons remain explicit. The interface does not rank incomplete data as better performance. | Data tests and design review. Data and design. |
| TC-19 | Create 10 private shares. Revoke each share. Test access without login. Test unauthorized access. | Only intended viewers can open the report. Revoked access fails. The system shares location or voice data only with consent. | Access tests and phone demonstration. Phone and service. |
| TC-20 | Run 10 N2 reconnect trials. Inspect all diagnostic output. | Requests remain idempotent. Stale replies never play. | Fault injection. Firmware and service. |

For TC-09, the 500 ms audio target uses the 95th percentile across 30 trials.
Test a 20-second capture limit and a speaking lease of at most 25 seconds.
No audio may play after its session, permission, or playback deadline expires.

For TC-08, unsupported posture or trajectory questions must retain explicit uncertainty.
Measure the first audible response from the user's OK release.
The Agent must distinguish source data from an illustrative explanation.
When speech recognition fails or has low confidence, the device asks the user to repeat the question.
The device offers no transcript editor, because three buttons cannot edit text.
The system must not invent the transcript.

For TC-06 and TC-07, a 500 ms UP hold produces the cancellation command.
Measure the time to stop capture from that command.
Measure the time to stop playback from that command.
The target is to stop playback within 700 ms of the initial UP press.
Measure normal OK release latency from release, without adding a hold interval.

## 4. Test assets

Create a synthetic fixture pack before the first Sprint demonstration.
Mark every synthetic report and turn as synthetic in its payload.
Show the synthetic label on every corresponding screen.
Include complete, partial, empty, oversized, stale, corrupted, and reordered reports.

Create turn fixtures with known geometry and coordinate definitions.
Include left and right turns, a straight segment, a path gap, and an illustrative curve.
Do not treat these fixtures as a ski-performance dataset.

Obtain a sanitized real HeygoAI example before claiming real report integration.
Obtain permission before retaining user speech or real ski-location data for tests.
Keep test credentials outside the repository.

## 5. Open questions and decision owners

All decisions below remain open unless the status column says otherwise.
The user parked these questions. The user will share answers after a review with the phone application team.
The target Sprint is a decision deadline proposal, not a calendar commitment.

| ID | Decision or missing evidence | Owner role | Decision target | Status and consequence |
| --- | --- | --- | --- | --- |
| Q-01 | Which HeygoAI APIs, authentication methods, tools, and report fields exist? Does the source timestamp mean generation time or availability time? | HeygoAI maintainer and service. | Before real integration in Sprint 2. | Open. Synthetic demos cannot close this question. |
| Q-02 | Which source provides turns and metrics? What are its units, coordinates, accuracy, and rights? | HeygoAI maintainer and data. | Sprint 1. | Open. No source means no measured-turn claim. |
| Q-03 | Which phone platform supports audio, reports, and background operation? | Phone and firmware. | Sprint 1. | D-08 selected the BLE phone bridge. The phone platform stays open. |
| Q-04 | What consent, retention, deletion, and model-provider policies apply? | Product Owner and service. | None in the first version. | Deferred by D-13. The first version builds no privacy feature. |
| Q-05 | Which speech recognition and synthesis services support the target language and latency? | Service and speech. | Sprint 1. | Open. The phone application may supply them. Do not assume HeygoAI includes them. |
| Q-06 | Who supplies the second intercom endpoint and room service? | Product Owner and service. | Sprint 1. | Open. A local loop does not satisfy human intercom. |
| Q-07 | What temperature, moisture, gloves, mounting, and visibility conditions define the outdoor pilot? | Product Owner and hardware. | Before any outdoor pilot. | Open. Indoor prototype only until evidence supports the environment. |
| Q-08 | What team capacity, device count, budget, and provider quota are available? | Product Owner and Developers. | Sprint 1 Planning. | Open. Four Sprints are a forecast, not a commitment. |
| Q-09 | Who reviews coaching content and approves the target user group? | Product Owner. | Before coaching pilot review. | Open. Practice suggestions stay provisional. |
| Q-10 | Which firmware baseline, licenses, and reusable intercom components will the project adopt? | Firmware and service. | Sprint 1. | Open. D-09 selected the frame format. The local intercom reference lacks a commit and test evidence. |
| Q-11 | What report size, point limit, audio queue, and memory reserve survive the combined workload? | Firmware and data. | Run a technical experiment in Sprint 1. Establish final limits in Sprint 4. | Open. Architecture limits are provisional until TC-16 passes. |
| Q-12 | What known conditions make two ski sessions comparable? | Data and product. | Before REQ-18 enters a Sprint. | Open. Show differences instead of a misleading rank. |
| Q-13 | Which BLE interface does the existing phone application offer? Can it stream audio in both directions, run speech recognition and synthesis, and support intercom? | Phone application team and firmware. | Before Sprint 1 device integration. | Open. Most device requirements depend on the answer. |

## 6. Initial decision record

| ID | Decision | Status | Reason and review trigger |
| --- | --- | --- | --- |
| D-01 | Keep reports, both voice directions, Agent control, human intercom, charts, and turn views in the core scope. | Requested scope. | These capabilities come from the user's request. |
| D-02 | Use AI Passport as the interaction device. Keep large analysis and inference elsewhere. | Proposed. | The board has no PSRAM. Review after resource measurements. |
| D-03 | Require explicit Rest mode for browsing and speech in the pilot. | Proposed. | Motion sensing lacks evidence. Review after user and field tests. |
| D-04 | Separate Coach and Team modes with one audio owner. | Proposed. | The destination must stay clear. Echo cancellation lacks evidence. |
| D-05 | Start with existing hardware and private indoor trials. | Proposed. | Environmental limits remain unknown. |
| D-06 | Use paired English and Chinese documents with the asd-ste100 skill. | User instruction. | Preserve facts and uncertainty in both versions. |
| D-07 | Keep all latency, gesture, and capacity values as proposed targets. | Proposed. | No product measurement exists yet. |
| D-08 | The device reaches the network through a BLE phone bridge. The device does not use Wi-Fi. | Accepted by the user. | A build without Wi-Fi left 180,840 bytes of static RAM free. See [decision amendments](05-decision-amendments.md). |
| D-09 | The device streams IMA ADPCM at 16 kHz with 20 ms frames. Raw PCM stays a fallback. Opus waits for measurement. | Accepted by the user. | ADPCM costs a few integer steps per sample and 8,000 bytes per second. |
| D-10 | Cancel and back use separate gestures. | Accepted by the user. | One gesture with a state-dependent meaning works against REQ-17. |
| D-11 | The device streams microphone frames to the phone. The Coach page starts at once. The Team page waits for a start message. | Accepted by the user. | The phone holds the Coach question. Early Team capture would exceed the 500 ms audio target. |
| D-12 | The first-version BLE link has no encryption, passkey, or binding check. | Accepted by the user. | The MVP needs the shortest path. Any nearby BLE client can connect. |
| D-13 | The first version builds no privacy or authorization feature. REQ-14 moves to Deferred. | Accepted by the user. | The MVP needs the shortest path. A later version needs a new decision. |
| D-14 | The device does four tasks. The existing phone application does the rest. | Accepted by the user. | Another team develops that application. Device requirements follow its capabilities. |
| D-15 | An OK hold in Ski mode enters Rest mode. A DOWN hold on the top Rest page and then a short OK exit. | Proposed. | The journey requires manual entry and exit. A hold avoids accidental entry with gloves. |

## 7. Risk register

| Risk | Effect | Next experiment or mitigation | Responsible role |
| --- | --- | --- | --- |
| HeygoAI lacks an expected interface. | A core integration cannot ship as planned. | Resolve Q-01 and Q-02. Keep a narrow adapter. Keep a labeled simulator. | Service and data. |
| Phone background limits interrupt audio. | The skier loses speech or connectivity. | Test the BLE link and background behavior on the chosen phone. | Phone and firmware. |
| The BLE stack, fonts, and audio exhaust RAM. | The device resets or loses input. | Stream data within the size limits. Test resource use under the combined workload. | Firmware. |
| Agent speech reaches the human room. | The user exposes a private question. | Show the recipient. Separate Agent and Team exchanges. Run TC-17. | Firmware and service. |
| Sparse coordinates produce misleading curves. | The user mistakes a diagram for a measured path. | Preserve gaps. Preserve source information. Run TC-05 with invalid inputs. | Data and design. |
| Cold, moisture, wind, or gloves impair operation. | Indoor results do not transfer to the slope. | Establish the conditions in Q-07 before field tests. Measure actual behavior. | Hardware and product. |
| Context arrives after the user selects another ski session. | The Agent answers about the wrong run. | Bind every request and reply to a ski session and report version. | Service and firmware. |
| Prototype work expands into commercial hardware. | Cost and verification obligations change. | Create a separate scope review before sales or hardware changes. | Product Owner. |
| The phone application cannot stream audio or lacks intercom. | Voice and intercom stories cannot ship as planned. | Resolve Q-13 first. Keep a raw PCM fallback. | Phone application team and firmware. |
| The open BLE link lets a nearby client connect. | A stranger can read reports or audio frames. | Keep pilots indoor and in a small group. Review D-12 before any public trial. | Product Owner. |

## 8. Evidence required for delivery

Every Sprint Review must identify what the team actually demonstrated.
Record the data source, real services, simulators, failed tests, and remaining dependencies.
Do not mark a core integration story Done when only its simulator works.

Before firmware delivery, complete these steps from the adopted baseline:

- Run the relevant host tests.
- Run the static checks.
- Build the firmware.
- Run the merged-image checks.

Report Build, Host tests, and Device tests separately.
Record any unverified physical or external-service behavior.

The current document package has no firmware implementation.
Its firmware status is therefore:

```text
Build: NOT RUN
Host tests: NOT RUN
Device tests: NOT RUN
Unverified: All product behavior, external interfaces, resource budgets, and field conditions.
```
