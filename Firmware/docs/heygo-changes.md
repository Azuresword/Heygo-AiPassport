[简体中文](heygo-changes.zh_CN.md) · **English**

# Heygo changes to the firmware baseline

This document lists what the Heygo team changed in the `Firmware` folder.
The other documents in that folder come from the baseline without changes.
All paths and commands in this document start from the `Firmware` folder.

## 1. Origin

| Item | Value |
| --- | --- |
| Source | [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport) |
| Commit | `3d685a9`, 2026-10-15 |
| License | MIT. The [LICENSE](../LICENSE) file keeps the FoloToy copyright. |
| Copy method | A plain copy of the files. It is not a Git submodule. |

The copy includes two fixes that let the gate run on macOS:

- `tools/install-actionlint.sh` computes the SHA-256 digest and compares it in the shell. The BSD `sha256sum` does not accept `--check`.
- `tools/validate.sh` uses `-dead_strip` on macOS and `--gc-sections` on Linux.

## 2. Sprint 0 tasks

The [Sprint 0 plan](../../%E9%9C%80%E6%B1%82%E6%96%87%E4%BB%B6%E5%A4%B9/06-sprint-0-plan.md) defines four tasks.

| Task | Files | Host test |
| --- | --- | --- |
| T-1 Mode and audio state machine | `flows/coach.flow.json`, `main/generated/coach_*` | `tests/test_coach_flow.c`, `tests/test_coach_paths.c` |
| T-2 Packet layer | `main/protocol/ble_proto.*`, `main/audio/play_gate.*` | `tests/test_ble_proto.c`, `tests/test_play_gate.c` |
| T-3 IMA ADPCM codec | `main/audio/ima_adpcm.*` | `tests/test_ima_adpcm.c` |
| T-4 Gate and build | `tools/validate.sh`, `main/CMakeLists.txt` | The full gate |

`tools/flow/flowc.py` generates every file in `main/generated/`.
Do not edit those files by hand.
Edit `flows/coach.flow.json` and run `python3 tools/flow/flowc.py flows/coach.flow.json`.

## 3. Build and check

1. Activate ESP-IDF 5.5.3: `. ~/esp/esp-idf-v5.5.3/export.sh`.
2. Run the full gate: `./tools/validate.sh`.
3. Make sure that the last line is `Firmware build: PASS`.

The gate runs these Heygo checks in addition to the baseline checks:

- The host tests for T-1, T-2, and T-3.
- The glyph check `../assets/tools/check_glyphs.py`.
- The screen check `tools/host_render/render.py`.
- The simulator check `tools/flow/flow_sim.py --check` and `--run-selftest`.

The screen check compiles LVGL for the host with the device settings from `sdkconfig`.
It draws each P0 screen state and fails on these conditions:

- A sample text has a glyph that the font does not have.
- A widget leaves the screen, or two widgets overlap.
- A setter does not take and give the lock in pairs.
- A second pass ends with more LVGL pool use than the first pass.
- A setter cuts a text inside a UTF-8 character.

To make the PNG files in `../assets/mockups`, run `python3 tools/host_render/render.py` with a Python that has Pillow.
The ESP-IDF Python has no Pillow. In the gate, the script therefore runs the checks only.

## 4. Flow simulator

`../assets/sim/coach-flow.html` is an interactive simulator of the Coach flow.
Open it in a browser. It needs no server and no network.

`tools/flow/flow_sim.py` makes the page from `flows/coach.flow.json`, `../assets/ui/strings.json`, and the theme colors.
After you change the flow graph, run `python3 tools/flow/flow_sim.py`.

The gate runs two checks:

- `--check` fails when the page does not match the flow graph.
- `--run-selftest` runs 35 tests in headless Chrome. Without Chrome, it reports SKIPPED.

## 5. Measured results

The firmware measurements are from 2026-10-17, after the Coach app replaced the demo menu.
The host UI measurements are from 2026-10-17, after the visual redesign. Device tests: NOT RUN.

| Item | Result | Method |
| --- | --- | --- |
| Full gate | Exit code 0 | `./tools/validate.sh` |
| Application image | 1,252,514 bytes of 8,323,072 bytes. It contains the Coach app and both fonts. | `idf.py size` |
| Static DRAM | 105,610 of 321,296 bytes, 32.9%. The baseline demo used 178,960 bytes. | `idf.py size` |
| LVGL pool before the Heygo screens | 9,352 bytes of 21,656 bytes | Host screen check |
| Coach screen | +6,184 bytes on first build. +5,856 bytes after rendering. | Host screen check |
| flowc estimate for all three screens | 12,930 bytes. Only one screen is active at a time. | `tools/flow/flowc.py` |
| LVGL pool peak | 17,128 of 21,656 bytes, 79% | Host screen check |

The static DRAM is lower than in the baseline because the app no longer links the Wi-Fi and BLE demos.
BLE comes back in Sprint 1, so this figure will increase again.

The host uses 8-byte pointers. The ESP32-C3 uses 4-byte pointers.
We expect the device pool use to be lower than the host figures. A device test must check this.

The flowc pool model now uses the host measurements: 360 bytes for each label, 330 bytes for each screen, and 120 bytes for each observer.
The model no longer counts the string subject buffers. Those buffers are static arrays, not pool memory.

## 6. Coach app on the device

`main/main.c` starts the Coach app. It no longer starts the BSP demo menu.
The `demo_*.c` files stay in the build as BSP references. The linker does not include them in the image.

| File | Task |
| --- | --- |
| `main/coach_app.c` | Owns the flow on one task. Shows the screen of each state. Runs the timers. |
| `main/coach_input.c` | Maps button events to flow events. Plain C with a host test. |
| `main/generated/coach_ui.c` | Sets the texts and icons that `ui_effects` in the flow graph gives. |

Key events and timer events reach the Coach task through one queue.
The task therefore handles one event at a time.

The key map:

| Button event | Flow event |
| --- | --- |
| LONG, at 500 ms | `UP_HOLD`, `DOWN_HOLD`, or `OK_HOLD` |
| RELEASE after LONG | `OK_RELEASE`. UP and DOWN give no event. |
| CLICK or DOUBLE | `UP_SHORT`, `DOWN_SHORT`, or `OK_SHORT`, once |

The task starts a 20-second timer in `CAPTURE` and a 10-second timer in `WAIT_ANSWER`.
These timers give `CAPTURE_LIMIT` and `AGENT_TIMEOUT`.

Only one screen exists at a time, because all three screens do not fit the LVGL pool.
At a screen change, the task loads a blank screen, deletes the old screen, and builds the new one.

The audio and link actions are not connected yet. The task writes each of them to the log.
The BLE link therefore stays down. The status line shows the "not connected" text, and each question ends with a timeout after 10 seconds.

## 7. Rules for integration

LVGL is not thread safe. Each generated setter takes the LVGL lock through hooks.
Give the BSP lock to the bindings before any task calls a setter:

```c
static bool coach_lock(void) { return bsp_lvgl_lock(100); }
static void coach_unlock(void) { bsp_lvgl_unlock(); }

coach_bindings_set_lock(coach_lock, coach_unlock);
```

The BSP lock is a recursive mutex in `esp_lvgl_port`.
Code that already holds the lock can therefore call a setter.
If the lock times out, the setter does nothing.

Each string setter cuts a text that is too long at a UTF-8 character boundary.
The body subject holds 239 bytes of text. At an MTU of 247, `AGENT_STATE` text can have 238 bytes.

## 8. Open items

- During capture or playback, the state machine ignores a DOWN hold. The requirements do not define this case.
- When the BLE link is down, an OK hold in READY still starts capture. The flow has no link check for this transition.
- The audio and link actions only write to the log. Sprint 1 connects them.
- The body label shows four lines. It ends a longer text with three dots. The device speaks the full answer.
