**简体中文** · [English](heygo-changes.md)

# Heygo 对固件基线的改动

本文列出 Heygo 团队在 `Firmware` 文件夹里做的改动。
该文件夹里的其他文档来自基线，没有改动。
本文的所有路径和命令都从 `Firmware` 文件夹开始。

## 1. 来源

| 项目 | 值 |
| --- | --- |
| 来源 | [FoloToy/ai-passport](https://github.com/FoloToy/ai-passport) |
| 提交 | `3d685a9`，2026-10-15 |
| 许可证 | MIT。[LICENSE](../LICENSE) 文件保留 FoloToy 的版权声明。 |
| 复制方式 | 直接复制文件。不是 Git 子模块。 |

副本包含两处修复，让门禁能在 macOS 上运行：

- `tools/install-actionlint.sh` 自己计算 SHA-256 摘要，再在 shell 里比较。BSD 版 `sha256sum` 不支持 `--check`。
- `tools/validate.sh` 在 macOS 上用 `-dead_strip`，在 Linux 上用 `--gc-sections`。

## 2. Sprint 0 任务

[Sprint 0 计划](../../需求文件夹/06-sprint-0-plan.zh_CN.md)定义了四个任务。

| 任务 | 文件 | 主机测试 |
| --- | --- | --- |
| T-1 模式与音频状态机 | `flows/coach.flow.json`、`main/generated/coach_*` | `tests/test_coach_flow.c`、`tests/test_coach_paths.c` |
| T-2 数据包层 | `main/protocol/ble_proto.*`、`main/audio/play_gate.*` | `tests/test_ble_proto.c`、`tests/test_play_gate.c` |
| T-3 IMA ADPCM 编解码 | `main/audio/ima_adpcm.*` | `tests/test_ima_adpcm.c` |
| T-4 门禁与构建 | `tools/validate.sh`、`main/CMakeLists.txt` | 完整门禁 |

`main/generated/` 里的所有文件都由 `tools/flow/flowc.py` 生成。
不要手工修改这些文件。
修改 `flows/coach.flow.json`，然后运行 `python3 tools/flow/flowc.py flows/coach.flow.json`。

## 3. 构建与检查

1. 激活 ESP-IDF 5.5.3：`. ~/esp/esp-idf-v5.5.3/export.sh`。
2. 运行完整门禁：`./tools/validate.sh`。
3. 确认最后一行是 `Firmware build: PASS`。

除了基线检查，门禁还运行以下 Heygo 检查：

- T-1、T-2、T-3 的主机测试。
- 字形检查 `../assets/tools/check_glyphs.py`。
- 界面检查 `tools/host_render/render.py`。
- 模拟器检查 `tools/flow/flow_sim.py --check` 和 `--run-selftest`。

界面检查用 `sdkconfig` 里的设备配置，在主机上编译 LVGL。
它绘制每个 P0 界面状态，遇到以下情况就报失败：

- 示例文字里有字体缺少的字形。
- 控件超出屏幕，或两个控件重叠。
- setter 加锁和解锁的次数不相等。
- 第二轮结束时，LVGL 内存池的占用高于第一轮。
- setter 把文字截断在一个 UTF-8 字符中间。

要生成 `../assets/mockups` 里的 PNG 文件，用装了 Pillow 的 Python 运行 `python3 tools/host_render/render.py`。
ESP-IDF 的 Python 没有 Pillow。所以在门禁里，脚本只运行检查。

## 4. 流程模拟器

`../assets/sim/coach-flow.html` 是 Coach 流程的交互模拟器。
用浏览器直接打开即可，不需要服务器，也不需要联网。

`tools/flow/flow_sim.py` 用 `flows/coach.flow.json`、`../assets/ui/strings.json` 和主题颜色生成这个页面。
修改流程图之后，运行 `python3 tools/flow/flow_sim.py`。

门禁运行两项检查：

- `--check`：页面和流程图不一致时报失败。
- `--run-selftest`：在无界面 Chrome 里运行 35 项测试。没有 Chrome 时报 SKIPPED。

## 5. 实测结果

固件数据测于 2026-10-17，此时 Coach 应用已取代演示菜单。
主机界面数据测于 2026-10-17，在视觉改版之后。设备测试：未运行。

| 项目 | 结果 | 方法 |
| --- | --- | --- |
| 完整门禁 | 退出码 0 | `./tools/validate.sh` |
| 应用镜像 | 1,649,886 字节，分区 8,323,072 字节。包含 Coach 应用、NimBLE 和四个字体。 | `idf.py size` |
| 静态 DRAM | 141,728 / 321,296 字节，44.1%。基线演示程序为 178,960 字节。 | `idf.py size` |
| 构建 Heygo 界面之前的 LVGL 内存池 | 9,352 字节，共 21,656 字节 | 主机界面检查 |
| Coach 界面 | 第一次构建 +6,768 字节 | 主机界面检查 |
| flowc 对三个界面的估算 | 13,970 字节。运行时只激活一个界面。 | `tools/flow/flowc.py` |
| LVGL 内存池峰值 | 17,584 / 21,656 字节，81% | 主机界面检查 |

Sprint 1 加入了 NimBLE 和语音任务，静态 DRAM 从 105,610 字节增加了 36,006 字节。
应用不链接 Wi-Fi，所以这个数字仍低于基线。
NimBLE 和各任务的栈在运行时还会占用堆，只有设备测试能测出这部分。

主机的指针是 8 字节，ESP32-C3 的指针是 4 字节。
我们预计设备上的内存池占用低于主机数据。需要设备测试来确认。

flowc 的内存池模型已改用主机实测值：每个标签 360 字节，每个界面 330 字节，每个观察者 120 字节。
模型不再计入字符串主题的缓冲区。这些缓冲区是静态数组，不占内存池。

## 6. 设备上的 Coach 应用

`main/main.c` 启动 Coach 应用，不再启动 BSP 演示菜单。
`demo_*.c` 文件作为 BSP 参考留在构建里，链接器会把它们去掉。

| 文件 | 职责 |
| --- | --- |
| `main/coach_app.c` | 在一个任务里运行状态机。显示每个状态对应的界面。管理计时器。 |
| `main/coach_input.c` | 把按键事件转成状态机事件。纯 C，有主机测试。 |
| `main/generated/coach_ui.c` | 按流程图里的 `ui_effects` 设置文字和图标。 |

按键事件和计时器事件经同一个队列送到 Coach 任务。
所以任务每次只处理一个事件。

按键映射：

| 按键事件 | 状态机事件 |
| --- | --- |
| LONG，500 ms 时 | `UP_HOLD`、`DOWN_HOLD` 或 `OK_HOLD` |
| LONG 之后的 RELEASE | `OK_RELEASE`。UP 和 DOWN 不产生事件。 |
| CLICK 或 DOUBLE | `UP_SHORT`、`DOWN_SHORT` 或 `OK_SHORT`，只产生一次 |

任务在 `CAPTURE` 状态启动 20 秒计时器，在 `WAIT_ANSWER` 状态启动 10 秒计时器。
这两个计时器分别产生 `CAPTURE_LIMIT` 和 `AGENT_TIMEOUT`。

同一时间只存在一个界面，因为三个界面同时存在会超出 LVGL 内存池。
切换界面时，任务先载入空白界面，删除旧界面，再构建新界面。

### Sprint 1：语音与蓝牙

| 文件 | 职责 |
| --- | --- |
| `main/link/link_core.c` | 文档 07 的会话：握手、语音可用性、xid、超时和错误。纯 C，有主机测试。 |
| `main/link/ble_link.c` | NimBLE、GATT 服务、广播、连接参数和两条发送队列。 |
| `main/audio/voice.c` | 录音编码成 ADPCM 块；ADPCM 块经过播放启动规则送到喇叭。 |
| `tools/phone_sim/` | 手机应用的替身。见它的 [README](../tools/phone_sim/README.zh_CN.md)。 |

设备以 `HeygoPP-XXXX` 广播，同一时间只接受一部手机。
只有完成订阅、收到 HELLO_ACK 0.1、MTU 不低于 172 之后，语音才可用。

Coach 任务遵守以下规则：

- 录音在本次转换的所有动作执行完之后才开始。所以第一个块就带上了新的 xid。
- 停止录音时等待最后一个块发出。所以 AGENT_END 一定在最后一个 AUDIO_UP 块之后。
- 停止播放时等待喇叭停下。所以播放时麦克风从不工作。
- 语音不可用时，READY 状态下的长按 OK 变成 `ASK_BLOCKED`。界面显示原因，不开始录音。

手机事件与状态机事件的对应关系：

| 手机事件 | 状态机事件 |
| --- | --- |
| 当前 xid 的 PLAY_START | `ANSWER_START` |
| PLAY_END 之后喇叭播完最后一块，或数据流停顿 2 秒 | `ANSWER_END` |
| AGENT_STATE 失败，失败码 1 或 3 | `ANSWER_FAILED` |
| AGENT_STATE 失败，失败码 2 | `AGENT_TIMEOUT` |
| 语音变为不可用 | `LINK_LOST` |

### 界面设计

界面遵循 Figma 里的 Heygo 品牌手册（文件 `d1RBbdR0K2sEScrmKP1efZ`，Brandbook-cn 页面）。
`../assets/mockups/compare-A-B.png` 是两个方案的对比，目前采用 B 版。

- 颜色：黑色底，`#1A1A1A` 卡片，白色文字，荧光绿强调色 `#B1FE2B`。
- 字体：英文和数字用 Staatliches 展示字体，状态词用 Noto Sans SC Bold。
- Coach 的每个状态显示一个大号英文词，例如 READY。失败时变成红色。
- 用线条控件画出山脊，滑行界面还有两道滑雪板刻出的雪痕。
- 底部提示是绿底黑字的胶囊。

flowc 为这些图形新增了 `line` 控件类型。它的坐标点存放在 flash 中。

### 调试日志

监视器日志能看到一次交流的每一步。手机团队和测试员可以看这些标签：

| 标签 | 内容 |
| --- | --- |
| `coach` | 每次状态转换，例如 `CAPTURE --OK_RELEASE--> WAIT_ANSWER`。 |
| `ble_link` | 设备发出（`tx`）和收到（`rx`）的每个包，带字段和文本。AUDIO_UP、AUDIO_DOWN 和 PHONE_STATUS 不逐包记录。 |
| `ble_link` | `answer received`：收到 PLAY_END 时，这段回答的 AUDIO_DOWN 块数。 |
| `ble_link` | `drop`：xid 过期或已取消的包。每个数据流只记一行。 |
| `voice` | 每次录音的块数和峰值，以及每次停止的耗时。 |
| `screen` | 每次切换界面，以及屏幕文字的每次新值，不管来源是什么。 |

### 给手机应用团队的协议说明

文档 07 的 0.4 版在第 13 节写明了第 1 至 6 条，0.5 版在第 14 节写明了第 7 条。手机应用团队需要逐条确认。

1. AGENT_END 排在本次交互最后一个 AUDIO_UP 块之后发送。其他控制包都排在音频之前。
2. HELLO 只声明 IMA ADPCM。codec 为 1 的 PLAY_START 会收到 ERROR 7。
3. 每个非失败的 AGENT_STATE 都会让 10 秒等待重新计时。所以在回答开始之前，手机至少每 10 秒发一次状态。
4. 设备不发起 MTU 交换，必须由手机请求 MTU。
5. 设备自己 10 秒超时后，会发送 AGENT_CANCEL。
6. 设备忽略 Team 通道的 PLAY_START，因为 Team 语音属于 P1。
7. 手机收到 AGENT_END 之后，才能发送这次交流的 PLAY_START。设备在录音时丢弃 PLAY_START。这条来自 D-20。

## 7. 集成规则

LVGL 不是线程安全的。每个生成的 setter 都通过钩子获取 LVGL 锁。
在任何任务调用 setter 之前，先把 BSP 锁交给绑定代码：

```c
static bool coach_lock(void) { return bsp_lvgl_lock(100); }
static void coach_unlock(void) { bsp_lvgl_unlock(); }

coach_bindings_set_lock(coach_lock, coach_unlock);
```

BSP 锁是 `esp_lvgl_port` 里的递归互斥锁。
所以已经持有锁的代码也可以调用 setter。
获取锁超时的时候，setter 什么也不做。

每个字符串 setter 都会在 UTF-8 字符边界处截断过长的文字。
body 主题最多容纳 239 字节文字。MTU 为 247 时，`AGENT_STATE` 文字最多 238 字节。

## 8. Sprint 2：滑行数据

Sprint 2 让设备收下增量 2 的两种消息。协议见文档 07 第 6 节，协议版本为 0.2。
本 Sprint 不改界面。滑行页和报告页在 Sprint 3 和 4 接入。

| 文件 | 改动 |
| --- | --- |
| `main/protocol/ble_proto.*` | 协议 0.2。解析和组装 LIVE_STATS（0x60）与 RUN_REPORT（0x61）。新增 UTF-8 检查。 |
| `main/link/link_core.*` | 保存最后一个 LIVE_STATS 和最后一份报告，以及收到时刻。`link_live()` 给出 5 秒过期标志，`link_report()` 给出报告的时长。 |
| `main/link/ble_link.*` | 把两种数据交给应用的回调。日志每包一行。 |
| `main/ui/stats_fmt.*` | 纯 C 的数值格式化。未知值显示"--"，绝不显示 0。 |
| `tools/phone_sim/` | 协议 0.2。`--runs N` 发送 N 趟模拟滑行。 |
| `tests/test_ble_proto.c`、`tests/test_link_core.c`、`tests/test_stats_fmt.c`、`tests/test_phone_sim.py` | 新消息、会话规则、格式化，以及 Python 与 C 的逐字节对比。 |

设备遵守以下规则：

- 只有收到同一协议版本的 HELLO_ACK 之后，设备才接受两种数据消息。
- MTU 低于 172 时，语音关闭，但数据消息仍然可用。
- 错误的报告得到 ERROR，设备保留旧报告。
- 链路断开后，设备保留两种数据。

只支持协议 0.1 的手机应用连接本固件时，设备回复错误码 3，并关闭语音功能。

## 9. 待定事项

- 在录音、等待回答或播放时，状态机忽略 DOWN 长按。D-19 确认了这个行为。
- 设备测试：未运行。板子不在手边，也还没有符合文档 07 的手机应用版本。
- 手机应用团队需要评审文档 07 的 0.6 版。它在第 6 节加入了协议说明和两种数据消息。
- 屏幕上的电量标签一直为空。电量通过 DEVICE_STATUS 发给手机。
- 录音期间到达的 PLAY_START 会丢失，设备随后显示超时。D-20 规定手机不能在这时发送。
- body 标签显示四行。更长的文字以三个点结尾。设备会用语音播完整的回答。
