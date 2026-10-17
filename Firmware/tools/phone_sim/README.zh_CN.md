**简体中文** · [English](README.md)

# phone_sim：手机应用的替身

`phone_sim.py` 通过 BLE 连接 AI Passport，按文档 07 的协议 0.2 通信。
在手机应用团队交付版本之前，用它做设备测试。
它不做语音识别，也不调用 Agent。

## 它做什么

1. 找到第一个名称以 `HeygoPP-` 开头的设备，并连接。
2. 用 HELLO_ACK 0.2 回应 HELLO，每 3 秒发送一次 PHONE_STATUS。
3. 以 AUDIO_UP 块接收每个问题，保存成 WAV 文件。
4. 按所选模式回答。
5. 加上 `--runs N` 时，发送 N 趟模拟的滑行数据。见"滑行数据"一节。
6. 加上 `--popups N` 时，发送 N 个弹窗。见"弹窗"一节。

| 模式 | 回答 |
| --- | --- |
| `echo` | 设备把问题原样播回来。这个模式测试完整的音频链路。默认模式。 |
| `wav` | 设备播放一个 WAV 文件：16 kHz、单声道、16 位。 |
| `fail` | 手机报告识别失败。设备提示用户重说。 |
| `silent` | 手机不回答。设备 10 秒后显示超时。 |

## 使用

```bash
pip install bleak
python3 tools/phone_sim/phone_sim.py --mode echo --out /tmp
```

在 macOS 上生成 WAV 文件：

```bash
say -o /tmp/a.aiff "上一趟重心偏后" && afconvert -f WAVE -d LEI16@16000 -c 1 /tmp/a.aiff /tmp/a.wav
python3 tools/phone_sim/phone_sim.py --mode wav --wav /tmp/a.wav
```

日志显示每个数据包、MTU、块数、丢失的块，以及从 AGENT_END 到回答的延迟。

## 滑行数据

`--runs N` 发送 N 趟。`--run-s` 设定每趟的秒数，默认 30。

```bash
python3 tools/phone_sim/phone_sim.py --runs 3 --run-s 30
```

1. 每趟开始时，发送一个 RUN_START，带趟数（D-32）。设备打开实时数据页。和教练对话中或弹窗中，设备稍后再打开。
2. 滑行中，每秒发送一个 LIVE_STATS。
3. 一趟不少于 20 秒时，中间发送 3 秒的"静止"状态。
4. 第 2 趟没有速度。设备必须把速度显示为"--"，绝不显示 0。
5. 一趟结束时，发送一个"本趟结束"状态的 LIVE_STATS。
6. 2 秒后，发送一个 RUN_REPORT。
7. 然后发送一个 TURN_SHAPE，内容是这一趟的弯数（D-30）。第 3 趟发送示意弯型。
   设备每发一个 TURN_REQUEST，就为每个请求的组回复一个 TURN_GROUP。
   加上 `--turns 44` 时，每趟都有 44 个弯，设备上就有 6 组。加上 `--turns 0` 时，发送没有弯型数据的一趟。
8. 接着 10 秒不发送任何数据，模拟坐缆车。5 秒后，设备标明数据过期。

加上 `--jump` 时，每份报告之后再发送 SHOW_PAGE。设备在滑行页或教练页空闲时打开报告页（D-28）。

## 弹窗

`--popups N` 发送 N 个弹窗（D-31）。`--popup-gap` 设定两个弹窗之间的秒数，默认 20。

```bash
python3 tools/phone_sim/phone_sim.py --popups 8
python3 tools/phone_sim/phone_sim.py --popups 6 --popup-gap 5
```

1. 第一个弹窗在 HELLO_ACK 之后 3 秒发出。
2. 类型轮流出现：摔倒提醒、语音播报、问题报告、成就报告，然后重新开始。
3. 摔倒提醒和语音播报带语音。语音是 `--wav` 文件，没有时用一段 2 秒的提示音。
4. 带语音的弹窗，0.3 秒后在通道 2 发送 PLAY_START，然后发送 AUDIO_DOWN 和 PLAY_END。
5. 每个弹窗使用新的手机 `xid`，从 0x80 到 0xFE。
6. 设备关闭弹窗后，停止发送这个弹窗的语音。

日志显示每个 POPUP_END 的原因，以及距 POPUP 的时间：

| 日志里的原因 | 含义 |
| --- | --- |
| `timeout` | 原因 0。倒计时结束，没有人按键。 |
| `key` | 原因 1。用户按了键。 |
| `replaced` | 原因 2。新弹窗替换了这个弹窗。 |

带 2 秒提示音的弹窗，大约在 POPUP 之后 12 秒关闭。间隔小于 13 秒时，旧弹窗还在显示，新弹窗就到了，设备会替换旧弹窗。
`--popups` 可以和 `--runs` 以及各种回答模式一起用。

## 设备测试

在刷了当前固件的板子上按下面的步骤做。
记录手机、日志和每一步的结果。

| 步骤 | 操作 | 预期结果 | 测试用例 |
| --- | --- | --- | --- |
| 1 | 启动设备。启动 `phone_sim.py`。 | 日志显示 HELLO，然后是 CONTEXT 和 DEVICE_STATUS。状态栏显示"已连接"。 | TC-01 |
| 2 | 长按 OK 打开教练页。再按住 OK 说话。松开 OK。 | 日志显示 AGENT_START、AUDIO_UP 块和 AGENT_END。WAV 文件里是问题录音。 | TC-06 |
| 3 | 在 `echo` 模式下等待回答。 | 设备把问题播回来。界面回到就绪。 | TC-07、TC-08 |
| 4 | 回答时短按 DOWN。 | 喇叭停止，设备翻到实时数据页。日志显示 AGENT_CANCEL。 | TC-07 |
| 5 | 提问过程中停止 `phone_sim.py`。 | 设备停止录音，并显示失败。 | TC-20 |
| 6 | 先用 `fail` 模式，再用 `silent` 模式。 | 设备先显示重说提示，再显示超时提示。 | TC-06 |
| 7 | 启动 `phone_sim.py --runs 3`。 | 日志每秒出现一行 LIVE_STATS，每趟结束后出现一行 RUN_REPORT。 | TC-02 |
| 8 | 启动 `phone_sim.py --runs 2 --jump`。第 1 趟停在滑行页。第 2 趟结束时提一个问题。 | 第 1 趟打开报告页。第 2 趟提问时页面不变。 | TC-03 |
| 9 | 启动 `phone_sim.py --runs 3 --turns 44`。每趟结束后进入弯型页：在实时数据页短按两次 DOWN。短按几次 OK，长按 UP，再长按 DOWN。 | 弯型页画出 8 个弯。OK 选中下一弯，长按 UP 选中上一弯，长按 DOWN 跳到下一页，提示条显示每个按键的作用。第 3 趟之后页面显示"示意"。 | TC-05 |
| 10 | 启动 `phone_sim.py --popups 8`，用按键关闭其中一个弹窗。再启动 `phone_sim.py --popups 6 --popup-gap 5`。 | 每个弹窗都立即显示。日志显示 POPUP_END，原因为 `timeout`、`key` 或 `replaced`。每个弹窗结束后，设备显示弹窗前的页面。 | TC-22 |
| 11 | 启动 `phone_sim.py --runs 20 --run-s 20`。每趟开始前，翻到报告页、弯型页或教练页。其中 3 趟开始时正在提问。再启动 `phone_sim.py --runs 5 --popups 10 --popup-gap 7`，找一次弹窗中开始滑行。 | 每趟都打开实时数据页，显示"第 N 趟 · 滑行中"。提问中，回答结束后才换页。弹窗关闭后回到实时数据页。`analyze_log.py` 的 RUN_START 行第 95 百分位不超过 300 ms。 | TC-23 |

## 日志分析

保存监视器日志，然后分析：

```bash
idf.py -p <端口> monitor | tee /tmp/monitor.log
python3 tools/phone_sim/analyze_log.py /tmp/monitor.log --csv /tmp/questions.csv
```

工具为每次提问输出一行：按住时长、块数、峰值、停止耗时和结果。
汇总部分检查两个上限：录音停止 100 ms（TC-06），播放停止 200 ms（TC-07）。
播放停止时间会加上 90 ms，因为停止后 I2S DMA 里还留有这么长的声音。

对于滑行数据，工具为每个 LIVE_STATS、RUN_REPORT、SHOW_PAGE 和 RUN_START 输出一行。
RUN_START 的计时从日志行"phone starts run N"开始（D-32）。
每行给出从收到数据包到屏幕第一次变化的时间，以及变化的是哪个标签。
汇总部分给出第 95 百分位，并对照 300 ms 的上限（TC-21、TC-03 和 TC-23）。
屏幕没有变化的数据包单独计数，例如和上一包数值完全相同的 LIVE_STATS。

## 限制

- macOS 的 BLE 协议栈自己决定 MTU，`phone_sim.py` 无法请求 247。
- `tests/test_phone_sim.py` 把数据包和编解码与固件的 C 代码逐字节对比。它不测试 BLE。
