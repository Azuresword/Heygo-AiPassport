<p align="right"><strong>简体中文</strong> · <a href="risks.html">English</a></p>

# 待办与风险

## P0：卡住交付

| # | 事项 | 下一步 | 来源 |
|---|---|---|---|
| 1 | 设备测试一次都没跑 | Sprint 5 开发板冒烟，之后 Sprint 6–10 联调和可用性测试 | `需求文件夹/08-sprint-roadmap.zh_CN.md:34-52` |
| 2 | 没有支持协议 0.2 的手机应用版本 | Sprint 6–8 依赖它 | 同上 :52 |
| 3 | D-20、D-31、D-32 等 App 团队确认 | 和 App 团队过一遍 07 号文档最新版 | `04-validation-and-decisions.zh_CN.md:152-153` |
| 4 | HeygoAI 教练能力不明（Q-01） | Sprint 8 依赖它 | `08-sprint-roadmap.zh_CN.md:41` |

## P1

| # | 事项 | 来源 |
|---|---|---|
| 5 | D-33 页面导轨固件未开始（Sprint 16） | `04-validation-and-decisions.zh_CN.md:154` |
| 6 | 弯型实测轨迹从哪来还没定（Q-02） | 同上 :101 |
| 7 | CI 可能从没跑过 | [门禁与部署](build-and-deploy.zh_CN.md) |

## 已知缺陷

来源：`Firmware/docs/heygo-changes.zh_CN.md:329-331`

- 屏幕上的电量标签一直为空。电量通过 DEVICE_STATUS 发给手机。
- 录音期间到达的 PLAY_START 会丢，设备随后显示超时。D-20 规定手机不能在这时发。
- 正文标签显示四行，更长的以三个点结尾。语音会播完整回答。

## 已接受的风险

来源：`04-validation-and-decisions.zh_CN.md` 风险表

| 风险 | 怎么处理 |
|---|---|
| 蓝牙不加密，附近的人能听到音频（D-12） | 已接受 |
| 没有功耗管理（D-18） | 已接受 |
| 摔倒弹窗 10 秒后自动关，用户可能按不了键（D-31） | 升级处理由手机负责；POPUP_END 原因 0 告诉手机没人按键 |
| 弹窗语音在旧回答停止前到达（D-31） | `audio_play_cut` 停喇叭，只接受当前流的 AUDIO_DOWN；TC-22 检查 |
| 手机误判开始滑行（D-32） | 判定由手机负责；在实时数据页短按 DOWN 回报告页 |
