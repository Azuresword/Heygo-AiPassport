<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# Heygo AI Passport

这是 Heygo AI Passport 的项目仓库。
设备通过蓝牙连接手机。手机负责语音识别、语音合成、滑行数据和 AI 教练。设备负责按键、麦克风、喇叭和屏幕。

## 设备做什么

| 增量 | 功能 | 状态 |
| --- | --- | --- |
| 增量 1：教练语音 | 休息时长按 OK 向 AI 教练提问，设备播放回答。 | 代码已完成。设备测试未运行。 |
| 增量 2：滑行数据与报告 | 滑行时显示速度、落差、距离、转弯和时间。每趟结束后显示 Heygo IQ、核心问题和一条小建议。 | 代码已完成。设备测试未运行。 |

蓝牙协议为 0.2 版，手机应用团队已确认。
固件门禁 `./tools/validate.sh` 在 `main` 上的结果是退出码 0。

## 交互模拟器

![交互模拟器](assets/sim/preview.png)

`assets/sim/coach-flow.html` 是一个单文件网页，用来在电脑上试用设备的按键和页面。
它不需要服务器，也不需要联网。

- 左侧是设备屏幕和三个按键。按住 500 ms 算长按。键盘的 ↑、空格、↓ 对应 UP、OK、DOWN。
- "手机端模拟"可以发送回答、断开蓝牙、开始一趟滑行、结束这一趟，或让设备跳到报告页。
- 中间是状态流转图和触发矩阵，显示当前状态和每个按键会去哪里。
- 右侧是状态机日志和蓝牙消息。

打开方法：

```bash
open assets/sim/coach-flow.html
```

页面由流程图 `Firmware/flows/coach.flow.json` 生成，和固件用同一份状态机。
不要手工修改这个页面。修改流程图后，在 `Firmware` 里运行 `python3 tools/flow/flow_sim.py`。
浏览器用系统字体绘制屏幕。真实像素以 `assets/mockups` 里的图片为准。

## 界面效果图

`assets/mockups` 里的 15 张图由 LVGL 在电脑上按设备配置绘制，不是手绘稿。
[contact-sheet.png](assets/mockups/contact-sheet.png) 把它们放在一张图里。

| 图片 | 内容 |
| --- | --- |
| 01–09 | 滑行页、教练页的各个状态，以及退出确认 |
| 10–12 | 滑行中的数据、数据过期、一趟结束后的报告 |
| 13–15 | 报告页：没有报告、有报告、最长文本 |

## 文件夹

| 文件夹 | 内容 |
| --- | --- |
| [需求文件夹](需求文件夹/README.zh_CN.md) | 需求、架构、待办列表、决策、蓝牙协议和 4 小时 Sprint 路线图。 |
| [Firmware](Firmware/docs/heygo-changes.zh_CN.md) | ESP-IDF 固件，基于 FoloToy/ai-passport。请先读 `docs/heygo-changes.zh_CN.md`。 |
| [assets](assets/README.zh_CN.md) | 屏幕文字、字体、界面效果图和交互模拟器。 |

## 获取和检查

```bash
git clone https://github.com/Azuresword/Heygo-AiPassport.git
cd Heygo-AiPassport/Firmware
. ~/esp/esp-idf-v5.5.3/export.sh
./tools/validate.sh
```

最后一行应为 `Firmware build: PASS`。
