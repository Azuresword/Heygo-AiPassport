**English** · [简体中文](06-sprint-0-two-day-plan.zh_CN.md)

# Sprint 0: Two-Day Plan

Version: 0.3. Date: 2026-10-16. Status: proposed.

This document selects the work that two days can finish.
It applies the decisions in [Decision Amendments](05-decision-amendments.md).
It follows the Scrum structure in [Scrum Backlog](03-scrum-backlog.md).

Two days cannot close a P0 story. Section 6 states why.
This plan therefore names a spike, not a product increment.

## 1. What blocks work today

Four dependencies block the stories that need an external party. The team cannot clear these blocks inside two days.

| Block | Open question | Consequence |
| --- | --- | --- |
| The phone application has not accepted the BLE protocol draft | Q-13 | No real streaming test. Device-side work follows doc 07. |
| No HeygoAI API or report fields | Q-01, Q-02 | No real report integration |
| No known speech recognition or synthesis service | Q-05, Q-13 | No real voice answer |
| No room service and no second endpoint | Q-06, Q-13 | No team push-to-talk test |

Q-13 matters most. D-14 makes most device requirements depend on the phone application.
D-16 offers a draft protocol, so device-side work can start before Q-13 has an answer.

One further limit applies. The AI Passport board is not at hand. Every device test therefore reports NOT RUN.

## 2. What two days can finish

The blocks above share one property. They gate work that needs an external party or the board.

Two days therefore target the work that a host test or a firmware build can check.
Synthetic data replaces the real report source. A local stub replaces the phone application.

### Committed core

| Task | Story slice | Requirement | Exit check |
| --- | --- | --- | --- |
| T-01 Record the eight decisions | None | Every device requirement | Both languages pass the structure check |
| T-02 Parse a report, and reject every bound violation | US-02, US-03 | REQ-02, REQ-03, REQ-16 | Host tests cover each limit in architecture section 5 |
| T-03 Generate the synthetic fixture set | Test material | REQ-02, REQ-04, REQ-05 | Seven fixture classes exist, and tests consume them |
| T-04 Implement the audio owner state machine | US-06, US-07, US-09, US-17 | REQ-06, REQ-07, REQ-09 | Generated table, plus one test per transition |
| T-05 Implement the Rest mode gate | US-17 | REQ-17 | Host tests prove that Ski mode blocks voice |
| T-06 Implement the turn geometry contract | US-05 | REQ-05 | Host tests cover left, right, straight, gap and invalid input |
| T-09 Implement the IMA ADPCM encoder and decoder | US-06, US-07 | REQ-06, REQ-07, REQ-16 | Host tests cover frame size, round trip, frame loss and full scale |
| T-10 Implement the packet layer of the BLE protocol draft | US-01, US-06, US-09 | REQ-01, REQ-06, REQ-09 | Host tests cover packet build, packet parse, `xid` rules, and report fragments |

T-01 is complete. Version 0.3 of these documents records D-08 to D-16, and every English document passes the structure check.

### Stretch

These two tasks carry tool risk. The team starts them only after the core passes.

| Task | Story slice | Requirement | Risk |
| --- | --- | --- | --- |
| T-07 Draw the report card, the metric chart and the turn view | US-03, US-04, US-05 | REQ-03, REQ-04, REQ-05 | The generator needs new widget types |
| T-08 Generate a Chinese font subset, and check glyph coverage | US-15 | REQ-15 | The host has no `node`, so `lv_font_conv` cannot run |

## 3. Day split

**Day 1 covers T-02 to T-06, T-09, and T-10.** Every item is plain C. None of them touches LVGL.
None of them needs the board or the phone application. Confidence stays high.
If Day 1 runs short, T-10 moves to the start of Day 2, before the stretch tasks.

**Day 2 covers T-07 and T-08.** Both items touch LVGL or a new tool. Confidence stays low.
The plan keeps Day 2 separate, so a Day 2 failure leaves Day 1 intact.

## 4. Task detail

### T-02: parse a report, and reject every bound violation

Architecture section 5 lists the limits. The parser must apply each limit that the device owns.
The parser must reject the message, and must return a named error.

| Limit | Value | Reject behavior |
| --- | --- | --- |
| Control message | 1 KiB | Reject, and name the error type |
| Report page | 8 KiB encoded | Request the next page |
| Cached reports | 64 KiB total | Delete the oldest report |
| Turn path | 128 points | Request a simplified segment |
| Concurrent Agent exchanges | 1 | Cancel the earlier exchange first |
| One capture | 20 seconds | Stop capture, and release the audio owner |

The phone and the room service own the speaker limit and the lease. The parser does not check them.

The parser must also apply the envelope rules that reject stale data.
It must reject an expired message. It must reject an old device session. It must reject an old connection epoch.
It must reject a repeated report revision that carries different content.

The parser must never allocate a buffer whose size follows the message.
A reassembly buffer that grows with input breaks REQ-16.

### T-03: generate the synthetic fixture set

The validation plan names seven report classes. The fixture set must cover all of them.

1. Complete.
2. Partial.
3. Empty.
4. Oversize.
5. Expired.
6. Corrupt.
7. Out of order.

Every fixture must carry `"synthetic": true` inside the payload.
Every screen that shows a fixture must show a synthetic label.

Turn fixtures need known geometry.
The set must cover a left turn, a right turn, a straight run, a segment gap, a mirrored path and an invalid coordinate frame.

### T-04: implement the audio owner state machine

The machine holds one owner with five values.
The values stay None, AgentCapture, AgentPlayback, TeamCapture and TeamPlayback.

D-11 sets the capture entry per page.

- On the Coach page, the microphone starts at the 500 ms threshold. The device streams each frame at once.
- On the Team page, the hold sends a speaking request. The microphone starts only after the start message, and the device plays a cue.

D-10 sets the cancel path. A long press on UP cancels the active audio owner. A long press on DOWN returns to the previous page.

The generated test must cover every transition. It must also cover these paths:

1. The user releases OK before the Team start message. The device ignores the late start message.
2. The user cancels during Coach capture. The device sends a cancel message to the phone.
3. The user cancels during playback. The device drops every frame that arrives later.
4. The link fails during capture or playback.
5. The phone reports a recognition failure.
6. The user holds OK during Agent playback. Playback stops before the microphone starts.

A local stub plays the phone role in these tests. The stub does not prove the real phone interface.
The state machine uses the message names in the BLE protocol draft.

### T-05: implement the Rest mode gate

The gate follows D-15.

The host tests must prove these rules:

- Ski mode blocks every voice action.
- An OK hold in Ski mode enters Rest mode on the Reports page.
- The OK hold that enters Rest mode does not start capture.
- An exit from Rest mode stops every audio owner.

### T-06: implement the turn geometry contract

Architecture section 4 defines the frame. The origin sits at the first valid sample of the turn.
The positive `y` axis follows the reference fall line. The positive `x` axis points right when the skier faces that line.

The heading equals `atan2(dx, dy)` in degrees. A clockwise rotation from the fall line gives a positive heading.
A positive unwrapped heading change means a right turn. A negative unwrapped heading change means a left turn.

The implementation must derive direction from the heading change.
It must never derive direction from the screen position of the curve. A host test must prove this with a mirrored fixture.

### T-09: implement the IMA ADPCM encoder and decoder

D-09 selects IMA ADPCM at 16 kHz, mono, in 20 ms frames. One frame holds 320 samples.
Each frame starts with its own predictor value and step index.
A lost frame therefore does not corrupt the next frame.

The host tests must prove these rules:

- One 320-sample block encodes to exactly 164 bytes in the layout of doc 07.
- A decoded 1 kHz sine wave keeps a signal-to-noise ratio above 20 dB. This threshold is an initial value.
- The decoder reads the frame after a lost frame without error.
- Full-scale input produces no integer overflow.

The codec is plain C. It has no dependency on ESP-IDF or LVGL.
The BLE protocol draft defines the block layout and the packet header.

### T-10: implement the packet layer of the BLE protocol draft

The packet layer builds and parses the packets in the [BLE protocol draft](07-ble-protocol-draft.md).
It is plain C. It has no dependency on NimBLE, ESP-IDF, or LVGL.

The host tests must prove these rules:

- Each message type builds, and parses back to the same fields.
- The parser rejects a packet with a wrong length, and names error code 2.
- The parser answers an unknown type with error code 1.
- The parser ignores extra bytes at the end of a payload.
- The device drops an audio packet or a state packet with a stale `xid`.
- Report fragments reassemble into one page. A skipped fragment index gives result 2. A total above 8,192 bytes gives result 1.

The packet layer does not prove the real phone link. Q-13 stays open.

### T-08: generate a Chinese font subset

The host has no `node`, so `lv_font_conv` cannot run. A check found a working substitute.
Pillow 11.3.0 reports `freetype2: True`, and it rasterized the string `休息模式` at 20 px.

The task therefore writes a Python generator. The generator reads the product vocabulary.
It rasterizes only the glyphs that the vocabulary needs. It emits an `lv_font_t` C array.

The coverage check must list every missing glyph.
A successful build proves nothing about glyph coverage. The upstream baseline warns about this trap.

## 5. What this plan excludes

| Excluded work | Reason |
| --- | --- |
| US-01 device connection | The phone application has not accepted the BLE protocol draft. T-10 builds the device side only. |
| US-09 team push-to-talk | Q-13 and Q-06 block the phone side and the room service |
| US-13 cache commit under power loss | The check needs the board |
| US-14 privacy and authorization | D-13 defers the story |
| Real speech recognition and synthesis | Q-05 and Q-13 block the service |
| Every device test | The board is not at hand |
| US-10 to US-12, US-18, US-19 | These stories stay P1 |

The team must not report an excluded item as deferred without a reason. Each row above names its reason.

## 6. Definition of Done for this slice

The project Definition of Done requires a device test and a real service.
This slice meets neither condition. The slice therefore reports a spike result.

The slice passes when every statement below holds.

- The firmware build passes.
- The host tests pass.
- The repository checks pass.
- The structure check reports zero violations for every new English document.
- Each new Chinese document matches its English pair.
- Every synthetic screen shows a synthetic label.
- The report records the measured static memory and the Flash size.

The slice reports results in the required format.

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: NOT RUN
Unverified: every external service, every device behavior, every field condition
```

No task in this plan closes a P0 story. Section 5 and section 1 name the missing evidence.

## 7. Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| The phone application team changes the BLE protocol draft | T-10 needs rework | Keep the packet layer separate from the codec and the state machine |
| The phone application cannot stream audio | D-11 fails | Resolve Q-13 first. Keep a raw PCM fallback |
| The generator needs new widget types | T-07 slips | Keep T-07 in the stretch set |
| The font generator emits a wrong glyph table | Chinese text breaks | Check coverage against the vocabulary, not the build result |
| Synthetic fixtures hide a real contract mismatch | Sprint 2 rework | Mark every fixture, and keep Q-01 open |
| Two days produce a demo that reads as a product | Wrong status report | Report the spike format in section 6 |

## 8. Related documents

- [Product Requirements](01-product-requirements.md).
- [System Architecture](02-architecture.md).
- [Scrum Backlog](03-scrum-backlog.md).
- [Validation and Decisions](04-validation-and-decisions.md).
- [Decision Amendments](05-decision-amendments.md).
- [BLE Protocol Draft](07-ble-protocol-draft.md).
- [Folder Index](README.md).
