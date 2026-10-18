<p align="right"><strong>简体中文</strong> · <a href="product.html">English</a></p>

# 产品与架构

## 四个页面

四个页面平级循环。短按 UP/DOWN 翻页，任何页面长按 OK 打开教练页（D-29）。

| 页面 | 内容 | 决策 |
|---|---|---|
| 实时数据 | 手机每 1 秒发一帧；Heygo IQ 取 0–200 | D-23、D-27 |
| 报告 | 每趟结束发一份 | D-26 |
| 弯型 | 一页 8 个弯；设备存 3 组，其余用 TURN_REQUEST 请求 | D-30 |
| AI 教练 | 按住说话，手机转写、问 AI、合成语音 | D-17 |

手机还能触发两类事件：

- **弹窗**（D-31）：POPUP 0x66 / POPUP_END 0x67。立即打断任何状态，播语音后倒计时 10 秒，任意键关闭。
- **开始滑行**（D-32）：RUN_START 0x68。设备打开实时数据页；对话中或弹窗中则等结束后再打开。

来源：`README.zh_CN.md:8-10`；`需求文件夹/04-validation-and-decisions.zh_CN.md:151-153`

## 设备和手机怎么分工

设备只管按键、麦克风、喇叭、屏幕。语音识别、语音合成、滑行数据、AI 教练都在手机上（D-14）。

```
 ┌──────── 设备 ────────┐   BLE NUS    ┌──────── 手机 ────────┐
 │ 3 键 / 麦克风 / 喇叭 │ ◄──────────► │ 语音识别 / 合成        │
 │ 240×320 屏幕         │  IMA ADPCM   │ 滑行数据 / AI 教练     │
 └──────────────────────┘  16kHz 20ms  └───────────────────────┘
```

- 只走 BLE，不用 Wi-Fi（D-08）。
- 音频 IMA ADPCM，裸 PCM 做回退（D-09）。
- 不加密、不配对（D-12）。
- 协议版本 0.2，App 团队已确认（D-16）。

## 硬件和软件

| 项 | 值 |
|---|---|
| 主控 | ESP32-C3，8 MB Flash，无 PSRAM |
| 屏幕 | ST7789P3，240×320，SPI2 |
| 按键 | UP/DOWN/OK 共用 GPIO0，ADC 分压 |
| 音频 | ES8311，I2S 全双工 |
| 电量计 | CW2017 |
| SDK | ESP-IDF 5.5.3，LVGL 9.5.0，NimBLE |

来源：`Firmware/docs/README.zh_CN.md:138-155`

## 仓库结构

| 路径 | 内容 |
|---|---|
| `Firmware/` | ESP-IDF 固件，从上游 FoloToy/ai-passport `3d685a9` 拷贝，保留 MIT 许可（`Firmware/docs/heygo-changes.zh_CN.md:9-21`） |
| `Firmware/flows/coach.flow.json` | 状态机，固件和模拟器共用 |
| `Firmware/tests/` | 33 个 `test_*` 主机测试 |
| `Firmware/tools/` | `phone_sim` 手机替身、`host_render` 主机界面检查、`flow/flow_sim.py` 生成模拟器 |
| `需求文件夹/` | 01–08 号文档，中英成对 |
| `assets/sim/coach-flow.html` | 单文件模拟器，由脚本生成，不要手改 |
| `summary/` | 本书（生成物） |
