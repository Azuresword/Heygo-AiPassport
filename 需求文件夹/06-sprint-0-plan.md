[简体中文](06-sprint-0-plan.zh_CN.md) · **English**

# Sprint 0: four-hour plan

Version: 0.6. Date: 2026-10-17. Status: Done. Commit `a917aaa` delivered all four tasks. Section 7 gives the results.

## 1. Sprint Goal

Build and host-test the device logic for one Coach question.

The board is not at hand. The phone application has no build that follows doc 07.
Sprint 0 therefore runs on the host only. A stub plays the phone.

## 2. Time box

Sprint 0 has four hours of development.
The team has no velocity data. The minutes below are planning limits, not measurements.

| Order | Task | Story | Time box | Exit check |
| --- | --- | --- | --- | --- |
| 1 | T-1 Mode and audio state machine | US-17, US-06, US-07 | 60 min | A generated table, and one host test for each transition. |
| 2 | T-2 Packet layer for the P0 messages | US-01, US-06, US-07 | 75 min | Host tests for build, parse, `xid`, and error codes. |
| 3 | T-3 IMA ADPCM encoder and decoder | US-06, US-07 | 60 min | Host tests for block size, round trip, block loss, and full scale. |
| 4 | T-4 Gate and build | None | 45 min | The static gate and the firmware build pass. The static RAM figure goes into the report. |
| | Total | | 240 min | |

**Cut line.** If T-1 and T-2 together take more than 150 minutes, drop T-3.
The raw PCM fallback in doc 07 keeps the chain testable without ADPCM.
T-4 always runs.

## 3. Task detail

### T-1: mode and audio state machine (60 min)

Use the existing flow generator in the firmware workspace. Replace the old voice graph.
The states cover Ski mode, Rest mode, and the audio owners None, AgentCapture, and AgentPlayback.

The host tests must cover these paths:

1. An OK hold in Ski mode enters Rest mode. That press never starts capture.
2. A new OK hold on the Coach page starts capture.
3. Release ends the question. The 20-second limit ends it too.
4. An UP hold cancels capture and cancels playback.
5. An OK hold during playback stops playback before capture starts.
6. A DOWN hold and a short OK return to Ski mode and stop all audio.
7. Link loss stops capture and playback.
8. Playback starts at three blocks, at PLAY_END, or 60 ms after the first block, whichever comes first.

### T-2: packet layer (75 min)

Write plain C with no dependency on NimBLE, ESP-IDF, or LVGL.
Cover the P0 messages in doc 07: HELLO, HELLO_ACK, PHONE_STATUS, DEVICE_STATUS, ERROR, CONTEXT, AGENT_START, AGENT_END, AGENT_CANCEL, AGENT_STATE, AUDIO_UP, AUDIO_DOWN, PLAY_START, and PLAY_END.

The host tests must prove these rules:

- Each message builds and parses back to the same fields.
- A wrong length gives error code 2. An unknown type gives error code 1.
- The parser ignores extra bytes at the end of a payload.
- HELLO carries major 0 and minor 1. A different minor from the phone gives error code 3.
- An unknown codec value gives error code 7.
- The device drops an audio block or a state packet with a stale `xid`.

### T-3: IMA ADPCM codec (60 min)

Write plain C. Use the block layout in doc 07, section 7.

The host tests must prove these rules:

- One block of 320 samples encodes to exactly 164 bytes.
- A short last block gets a silence fill and still encodes to 164 bytes.
- A decoded 1 kHz sine keeps a signal-to-noise ratio above 20 dB. This threshold is an initial value.
- The decoder reads the block after a lost block without error.
- Full-scale input causes no integer overflow.

### T-4: gate and build (45 min)

Add the three test sets to the repository gate.
Run the static gate and the firmware build.
Record the static RAM figure from the build map.

## 4. Definition of Done for Sprint 0

- The host tests pass.
- The static gate passes.
- The firmware build passes.
- The report states the static RAM figure.

Sprint 0 closes no story. Each P0 story still needs a device test and the real phone application.

```text
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: NOT RUN
Unverified: the real phone application, every device behavior, every field condition
```

## 5. Excluded from Sprint 0

| Work | Reason |
| --- | --- |
| The Coach page drawing in LVGL | Sprint 1. The drawing needs the board for a check. |
| The Chinese font subset | Sprint 1. The P0 vocabulary is short. |
| NimBLE integration | Sprint 1. It needs the board and the phone application. |
| Every device test | The board is not at hand. |
| Every P1, P2, and Deferred item | D-17. |

## 6. Risks

| Risk | Effect | Mitigation |
| --- | --- | --- |
| The phone team changes doc 07. | T-2 needs rework. | Keep the packet layer apart from T-1 and T-3. |
| Four hours prove too short. | T-3 slips. | Apply the cut line in section 2. |
| A host-only result reads as a product result. | A wrong status report. | Report in the format of section 4. |

## 7. Results

Sprint 0 ended on 2026-10-17. Commit `a917aaa` delivered the four tasks.
T-3 shipped as planned. The cut line did not apply.
The team did not record the actual time. So no record shows whether Sprint 0 stayed inside four hours.

| Task | Result | Evidence |
| --- | --- | --- |
| T-1 | Done | `Firmware/flows/coach.flow.json` generates `Firmware/main/generated/coach_*`. Host tests `test_coach_flow.c` and `test_coach_paths.c`. |
| T-2 | Done | `Firmware/main/protocol/ble_proto.*` and `Firmware/main/audio/play_gate.*`. Host tests `test_ble_proto.c` and `test_play_gate.c`. |
| T-3 | Done | `Firmware/main/audio/ima_adpcm.*`. Host test `test_ima_adpcm.c`. |
| T-4 | Done | `./tools/validate.sh` exit code 0. Application image 1,252,514 bytes. Static DRAM 105,610 bytes. |

The T-4 figures come from the message of commit `a917aaa`.
Section 2 of the [firmware change notes](../Firmware/docs/heygo-changes.md) lists the files and the tests.

Sprint 0 also delivered two items from section 5: the Coach screens in LVGL and the Chinese font subset.
Only the host screen check and the glyph check cover these two items. They still need a check on the board.
NimBLE integration came in Sprint 1, in commit `941f4e7`.

```text
Build: PASS (commit a917aaa)
Host tests: PASS (commit a917aaa)
Device tests: NOT RUN
Unverified: the real phone application, every device behavior, every field condition
```

Per section 4, Sprint 0 closes no story.

## 8. Related documents

- [Scrum backlog](03-scrum-backlog.md).
- [Decision amendments](05-decision-amendments.md).
- [BLE protocol draft](07-ble-protocol-draft.md).
- [Document index](README.md).
