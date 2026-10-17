[简体中文](07-ble-protocol-draft.zh_CN.md) · **English**

# BLE Protocol Draft

Version: 0.5. Date: 2026-10-17. Status: draft from the device team. The phone application team must review it.

## 1. Purpose and scope

This draft defines the BLE link between AI Passport and the phone application.
D-14 limits the device to four tasks. This protocol carries the data for those four tasks.
The phone application team owns the other side of the link.
Q-13 stays open until that team accepts or replaces this draft.

Version 0.2 marks the Team and Report messages as P1, because D-17 moved intercom and reports out of P0.
The first version builds only the P0 messages.

Document version 0.3 defines protocol version 0.1. It fixes every field value that the phone application needs.

Version 0.3 follows D-18. The first version requests the best-performance link settings and ignores their power cost.
It also uses one fixed advertising interval.

Version 0.4 adds six rules from the Sprint 1 firmware. It changes no packet format, so the protocol version stays 0.1.
Section 13 lists the six rules.

Version 0.5 adds a seventh rule from D-20: the phone starts an answer only after AGENT_END. The protocol version stays 0.1.
Section 14 gives this rule.

The draft follows three rules:

1. The device does no work beyond its own capability.
2. Each packet fits in one ATT packet. The device reassembles nothing except one report page.
3. The phone makes every decision about the gateway, the room, and retries.

## 2. Roles and connection

- The device acts as the GATT server and the BLE peripheral.
- The phone application acts as the GATT client and the BLE central.
- The device accepts one connection at a time. It stops advertising while a phone stays connected.
- D-12 applies. The link uses no encryption, no pairing, and no bonding. Any nearby BLE client can connect.

The device advertises as a connectable device.
The advertising data carries the service UUID. The scan response carries the device name.
The name has the form `HeygoPP-XXXX`. `XXXX` holds the last two bytes of the device address in hexadecimal.

The device uses its public address. The baseline configuration disables host-based privacy.
The address therefore stays the same after a restart. Check this behavior on hardware.
The phone application can therefore remember a device by its address.

The device advertises at a fixed interval of 100 ms. D-18 defers a slower interval that saves power.

## 3. GATT layout

Service UUID: `6e400001-b5a3-f393-e0a9-e50e24dcca9e`.

| Characteristic | UUID | Direction | Property |
| --- | --- | --- | --- |
| RX | `6e400002-b5a3-f393-e0a9-e50e24dcca9e` | Phone to device | Write Without Response |
| TX | `6e400003-b5a3-f393-e0a9-e50e24dcca9e` | Device to phone | Notify |

This layout follows the Nordic UART Service. Generic BLE tools can therefore read the link.
A reference on this board used the same UUIDs.

## 4. Link setup

1. The phone connects.
2. The phone requests an ATT MTU of 247. The device needs at least 172. The device does not start the MTU exchange.
3. The phone subscribes to TX notifications.
4. The device sends HELLO.
5. The phone answers with HELLO_ACK within 2 seconds.

The device disables voice features in these cases:

- The MTU stays below 172.
- No HELLO_ACK arrives within 2 seconds.
- The phone reports an unsupported major version.

The link stays up in these cases. P1 reports still work, because report pages use fragments.
The device shows a short message that names the cause.

### Link settings

The first version requests one best-performance setting. D-18 lets it ignore the power cost.

| Setting | Device request | Reason |
| --- | --- | --- |
| Connection interval | 7.5 ms minimum, 15 ms maximum | A short interval gives the lowest latency and the most packets per second. 7.5 ms is the BLE minimum. |
| Peripheral latency | 0 | The device answers in every connection event. |
| Supervision timeout | 4 seconds | Link loss shows within a few seconds. |
| PHY | LE 2M, with LE 1M as the fallback | LE 2M halves the air time of each packet. |
| Link-layer data length | 251 bytes | One 168-byte audio packet fits in one link-layer packet. The default length of 27 bytes splits it into 7 parts. |
| ATT MTU | 247 bytes | One packet carries a full audio block. |

The device requests these values once, after the link setup. It keeps them in both modes.
The phone has the final say. Each platform limits the interval, and some phones lack LE 2M.
For example, Apple's accessory guidelines set a 15 ms minimum interval for most accessories.
The device logs the values that the phone grants. The phone application team must check them on the target phones.

## 5. Packet format

Each write and each notification carries exactly one packet.
A packet never spans two ATT packets. Two packets never share one ATT packet.

| Offset | Size | Field | Meaning |
| --- | --- | --- | --- |
| 0 | 1 | `type` | Message type. Section 6 lists the values. |
| 1 | 1 | `flags` | Bit 0 marks the last fragment. The other bits stay zero. |
| 2 | 1 | `xid` | Exchange ID. Zero means no exchange. |
| 3 | 1 | `seq` | Sequence number inside the exchange. It wraps after 255. |
| 4 | 0 to MTU − 7 | `payload` | Content for the type. |

The ATT packet length gives the payload length. The packet carries no length field.
Multi-byte numbers use little-endian order. Text uses UTF-8.

**Why this format differs from the board reference.**
The reference used a magic word, a length field, and a stream parser.
It needed them, because one notification could carry several frames, and one frame could span several notifications.
This draft forbids both cases. The device therefore needs no stream parser.
The phone must obey the one-packet rule.

### Exchange IDs

- The device starts exchanges with `xid` values from 0x01 to 0x7F.
- The phone starts exchanges with `xid` values from 0x80 to 0xFE.
- Zero means no exchange. 0xFF stays reserved.
- After each new connection, the device uses 0x01 first, and the phone uses 0x80 first.
- Each side adds one to its own `xid` for each new exchange. After 0x7F the device wraps to 0x01. After 0xFE the phone wraps to 0x80.
- An Agent answer uses the `xid` of the Agent question.

The device keeps one current `xid` for each audio owner.
The device drops each audio packet and each state packet that carries another `xid`.
A cancellation clears the current `xid`. The device then drops every late packet for that exchange.

A new BLE connection resets every `xid`.
Packets from an old connection cannot arrive on a new connection. The link therefore needs no epoch field.

### Sequence numbers

- The first audio block of each stream has `seq` 0. Each next block adds one. After 255, `seq` wraps to 0.
- The first fragment of each report page has `seq` 0.
- Every other packet uses `seq` 0.

### Protocol version

Document versions 0.3 and 0.4 define protocol version 0.1.
HELLO and HELLO_ACK therefore carry major 0 and minor 1.
Earlier document versions had no implementation, so they carry no protocol number.

While the major version is 0, the protocol is a draft, and incompatible changes stay allowed.
Both sides must therefore use the same major and the same minor.
Any difference counts as an unsupported version. The device sends ERROR with code 3, and disables voice.

From major version 1, a new minor only adds fields at the end of a payload.
A receiver ignores extra bytes at the end of a payload.
A receiver answers an unknown type with ERROR code 1, and ignores the packet.

## 6. Message types

D means the device. P means the phone.

### Field values

| Field | Size | Values |
| --- | --- | --- |
| Mode | 1 byte | 0 Ski mode. 1 Rest mode. |
| Page | 1 byte | 0 Ski status page. 1 Coach page. 2 Exit question. 3 Reports page, P1. 4 Team page, P1. |
| Codec | 1 byte | 0 IMA ADPCM. 1 Raw PCM. |
| Codec mask | 1 byte | Bit 0 IMA ADPCM. Bit 1 raw PCM. Bit n matches codec value n. |
| Sample rate | 1 byte | 16, in kHz. No other value is valid. |
| Yes or no flag | 1 byte | 0 no. 1 yes. |
| Battery percent | 1 byte | 0 to 100. 0xFF unknown. |
| UTC time | 4 bytes | Unix time in seconds, unsigned. |
| Text | Rest of the payload | UTF-8, with no terminating zero. A version text uses at most 32 bytes. |

A receiver treats an unknown page value as an unknown page. It sends no ERROR for it.
A receiver answers an unknown codec value with ERROR code 7.

### Link

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x01 | HELLO | D to P | Major (1), minor (1), codec mask (1), MTU (2), firmware version (text) | After the phone subscribes to TX |
| 0x02 | HELLO_ACK | P to D | Major (1), minor (1), UTC time in seconds (4), application version (text) | Within 2 seconds of HELLO |
| 0x03 | PHONE_STATUS | P to D | UTC time (4), network online (1), gateway reachable (1), application in background (1) | On each change, and at least every 5 seconds |
| 0x04 | DEVICE_STATUS | D to P | Battery percent (1, 0xFF when unknown), mode (1), page (1), free heap in KiB (2) | On each change, and every 30 seconds |
| 0x0F | ERROR | Both | Error code (1), referenced type (1), referenced `xid` (1) | After a packet that the receiver rejects |

The codec mask uses bit 0 for IMA ADPCM and bit 1 for raw PCM.
The device needs the UTC time from the phone, because it has no other clock source.

| Error code | Meaning |
| --- | --- |
| 1 | Unknown type |
| 2 | Wrong length |
| 3 | Unsupported version |
| 4 | Content too large |
| 5 | Sequence gap |
| 6 | Type not valid in the current state |
| 7 | Unknown field value |

### Context

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x10 | CONTEXT | D to P | Mode (1), page (1) | On each change of mode or page |

In P0 the phone attaches the current ski session to each Agent request.
P1 adds report fields at the end of this payload.

### Agent

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x20 | AGENT_START | D to P | Codec (1), sample rate in kHz (1) | At the 500 ms OK hold on the Coach page |
| 0x21 | AGENT_END | D to P | Reason (1): 0 release, 1 the 20-second limit | When capture stops |
| 0x22 | AGENT_CANCEL | D to P | None | At an UP hold, a page change, or a mode change |
| 0x23 | AGENT_STATE | P to D | State (1), failure code (1), text (optional) | On each change of the exchange state |

AGENT_START uses a new device `xid`. AUDIO_UP packets follow at once. The device does not wait for an answer.
AGENT_CANCEL ends the whole exchange. The phone drops the input, and stops the answer.

| State value | Meaning |
| --- | --- |
| 1 | Accepted |
| 2 | Recognizing |
| 3 | Thinking |
| 4 | Answering |
| 5 | Done |
| 6 | Failed. The failure code gives the cause: 1 recognition, 2 timeout, 3 gateway. |

For every other state, the failure code is 0.

The device shows a timeout for failure code 2. For codes 1 and 3, it asks the user to ask again.
Before the answer starts, each AGENT_STATE that is not a failure starts the 10-second wait of the device again.

The optional text can hold a transcript or a short answer. It must fit in the packet.
With an MTU of 172, the text holds at most 163 bytes.

### Audio

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x30 | AUDIO_UP | D to P | One audio block | Every 20 ms while capture runs |
| 0x31 | AUDIO_DOWN | P to D | One audio block | At real-time pace during playback |
| 0x32 | PLAY_START | P to D | Channel (1): 0 Agent, 1 Team. Codec (1). Speaker name (text, Team only). | Before the first AUDIO_DOWN of an answer or a Team speech |
| 0x33 | PLAY_END | P to D | None | After the last AUDIO_DOWN |
| 0x34 | PLAY_STOP | D to P | None | At an UP hold during Team playback. P1. |

Each audio packet carries the `xid` of its exchange. `seq` increases by one for each block.
After PLAY_END, the device plays the frames in its queue, and then frees the audio owner.
At an UP hold during Agent playback, the device sends AGENT_CANCEL.
At an UP hold during Team playback, the device sends PLAY_STOP.
In both cases the device stops the speaker at once. It does not wait for the phone.

The first version offers only IMA ADPCM in HELLO. The device answers a PLAY_START with codec 1 with ERROR code 7.
The phone sends the PLAY_START of an exchange only after it receives AGENT_END. During capture, the device drops a PLAY_START.
The first version has no Team owner. The device ignores a PLAY_START for the Team channel.

### Team (P1)

The first version does not build these messages.

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x50 | TEAM_REQUEST | D to P | None | At the 500 ms OK hold on the Team page |
| 0x51 | TEAM_GRANT | P to D | Lease length in ms (2) | After the room service grants the floor |
| 0x52 | TEAM_DENY | P to D | Reason (1), current speaker (text) | When the phone cannot grant the floor |
| 0x53 | TEAM_RELEASE | D to P | Reason (1) | When capture stops, or after a late grant |
| 0x54 | TEAM_REVOKE | P to D | Reason (1): 1 lease expired, 2 room closed | When the lease ends early |
| 0x55 | ROOM_STATE | P to D | Room available (1), member count (1), room name length (1), room name (text), current speaker (text, empty for nobody) | On each change of the room |

TEAM_REQUEST uses a new device `xid`. The microphone stays closed.
On TEAM_GRANT, the device checks two conditions: OK is still held, and the `xid` is current.
If both conditions hold, the device plays the cue, starts the microphone, and sends AUDIO_UP packets.
If either condition fails, the device sends TEAM_RELEASE with reason 3, and keeps the microphone closed.

| TEAM_DENY reason | Meaning |
| --- | --- |
| 1 | Another member speaks. The text names that member. |
| 2 | No room |
| 3 | Timeout |
| 4 | Gateway error |

| TEAM_RELEASE reason | Meaning |
| --- | --- |
| 0 | The user released OK |
| 1 | The 20-second limit |
| 2 | Cancellation |
| 3 | Late grant |
| 4 | Link problem |

### Reports (P1)

The first version does not build these messages.

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x60 | REPORT_REQUEST | D to P | Page index (1), report ID (text, empty for the session list) | When the user opens a report or a page |
| 0x61 | REPORT_PAGE | P to D | Fragment of one JSON page | In answer to REPORT_REQUEST, or when the phone pushes a new report |
| 0x62 | REPORT_ACK | D to P | Result (1) | After the last fragment, or after a rejection |

Each REPORT_PAGE transfer uses a new phone `xid`. `seq` holds the fragment index, from zero.
The first fragment starts with the total length (2). JSON bytes follow.
The last fragment sets bit 0 of `flags`.

The device reassembles a page in one fixed buffer of 8,192 bytes.
The device rejects the transfer in these cases:

- The total length exceeds 8,192 bytes.
- A fragment index skips a value.
- The JSON does not parse.
- The report revision is older than the cached revision.

| REPORT_ACK result | Meaning |
| --- | --- |
| 0 | Accepted |
| 1 | Too large |
| 2 | Sequence gap |
| 3 | Parse error |
| 4 | Stale revision |

A new transfer `xid` cancels an incomplete transfer. The phone can retry a rejected transfer once.
The report page follows the JSON contract in architecture section 5.
The session list uses this JSON form: `{"sessions":[{"id":"...","date":"...","title":"..."}]}`.

## 7. Audio blocks

### IMA ADPCM block

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 2 | Predictor, signed 16-bit |
| 2 | 1 | Step index, from 0 to 88 |
| 3 | 1 | Reserved, zero |
| 4 | 160 | 320 samples, 4 bits each, low nibble first |

One block holds 20 ms at 16 kHz, mono. The block uses 164 bytes. With the header, one packet uses 168 bytes.
The predictor and the step index give the decoder state before the first sample. The block therefore codes all 320 samples.
Each block carries its own state. A lost block therefore does not corrupt the next block.

This layout differs from the IMA ADPCM block in WAV files. A WAV block stores the first sample in its header.
Do not reuse a WAV decoder without a change.
Use the standard IMA step table with 89 entries, and the standard index table.

### Raw PCM block

The raw PCM fallback uses 16 kHz, 16-bit little-endian, mono samples.
One block holds 5 ms, which is 80 samples or 160 bytes.
The fallback uses four times the packet rate of ADPCM. Use it for diagnostics only.

### Last block

Every block has a fixed size: 164 bytes for IMA ADPCM, and 160 bytes for raw PCM.
The last block of a stream usually holds fewer real samples.
The sender fills the rest of that block with samples of value 0, and then encodes the full block.
The receiver plays the fill as at most 20 ms of silence.

The device sends the last uplink block, and then AGENT_END or TEAM_RELEASE.
The phone sends the last downlink block, and then PLAY_END.
A receiver rejects a block of another size with ERROR code 2.

### Playback start

The device starts the speaker at the first of these events:

1. Three blocks wait in the playback queue. That gives a 60 ms jitter margin.
2. PLAY_END arrives. The device then plays the waiting blocks, even if fewer than three wait.
3. 60 ms pass after the first block without a third block. This rule covers a phone that stalls.

If PLAY_END arrives with no block, the device plays nothing, and frees the audio owner.

After the start, the device does not wait for three blocks again.
If the queue runs empty before PLAY_END, the device plays silence until the next block arrives.
If no block and no PLAY_END arrive for 2 seconds, the device ends playback and frees the audio owner.

### Pacing and queues

- The device sends one AUDIO_UP block every 20 ms while capture runs.
- The phone sends AUDIO_DOWN blocks at real-time pace. It never runs more than 8 blocks ahead of playback.
- The device starts the speaker by the rules in the section above.
- The device TX queue holds 8 audio packets. On overflow, the device drops the oldest audio packet.
- Control packets go before audio packets. A control packet never waits behind audio.
- One exception applies: AGENT_END waits behind the last AUDIO_UP block of its exchange. The section "Last block" requires this order.
- The device playback queue holds 8 packets. On overflow, the device drops the oldest packet.
- After a sequence gap, the device plays nothing for the missing block. The next block header resets the decoder.

## 8. Timing rules

| Rule | Value | Owner |
| --- | --- | --- |
| HELLO_ACK after HELLO | At most 2 seconds | Phone |
| Interval between PHONE_STATUS packets | At most 5 seconds | Phone |
| Silence before the device shows that the phone application does not respond | 15 seconds | Device |
| P1: TEAM_GRANT or TEAM_DENY after TEAM_REQUEST | At most 1,500 ms. After that, the device treats the request as a timeout. | Device |
| Before the answer starts: from AGENT_END to the first AGENT_STATE, and between two AGENT_STATE packets | At most 10 seconds. After that, the device shows a timeout and sends AGENT_CANCEL. | Device |
| Capture stop after OK release or cancellation | At most 100 ms | Device |
| Speaker stop after the cancellation command | At most 200 ms | Device |
| One capture | At most 20 seconds | Device |
| Speaker start | Three blocks, PLAY_END, or 60 ms after the first block | Device |
| Stall during playback | 2 seconds with no block and no PLAY_END ends playback | Device |

The 1,500 ms value comes from the local intercom reference.
TC-09 targets 800 ms for a grant at the 95th percentile. The timeout covers the slow tail.

## 9. Sequences

### Coach question

```mermaid
sequenceDiagram
    participant D as Device
    participant P as Phone
    D->>P: AGENT_START xid=05
    D->>P: AUDIO_UP xid=05 seq=0..n
    P-->>D: AGENT_STATE xid=05 recognizing
    D->>P: AGENT_END xid=05 release
    P-->>D: AGENT_STATE xid=05 answering
    P-->>D: PLAY_START xid=05 Agent
    P-->>D: AUDIO_DOWN xid=05 seq=0..m
    P-->>D: PLAY_END xid=05
```

### Team speech, and a late grant (P1)

```mermaid
sequenceDiagram
    participant D as Device
    participant P as Phone
    D->>P: TEAM_REQUEST xid=06
    P-->>D: TEAM_GRANT xid=06
    D->>D: Play cue, start microphone
    D->>P: AUDIO_UP xid=06 seq=0..n
    D->>P: TEAM_RELEASE xid=06 release
    D->>P: TEAM_REQUEST xid=07
    Note over D: The user releases OK
    P-->>D: TEAM_GRANT xid=07
    D->>P: TEAM_RELEASE xid=07 late grant
```

### Speech from another member (P1)

```mermaid
sequenceDiagram
    participant D as Device
    participant P as Phone
    P-->>D: ROOM_STATE speaker=Li
    P-->>D: PLAY_START xid=81 Team Li
    P-->>D: AUDIO_DOWN xid=81 seq=0..m
    P-->>D: PLAY_END xid=81
```

## 10. Device memory cost

| Item | P0 bytes | With P1 reports |
| --- | --- | --- |
| TX control queue, 4 packets of 172 bytes | 688 | 688 |
| TX audio queue, 8 packets of 172 bytes | 1,376 | 1,376 |
| Playback queue, 8 packets of 172 bytes | 1,376 | 1,376 |
| Report page buffer | None | 8,192 |
| Total | 3,440 | 11,632 |

The ADPCM codec state adds a few bytes for each direction.
The NimBLE host and controller need extra memory. Measure them on hardware.

## 11. What the phone application must do

1. Request an ATT MTU of 247, and accept no less than 172 for voice.
2. Subscribe to TX notifications, and answer HELLO within 2 seconds.
3. Send one packet in each write. Never join two packets. Never split one packet.
4. Send PHONE_STATUS at least every 5 seconds.
5. Hold the Agent input until the gateway accepts it. D-11 requires this.
6. Send AUDIO_DOWN at real-time pace, never more than 8 blocks ahead.
7. Decode and encode the ADPCM block layout in section 7.
8. Drop the input and the answer after AGENT_CANCEL.
9. P1: Request the floor after TEAM_REQUEST, and answer within 1,500 ms.
10. P1: Release the floor after TEAM_RELEASE.
11. P1: Split each report page into fragments that fit the MTU.
12. Send major 0 and minor 1 in HELLO_ACK.
13. Fill the last downlink block with silence to the full block size.
14. Before the answer starts, send an AGENT_STATE at least every 10 seconds.

## 12. Open items

- The phone application team accepts or replaces this draft. Q-13 tracks this item.
- Measure the MTU, the connection interval, and the throughput on the target phones.
- Check the background behavior of the phone application while a voice exchange runs.
- Fix the JSON report schema with real HeygoAI data. Q-01 tracks this item.
- Record the interval, the PHY, and the data length that each target phone grants.
- A later version measures the battery cost of the link settings. D-18 defers this item.

## 13. Changes in version 0.4

The Sprint 1 firmware follows these rules. None of them changes a packet format.

| No. | Rule | Section |
| --- | --- | --- |
| 1 | AGENT_END waits behind the last AUDIO_UP block of its exchange. Every other control packet goes before the audio. | 7 |
| 2 | HELLO offers IMA ADPCM only. A PLAY_START with codec 1 gets ERROR code 7. | 6 |
| 3 | Before the answer starts, each AGENT_STATE that is not a failure starts the 10-second wait again. | 6, 8 |
| 4 | The device does not start the MTU exchange. The phone requests the MTU. | 4 |
| 5 | After its own 10-second timeout, the device sends AGENT_CANCEL. | 8 |
| 6 | The device ignores a PLAY_START for the Team channel, because Team speech is P1. | 6 |

The device also maps failure code 2 to its timeout message, and codes 1 and 3 to its retry message.

## 14. Changes in version 0.5

| No. | Rule | Section |
| --- | --- | --- |
| 7 | The phone sends the PLAY_START of an exchange only after it receives AGENT_END. During capture, the device drops the PLAY_START and the AUDIO_DOWN blocks after it. The device then shows a timeout. | 6 |

This rule comes from D-20. It changes no packet format, so the protocol version stays 0.1.
The phone application team must accept this rule.

## 15. Related documents

- [System Architecture](02-architecture.md).
- [Validation and Decisions](04-validation-and-decisions.md).
- [Decision Amendments](05-decision-amendments.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [Folder Index](README.md).
