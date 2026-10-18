<p class="lang"><strong>English</strong> · <a href="README.zh_CN.html">简体中文</a></p>

<div class="hero">
<p class="eyebrow">Heygo · AI Passport · Project summary</p>
<h1>Ask the coach during a rest</h1>
<p class="lead">AI Passport is a small handheld device for skiers. The user sees the data of the last run and hears the AI coach. The user does not take out the phone or remove the gloves. All 17 sprints of the project are complete.</p>
<img src="../assets/sim/preview.png" alt="Simulator: device screen, phone controls, and state graph">
</div>

<div class="tiles">
<div class="tile accent"><div class="k">Sprints</div><div class="v">17<small>/17</small></div></div>
<div class="tile"><div class="k">Increments</div><div class="v">4<small>/4</small></div></div>
<div class="tile"><div class="k">Decisions</div><div class="v">33</div></div>
<div class="tile"><div class="k">Requirements</div><div class="v">24</div></div>
<div class="tile"><div class="k">Screens</div><div class="v">25</div></div>
<div class="tile"><div class="k">Self-test</div><div class="v">122<small>/122</small></div></div>
</div>

## What the team delivered

<div class="cards">
<div class="card"><span class="tag">Increment 1</span><h4>Coach voice</h4><p>A long press of OK opens the Coach page. The user holds OK and asks a question. The phone does speech-to-text, the AI answer, and text-to-speech. The device plays the answer.</p></div>
<div class="card"><span class="tag">Increment 2</span><h4>Run data and report</h4><p>Live speed, drop, distance, turns, and time. After each run, a Heygo IQ (0 to 200) and one tip. The device opens the live data page at run start.</p></div>
<div class="card"><span class="tag">Increment 3</span><h4>Turns</h4><p>The line of each run, 8 turns on each page. The page shows the speed, angle, and radius of each turn.</p></div>
<div class="card"><span class="tag">Increment 4</span><h4>Phone popups</h4><p>Fall alert, voice notice, issue report, and achievement report. A popup opens at once and counts down 10 s. Any key closes it.</p></div>
</div>

## Quick answers

| Question | Answer | More |
|---|---|---|
| What does the device do? | It controls only the keys, microphone, speaker, and screen. The phone does all analysis and voice work. | [Architecture and protocol](architecture.md) |
| How is the firmware organized? | 7 layers, 3 own tasks, 10 states, and 83 transitions from a flow graph. | [Firmware architecture](firmware.md) |
| How is the firmware made? | Scripts make the flow code, fonts, and images. `validate.sh` builds and checks. You flash the merged image at 0x0. | [Firmware build](build-flash.md) |
| Is the memory sufficient? | The app uses 23.4% of the factory partition. Static DRAM is 45.9%. The LVGL pool peak is 86.6%. | [Memory map](memory.md) |
| How does the user operate it? | Four pages at the same level make a loop. A short press of UP or DOWN changes the page. A long press of OK opens the coach. | [Product and pages](product.md) |
| What does it look like? | 25 screens that LVGL renders on the host, and two animations. | [Screen gallery](screens.md) |
| What is the status? | Sprints 0 to 16 are complete. | [Sprints](sprints.md) |
| How long did it take? | 55 commits on main, from 10-16 18:02 to 10-18 10:48. | [Project history](history.md) |
| How does the team control quality? | A gate runs before each commit. Host tests, screen checks, and the simulator self-test must pass. | [Quality and deployment](quality.md) |

## Links

- Online simulator: <https://azuresword.github.io/Heygo-AiPassport/>. You can push the device keys in the browser.
- Repository: <https://github.com/Azuresword/Heygo-AiPassport>
- The book source is in `summary-book/`. Run `figures.py` to make the figures. Then run `build.sh` to make `summary/`.
