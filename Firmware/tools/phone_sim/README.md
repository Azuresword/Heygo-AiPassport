[简体中文](README.zh_CN.md) · **English**

# phone_sim: a stand-in for the phone application

`phone_sim.py` connects to AI Passport over BLE and speaks protocol 0.2 of doc 07.
Use it for device tests before the phone application team delivers a build.
It does no speech recognition and calls no Agent.

## What it does

1. It finds the first device with a name that starts with `HeygoPP-`, and connects.
2. It answers HELLO with HELLO_ACK 0.2, and sends PHONE_STATUS every 3 seconds.
3. It receives each question as AUDIO_UP blocks, and saves it as a WAV file.
4. It answers in the selected mode.
5. With `--runs N`, it sends N simulated runs of ski data. See the section "Ski data".

| Mode | Answer |
| --- | --- |
| `echo` | The device plays the question back. This mode tests the full audio path. It is the default. |
| `wav` | The device plays a WAV file: 16 kHz, mono, 16-bit. |
| `fail` | The phone reports a recognition failure. The device asks the user to ask again. |
| `silent` | The phone sends no answer. The device shows a timeout after 10 seconds. |

## Use

```bash
pip install bleak
python3 tools/phone_sim/phone_sim.py --mode echo --out /tmp
```

To make a WAV file on macOS:

```bash
say -o /tmp/a.aiff "Your weight was too far back" && afconvert -f WAVE -d LEI16@16000 -c 1 /tmp/a.aiff /tmp/a.wav
python3 tools/phone_sim/phone_sim.py --mode wav --wav /tmp/a.wav
```

The log shows each packet, the MTU, the number of blocks, the missing blocks, and the delay from AGENT_END to the answer.

## Ski data

`--runs N` sends N runs. `--run-s` sets the seconds of each run. The default is 30.

```bash
python3 tools/phone_sim/phone_sim.py --runs 3 --run-s 30
```

1. During a run, it sends one LIVE_STATS each second.
2. In a run of 20 seconds or more, it sends 3 seconds of the stopped state in the middle.
3. Run 2 has no speed. The device must show "--" for the speed, never 0.
4. When a run ends, it sends one LIVE_STATS with the ended state.
5. After 2 seconds, it sends one RUN_REPORT.
6. Then it sends one TURN_SHAPE with a demo shape of the run (D-29). Run 3 sends a schematic shape.
7. It then sends nothing for 10 seconds, as on a lift. The device marks the data out of date after 5 seconds.

Add `--jump` to send SHOW_PAGE after each report. The device opens the Report page from the Ski page or the idle Coach page (D-28).

## Device test

Run these steps on a board with the current firmware.
Record the phone, the log, and the result of each step.

| Step | Action | Expected result | Test case |
| --- | --- | --- | --- |
| 1 | Start the device. Start `phone_sim.py`. | The log shows HELLO, then CONTEXT and DEVICE_STATUS. The status line shows the connected text. | TC-01 |
| 2 | Hold OK to enter Rest mode. Hold OK again and speak. Release OK. | The log shows AGENT_START, the AUDIO_UP blocks, and AGENT_END. A WAV file holds the question. | TC-06 |
| 3 | Wait for the answer in `echo` mode. | The device plays the question back. The screen returns to ready. | TC-07, TC-08 |
| 4 | Hold UP during the answer. | The speaker stops. The log shows AGENT_CANCEL. | TC-07 |
| 5 | Stop `phone_sim.py` during a question. | The device stops capture and shows the failure. | TC-20 |
| 6 | Use `fail` mode, then `silent` mode. | The device shows the retry text, and then the timeout text. | TC-06 |
| 7 | Start `phone_sim.py --runs 3`. | The log shows a LIVE_STATS line each second and a RUN_REPORT line after each run. | TC-02 |
| 8 | Start `phone_sim.py --runs 2 --jump`. Stay on the Ski page for run 1. Ask a question when run 2 ends. | Run 1 opens the Report page. During the question in run 2, the page does not change. | TC-03 |
| 9 | Start `phone_sim.py --runs 3`. After each run, open the Report page, and then press DOWN. Press OK a few times. | The Turns page shows the run and its turns. OK selects the next turn. After run 3, the page shows the schematic tag. | TC-05 |

## Log analysis

Keep the monitor log, and then analyze it:

```bash
idf.py -p <port> monitor | tee /tmp/monitor.log
python3 tools/phone_sim/analyze_log.py /tmp/monitor.log --csv /tmp/questions.csv
```

The tool prints one row for each question: hold time, blocks, peak level, stop times, and the outcome.
The summary checks two limits: 100 ms for the capture stop (TC-06), and 200 ms for the playback stop (TC-07).
The playback figure adds 90 ms, because the I2S DMA still holds that much sound after the stop.

## Limits

- The macOS BLE stack picks the MTU. `phone_sim.py` cannot request 247.
- `tests/test_phone_sim.py` checks the packets and the codec against the firmware C code. It does not test BLE.
