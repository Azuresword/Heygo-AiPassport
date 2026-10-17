[简体中文](07-ble-protocol-draft.zh_CN.md) · **English**

# BLE Protocol Draft

Version: 0.12. Date: 2026-10-17. Status: the phone application team accepted protocol 0.2. That team must accept SHOW_PAGE from version 0.7, the field meanings from version 0.8, and the three turn messages from version 0.9. Version 0.10 adds the device rules that the phone application team asked for. That team must also accept the popup messages from version 0.11 and RUN_START from version 0.12.

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

Version 0.6 adds the two Increment 2 ski data messages LIVE_STATS and RUN_REPORT, from D-23, D-24, and D-26. The protocol version becomes 0.2.
Section 15 lists these changes.
Version 0.8 removes Ski mode and Rest mode, per D-29. The mode and page fields keep their values but change their meaning. The protocol version stays 0.2.
Section 16 lists these changes.

Version 0.9 adds the Increment 3 messages TURN_SHAPE, TURN_GROUP, and TURN_REQUEST, and the Turns page, per D-30. The protocol version stays 0.2.
Section 17 lists these changes.

Version 0.11 adds the Increment 4 messages POPUP and POPUP_END, the popup audio channel, and the popup page, per D-31. The protocol version stays 0.2.
Section 19 lists these changes.

Version 0.12 adds RUN_START, per D-32. When the phone starts a run, the device opens the live data page. The protocol version stays 0.2.
Section 20 lists these changes.

The draft follows three rules:

1. The device does no work beyond its own capability.
2. Each packet fits in one ATT packet. The device reassembles nothing.
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

The link stays up in these cases. Below an MTU of 172, the ski data still works: LIVE_STATS needs an MTU of 19, and RUN_REPORT needs 110. With an unsupported version, the device also ignores both messages.
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
| 1 | 1 | `flags` | Bit 0 belongs to fragments. Protocol version 0.2 has no fragment message, so bit 0 is zero. The other bits stay zero. |
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
- Every other packet uses `seq` 0.

### Protocol version

Document versions 0.3 to 0.5 define protocol version 0.1. Document version 0.6 defines protocol version 0.2.
HELLO and HELLO_ACK therefore carry major 0 and minor 2.
Protocol version 0.2 is version 0.1 plus LIVE_STATS and RUN_REPORT.
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
| Mode | 1 byte | 0 live data page, Report page, Turns page, or popup page. 1 Coach page. Before D-29, the values meant Ski mode and Rest mode. |
| Page | 1 byte | 0 live data page. 1 Coach page. 2 no longer used (the old exit question). 3 Report page. 4 Team page, P1. 5 Turns page, Increment 3 (D-30). 6 popup page, Increment 4 (D-31). |
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
| 0x10 | CONTEXT | D to P | Mode (1), page (1) | On each page change |

In P0 the phone attaches the current ski session to each Agent request.
On the Report page, the page field of CONTEXT is 3. On the Turns page, it is 5. During a popup, the mode field is 0 and the page field is 6.

### Agent

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x20 | AGENT_START | D to P | Codec (1), sample rate in kHz (1) | At the 500 ms OK hold on the Coach page |
| 0x21 | AGENT_END | D to P | Reason (1): 0 release, 1 the 20-second limit | When capture stops |
| 0x22 | AGENT_CANCEL | D to P | None | At a page change, or when a new question cancels the open exchange |
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
| 0x32 | PLAY_START | P to D | Channel (1): 0 Agent, 1 Team, 2 popup (D-31). Codec (1). Speaker name (text, Team only). | Before the first AUDIO_DOWN of an answer, a Team speech, or a popup |
| 0x33 | PLAY_END | P to D | None | After the last AUDIO_DOWN |
| 0x34 | PLAY_STOP | D to P | None | At a page change during Team playback. P1. |

Each audio packet carries the `xid` of its exchange. `seq` increases by one for each block.
After PLAY_END, the device plays the frames in its queue, and then frees the audio owner.
At a page change during Agent playback, the device sends AGENT_CANCEL.
At a page change during Team playback, the device sends PLAY_STOP.
In both cases the device stops the speaker at once. It does not wait for the phone.

The first version offers only IMA ADPCM in HELLO. The device answers a PLAY_START with codec 1 with ERROR code 7.
The phone sends the PLAY_START of an exchange only after it receives AGENT_END. During capture, the device drops a PLAY_START.
The first version has no Team owner. The device ignores a PLAY_START for the Team channel.
For the popup channel, the "Popups" section gives the rules.

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

### Ski data (Increment 2)

D-23 lets the device team define these fields. The phone application team calculates and sends them.
Document version 0.6 replaces the JSON report messages of version 0.3 with LIVE_STATS and RUN_REPORT. Those messages were REPORT_REQUEST, REPORT_PAGE, and REPORT_ACK.
No side built those three messages.

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x60 | LIVE_STATS | P to D | The 12 bytes in the next table | Once each second from the start of a run, until the run ends |
| 0x61 | RUN_REPORT | P to D | 7 fixed bytes, then two texts | Once after each run |

RUN_START (0x68) marks the start of a run. The section "Run start (D-32)" gives it.

Both messages use `xid` 0 and `seq` 0. The device sends no acknowledgment.

**LIVE_STATS payload**

| Offset | Size | Field | Unit | Unknown value |
| --- | --- | --- | --- | --- |
| 0 | 1 | Run state: 0 unknown, 1 skiing, 2 stopped, 3 run ended | None | 0 |
| 1 | 1 | Run number today, from 1 | Runs | 0xFF |
| 2 | 2 | Current speed | 0.1 km/h | 0xFFFF |
| 4 | 2 | Run vertical drop | Meters | 0xFFFF |
| 6 | 2 | Run distance | Meters | 0xFFFF |
| 8 | 2 | Run turn count | Turns | 0xFFFF |
| 10 | 2 | Run time | Seconds | 0xFFFF |

If the phone has no speed, it sends the unknown value.

**RUN_REPORT payload**

| Offset | Size | Field | Unit | Unknown value |
| --- | --- | --- | --- | --- |
| 0 | 1 | Run number, as in LIVE_STATS | Runs | 0xFF |
| 1 | 2 | Distance | Meters | 0xFFFF |
| 3 | 2 | Turn count | Turns | 0xFFFF |
| 5 | 1 | Heygo IQ, 0 to 200 | Points | 0xFF |
| 6 | 1 | Byte count n of the core problem, at most 36 | Bytes | 0 for none |
| 7 | n | Core problem, UTF-8, at most 12 Chinese characters | None | Empty |
| 7 + n | The rest | Short tip, UTF-8, at most 20 Chinese characters or 60 bytes | None | Empty |

One RUN_REPORT holds at most 107 bytes. It needs an MTU of 110 or more.

The phone follows these rules:

- A run starts when the phone detects skiing. It ends when the phone detects the end of the run. The phone application team sets the detection rules.
- At the start of a run, the phone sends RUN_START first (D-32).
- During a run, the phone sends one LIVE_STATS each second.
- When a run ends, the phone sends one LIVE_STATS with run state 3. It then sends no LIVE_STATS until the next run starts.
- After a run ends, the phone sends the RUN_REPORT of that run. The report can arrive later than the end packet.
- When the phone cannot calculate a field, it sends the unknown value. The phone never sends 0 in place of an unknown value.

The device follows these rules:

- The device keeps only the last LIVE_STATS and the last RUN_REPORT. A new packet replaces the old packet completely.
- The device shows an unknown value as "--".
- The device treats an unknown run state value as 0. It sends no ERROR for it.
- If a LIVE_STATS payload is shorter than 12 bytes, the device sends ERROR with code 2 and keeps the old packet.
- For a RUN_REPORT, a text above its limit gets code 4. A length past the end gets code 2. An IQ above 200 or text that is not valid UTF-8 gets code 7. In all three cases, the device keeps the old report.
- The device uses its own receive time to decide that data is out of date. It does not use the phone time.
- After a link loss, the device keeps both kinds of data, and the screen still shows them.

### Page change (D-28)

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x62 | SHOW_PAGE | P to D | Page (1): only 3, the Report page | When the phone wants the device to show the Report page |

The message uses `xid` 0 and `seq` 0. The device sends no acknowledgment.

The device follows these rules:

- On the live data page (LIVE) or the idle Coach page (READY), SHOW_PAGE 3 changes the device to the Report page.
- During capture, the wait, or the playback, the device ignores SHOW_PAGE, so it never stops the user. It sends no ERROR for it.
- On the Report page, the device does nothing.
- For a page value other than 3, the device sends ERROR with code 7 and stays on its page.
- The device accepts SHOW_PAGE only after a HELLO_ACK with the same protocol version.

After the change, CONTEXT reports mode 0 and page 3.

### Turn shape (D-30)

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x63 | TURN_SHAPE | P to D | 3 bytes | Once after each RUN_REPORT |
| 0x64 | TURN_GROUP | P to D | 4 fixed bytes, then the turns, then the points | Once for each group that the device asks for |
| 0x65 | TURN_REQUEST | D to P | 3 bytes | When the device lacks a group that it keeps |

The three messages use `xid` 0 and `seq` 0. No message has an acknowledgment.
A group is 8 turns: group g holds turns 8g to 8g + 7 of the run. The last group can hold fewer turns.
The device keeps at most 3 groups: the group on the screen and the groups next to it.
The device asks for each group that it lacks. The phone keeps every group of the last run.

**TURN_SHAPE payload**

| Offset | Size | Field | Unknown value |
| --- | --- | --- | --- |
| 0 | 1 | Run number, as in RUN_REPORT | 0xFF |
| 1 | 1 | Source: 0 measured track, 1 schematic shape | None |
| 2 | 1 | Turn count n of the whole run, 0 to 255 | None |

**TURN_REQUEST payload**

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | Run number of the last TURN_SHAPE |
| 1 | 1 | First group |
| 2 | 1 | Group count, 1 to 3 |

**TURN_GROUP payload**

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | Run number of the last TURN_SHAPE |
| 1 | 1 | Group g |
| 2 | 1 | Turn count k of this group, 1 to 8. It is 8, except in the last group |
| 3 | 1 | Point count m of this group, 2 to 33 |
| 4 | 6k | Turns. The next table gives one turn |
| 4 + 6k | 2m | Points. Each point is x (1 byte), then y (1 byte), from 0 to 255 |

**One turn**

| Offset | Size | Field | Unit | Unknown value |
| --- | --- | --- | --- | --- |
| 0 | 1 | Index of the first point of the turn in this group | None | None |
| 1 | 1 | Index of the last point. It is more than the first index and less than m | None | None |
| 2 | 1 | Side: 0 toe side, 1 heel side | None | 0xFF |
| 3 | 1 | Speed in the turn | km/h | 0xFF |
| 4 | 1 | Edge angle, 0 to 90 | Degrees | 0xFF |
| 5 | 1 | Turn radius | Meters | 0xFF |

The y axis goes down the slope. The x axis goes across the slope.
The points of each group use their own scale. The device maps 0 to 255 on each axis to the full box on the screen.
The phone therefore sets the scale of each group. Each group fills the box.

The drawing area is 200 x 76 pixels: the box of 220 x 96 pixels with a margin of 10 pixels.
The device places a point at these box pixels, with integer division:

- Screen x = 10 + (y x 200 + 127) / 255. Down the slope goes to the right.
- Screen y = 10 + (x x 76 + 127) / 255. Across the slope goes down.

The phone can use one scale k, in pixels for each meter, on both axes. Then it sends these values:

- y = d x k x 255 / 200
- x = s x k x 255 / 76

Here d is the distance down the slope and s is the distance across the slope, in meters from the corner of the group.

TURN_GROUP holds at most 122 bytes with its header. It fits in the MTU of 172 that voice needs.

The phone follows these rules:

- After each RUN_REPORT, the phone sends the TURN_SHAPE of the same run.
- For each TURN_REQUEST, the phone sends one TURN_GROUP for each asked group, in order.
- The phone answers within 300 ms. This limit is a proposal. The phone application team must accept it.
- The phone uses at most 4 segments for each turn, so that a group has at most 33 points.
- If the shape does not come from a measured track, the phone sets source 1. The device then shows "示意" on the page.
- The phone sends the unknown value for a turn value that it cannot calculate. It never sends 0 in place of an unknown value.

The device follows these rules:

- A new TURN_SHAPE replaces the old one and clears the cache. The device then asks for groups 0 to 2 at once.
- When the screen shows group g, the device keeps groups g − 1 to g + 1.
- At the start and at the end of the run, the device keeps the first 3 or the last 3 groups.
- The device drops each other group, and asks for the missing ones in one TURN_REQUEST.
- If the groups do not arrive within 1 second, the device asks once again.
- After a reconnect and a HELLO_ACK, the device asks again for the groups that it lacks.
- The device drops a TURN_GROUP for a group that it does not keep. It sends no ERROR for it.
- The device draws a group by itself, one point each 40 ms. Thus a group of 33 points takes 1.32 seconds.
- For TURN_SHAPE, a source above 1 gets ERROR code 7.
- For TURN_GROUP, a payload length that is not 4 + 6k + 2m gets code 2. A k above 8 or an m above 33 gets code 4.
- For TURN_GROUP, k equal to 0, m below 2, and turn indexes that do not follow the rule get code 7.
- A side above 1 and an edge angle above 90 also get code 7. The unknown value 0xFF is valid.
- A TURN_GROUP before any TURN_SHAPE, or with another run number, gets code 6.
- A group past the end of the run, or a turn count that does not match the run, gets code 7.
- After each ERROR, the device keeps the old shape and the old groups.
- The device accepts these messages only after a HELLO_ACK with the same protocol version.
- A RUN_REPORT of a new run clears the turn cache. The Turns page then shows the run of that RUN_REPORT, and "加载中" (loading) until its TURN_SHAPE arrives. The device sends no TURN_REQUEST for this run before its TURN_SHAPE.
- A RUN_REPORT of the run of the turn cache keeps the cache.
- A TURN_SHAPE with 0 turns shows "暂无弯型数据" (no turn data) for that run. The title shows the run and no turn count.
- If no TURN_SHAPE arrives within 90 seconds of the RUN_REPORT, the page also shows "暂无弯型数据". A later TURN_SHAPE of that run still arrives.
- While the page waits and the link is down, it shows "蓝牙断开" (Bluetooth is off).
- With no turn to show, the OK key and the two holds have no action on the Turns page.

### Popups (D-31)

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x66 | POPUP | P to D | 3 fixed bytes, then the title, then the body | When the phone wants the device to show a popup at once |
| 0x67 | POPUP_END | D to P | Reason (1) | When the popup closes on the device |

POPUP uses a new phone `xid`, from 0x80 to 0xFE, and `seq` 0. POPUP_END uses the `xid` of its POPUP.
The popup audio uses the same `xid`.

**POPUP payload**

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | Kind: 1 fall alert, 2 voice message, 3 problem report, 4 achievement |
| 1 | 1 | Audio flag: 0 no audio, 1 a PLAY_START on channel 2 follows |
| 2 | 1 | Byte count t of the title, at most 36 |
| 3 | t | Title, UTF-8, at most 12 Chinese characters. If t is 0, the device shows a default title for the kind |
| 3 + t | The rest | Body, UTF-8, at most 120 bytes or 40 Chinese characters |

One POPUP holds at most 163 bytes. It needs an MTU of 166 or more.

| POPUP_END reason | Meaning |
| --- | --- |
| 0 | Timeout. The countdown ended with no key press. |
| 1 | Key. The user pressed a key. |
| 2 | Replaced. A new POPUP replaced this popup. |

The phone follows these rules:

- The phone can send POPUP at any time after HELLO_ACK, on every page and in every state.
- For a popup with audio, the phone sends PLAY_START with channel 2 and the popup `xid`. Then it sends AUDIO_DOWN and PLAY_END with the same `xid`.
- The phone sends PLAY_START within 5 seconds of the POPUP. After that, the device starts the countdown and drops the audio.
- After POPUP_END, the phone stops the audio of that popup.
- The phone owns the escalation of a fall alert. The device closes a fall popup after 10 seconds, as it closes the other kinds.

The device follows these rules:

- A POPUP interrupts every state at once. During capture, the device stops capture and sends AGENT_CANCEL. During the wait, it sends AGENT_CANCEL. During playback, it stops the speaker and sends AGENT_CANCEL.
- If a popup shows already, the device sends POPUP_END with reason 2 for it. Then it shows the new popup. The link core sends this packet.
- The device accepts PLAY_START on channel 2 only with the `xid` of the current popup, and only if its audio flag is 1. It drops the other packets and sends no ERROR for them.
- The device plays the popup audio. After PLAY_END and the last block, it counts down 10 seconds.
- With audio flag 0, the countdown starts at once. With audio flag 1 and no PLAY_START within 5 seconds, the countdown starts.
- After the countdown, the device sends POPUP_END with reason 0. At a key press, it sends POPUP_END with reason 1 and stops the popup audio.
- Then the device shows the page before the popup again. From the Coach page, it shows the ready state.
- After a link loss, the device keeps the popup and starts the countdown. It sends no POPUP_END. A new connection resets every `xid`, so the phone treats the popup as closed.
- A kind outside 1 to 4, an audio flag above 1, or text that is not valid UTF-8 gets ERROR code 7.
- A POPUP with a device `xid` (0x00 to 0x7F) or with `xid` 0xFF also gets code 7.
- A title above 36 bytes or a body above 120 bytes gets code 4. A title length past the end of the payload, or a payload below 3 bytes, gets code 2.
- After each ERROR, the device shows nothing new. A popup that shows already stays.
- The device accepts POPUP only after a HELLO_ACK with the same protocol version. Before that, it ignores the packet and sends no ERROR.

### Run start (D-32)

| Type | Name | Direction | Payload | When |
| --- | --- | --- | --- | --- |
| 0x68 | RUN_START | P to D | Run number (1) | When the phone detects the start of a run, before the first LIVE_STATS of that run |

The message uses `xid` 0 and `seq` 0. The device sends no acknowledgment.

**RUN_START payload**

| Offset | Size | Field |
| --- | --- | --- |
| 0 | 1 | Run number today, from 1 to 254, as in LIVE_STATS. 0 and 0xFF are not valid. |

The phone follows these rules:

- At the start of each run, the phone sends one RUN_START. Then it sends LIVE_STATS once each second, as before.
- The run number of RUN_START is the run number of the LIVE_STATS packets that follow.
- The phone can send RUN_START on every page and in every state. The device decides when the page changes.

The device follows these rules:

- The device puts the run in its live data at once. It sets run state 1 (skiing), the run number, and the unknown value in each other field. The live data page shows "第 N 趟 · 滑行中" and "--".
- If the live data holds the same run number already, and that run did not end, the device keeps the data.
- On the Report page, the Turns page, or the idle Coach page, the device opens the live data page at once. On the live data page, only the data changes.
- During capture, the wait, or the playback, the device keeps the run start and cancels nothing. When the talk with the coach ends, the device opens the live data page.
- During a popup, the popup stays. When it closes, the device opens the live data page, not the page before the popup.
- After the page change, CONTEXT reports mode 0 and page 0.
- Run number 0 or 0xFF gets ERROR code 7. An empty payload gets code 2. After each ERROR, the page and the data do not change.
- The device accepts RUN_START only after a HELLO_ACK with the same protocol version. Before that, it ignores the packet and sends no ERROR.

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
| Interval between LIVE_STATS packets during a run | 1 second | Phone |
| Silence before the device marks data out of date during a run | 5 seconds with no LIVE_STATS | Device |
| Time before the report shows its receive time | 60 seconds after RUN_REPORT arrives | Device |
| Screen update after a LIVE_STATS or RUN_REPORT packet arrives | At most 300 ms at the 95th percentile | Device |
| Change to the live data page after RUN_START arrives, with no talk and no popup | At most 300 ms at the 95th percentile | Device |
| P1: TEAM_GRANT or TEAM_DENY after TEAM_REQUEST | At most 1,500 ms. After that, the device treats the request as a timeout. | Device |
| Before the answer starts: from AGENT_END to the first AGENT_STATE, and between two AGENT_STATE packets | At most 10 seconds. After that, the device shows a timeout and sends AGENT_CANCEL. | Device |
| Capture stop after OK release or cancellation | At most 100 ms | Device |
| Speaker stop after the cancellation command | At most 200 ms | Device |
| One capture | At most 20 seconds | Device |
| Speaker start | Three blocks, PLAY_END, or 60 ms after the first block | Device |
| Stall during playback | 2 seconds with no block and no PLAY_END ends playback | Device |
| PLAY_START on channel 2 after a POPUP with audio flag 1 | At most 5 seconds. After that, the device starts the countdown. | Phone |
| Popup countdown | 10 seconds after the popup audio ends, or 10 seconds after a POPUP with no audio | Device |

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

### Popup with audio, during a question (D-31)

```mermaid
sequenceDiagram
    participant D as Device
    participant P as Phone
    D->>P: AGENT_START xid=05
    D->>P: AUDIO_UP xid=05 seq=0..n
    P-->>D: POPUP xid=81 kind=1 audio=1
    D->>P: AGENT_CANCEL xid=05
    D->>P: CONTEXT mode=0 page=6
    P-->>D: PLAY_START xid=81 channel=2
    P-->>D: AUDIO_DOWN xid=81 seq=0..m
    P-->>D: PLAY_END xid=81
    Note over D: Play, then count down 10 s
    P-->>D: POPUP xid=82 kind=4 audio=0
    D->>P: POPUP_END xid=81 replaced
    Note over D: Count down 10 s at once
    D->>P: POPUP_END xid=82 timeout
    D->>P: CONTEXT mode=1 page=1
```

In this example, the popup comes on the Coach page. So the device goes back to the ready state of the Coach page.

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

| Item | P0 bytes | With Increment 2 |
| --- | --- | --- |
| TX control queue, 4 packets of 172 bytes | 688 | 688 |
| TX audio queue, 8 packets of 172 bytes | 1,376 | 1,376 |
| Playback queue, 8 packets of 172 bytes | 1,376 | 1,376 |
| The last LIVE_STATS and the last RUN_REPORT, with their receive times | None | 128 |
| Total | 3,440 | 3,568 |

Increment 3 adds about 4.0 KB of static RAM. The cache of 3 groups uses about 365 bytes, and a copy for the screen uses 359 bytes.
The Turns page view uses two copies of 816 bytes. The line points and their lengths use 1,584 bytes.
The rider sprite uses 25.6 KB of Flash and no RAM.
The symbol table of the firmware build gives these sizes.

Increment 4 adds about 0.7 KB of static RAM: three copies of the current popup of 161 bytes each, and the popup page texts. This value is an estimate from the struct sizes. Nobody measured it.

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
11. Increment 2: during a run, send one LIVE_STATS each second. When the run ends, send the end packet, and then the RUN_REPORT of that run.
12. Send major 0 and minor 2 in HELLO_ACK.
13. Fill the last downlink block with silence to the full block size.
14. Before the answer starts, send an AGENT_STATE at least every 10 seconds.
15. To show the Report page on the device, send SHOW_PAGE 3. The device changes only from the live data page or the idle Coach page.
16. Increment 3: after each RUN_REPORT, send the TURN_SHAPE of that run. Set source 1 for a shape that does not come from a measured track.
17. Keep every group of the last run. Answer each TURN_REQUEST with one TURN_GROUP for each asked group.
18. Increment 4: to show a popup, send POPUP with a new phone `xid`. For a popup with audio, send PLAY_START on channel 2 within 5 seconds.
19. Read POPUP_END. A fall alert can end with reason 0 and no key press. Start the escalation of a fall alert on the phone.
20. At the start of each run, send RUN_START with the run number, before the first LIVE_STATS (D-32).

## 12. Open items

- The phone application team accepts or replaces this draft. Q-13 tracks this item.
- Measure the MTU, the connection interval, and the throughput on the target phones.
- Check the background behavior of the phone application while a voice exchange runs.
- The phone application team checks that it can calculate each LIVE_STATS and RUN_REPORT field. It also gives the rules that detect the start and the end of a run. Q-16 tracks this item.
- Record the interval, the PHY, and the data length that each target phone grants.
- A later version measures the battery cost of the link settings. D-18 defers this item.
- The phone application team finds a source of measured track data, and sets the scale of each group. Q-02 tracks this item.
- The phone application team decides when to send each popup kind, and how to escalate a fall alert.
- Measure on the board whether the first blocks of the popup audio play when a popup interrupts an answer.
- Measure on the board the time from RUN_START to the live data page (TC-23).

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

## 15. Changes in version 0.6

| No. | Change | Section |
| --- | --- | --- |
| 1 | Add LIVE_STATS (0x60) and RUN_REPORT (0x61), both from the phone to the device. | 6 |
| 2 | Remove REPORT_REQUEST, REPORT_PAGE, REPORT_ACK, and the report fragment rules. No side built them. | 5, 6 |
| 3 | Change the protocol version from 0.1 to 0.2. HELLO and HELLO_ACK carry minor 2. | 5 |
| 4 | Add four timing rules for the ski data. | 8 |
| 5 | Remove the report page buffer of 8,192 bytes from the device memory cost. Add 128 bytes. | 10 |
| 6 | Version 0.7 adds SHOW_PAGE (0x62), from the phone to the device. Per D-28, the device opens the Report page from the live data page or the idle Coach page. | 6 |

The major version is still 0, so both sides must use the same minor.
If a phone application that supports only 0.1 connects to 0.2 firmware, the device sends code 3 and disables voice.
The phone application team and the firmware must therefore change to 0.2 together. D-24 sets the time of the change.

SHOW_PAGE adds one message and changes no current packet, so the protocol version stays 0.2.
A phone application that never sends SHOW_PAGE sees no change. Firmware at commit `31e511c` or older answers SHOW_PAGE with ERROR code 1.

## 16. Changes in version 0.8

D-29 replaces Ski mode and Rest mode with three pages at one level. No packet format or value changes.

| No. | Change | Section |
| --- | --- | --- |
| 1 | Mode field: 0 means the live data page or the Report page. 1 means the Coach page. | 6 |
| 2 | Page field: 0 live data page, 1 Coach page, 3 Report page. The device no longer sends 2, the old exit question. | 6 |
| 3 | The device sends AGENT_CANCEL at a page change, not at an UP hold. | 6 |
| 4 | SHOW_PAGE works only on the live data page and on the idle Coach page. | 6 |

The phone application team must read CONTEXT and DEVICE_STATUS with the new meanings.

## 17. Changes in version 0.9

D-30 adds the Turns page and three turn messages for Increment 3.

| No. | Change | Section |
| --- | --- | --- |
| 1 | Add TURN_SHAPE (0x63) and TURN_GROUP (0x64), from the phone to the device. | 6 |
| 2 | Add TURN_REQUEST (0x65), from the device to the phone. It is the first request for data from the device. | 6 |
| 3 | Add page 5, the Turns page. On it, the mode field is 0. | 6 |
| 4 | Add about 4.0 KB of static RAM for the turn cache and the page. | 10 |
| 5 | The Heygo IQ in RUN_REPORT runs from 0 to 200, not 0 to 100 (D-27). Firmware at commit `49573f9` or older answers an IQ above 100 with ERROR code 7. | 6 |

The turn messages add messages and change no current packet, so the protocol version stays 0.2.
Firmware at commit `bedcf1a` or older answers TURN_SHAPE and TURN_GROUP with ERROR code 1. A phone application that does not know page 5 treats it as an unknown page.

## 18. Changes in version 0.10

The phone application team asked for these device rules on 2026-10-17, so that the Turns page shows the same run as the Report page.
No packet format changes. The protocol version stays 0.2.

| No. | Change | Section |
| --- | --- | --- |
| 1 | A RUN_REPORT of a new run clears the turn cache. The page shows "加载中" until TURN_SHAPE arrives. | 6 |
| 2 | A TURN_SHAPE with 0 turns shows "暂无弯型数据". The title never shows "0 弯". | 6 |
| 3 | The wait for TURN_SHAPE ends after 90 seconds, with "暂无弯型数据". | 6 |
| 4 | The drawing area of 200 x 76 pixels, and the formula that places each point. | 6 |

Firmware at commit `98f80ef` or older keeps the shape of the previous run until the new TURN_SHAPE arrives. It shows "0 弯" for a shape of 0 turns.

## 19. Changes in version 0.11

D-31 adds phone popups for Increment 4.

| No. | Change | Section |
| --- | --- | --- |
| 1 | Add POPUP (0x66), from the phone to the device. | 6 |
| 2 | Add POPUP_END (0x67), from the device to the phone, with reason 0 timeout, 1 key, or 2 replaced. | 6 |
| 3 | Add channel 2, the popup audio, to PLAY_START. | 6 |
| 4 | Add page 6, the popup page. On it, the mode field is 0. | 6 |
| 5 | Add the 5-second wait for the popup audio and the 10-second countdown. | 8 |
| 6 | Add about 0.7 KB of static RAM for the popup. | 10 |

The popup messages add messages and a channel value. They change no current packet, so the protocol version stays 0.2.
Firmware at commit `421b15c` or older answers POPUP with ERROR code 1, and ignores a PLAY_START on channel 2. A phone application that does not know page 6 treats it as an unknown page.

## 20. Changes in version 0.12

D-32 lets the phone start a run on the device. This change belongs to Increment 2.

| No. | Change | Section |
| --- | --- | --- |
| 1 | Add RUN_START (0x68), from the phone to the device, with the run number. | 6 |
| 2 | The device opens the live data page at RUN_START. During a talk with the coach, it opens the page after the talk ends. During a popup, the popup closes to the live data page. | 6 |
| 3 | Add the 300 ms limit for the screen change after RUN_START. | 8 |

RUN_START adds one message and changes no current packet, so the protocol version stays 0.2.
Firmware at commit `8515ef3` or older answers RUN_START with ERROR code 1 (unknown type). The phone application can ignore that error. The page then changes only with the keys.

## 21. Related documents

- [System Architecture](02-architecture.md).
- [Validation and Decisions](04-validation-and-decisions.md).
- [Decision Amendments](05-decision-amendments.md).
- [Sprint 0 plan](06-sprint-0-plan.md).
- [Four-hour Sprint roadmap](08-sprint-roadmap.md).
- [Folder Index](README.md).
