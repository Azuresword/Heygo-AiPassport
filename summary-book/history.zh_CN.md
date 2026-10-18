<p class="lang"><strong>简体中文</strong> · <a href="history.html">English</a></p>

<p class="eyebrow">History</p>

# 开发历程

本快照截止到提交 `284ab00`，时间为 2026-10-18 12:05（UTC+8）。后续开发不改变本页统计。

<div class="tiles">
<div class="tile accent"><div class="k">main 上的提交</div><div class="v">209</div></div>
<div class="tile"><div class="k">时间跨度</div><div class="v">42<small>小时</small></div></div>
<div class="tile"><div class="k">开发阶段</div><div class="v">7</div></div>
<div class="tile"><div class="k">首个提交</div><div class="v" style="font-size:20px">10-16 18:02</div></div>
</div>

约 42 小时是自然时间跨度，不代表开发工时或 Sprint 实际用时。

## 阶段与结果

{% include "./_partials/commits-zh.html" %}

| 阶段 | 时间 | 提交数 | 结果 |
|---|---|---|---|
| 1 需求 | 10-16 晚 | 6 | 产品范围、BLE 协议草案，P0 收窄至教练语音 |
| 2 语音链路 | 10-17 上午 | 10 | 状态机、音频编解码、语音与 BLE 集成、品牌界面、播放起播修正 |
| 3 滑行数据 | 10-17 中午 | 9 | LIVE_STATS、每趟报告、实时数据页、报告页和 SHOW_PAGE |
| 4 页面循环与弯型 | 10-17 傍晚 | 4 | 平级页面导航、弯型页及分组请求 |
| 5 弹窗与开始滑行 | 10-17 晚 | 8 | 手机弹窗、IQ 范围 0–200、动画徽章和 RUN_START |
| 6 页面导轨设计 | 10-18 上午 | 18 | 导轨需求、模拟器行为、自测和预览 |
| 7 导轨固件与连接状态 | 10-18 10:55–12:05 | 149 | 共享控件绑定、五屏连接状态、导轨固件、28 KiB LVGL 内存池、25 张渲染界面图 |
| 报告编制 | 10-18 11:00–11:23 | 5 | 初版技术报告，未计入阶段 7 |
| 合计 | | 209 | 七个开发阶段及报告编制 |

各阶段统计的是提交数，包含生成的素材，不能用于比较工程工作量。

## 交互演变

- P0 收窄至教练语音链路，数据报告、弯型和弹窗随后按独立增量交付。
- D-29 用页面循环替代滑行和休息模式，D-30 加入第四个页面：弯型页。
- D-33 加入页面导轨，评审后移除方向箭头，并统一所有格子的颜色。
- D-34 在四个页面及弹窗页加入统一连接状态栏。

项目负责人的完成确认及验证边界见 [Sprint 完成情况](sprints.zh_CN.html)。
