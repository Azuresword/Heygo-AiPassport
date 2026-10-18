<p class="lang"><strong>English</strong> · <a href="product.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Product</p>

# Product and pages

## Users and use

A skier uses the device after a run, on the lift or during a rest break. The phone stays in a pocket with Bluetooth on. The user keeps the gloves on and pushes only three physical keys: UP, OK, and DOWN.

Source: `需求文件夹/01-product-requirements.md`, lines 12 to 26.

## Four pages in one loop

The four pages are at the same level. A short press of DOWN opens the next page. A short press of UP opens the previous page. At the end, the loop starts again (D-29, D-30). A long press of OK on any page opens the Coach page.

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
<figcaption>Figure 1. The page loop. LVGL renders these screens on the host. Source: <code>assets/mockups/</code>.</figcaption>
</figure>

| Page | Content | Decision |
|---|---|---|
| Live data | Speed, drop, distance, turns, and time. The phone sends one frame each second. | D-23 |
| Run report | Heygo IQ (0 to 200), the main issue, and one tip. One report for each run. | D-26, D-27 |
| Turns | The line of the run, 8 turns on each page, with speed, angle, and radius. | D-30 |
| AI coach | The user holds OK, speaks, and releases OK. The device plays the answer. | D-17 |

## How to read the Turns page

On the Turns page, OK selects the next turn. A long press of UP selects the previous turn. A long press of DOWN opens the next page. The device keeps 3 groups of turns. It asks the phone for other groups with TURN_REQUEST.

<figure>
<div class="figure-panel">
<div class="screens big">
<div class="screen"><img src="../assets/sim/turns-demo.gif" alt="Turns page animation"><div class="cap">Turn by turn</div><div class="sub">turns-demo.gif</div></div>
</div>
</div>
<figcaption>Figure 2. Turns page animation. Source: <code>assets/sim/turns-demo.gif</code>.</figcaption>
</figure>

## Events that the phone starts

<div class="cards">
<div class="card"><span class="tag">D-31 · POPUP 0x66</span><h4>Popup</h4><p>A popup stops any state. The device plays the voice and then counts down 10 s. Any key closes the popup. The device then shows the previous page. A new popup replaces an old popup.</p></div>
<div class="card"><span class="tag">D-32 · RUN_START 0x68</span><h4>Run start</h4><p>The phone finds a run start. The device opens the live data page. During a talk or a popup, the device waits until it ends. A run start cancels nothing.</p></div>
<div class="card"><span class="tag">D-33</span><h4>Page rail</h4><p>A rail on the right side shows the position in the loop. A lime thumb shows the current page. All stops have one color. The rail does not mark the UP and DOWN targets, and it has no arrows. The thumb slides to the new stop in 180 ms.</p></div>
<div class="card"><span class="tag">D-34</span><h4>Link state on every page</h4><p>All five screens use one status bar.
It has a page dot, the page name, a link dot, and the link text. The link text is "connected", "not connected", or "no voice".</p></div>
</div>

<figure>
<div class="figure-panel">
<div class="screens">
<div class="screen"><img src="../assets/mockups/popup-anim.gif" alt="Popup animation"><div class="cap">Popup entry</div><div class="sub">popup-anim.gif</div></div>
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="Fall alert"><div class="cap">Fall alert</div><div class="sub">FALL ALERT</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="Achievement report"><div class="cap">Achievement</div><div class="sub">ACHIEVED</div></div>
</div>
</div>
<figcaption>Figure 3. Phone popups. Each popup kind has its own color and animated badge.</figcaption>
</figure>

Source: `需求文件夹/04-validation-and-decisions.md`, rows D-29 to D-34.
