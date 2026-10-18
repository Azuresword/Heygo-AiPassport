<p class="lang"><strong>English</strong> · <a href="followups.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Follow-up</p>

# Limits and next steps

The delivery scope excludes power management, privacy functions, and team talk. These limits and integration gaps define the next phase.

## Design limits

<div class="cards">
<div class="card"><span class="tag">D-12</span><h4>No BLE encryption</h4><p>The link has no pairing or binding check. A nearby client can connect and receive audio blocks. Review this decision before a public trial.</p></div>
<div class="card"><span class="tag">D-18</span><h4>No power management</h4><p>The firmware has no power-saving strategy. Battery use needs assessment before a field pilot.</p></div>
<div class="card"><span class="tag">D-31</span><h4>Fall alert timeout</h4><p>The popup closes after a 10 s countdown. POPUP_END reason 0 reports no key press. The phone owns escalation.</p></div>
<div class="card"><span class="tag">D-32</span><h4>Phone detects run start</h4><p>A false run start can interrupt report reading. It cancels no audio. A short DOWN press reopens the Report page from live data.</p></div>
</div>

| Limit | Effect |
|---|---|
| No battery level on the screen | DEVICE_STATUS sends the battery level to the phone. |
| PLAY_START does not switch capture to playback | The answer may time out. D-20 requires the phone to wait for AGENT_END. |
| Body text has a four-line limit | Longer text ends with three dots. Audio supplies the full answer. |

## Integration and acceptance records

| Item | Required result |
|---|---|
| App confirmation | Resolve D-20 audio timing, D-31 popups, and D-32 run start with the phone app team. |
| Device acceptance | Record firmware and app versions, test conditions, measurements, and results for each acceptance test. |
| Delivery status | Reconcile the roadmap and acceptance tables with the recorded Sprint completion. |
| CI activation | Place the required workflows in the repository root under `.github/workflows/`. |

## Later features

| Priority | Scope |
|---|---|
| P1 | Metric charts (REQ-04), team talk (REQ-09), and cached reports (REQ-13). |
| P2 | Focused practice, quests and badges, reflection journal, session comparison, and sharing (REQ-10, 11, 12, 18, 19). |
| Deferred | Privacy and authorization (REQ-14), and resource limits (REQ-16). |

The protocol reserves messages 0x50–0x55 for team talk. The current firmware ignores playback for the Team channel.
