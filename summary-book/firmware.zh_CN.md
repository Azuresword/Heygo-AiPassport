<p class="lang"><strong>简体中文</strong> · <a href="firmware.html">English</a></p>

<p class="eyebrow">Firmware</p>

# 固件架构

固件运行于 ESP32-C3，使用 ESP-IDF 5.5.3 和 FreeRTOS。应用代码位于 `Firmware/main/`，板级支持位于 `Firmware/components/bsp/`。以下数据对应 `284ab00` 快照。

<div class="tiles">
<div class="tile accent"><div class="k">状态</div><div class="v">10</div></div>
<div class="tile accent"><div class="k">转移</div><div class="v">83</div></div>
<div class="tile"><div class="k">应用任务</div><div class="v">3</div></div>
<div class="tile"><div class="k">C 代码</div><div class="v">8.6<small>k 行</small></div></div>
<div class="tile"><div class="k">主机测试文件</div><div class="v">33</div></div>
</div>

## 分层

<figure>
<img src="images/fw-layers-zh.svg" alt="固件分层图">
<figcaption>图 9　固件分层。绿框表示纯 C 模块，可脱离 LVGL 和 ESP-IDF 运行主机测试。</figcaption>
</figure>

设计分为两个部分：

1. **逻辑独立于硬件。** 状态机、按键映射、协议编解码、会话规则和 ADPCM 使用纯 C。
2. **构建时编译流程图。** `flowc.py` 将 `coach.flow.json` 转换为 C 转移表。设备执行转移表，无需解析流程图或运行图解释器。

下表路径均相对于 `Firmware/`。行数包含 BSP 参考程序。

| 模块 | 路径 | 行数 | 职责 |
|---|---|---|---|
| 应用 | `main/main.c`、`coach_app.c`、`coach_input.c` | 927 | 应用初始化、状态转移串行处理、按键转事件 |
| 生成代码 | `main/generated/` | 1,486 | 转移表、界面构建、数据绑定和界面效果。不要手工修改生成文件 |
| 界面 | `main/ui/` | 746 | 实时数据、弯型、弹窗、页面导轨等组件 |
| 链路与协议 | `main/link/`、`main/protocol/` | 1,824 | 数据包编解码、握手、会话规则和 NimBLE GATT |
| 语音 | `main/audio/` | 450 | 录音、播放控制、音量包络和 ADPCM |
| BSP | `components/bsp/src/` | 1,646 | 显示、LVGL 适配、按键 ADC、ES8311、电量计和 I2C |
| BSP 参考 | `main/demo_*.c` 等 | 1,521 | 上游演示程序。链接器移除未使用的函数 |

## 启动顺序

<div class="timeline">
<div class="ev"><div class="when">01</div><div class="what">读取唤醒原因</div><div class="how">按键唤醒后屏蔽按键操作，直到按键释放或达到 10 s 超时。</div></div>
<div class="ev"><div class="when">02</div><div class="what">初始化 I2C、显示和 LVGL</div><div class="how">显示或 LVGL 初始化失败时，记录 SPI 引脚并停止应用启动。初始化成功后将背光设为 100%。</div></div>
<div class="ev"><div class="when">03</div><div class="what">初始化音频和电量计</div><div class="how">初始化 ES8311 和 CW2017。失败时记录状态，不中止应用启动。</div></div>
<div class="ev"><div class="when">04</div><div class="what">启动 Coach 应用</div><div class="how">创建队列和计时器，启动 voice、BLE 和 coach 任务。语音或 BLE 启动失败时，界面和按键仍可使用。</div></div>
<div class="ev"><div class="when">05</div><div class="what">启用按键输入</div><div class="how">应用就绪后才将按键事件送入队列。</div></div>
</div>

## 任务和队列

<figure>
<img src="images/fw-tasks-zh.svg" alt="任务和队列">
<figcaption>图 10　任务优先级、栈大小和队列。</figcaption>
</figure>

任务优先级向音频和传输倾斜：

- **voice：7。** 处理 20 ms 音频块。交付延迟可能导致播放断音。
- **ble_tx：6。** 优先发送控制队列中的包。`AGENT_END` 使用音频队列，排在最后一个 `AUDIO_UP` 之后。
- **coach：5。** 按键、计时器和链路事件进入同一队列，状态转移按顺序执行。
- **LVGL：4。** 绘制优先级低于音频和传输。

状态转移串行执行不代表没有并发约束。其他任务访问 LVGL 前必须持有 `bsp_lvgl_lock()`。链路回调同样需要链路互斥锁保护。

队列容量固定。控制发送队列和音频发送队列各容纳八个包。音频队列溢出时丢弃最旧的包；控制队列溢出时丢弃新包并记录警告。

## 状态机

`flowc.py` 从 `coach.flow.json` 生成 10 个状态和 83 条转移，输出至 `main/generated/coach_flow.c`。

| 组 | 状态 | 说明 |
|---|---|---|
| 页面 | `LIVE`、`REPORT`、`TURNS` | 实时数据、报告和弯型。短按 UP、DOWN 在页面循环中导航 |
| 教练 | `READY`、`CAPTURE`、`WAIT_ANSWER`、`PLAYBACK` | 就绪、录音、等待回答和播放 |
| 弹窗 | `POPUP_WAIT`、`POPUP_PLAY`、`POPUP_HOLD` | 等待语音、播放语音和 10 s 倒计时 |

两个计时器控制录音时长和回答进展：

- `CAPTURE` 持续 20 s 后产生 `CAPTURE_LIMIT`，结束录音。
- `WAIT_ANSWER` 连续 10 s 无进展后产生 `AGENT_TIMEOUT`。每条非失败状态的 `AGENT_STATE` 消息都会重新计时。

同一时间只保留一个应用页面。切换时先加载常驻的空白界面，再删除旧页面并创建新页面，以限制 LVGL 内存分配峰值。

页面导轨常驻 LVGL 顶层，切换页面时不删除。四个刻度和滑块均由同一个对象绘制。

## 音频通路

<div class="cards">
<div class="card"><span class="tag">上行 · 录音</span><h4>麦克风 → 手机</h4><p>ES8311 提供 16 kHz、16-bit 单声道采样。voice 任务每 20 ms 编码一个 164 字节的 ADPCM 块。音频发送队列容纳八个包，溢出时丢弃最旧的包。ble_tx 通过 NimBLE 通知发送数据包。</p></div>
<div class="card"><span class="tag">下行 · 播放</span><h4>手机 → 喇叭</h4><p>NimBLE 接收数据包。link_core 检查会话标识 xid，只接受当前流。下行队列容纳八个块，溢出时丢弃最旧的块。play_gate 控制播放启动。voice 任务将数据解码为 PCM，再经 I2S 和 ES8311 输出至喇叭。</p></div>
</div>

20 s、16 kHz、16-bit 单声道录音需要 640,000 字节的裸 PCM。设备在录音时持续发送音频块，不在 RAM 中保存完整录音。
