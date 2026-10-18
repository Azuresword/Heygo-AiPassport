<p class="lang"><strong>简体中文</strong> · <a href="index.html">English</a></p>

<div class="hero">
<p class="eyebrow">Heygo · AI Passport · 技术报告</p>
<h1>滑雪数据与语音教练终端</h1>
<p class="lead">AI Passport 基于 ESP32-C3，通过 BLE 连接手机，提供滑行数据、单趟报告、弯型浏览和教练语音交互。设备负责按键、显示、录音和播放。手机负责数据分析、语音处理及 AI 服务接入。</p>
<img src="../assets/sim/preview.png" alt="交互模拟器：设备屏幕、手机端模拟、状态流转图">
</div>

报告基线：2026-10-18，版本 `284ab00`，覆盖 Sprint 0–17。以下指标对应这一阶段。

<div class="tiles">
<div class="tile accent"><div class="k">Sprint 完成记录</div><div class="v">18<small>/18</small></div></div>
<div class="tile"><div class="k">功能增量</div><div class="v">4<small>/4</small></div></div>
<div class="tile"><div class="k">决策条目</div><div class="v">34</div></div>
<div class="tile"><div class="k">需求条目</div><div class="v">25</div></div>
<div class="tile"><div class="k">界面样例</div><div class="v">25</div></div>
<div class="tile"><div class="k">模拟器自测</div><div class="v">128<small>/128</small></div></div>
</div>

## 功能范围

<div class="cards">
<div class="card"><span class="tag">增量 1</span><h4>教练语音</h4><p>进入教练页后，长按 OK 录音，松开发送。手机完成转写、AI 请求和语音合成，设备播放回答。单次录音最长 20 秒。</p></div>
<div class="card"><span class="tag">增量 2</span><h4>滑行数据与报告</h4><p>显示速度、落差、距离、转弯数和时间。每趟生成 Heygo IQ（0–200）及一条建议。手机检测到开始滑行后，通知设备切换到实时数据页。</p></div>
<div class="card"><span class="tag">增量 3</span><h4>弯型浏览</h4><p>每页显示 8 个弯，逐弯查看速度、角度和半径。设备缓存 3 组数据，按需向手机请求其他组。</p></div>
<div class="card"><span class="tag">增量 4</span><h4>手机弹窗</h4><p>支持摔倒提醒、语音播报、问题报告和成就报告。语音结束后倒计时 10 秒，无语音时直接倒计时。任意键关闭弹窗。</p></div>
</div>

## 技术摘要

| 维度 | 设计与结果 | 章节 |
|---|---|---|
| 硬件与通信 | ESP32-C3、8 MiB Flash、无 PSRAM。BLE 协议 0.2，16 kHz IMA ADPCM | [架构与协议](architecture.zh_CN.md) |
| 固件组织 | 7 层架构、3 个应用任务。流程图生成 10 个状态和 83 条转移 | [固件架构](firmware.zh_CN.md) |
| 交互 | 四页循环、三键操作。弹窗可打断对话，开始滑行事件延后到当前操作结束 | [产品与页面](product.zh_CN.md) |
| 资源占用 | 应用占 factory 分区 23.4%。静态内部 RAM 占 47.2%。主机 LVGL 内存池峰值 76.6% | [内存分布](memory.zh_CN.md) |
| 构建与测试 | 完整门禁通过，模拟器自测 128/128。设备验收与主机测试分别记录 | [质量与部署](quality.zh_CN.md) |
| 交付 | 4 个功能增量、18 个 Sprint。剩余集成事项与能力限制单独列出 | [限制与后续工作](followups.zh_CN.md) |

## 项目入口

- [在线模拟器](https://azuresword.github.io/Heygo-AiPassport/)
- [界面一览](screens.zh_CN.md)
- [项目仓库](https://github.com/Azuresword/Heygo-AiPassport)
