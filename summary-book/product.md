<p align="right"><strong>English</strong> · <a href="product.zh_CN.html">简体中文</a></p>

# Product and architecture

## Four pages

The four pages are at the same level and make a loop.
A short press of UP or DOWN changes the page.
A long press of OK on any page opens the Coach page (D-29).

| Page | Content | Decision |
|---|---|---|
| Live data | The phone sends one frame each second. Heygo IQ is 0 to 200. | D-23, D-27 |
| Report | One report at the end of each run. | D-26 |
| Turns | 8 turns on each page. The device keeps 3 groups and asks for others with TURN_REQUEST. | D-30 |
| AI coach | The user holds OK and speaks. The phone does speech-to-text, the AI answer, and text-to-speech. | D-17 |

The phone can also start two events:

- **Popup** (D-31): POPUP 0x66 and POPUP_END 0x67. A popup stops any state. The device plays the voice and then counts down 10 s. Any key closes the popup.
- **Run start** (D-32): RUN_START 0x68. The device opens the live data page. During a talk or a popup, the device opens the page after the talk or popup ends.

Sources: `README.md`. `需求文件夹/04-validation-and-decisions.md`, rows D-30 to D-32.

## Device and phone tasks

The device controls only the keys, microphone, speaker, and screen.
The phone does speech recognition, speech synthesis, run data, and the AI coach (D-14).

```
 +-------- Device --------+   BLE NUS    +--------- Phone ---------+
 | 3 keys / mic / speaker | <----------> | speech in / speech out  |
 | 240x320 screen         |  IMA ADPCM   | run data / AI coach     |
 +------------------------+  16kHz 20ms  +-------------------------+
```

- The link is BLE only. The device does not use Wi-Fi (D-08).
- Audio is IMA ADPCM. Raw PCM is the fallback (D-09).
- The link has no encryption and no pairing (D-12).
- The protocol version is 0.2. The phone app team confirmed it (D-16).

## Hardware and software

| Item | Value |
|---|---|
| Controller | ESP32-C3, 8 MB flash, no PSRAM |
| Screen | ST7789P3, 240 × 320, SPI2 |
| Keys | UP, DOWN, and OK on GPIO0 through an ADC divider |
| Audio | ES8311, full-duplex I2S |
| Fuel gauge | CW2017 |
| SDK | ESP-IDF 5.5.3, LVGL 9.5.0, NimBLE |

Source: `Firmware/docs/README.md`, hardware section.

## Repository layout

| Path | Content |
|---|---|
| `Firmware/` | ESP-IDF firmware. It is a copy of upstream FoloToy/ai-passport at `3d685a9`, with the MIT license (`Firmware/docs/heygo-changes.md`). |
| `Firmware/flows/coach.flow.json` | The state machine. The firmware and the simulator use the same file. |
| `Firmware/tests/` | 33 `test_*` host tests. |
| `Firmware/tools/` | `phone_sim` (phone stand-in), `host_render` (screen tests on the host), `flow/flow_sim.py` (makes the simulator). |
| `需求文件夹/` | Documents 01 to 08, in English and Chinese. |
| `assets/sim/coach-flow.html` | The single-file simulator. A script makes it. Do not edit it manually. |
| `summary/` | This book (generated). |
