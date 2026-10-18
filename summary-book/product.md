<p class="lang"><strong>English</strong> · <a href="product.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Product</p>

# Product and pages

## Use case

A skier uses the device after a run, on the lift or during a rest break. The phone stays in a pocket with Bluetooth on. The user keeps the gloves on and pushes only three physical keys: UP, OK, and DOWN.

## Four pages in one loop

The four pages are at the same level. A short press of DOWN opens the next page. A short press of UP opens the previous page. The loop wraps at each end. A long press of OK opens the Coach page from Live data, Run report, or Turns. On the Coach page, another long press of OK starts recording. Releasing OK sends the question.

<figure>
<div class="figure-panel">
<div class="loop">
<div class="screen"><img src="../assets/mockups/10-ski-live.png" alt="Live data page"><div class="cap">Live data</div><div class="sub">LIVE RUN</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/14-report.png" alt="Run report page"><div class="cap">Run report</div><div class="sub">RUN REPORT</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/18-turns-heel.png" alt="Turns page"><div class="cap">Turns</div><div class="sub">TURN SHAPE</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/02-ready.png" alt="AI coach page"><div class="cap">AI coach</div><div class="sub">AI COACH</div></div>
</div>
<div class="loop-back">◀ DOWN on the Coach page opens the live data page · UP goes in the other direction</div>
</div>
<figcaption>Figure 1. Page loop and key directions.</figcaption>
</figure>

| Page | Content |
|---|---|
| Live data | Speed, drop, distance, turns, and time. The phone sends one frame each second. |
| Run report | Heygo IQ (0 to 200), the main issue, and one tip. One report for each run. |
| Turns | The line of the run, 8 turns on each page, with speed, angle, and radius. |
| AI coach | The user holds OK, speaks, and releases OK. The device plays the answer. |

## Turn browsing

A short press of OK selects the next turn. A long press of UP selects the previous turn. A long press of DOWN selects the next group. Each group contains at most 8 turns. The device caches 3 groups and requests missing groups through TURN_REQUEST.

<figure>
<div class="figure-panel">
<div class="screens big">
<div class="screen"><img src="../assets/sim/turns-demo.gif" alt="Turns page animation"><div class="cap">Turn by turn</div><div class="sub">8 turns per group · loaded as needed</div></div>
</div>
</div>
<figcaption>Figure 2. Turn selection and group changes.</figcaption>
</figure>

## Phone events

<div class="cards">
<div class="card"><span class="tag">POPUP 0x66</span><h4>Popup</h4><p>A popup opens immediately and cancels an active coach conversation. A 10-second countdown starts after playback, or immediately without audio. Any key closes the popup and restores the previous page. The Coach page returns to Ready. A new popup replaces the old popup.</p></div>
<div class="card"><span class="tag">RUN_START 0x68</span><h4>Run start</h4><p>When the phone detects a run start, it requests the live data page. The device waits for an active conversation or popup to end before it changes the page.</p></div>
</div>

## Navigation and link feedback

<div class="cards">
<div class="card"><span class="tag">Navigation</span><h4>Page rail</h4><p>The right rail has four positions in page order. A lime thumb marks the current page. Each page change animates for 180 ms. Popups hide the rail.</p></div>
<div class="card"><span class="tag">Connection</span><h4>Link state on every page</h4><p>All five screens use one status bar.
It has a page dot, the page name, a link dot, and the link text. The link text is "connected", "not connected", or "no voice".</p></div>
</div>

<figure>
<div class="figure-panel">
<div class="screens">
<div class="screen"><img src="../assets/mockups/popup-anim.gif" alt="Popup animation"><div class="cap">Popup entry</div><div class="sub">Countdown after playback</div></div>
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="Fall alert"><div class="cap">Fall alert</div><div class="sub">FALL ALERT</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="Achievement report"><div class="cap">Achievement</div><div class="sub">ACHIEVED</div></div>
</div>
</div>
<figcaption>Figure 3. Phone popups. Each popup kind has its own color and animated badge.</figcaption>
</figure>
