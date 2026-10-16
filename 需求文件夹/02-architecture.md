[简体中文](02-architecture.zh_CN.md) · **English**

# HeygoAI × AI Passport: proposed architecture

Version: 0.2

Date: 2026-10-16

Status: Draft for engineering review

This document proposes a design. It does not describe an implemented integration.
Requirement IDs refer to [product requirements](01-product-requirements.md).
Delivery order appears in the [Scrum backlog](03-scrum-backlog.md).
Open decisions and checks appear in [validation and decisions](04-validation-and-decisions.md).
Accepted decisions appear in [decision amendments](05-decision-amendments.md).

## 1. Evidence and limits

| State | Evidence or proposal | Design consequence |
|---|---|---|
| Product source evidence | ESP32-C3, 8 MB Flash, no PSRAM | Limit memory and message sizes. |
| Product source evidence | 240 × 320 screen, RGB565 | Use buttons and compact pages. |
| BSP source evidence | UP, DOWN, and OK share an ADC input | Do not require simultaneous button combinations. |
| BSP source evidence | Default hold threshold: 500 ms | Use 500 ms as the initial interaction threshold. |
| BSP source evidence | ES8311 microphone and speaker APIs support mono PCM at 16 kHz and 16 bits | Test voice under actual snow conditions. |
| Product source evidence | Battery capacity: 500 mAh | Measure runtime. Do not derive runtime from capacity alone. |
| Conditional | A battery gauge may be absent or return `-1` | Show an unknown state instead of a false percentage. |
| Baseline behavior evidence | BLE advertising | The device link to the phone application needs further work. The device does not use Wi-Fi (D-08). |
| Local reference only | Uncommitted BLE intercom work exists | Review this work before reuse. Test this work before reuse. |
| Team statement | A phone application exists. Another team develops it. | Follow its BLE interface. Q-13 tracks that interface. |
| Measured build | A build without Wi-Fi left 180,840 bytes of static D/IRAM free. | Use this figure as the starting budget. Measure the run-time heap separately. |
| Unverified | HeygoAI APIs, report fields, permissions, and voice contracts | Use an adapter. Keep integration blocked until contracts exist. |
| Unverified | IMU, GNSS, barometer, motion sensing, or touch interface on the inspected board | Use verified sources and button controls. |
| Unverified | Water resistance, cold limits, wind audio, gloves, sunlight, radio range, and runtime | Keep these items in pilot acceptance tests. |

The passive NTAG213 tag has no verified MCU access API.
Do not use it as a live sensor or a device communication channel.
Concurrent microphone and speaker APIs do not prove acoustic echo cancellation.

Source baseline: `/Users/dylan/AI-passport`, commit `3d685a90bdb96dd8526d84edde1e63fec25b460e`, plus inspected working files.
Working files can differ from the commit.
The local intercom work does not prove a released feature, a phone application, or device test success.

Primary evidence paths:

- `docs/hardware-design/specifications.md`
- `components/bsp/include/bsp_pins.h`
- `components/bsp/include/bsp_display.h`
- `components/bsp/src/bsp_display_lvgl.c`
- `components/bsp/include/bsp_audio.h`
- `components/bsp/src/bsp_audio.c`
- `sdkconfig.defaults`
- `docs/development/engineering/passport-intercom.md` in the current local working copy

These paths belong to `/Users/dylan/AI-passport`. They are not files in this new repository.

## 2. System boundary

```mermaid
flowchart LR
    S[Phone or verified external sensors] --> H[HeygoAI services]
    P[AI Passport] <-->|BLE: buttons, audio frames, reports, status| U[Existing phone application]
    U <-->|Internet| G[Gateway and adapter]
    H <-->|Verified contracts only| G
    G <-->|Room audio and speaking permission| R[Team members]
```

D-08 selects a BLE phone bridge.
The device acts as a BLE peripheral. The existing phone application acts as the central.
The device does not join Wi-Fi. It needs no provisioning screen.

D-12 keeps the first-version link open.
The link uses no encryption, no passkey, and no binding check.
Any nearby BLE client can connect. The first version accepts that risk.
The device accepts one phone connection at a time. The baseline configuration already sets this limit.

The existing phone application belongs to another team. Its BLE interface remains unknown.
Q-13 tracks that interface.
The device protocol must follow that interface. This document therefore defines device behavior, not a wire format.

Team PTT means network voice between team members.
Team PTT requires these conditions:

- An available phone link.
- A reachable room service.
- Valid room membership.

It is not an offline radio walkie-talkie.

### Responsibility map

D-14 limits the device to four tasks:

1. Tell the phone about button events.
2. Stream microphone frames to the phone.
3. Play the audio frames that the phone streams.
4. Show the status, text, and reports that the phone supplies.

| Component | Owns | Does not claim |
|---|---|---|
| HeygoAI | Existing account, ski analysis, reports, and coaching capabilities after contract checks | Undocumented APIs or available raw sensor data |
| Phone application | Device connection, account, speech recognition, speech synthesis, gateway requests, retries, room selection, and speaking permission | Guaranteed background execution |
| Gateway and adapter | Contract conversion, device session routing, room service, and expiry | New ski measurements without a verified source |
| Passport application | The four device tasks, pages, button state, the bounded cache, and audio ownership | Local speech recognition, local language models, or verified motion sensing |
| Passport BSP | Screen, buttons, audio format, volume, buses, and other board operations | Product pages or coaching logic |
| External data source | Timestamped samples and declared measurement quality | Trust without source identity and validation |

Keep HeygoAI-specific fields inside the adapter.
Passport uses only normalized messages.
Test the adapter with synthetic fixtures before a real account connection.

## 3. Modes, buttons, and screen layout

### Explicit modes

The device starts in Ski mode.
Ski mode shows a small status page with no detailed chart or voice interaction.
The pilot permits report browsing and voice interactions only in Rest mode.
The device cannot check whether the user stopped moving.

D-15 proposes the Rest mode gestures. They stay proposals until the user accepts them.

- An OK hold in Ski mode enters Rest mode.
- Rest mode opens on the Reports page.
- A DOWN hold on the top Rest page opens a confirmation. A short OK then returns to Ski mode.

A hold enters Rest mode because a gloved hand can press a button by accident while skiing.
The Reports page ignores an OK hold. The OK hold that entered Rest mode therefore cannot start capture.

Rest mode offers Reports, Coach, and Team pages.
The Coach page sends voice to the AI Agent.
The Team page sends voice to people in the selected room.
Show the destination before microphone capture and throughout each exchange.
Use text and icons. Do not use color alone.

| Input | Proposed behavior |
|---|---|
| Hold OK for at least 500 ms in Ski mode | Enter Rest mode. This gesture is a D-15 proposal. |
| Short UP or DOWN | Move focus, change pages, or select a report. |
| Short OK | Select the focused item. |
| Hold OK for at least 500 ms on the Coach page | Start capture. Stream each frame to the phone. |
| Hold OK for at least 500 ms on the Team page | Send a speaking request to the phone. Start capture only after the phone returns a start message. |
| Hold OK on the Reports page | Do not capture audio. |
| Release OK after capture | Stop capture. Tell the phone that the input ended. |
| Continue a capture for 20 seconds | Stop capture. Show the limit. |
| Hold UP for at least 500 ms | Cancel the active audio owner. Do nothing when no audio owner is active. |
| Hold DOWN for at least 500 ms | Return to the preceding page. On the top Rest page, open the exit confirmation. |

This proposal uses these timings as initial acceptance values.
A long OK press must suppress the short OK event on release.
A long UP event must suppress the short UP event.
A long DOWN event must suppress the short DOWN event.
The application recognizes cancellation when the UP hold reaches 500 ms.

The 200 ms playback stop budget starts at that threshold.
The total target from UP press to playback stop is at most 700 ms.
Capture must stop within 100 ms after recognized cancellation.
For a regular OK release, the 100 ms capture stop budget starts at release.

Button callbacks emit events and return immediately.
A task handles the link, storage, and audio work.

### Proposed 240 × 320 report page

This page uses a new application design. It does not reuse the demo menu or its visual shell.
Coordinates use screen pixels, with origin at the upper-left corner.
The current screen implementation masks corners with a 30-pixel radius.
Keep essential text outside the masked corners.

| Region | Bounds | Content |
|---|---|---|
| Status | `x=32..207, y=0..35` | Rest label, connection state, and battery state |
| Report identity | `x=12..227, y=36..67` | Ski session date, report title, and cached status |
| Metric row | `x=12..227, y=68..103` | Two or three available metrics with units |
| Main visual | `x=12..227, y=104..247` | One chart or one turn view |
| Context | `x=12..227, y=248..283` | Selected turn, source quality, or missing-data notice |
| Button hints | `x=32..207, y=284..319` | UP/DOWN action and short OK action |

Use separate pages for the ski session summary, trend chart, and selected turn.
Do not place every metric on one page.
Use the same visual scale when users compare two turns.
If the scale changes, show the change.

The Coach page shows the Agent destination and capture, wait, playback, or failure state.
The Team page shows the room, current speaker, speaking permission, and microphone state.
An active microphone indicator remains visible during capture.
A disconnected Team page disables capture.

Check Chinese glyph coverage for every UI string.
Select the intended Chinese font on each widget.
UTF-8 input and a successful build do not prove intended rendering.
Test button use with gloves and text readability outdoors.

## 4. Data origin and visual meaning

Every report carries a source, schema version, report revision, capture period, and generation time.
Every metric carries a unit and quality state.
Use `null` for unknown values. Do not substitute zero.
Show unsupported fields as unavailable.

| Data | Allowed source | Required visual rule |
|---|---|---|
| Ski session summary | Verified HeygoAI report contract | Show report date and cached status. |
| Speed, distance, or altitude | A named phone or external sensor source | Show units and missing intervals. |
| Coaching text | HeygoAI response through the adapter | Identify advice as AI advice. |
| Turn direction or phase | A documented HeygoAI algorithm or verified source | Preserve the algorithm version and confidence meaning. |
| Turn trajectory | Verified position samples in a declared coordinate frame | Identify the view as measured or estimated. |
| Turn schematic | Synthetic points or a shape template | Show “Schematic”. Do not show meter units. |
| Demo content | Synthetic fixture | Show “Demo data” on every relevant page. |

The system must not manufacture a ski score, edge angle, turn radius, or posture measurement.
A new derived metric needs a documented formula, inputs, limits, and tests.
A confidence value is not a probability unless the provider defines it as a probability.

### Time, gaps, and charts

Use UTC timestamps for transport and storage.
The phone supplies the user's preferred timezone.
Preserve source timestamps separately from receive timestamps.
Record clock uncertainty when sources lack a shared clock.

Use labeled axes, units, and a visible time period.
Do not connect a chart line across an unknown interval.
Downsample charts with a method that preserves important extrema and gaps.
Keep exact values available for the selected sample.
Show cached reports with their generation time and elapsed age.
If the clock cannot establish age, show that age is unknown.

### Turn coordinates and direction

A measured turn uses a local slope plane only when its source defines that plane.
Its origin is the first valid sample of the selected turn.
The positive `y` axis follows the reference downhill direction.
The positive `x` axis points right when the skier faces that reference direction.
Distances use meters.

Define heading as `atan2(dx, dy)` in degrees.
Positive heading rotates clockwise from the reference downhill direction.
A positive unwrapped heading change indicates a right turn.
A negative unwrapped heading change indicates a left turn.
Use the skier's change of travel direction. Do not infer direction from the graph's left or right side.

The adapter preserves the provider's turn label only when its definition matches this contract.
Otherwise, the adapter marks direction as unknown or applies a reviewed conversion.
A diagram alone cannot prove ski carving, edge angle, or body posture.

A schematic uses dimensionless `u` and `v` coordinates from 0 to 1.
It must not imply measured distance, radius, or position.
A missing coordinate frame blocks a measured trajectory view.
The UI can still show a labeled schematic or available turn metrics.

Each turn includes its start, end, direction, source, and quality state.
Measured points contain `x_m`, `y_m`, and `t_ms`.
Schematic points contain `u`, `v`, and `t_ms`.
`t_ms` means elapsed milliseconds from the turn start.
Use `null` when source timing is unknown.
Known point times must increase within each segment.

Reject conflicting time order. Do not invent sample times.
Without source timing, show a static shape or labeled schematic steps.
Do not claim real-time replay when sample timing is absent.

Missing intervals break the path into segments.
Do not interpolate through a sensor outage.
Mark clipping, sparse samples, and uncertain direction.

## 5. Proposed normalized contracts

These names describe a proposal. They are not verified HeygoAI endpoints.
The adapter must map actual contracts after the discovery work.

### Message envelope

| Field | Meaning |
|---|---|
| `schema_version` | Major and minor contract version, initially `1.0` |
| `type` | Report, control, Agent event, audio frame, or room event |
| `message_id` | Unique message identifier for duplicate detection |
| `session_id` | Authorized device session |
| `epoch` | New connection generation after each reconnect |
| `exchange_id` | Agent exchange identifier when applicable |
| `room_id` | Authorized Team room when applicable |
| `lease_id` | Current speaking permission when applicable |
| `sequence` | Monotonic number within the current stream |
| `created_at` / `expires_at` | UTC validity interval |
| `payload` | Bounded content for the declared type |

The receiver authenticates the device session before it accepts content.
Do not trust a message's room or account fields without server authorization.
Reject expired content. Reject content from an old device session or epoch.
A sequence gap produces a visible quality state or an audio loss event.

D-13 defers authorization in the first version.
The first version still uses `session_id`, `epoch`, and `expires_at`, because these fields reject stale messages.
The first two rules above apply to a later version that adds authorization.

Add optional fields in a minor version.
Use a new major version when field meaning or required fields change.
Negotiate supported versions before the first report or audio frame.
Ignore unknown optional fields. Reject unsupported major versions.
Document enum expansion rules and required fields in the schema.

`schema_version` identifies the contract version.
`report_revision` identifies one report revision.
The provider or adapter assigns increasing revisions within each `report_id`.
The proposed `generated_at` field records the report revision's generation time.
Its mapping to HeygoAI data remains unverified.

Apply each report revision only once. Ignore an older revision.

Reject different content with the same report identifier and revision.
An identical duplicate must not reset the current page or audio context.

Agent context includes the selected `report_id`, `report_revision`, and optional `selected_turn_id`.
`selected_turn_id` identifies a ski turn. `exchange_id` identifies an Agent exchange.
Keep this context constant throughout one Agent exchange.
A newer report must not silently change the context of an active exchange.

### Synthetic report example

This example describes a schematic. It contains no measured ski trajectory.
All identifiers and values are synthetic.

```json
{
  "schema_version": "1.0",
  "type": "report.snapshot",
  "message_id": "demo-message-001",
  "session_id": "demo-device-session-001",
  "epoch": 1,
  "sequence": 1,
  "created_at": "2026-10-16T03:00:00Z",
  "expires_at": "2026-10-16T04:00:00Z",
  "payload": {
    "synthetic": true,
    "report_id": "demo-report-001",
    "report_revision": 1,
    "generated_at": "2026-10-16T03:00:00Z",
    "activity_id": "demo-ski-session-001",
    "source": {
      "provider": "synthetic_fixture",
      "algorithm_version": "fixture-1",
      "captured_from": "2026-10-16T02:50:00Z",
      "captured_to": "2026-10-16T02:50:04Z"
    },
    "metrics": [
      {"key": "speed_mean", "value": null, "unit": "m/s", "quality": "unavailable"}
    ],
    "turns": [
      {
        "turn_id": "demo-turn-001",
        "started_at": "2026-10-16T02:50:00Z",
        "ended_at": "2026-10-16T02:50:04Z",
        "direction": "right",
        "quality": "synthetic",
        "confidence": null,
        "path_kind": "schematic",
        "coordinate_frame": "normalized_uv",
        "unit": "1",
        "segments": [
          [{"u": 0.2, "v": 0.1, "t_ms": 0}, {"u": 0.4, "v": 0.5, "t_ms": 2000}, {"u": 0.8, "v": 0.9, "t_ms": 4000}]
        ]
      }
    ]
  }
}
```

`unit: "1"` means dimensionless. The UI shows no physical unit for this schematic.
A cached report stores its content and source time separately from transport expiry.
Transport expiry prevents replay. It does not hide an explicitly labeled historical report.
The cache still follows account access and retention rules.

### Candidate limits

| Item | Initial limit | Failure behavior |
|---|---|---|
| Control message | 1 KiB | Reject excess content with a typed error. |
| Report page | 8 KiB encoded | Request another page. Do not allocate an unbounded document. |
| Reports in cache | 3 summaries and one selected detail page, within 64 KiB total | Delete the oldest report from the cache. |
| Turn path | 128 points per selected turn | Request simplified segments with preserved gaps. |
| Concurrent Agent exchanges | 1 | Cancel or finish the previous exchange first. |
| Concurrent Team transmitters | 1 per room | The phone and the room service enforce this limit. |
| Audio capture | 20 seconds per hold | Stop capture. Release audio ownership. |

These limits need heap measurements and real report samples.
Do not split one oversized message into an unbounded reassembly buffer.

## 6. Agent and Team audio state

The application has one audio owner: None, AgentCapture, AgentPlayback, TeamCapture, or TeamPlayback.
An owner holds permission for one audio direction.
Before a page change or a mode change, the application cancels the current audio owner.
Capture and playback never run together in the pilot.

D-11 makes the device a streaming microphone and speaker for the phone.
The device holds a few frames at most. It never holds a whole question or a whole answer.
D-09 sets the frame format: IMA ADPCM at 16 kHz, mono, with 20 ms frames.

### Coach flow

```mermaid
sequenceDiagram
    participant U as User
    participant P as Passport
    participant A as Phone application
    participant G as Gateway and Agent
    U->>P: Select Rest and Coach
    U->>P: Hold OK for 500 ms
    P->>P: Take audio owner AgentCapture
    P->>A: Start input with report context
    loop While held, up to 20 seconds
        P->>A: Audio frame with sequence number
    end
    A->>G: Forward input after the gateway accepts
    U->>P: Release OK
    P->>A: End input
    G-->>A: Text and speech answer
    A-->>P: Answer frames and status
    P->>P: Take audio owner AgentPlayback
    P-->>U: Show status and play each frame
    opt User cancels or changes page
        P->>A: Cancel exchange
        P->>P: Stop capture and drop pending frames
    end
```

The microphone starts when the OK hold reaches 500 ms.
The device streams each frame to the phone at once.
The device does not wait for the gateway.
The phone holds the input until the gateway accepts it.
This rule keeps the first words of the question.

The phone runs speech recognition and speech synthesis.
If recognition fails, the phone reports the failure. The device shows a short retry message.
The device shows no transcript editor. Three buttons cannot edit text.

The user can cancel during capture or playback.
Cancellation ends the exchange. The device drops any frame that arrives later.
A late answer from an old exchange must not play.
A reconnect cancels the previous exchange.

### Team PTT flow

```mermaid
sequenceDiagram
    participant U as User
    participant P as Passport
    participant A as Phone application
    participant R as Room service
    U->>P: Select Rest and Team room
    U->>P: Hold OK for 500 ms
    P->>A: Speaking request
    Note over P: Microphone remains closed
    A->>R: Request speaking permission
    alt Permission arrives and OK is still held
        R-->>A: Lease with expiry
        A-->>P: Start message
        P->>P: Play cue and start microphone
        loop While held, up to 20 seconds
            P->>A: Audio frame with sequence number
            A->>R: Current speaker audio
        end
        U->>P: Release OK
        P->>A: End speech
        A->>R: Release lease
    else Busy, timeout, release, or link loss
        P->>P: Keep microphone closed
    end
```

The Team page does not start capture early.
Early capture makes the listener hear every word late.
A grant can take 800 ms at the 95th percentile. That delay alone exceeds the 500 ms audio target.
The user therefore waits for the cue before speaking.

The phone and the room service own the room, the speaking lease, the current speaker, and reconnection.
The proposed lease expires within 25 seconds of grant.
The device stops capture when any of these conditions occurs:

- Capture reaches 20 seconds.
- The user releases OK.
- Cancellation occurs.
- The phone reports that the lease ended.
- The BLE link fails.

If the user already released OK when the start message arrives, the device ignores the message.
The device tells the phone. The phone releases the lease.

Never queue Team voice for later delivery.
Drop late audio frames. Do not replay old speech.

Team playback requires all these conditions:

- Rest mode is active.
- The Team page is open.
- The audio belongs to the current room.
- Audio ownership is idle.

While another member speaks, the device plays the received frames and keeps its microphone closed.
Do not mix Team voice with Agent input or replies.
Stop capture before a room change.
A lost release event must not bypass the 20-second capture limit.

## 7. Device boundaries and resources

Use the existing BSP for hardware operations.
Keep pages, protocol state, and application tasks in the application layer.
Code outside the LVGL task must hold `bsp_lvgl_lock()` before it accesses LVGL objects.
Before deleting a page, stop every task, timer, callback, and handler that can access the page.

Change the audio codec, clock, or sample format through BSP APIs.
Do not change codec registers from button handlers or application pages.
Measure audio behavior before changing the existing hardware configuration.

### Initial memory budget

PCM at 16 kHz, mono, and 16 bits uses 32,000 bytes per second.
A 20-second clip uses 640,000 bytes before protocol overhead.
The device has no PSRAM. A complete clip does not belong in RAM.
Streaming makes a complete clip unnecessary.

A real build measured the static RAM. The build excluded Wi-Fi, as D-08 requires.
The result left 180,840 bytes of static D/IRAM free.
That figure excludes the heap that the BLE stack allocates at run time.

| Allocation | Initial design | Evidence or limit |
|---|---|---|
| LVGL draw buffer | One 40-line RGB565 buffer | Current source uses 19,200 bytes. |
| LVGL memory pool | 24 KiB | Current `sdkconfig.defaults` value. |
| Audio frames | At most 8 frames of 20 ms in each direction | Proposal. Eight PCM frames use 5,120 bytes. Eight ADPCM frames use about 1,300 bytes. |
| Control queue | 8 entries of at most 1 KiB | Proposal. Reject excess messages. |
| Report parsing | One page of at most 8 KiB | Proposal. Include parser overhead in measurement. |
| Turn points | At most 128 points | Proposal. Use constant bounds for segments and points. |
| BLE stack, task stacks, fonts, and firmware | Measure on hardware | Do not claim that the remaining memory is sufficient. |

Each frame queue needs a defined overflow policy.
Audio overflow drops obsolete frames or stops the exchange according to the channel rule.
Never grow a queue because the link is slow.

The source uses an SPI clock of 80 MHz.
This value does not establish screen frame rate or available link throughput.
An older guide mentions a 20-line buffer. Use the current implementation for this design.

### Audio frame format

D-09 selects IMA ADPCM at 16 kHz, mono.
Each frame carries 20 ms of audio.
The device encodes microphone frames. The device decodes the frames that it receives.

| Format | Bytes per second | Device cost |
|---|---|---|
| Raw PCM | 32,000 | No codec work. Four times the ADPCM link load. |
| IMA ADPCM | 8,000 | A few integer steps per sample. A few bytes of state. |
| Opus | About 3,000 | A new library. An Opus reference on this board used a 24 KB encoder task stack. |

One 20 ms ADPCM frame carries about 164 bytes of audio data.
The local BLE reference uses the same format. It requires an ATT MTU of at least 172.
The exact frame header must follow the phone application interface.

The protocol keeps a raw PCM frame type as a fallback.
Opus stays a later upgrade. The team considers it only after ADPCM works and the team measures its CPU and RAM cost.

Current audio configuration uses six DMA descriptors with 240 frames each.
This capacity does not establish total voice latency.

### Flash and stored data

Keep the default layout minimal: NVS, PHY data, and one factory application.
Check all images against the 8 MB Flash size.
Do not assume that two OTA application slots fit with fonts and firmware.
OTA needs a separate capacity decision and a recovery test.

A persistent report cache needs an explicit storage decision.
Do not add a cache partition as an implicit template requirement.
Limit the cache size.
REQ-13 blocks pilot acceptance until the storage design preserves one complete report across network loss and restart.

Commit a new cached snapshot atomically. Preserve the previous complete snapshot if an update fails.
The 64 KiB cache limit does not reserve space in the current partition table.
The selected layout may use a smaller cache if it still satisfies REQ-13.

## 8. Access and failure behavior

D-12 and D-13 exclude security and privacy work from the first version.
The BLE link has no encryption, passkey, or binding check.
The device builds no access control and no data deletion flow.
The phone application handles accounts.
The device stores no account credential and no gateway token.

These decisions carry known limits:

- Any nearby BLE client can connect to the device.
- Any connected client can read reports and audio frames.
- The device cannot delete cached reports on remote request.

A later version needs a separate decision before it adds pairing, encryption, or deletion.

| Failure | Device behavior | Phone or server behavior |
|---|---|---|
| Link loss | Stop capture. Show offline state and cached report time. | Expire requests and speaking leases. |
| Agent timeout | End the exchange. Offer a new user action. | Reject late or canceled output. |
| Recognition failure | Show a short retry message. | Report the failure to the device. |
| Team room busy | Keep the microphone inactive. Show the current speaker. | Keep one valid speaker. |
| Invalid report | Preserve the last valid view. Show unavailable fields. | Return a typed error. |
| Audio queue overflow | Drop obsolete frames or stop within the channel rule. | Report bounded loss without replay. |
| Low memory | Stop audio before allocation failure. Show a recoverable error. | Stop streaming to the failed exchange. |
| Unknown battery | Show an unknown battery state. | Do not derive a percentage. |
| Room change | Stop audio. End the old context. | Reject old room permissions. |

Offline mode supports only locally available report content and basic UI.
Offline mode does not imply offline AI inference, speech recognition, or Team PTT.

## 9. Review gates and requirement trace

| Area | Requirements | Evidence required before pilot acceptance |
|---|---|---|
| Reports, charts, and turns | REQ-03, REQ-04, REQ-05 | Real contract samples, source definitions, and synthetic UI review |
| Microphone and speech | REQ-06, REQ-07 | Streaming tests with the phone application, latency, noise, and resource measurements |
| Coach interaction | REQ-08 | Exchange cancellation and stale-response tests |
| Team PTT | REQ-09 | Phone and room service prototype, exclusive lease tests, and reconnect tests |
| Offline reports | REQ-13 | Cache design and power-loss tests |
| Buttons and fonts | REQ-15 | Hold and release tests, glyph checks, and glove tests |
| Device resources | REQ-16 | Peak heap, largest free block, stack, Flash, and runtime measurements |
| Explicit modes | REQ-17 | Rest-mode gating and visible recipient tests |
| Diagnostics | REQ-20 | Failure states and reconnect tests |

The architecture gates follow this order:

1. Select a transport. D-08 closed this gate.
2. Document the BLE interface of the phone application.
3. Approve HeygoAI contracts and report semantics.
4. Approve the device resource budget and safety limits.
5. Accept the complete Rest-mode pilot on real hardware.

These decisions remain open:

- The BLE interface of the phone application.
- Phone platform.
- Provider APIs.
- Gateway ownership.
- Speech services.
- Cache storage.
- Authorized sensor source.

Track decisions in [validation and decisions](04-validation-and-decisions.md).
Do not treat a diagram, a synthetic fixture, or a successful build as hardware validation.
