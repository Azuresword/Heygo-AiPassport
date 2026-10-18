<p class="lang"><strong>English</strong> · <a href="screens.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Screens</p>

# Screen gallery

These 25 examples use the firmware LVGL screen code on the host. They cover normal, boundary, and error states. Screen color, refresh behavior, and display quality require device checks.

## Coach talk: the four states of one question

<figure>
<div class="figure-panel">
<div class="loop">
<div class="screen"><img src="../assets/mockups/02-ready.png" alt="Ready"><div class="cap">Ready</div><div class="sub">READY</div></div>
<div class="arrow"><b>▶</b>hold OK</div>
<div class="screen"><img src="../assets/mockups/03-listening.png" alt="Listening"><div class="cap">Listening</div><div class="sub">LISTENING</div></div>
<div class="arrow"><b>▶</b>release</div>
<div class="screen"><img src="../assets/mockups/04-thinking.png" alt="Thinking"><div class="cap">Thinking</div><div class="sub">THINKING</div></div>
<div class="arrow"><b>▶</b>answer</div>
<div class="screen"><img src="../assets/mockups/05-answering.png" alt="Answering"><div class="cap">Answering</div><div class="sub">ANSWERING</div></div>
</div>
</div>
<figcaption>Figure 4. One coach question. For the BLE messages, see Figure 7 in <a href="architecture.html">Architecture and protocol</a>.</figcaption>
</figure>

After a recognition error, the user can retry with a long press of OK. A lost phone connection must recover first.

<div class="screens">
<div class="screen"><img src="../assets/mockups/07-failed.png" alt="Not heard"><div class="cap">Not heard</div><div class="sub">FAILED</div></div>
<div class="screen"><img src="../assets/mockups/08-link-lost.png" alt="Phone not connected"><div class="cap">No phone</div><div class="sub">LINK LOST</div></div>
<div class="screen"><img src="../assets/mockups/06-answer-max.png" alt="Long answer"><div class="cap">Long answer</div><div class="sub">4 lines, full voice</div></div>
</div>

## Run data and report

<div class="screens">
<div class="screen"><img src="../assets/mockups/01-ski.png" alt="Waiting for data"><div class="cap">Waiting</div><div class="sub">No run yet</div></div>
<div class="screen"><img src="../assets/mockups/10-ski-live.png" alt="Run in progress"><div class="cap">Running</div><div class="sub">Run 4 · 38 km/h</div></div>
<div class="screen"><img src="../assets/mockups/11-ski-stale.png" alt="Old data"><div class="cap">Old data</div><div class="sub">No frame for 5 s</div></div>
<div class="screen"><img src="../assets/mockups/14-report.png" alt="Run report"><div class="cap">Report</div><div class="sub">Heygo IQ 82</div></div>
<div class="screen"><img src="../assets/mockups/15-report-max.png" alt="Report limits"><div class="cap">Limits</div><div class="sub">IQ 200 · 136 turns</div></div>
</div>

## Turns

<div class="screens">
<div class="screen"><img src="../assets/mockups/17-turns-drawing.png" alt="Drawing"><div class="cap">Drawing</div><div class="sub">TOE SIDE</div></div>
<div class="screen"><img src="../assets/mockups/18-turns-heel.png" alt="Heel side turn"><div class="cap">Heel side</div><div class="sub">HEEL SIDE</div></div>
<div class="screen"><img src="../assets/mockups/19-turns-schematic.png" alt="Schematic"><div class="cap">Schematic</div><div class="sub">255 turns</div></div>
<div class="screen"><img src="../assets/mockups/20-turns-loading.png" alt="Loading"><div class="cap">Loading</div><div class="sub">Waits for TURN_GROUP</div></div>
<div class="screen"><img src="../assets/mockups/21-turns-short.png" alt="Short run"><div class="cap">Short run</div><div class="sub">5 turns</div></div>
</div>

## Phone popups

<div class="screens">
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="Fall alert"><div class="cap">Fall alert</div><div class="sub">Red</div></div>
<div class="screen"><img src="../assets/mockups/23-popup-voice.png" alt="Voice notice"><div class="cap">Voice notice</div><div class="sub">Green</div></div>
<div class="screen"><img src="../assets/mockups/24-popup-issue.png" alt="Issue report"><div class="cap">Issue report</div><div class="sub">Orange</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="Achievement report"><div class="cap">Achievement</div><div class="sub">Yellow</div></div>
</div>

## All screens

<figure>
<div class="figure-panel"><img src="../assets/mockups/contact-sheet.png" alt="All 25 screens"></div>
<figcaption>Figure 5. Overview of 25 screen states.</figcaption>
</figure>
