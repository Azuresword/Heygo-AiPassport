<p class="lang"><strong>English</strong> · <a href="README.zh_CN.html">简体中文</a></p>

<div class="hero">
<p class="eyebrow">Heygo · AI Passport · Technical report</p>
<h1>Ski data and voice coach terminal</h1>
<p class="lead">AI Passport connects an ESP32-C3 device to a phone through BLE. It provides live ski data, run reports, turn browsing, and voice coaching. The device handles keys, display, recording, and playback. The phone handles data analysis, speech processing, and access to the AI service.</p>
<img src="../assets/sim/preview.png" alt="Simulator: device screen, phone controls, and state graph">
</div>

Report baseline: 2026-10-18, version `284ab00`, Sprints 0–17. The metrics below describe this stage.

<div class="tiles">
<div class="tile accent"><div class="k">Sprints recorded complete</div><div class="v">18<small>/18</small></div></div>
<div class="tile"><div class="k">Increments</div><div class="v">4<small>/4</small></div></div>
<div class="tile"><div class="k">Decision entries</div><div class="v">34</div></div>
<div class="tile"><div class="k">Requirements</div><div class="v">25</div></div>
<div class="tile"><div class="k">Screen examples</div><div class="v">25</div></div>
<div class="tile"><div class="k">Simulator self-test</div><div class="v">128<small>/128</small></div></div>
</div>

## Functional scope

<div class="cards">
<div class="card"><span class="tag">Increment 1</span><h4>Coach voice</h4><p>On the Coach page, the user holds OK to record and releases it to send. The phone transcribes speech, requests an AI answer, and generates speech. The device plays the answer. Recording lasts at most 20 seconds.</p></div>
<div class="card"><span class="tag">Increment 2</span><h4>Run data and report</h4><p>The device displays speed, drop, distance, turn count, and time. Each run has a Heygo IQ score (0–200) and one tip. When the phone detects a run start, it requests the live data page.</p></div>
<div class="card"><span class="tag">Increment 3</span><h4>Turn browsing</h4><p>Each page displays 8 turns with speed, angle, and radius. The device caches 3 groups and requests other groups from the phone as needed.</p></div>
<div class="card"><span class="tag">Increment 4</span><h4>Phone popups</h4><p>Popups include fall alerts, voice notices, issue reports, and achievement reports. A 10-second countdown starts after playback, or immediately without audio. Any key closes the popup.</p></div>
</div>

## Technical summary

| Area | Design and results | Chapter |
|---|---|---|
| Hardware and communication | ESP32-C3, 8 MiB flash, no PSRAM. BLE protocol 0.2 and 16 kHz IMA ADPCM. | [Architecture and protocol](architecture.md) |
| Firmware | 7 layers and 3 application tasks. A flow graph generates 10 states and 83 transitions. | [Firmware architecture](firmware.md) |
| Interaction | Four pages and three keys. Popups can interrupt a conversation. A run start waits for the current operation to end. | [Product and pages](product.md) |
| Memory | The app uses 23.4% of the factory partition. Static internal RAM uses 47.2%. The host LVGL pool peak is 76.6%. | [Memory map](memory.md) |
| Build and tests | The full gate passed, including 128/128 simulator checks. Device acceptance and host tests have separate results. | [Quality and deployment](quality.md) |
| Delivery | 4 increments and 18 sprints. Integration work and capability limits appear separately. | [Limits and next steps](followups.md) |

## Project links

- [Online simulator](https://azuresword.github.io/Heygo-AiPassport/)
- [Screen gallery](screens.md)
- [Project repository](https://github.com/Azuresword/Heygo-AiPassport)
