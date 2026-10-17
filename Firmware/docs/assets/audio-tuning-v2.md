**English** · [简体中文](audio-tuning-v2.zh_CN.md)

# Heygo audio tuning V2

## Purpose and evidence

V2 targets short playback gaps and abrupt starts. It keeps the V1 output volume at `90` and retains the level counters.
The audio format remains 16 kHz, 16-bit, mono, with IMA ADPCM over BLE.

The baseline logs contain these separate samples:

| Sample | Recorded values |
| --- | --- |
| One answer | 248 blocks, 4.96 s, 0 missing blocks, peak 28927, 0 near-full-scale samples |
| Playback of that answer | 4 underruns, 0 dropped blocks, 0 invalid blocks |
| One capture | 312 blocks, peak magnitude 32768, 178 near-full-scale samples |

The `underruns` counter records attempted silence blocks when the application queue is empty. It does not measure hardware DMA underruns.
The level counters measure PCM samples. They do not measure speaker output.
The near-full-scale threshold is an absolute sample value of 32700. A signed 16-bit sample of −32768 has magnitude 32768.
These samples justify checking playback timing and capture clipping separately. They do not establish the cause of audible distortion.

## Selected changes

1. Enable the 20 ms grace period only after three successful 20 ms BSP writes.
2. Request task polling delays of at most 5 ms during `WAIT`.
3. Resume playback when a block arrives. Do not wait for three blocks again after playback starts.
4. Retain the existing end-of-stream and 2 s stall rules ahead of the grace wait.
5. Apply a linear fade-in across 64 samples, or 4 ms, at the first valid block.
6. Apply that fade-in again after inserted silence or invalid data. Do not apply it after an ordinary `WAIT`.

Each empty queue episode then receives one grace period before silence. Successful writes of audio or zeros count toward this guard.
Removing or dropping queued blocks does not count. Failed writes do not count.
Before three successful writes, an empty queue during playback causes an immediate silence write, as before.
`PLAY_END` still ends an empty stream immediately. The guard does not add padding after `PLAY_END`.

At startup, one or two writes can leave a partially filled DMA descriptor. A further 20 ms wait could leave that partial descriptor unused.
Three writes supply 960 frames, which fill four current DMA descriptors of 240 frames each.
This gives the grace period a margin. Recheck this guard if the BSP DMA configuration changes.

The original startup buffer rule remains: three blocks, 60 ms after the first block, or `PLAY_END` can start playback.
The three-write guard, 20 ms, 5 ms, and 64-sample values are local design choices. The sources below do not prescribe these parameters.
The requested polling delay does not guarantee task scheduling latency.

V2 keeps the existing playback summary for comparisons. It adds a `playback feed` summary:

| Field | Meaning |
| --- | --- |
| Feed duration | Elapsed time from entry into `play_loop` to the summary |
| First silence | Time from entry into `play_loop` to the first attempted silence write, or `-1` if none |
| Maximum write time | Longest observed duration of an audio write call |
| Write errors | Number of audio write errors |
| Fades | Number of applied fade-ins |

These times measure software calls. They do not measure DMA completion or sound at the speaker.

## Research and scope

We retrieved the following official sources on 2026-10-17.

| Source | Source principle | Local decision |
| --- | --- | --- |
| [ESP-IDF 5.5.3 I2S](https://docs.espressif.com/projects/esp-idf/en/v5.5.3/esp32c3/api-reference/peripherals/i2s.html) | I2S writes copy PCM into DMA buffers. They can wait for buffer space. Return does not mean audible playback is complete. | Use the existing blocking audio write. Do not add a fixed 20 ms delay after each write. |
| [WebRTC NetEq Expand](https://webrtc.googlesource.com/src/+/refs/heads/main/modules/audio_coding/neteq/expand.cc) | Packet loss handling uses attenuation windows, overlap, and fades. | Use a short fade-in after silence. This is not the NetEq algorithm or full packet loss concealment. |
| [WebRTC GainController2](https://webrtc.googlesource.com/src/+/refs/heads/main/modules/audio_processing/gain_controller2.cc) and [adaptive gain control](https://webrtc.googlesource.com/src/+/refs/heads/main/modules/audio_processing/agc2/adaptive_digital_gain_controller.cc) | Gain precedes limiting. Adaptive gain also considers headroom, noise, and gain change limits. | Do not add automatic gain control or further amplification before checking the existing peaks. |
| [WebRTC high-pass filter](https://webrtc.googlesource.com/src/+/refs/heads/main/modules/audio_processing/high_pass_filter.cc) | Filter coefficients depend on the sample rate. | Do not add equalization without frequency measurements. |
| [Speex manual](https://www.speex.org/docs/manual/speex-manual/node7.html) | The preprocessor provides gain control and noise reduction. It supports processing by audio frame. | Do not add a new processing library for this bounded timing change. |

V2 adds no automatic gain control, equalization, or external library. It does not increase the V1 output volume.
V2 does not implement fade-out. It does not address capture clipping, physical amplifier distortion, or every DMA underrun.
A 20 ms grace period can exhaust the remaining DMA audio. Device measurements must check this tradeoff.

## V2 validation results

Validation date: 2026-10-17. The complete gate and firmware archive verification passed, including the firmware build and host tests.
Segmented flashing passed. The flash operation did not write NVS.
Startup logs showed `16000Hz/16bit/1ch` and output volume `90`. The phone ran App `1.0.3-dev`.

| Artifact | SHA-256 |
| --- | --- |
| Full firmware | `cc5190ecdc2f1807344089ec32b25296653217263c63f713a71dc0eb40a3e1ad` |
| ELF | `a6c17e39d3d9d9916b9e12314f8f577424152728248417d190a24f192d13d165` |

Four answer cycles completed:

| Answer | Blocks | Audio duration, ms | Playback peak | Attempted silence blocks | First silence, ms | Capture near-full samples | Capture stop, ms |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 216 | 4320 | 29468 | 4 | 84 | 1 | 15 |
| 2 | 448 | 8960 | 31675 | 14 | 69 | 601 | 5 |
| 3 | 376 | 7520 | 29712 | 4 | 84 | 219 | 5 |
| 4 | 1312 | 26240 | 31241 | 10 | 35 | 67 | 5 |

Each cycle recorded a maximum write time of 30 ms. All cycles recorded zero missing, dropped, and invalid blocks.
All playback near-full-scale counters and write error counters were zero. All cycles reached `PLAYBACK --ANSWER_END--> READY`.
The user reported normal playback. The device playback functional check passed.
Analysis of a copy without terminal color codes reported `questions: 4` and `ANSWER_END: 4`.

These cycles did not test TC-07 cancellation latency or the full TC-06 matrix. The analyzer left `playback_stop_ms` empty.
The largest observed capture stop time was 15 ms.

These answers differ from the V1 sample. This was not a controlled A/B comparison, so the results do not establish acoustic improvement.
Acoustic gain, frequency response, and total harmonic distortion remain unverified. Silence insertions and capture samples near full scale remain present.

## Verification and next tuning

Host checks must cover one or two successful writes, the third successful write, zero writes, and excluded failed writes or dropped blocks.
They must also cover grace expiry, arrivals during grace, immediate end without padding, stall expiry, and timer wraparound.
Fade checks must cover signed extremes, the first valid block, silence recovery, invalid data, and ordinary waits.

Use this sequence for the physical A/B comparison:

1. Replay the same clean speech sample on V1 and V2.
2. Keep the device position, listening distance, output volume, and battery condition comparable.
3. Record application underruns, missing blocks, dropped blocks, invalid blocks, and input peaks.
4. Compare initial clicks, gaps, speech clarity, and playback completion.
5. Repeat the test with natural speech and deliberate BLE delays.
6. Inspect the captured WAV before changing microphone gain.
7. Measure speaker response and distortion before adding equalization or further output gain.

If capture clipping persists, test microphone gain separately. If clean PCM still sounds distorted, compare lower output levels and inspect the amplifier path.
Record each change and its result separately. Do not infer acoustic improvement from a successful build or lower application counters alone.
