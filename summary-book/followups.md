<p class="lang"><strong>English</strong> · <a href="followups.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Follow-up</p>

# Follow-up items

The team delivered the project. This page lists items outside the scope and items that the team did not do on purpose. Use it for the next phase.

## Accepted risks

Source: `需求文件夹/04-validation-and-decisions.md`, risk table.

<div class="cards">
<div class="card"><span class="tag">D-12</span><h4>No BLE encryption</h4><p>People near the user can hear the audio. The team did not use pairing, to keep the connection simple.</p></div>
<div class="card"><span class="tag">D-18</span><h4>No power management</h4><p>This delivery has no power-saving strategy.</p></div>
<div class="card"><span class="tag">D-31</span><h4>Fall popup closes after 10 s</h4><p>The phone controls the escalation. POPUP_END reason 0 tells the phone that nobody pushed a key.</p></div>
<div class="card"><span class="tag">D-32</span><h4>The phone finds the run start</h4><p>If the phone is not correct, the device changes the page but cancels nothing. On the live data page, a short press of DOWN opens the report page.</p></div>
</div>

## Design limits in the documents

Source: `Firmware/docs/heygo-changes.md`, known issues.

| Limit | Explanation |
|---|---|
| The screen does not show the battery level | The device sends the battery level to the phone in DEVICE_STATUS. |
| A PLAY_START during a recording is lost | D-20 tells the phone not to send it at that time. |
| The body text has a maximum of four lines | Longer text ends with three dots. The voice plays the full answer. |

## Possible next work

| Area | Basis |
|---|---|
| Team talk | The protocol defines 0x50 to 0x55. It is P1. |
| P1 and P2 requirements | REQ-04, 09, and 13 (P1). REQ-10, 11, 12, 18, and 19 (P2). |
| Document update | Make the status text of Sprints 5 to 16 and D-33 in documents `08` and `04` agree with "all complete". |
| CI | Move the workflows to `.github/workflows/` at the repository root. |
