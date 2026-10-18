<p class="lang"><strong>English</strong> · <a href="architecture.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Architecture</p>

# Architecture and protocol

## Tasks

The device handles keys, audio capture, audio playback, and display. The phone handles speech recognition, speech synthesis, run data, and the AI coach. BLE carries control messages and audio. The application does not use Wi-Fi.

<figure>
<img src="images/architecture-en.svg" alt="System architecture">
<figcaption>Figure 6. Device and phone responsibilities.</figcaption>
</figure>

| Item | Value |
|---|---|
| Controller | ESP32-C3, 8 MiB flash, no PSRAM |
| Screen | ST7789P3, 240 × 320, SPI2 |
| Keys | UP, DOWN, and OK share GPIO0 through an ADC divider |
| Audio | ES8311, full-duplex I2S |
| Fuel gauge | CW2017 |
| Software | ESP-IDF 5.5.3, LVGL 9.5.0, NimBLE |
| Audio codec | IMA ADPCM, 16 kHz, 20 ms blocks. The protocol defines raw PCM, but this firmware supports ADPCM only. |
| BLE security | No encryption or pairing |

## Messages for one question

<figure>
<img src="images/coach-sequence-en.svg" alt="Sequence of one coach question">
<figcaption>Figure 7. One coach question. The left column displays the device state. Figure 4 in the <a href="screens.html">Screen gallery</a> displays the corresponding screens.</figcaption>
</figure>

The phone must receive `AGENT_END` before it sends `PLAY_START`. The `CAPTURE` state has no transition to playback. A premature `PLAY_START` does not interrupt recording.

## Messages in protocol 0.2

D denotes the device. P denotes the phone.

| Group | Message | Direction | Use |
|---|---|---|---|
| Link | HELLO 0x01 · HELLO_ACK 0x02 | D↔P | Handshake: versions, codec, and MTU |
| | PHONE_STATUS 0x03 · DEVICE_STATUS 0x04 | Both | Network, battery, page, and free heap |
| | ERROR 0x0F | Both | Reports a rejected packet |
| Context | CONTEXT 0x10 | D→P | Page change |
| Coach | AGENT_START 0x20 · AGENT_END 0x21 · AGENT_CANCEL 0x22 | D→P | Start, end, or cancel one question |
| | AGENT_STATE 0x23 | P→D | Exchange state, such as thinking or failed |
| Audio | AUDIO_UP 0x30 | D→P | Recording blocks |
| | AUDIO_DOWN 0x31 · PLAY_START 0x32 · PLAY_END 0x33 | P→D | Playback. Channel 0 is the coach. Channel 2 is a popup. |
| Run | LIVE_STATS 0x60 · RUN_REPORT 0x61 · RUN_START 0x68 | P→D | Live data, report, and run start |
| | SHOW_PAGE 0x62 | P→D | Requests the report page |
| Turns | TURN_SHAPE 0x63 · TURN_GROUP 0x64 | P→D | Turn summary and groups |
| | TURN_REQUEST 0x65 | D→P | Requests a missing group |
| Popup | POPUP 0x66 | P→D | Opens a popup |
| | POPUP_END 0x67 | D→P | Reports the closure reason |

The protocol also defines team talk through messages 0x50 to 0x55 and `PLAY_STOP` 0x34. Team talk remains P1 and is outside this delivery.
