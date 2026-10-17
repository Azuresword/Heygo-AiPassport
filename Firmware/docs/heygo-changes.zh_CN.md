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
| 应用镜像 | 1,252,514 字节，分区 8,323,072 字节。包含 Coach 应用和两个字体。 | `idf.py size` |
| 静态 DRAM | 105,610 / 321,296 字节，32.9%。基线演示程序为 178,960 字节。 | `idf.py size` |
| 构建 Heygo 界面之前的 LVGL 内存池 | 9,352 字节，共 21,656 字节 | 主机界面检查 |
| Coach 界面 | 第一次构建 +6,184 字节。渲染后 +5,856 字节。 | 主机界面检查 |
| flowc 对三个界面的估算 | 12,930 字节。运行时只激活一个界面。 | `tools/flow/flowc.py` |
| LVGL 内存池峰值 | 17,128 / 21,656 字节，79% | 主机界面检查 |

静态 DRAM 比基线低，因为应用不再链接 Wi-Fi 和 BLE 演示。
Sprint 1 会重新加入 BLE，这个数字会再升高。

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

录音、播放和蓝牙动作还没有接上，任务只把它们写进日志。
所以蓝牙一直是断开状态。状态栏显示"未连接"，每次提问 10 秒后以超时结束。

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

## 8. 待定事项

- 在录音或播放时，状态机忽略 DOWN 长按。需求没有定义这种情况。
- 蓝牙断开时，在 READY 长按 OK 仍会开始录音。流程里这条转换没有检查连接状态。
- 录音、播放和蓝牙动作目前只写日志。Sprint 1 接上它们。
- body 标签显示四行。更长的文字以三个点结尾。设备会用语音播完整的回答。
