<p class="lang"><strong>简体中文</strong> · <a href="history.html">English</a></p>

<p class="eyebrow">History</p>

# 开发历程

<div class="tiles">
<div class="tile accent"><div class="k">main 上的提交</div><div class="v">55</div></div>
<div class="tile"><div class="k">跨度</div><div class="v">41<small>小时</small></div></div>
<div class="tile"><div class="k">阶段</div><div class="v">6</div></div>
<div class="tile"><div class="k">首个提交</div><div class="v" style="font-size:20px">10-16 18:02</div></div>
</div>

## 每个阶段的提交数

{% include "./_partials/commits-zh.html" %}

**表格视图**

| 阶段 | 时间 | 提交 | 范围 |
|---|---|---|---|
| 1 需求 | 10-16 晚 | 6 | `f431fe6` … `cfa30fe` |
| 2 语音链路 | 10-17 上午 | 10 | … `344c8e5` |
| 3 滑行数据 | 10-17 中午 | 9 | … `c996e14` |
| 4 四页 + 弯型 | 10-17 傍晚 | 4 | … `49573f9` |
| 5 弹窗 + 开始滑行 | 10-17 晚 | 8 | … `5658911` |
| 6 页面导轨 | 10-18 上午 | 18 | … `c865ba8` |
| 合计 | | 55 | |


来源：`git rev-list --count <范围>`，按上表的提交边界统计。

## 时间线

<div class="timeline">
<div class="ev"><div class="when">10-16 · 18:02</div><div class="what">需求阶段</div><div class="how">起始 <code>f431fe6</code>，对齐八项决策 <code>6f5b6af</code>，BLE 协议草案 <code>970bb8e</code>，P0 收窄 <code>48926ff</code>。</div></div>
<div class="ev"><div class="when">10-17 · 上午</div><div class="what">Sprint 0–1：语音链路跑通</div><div class="how">固件入库 406 个文件 <code>a917aaa</code>，语音和 BLE 接通 <code>941f4e7</code>，Heygo 品牌界面 <code>4ff7f13</code>，修音频起播 <code>344c8e5</code>。</div></div>
<div class="ev"><div class="when">10-17 · 中午到下午</div><div class="what">增量 2：滑行数据与报告</div><div class="how">Sprint 2 数据层 <code>d371c1d</code>，Sprint 3–4 实时数据页和报告页，D-28 SHOW_PAGE <code>9238eaf</code>。</div></div>
<div class="ev"><div class="when">10-17 · 傍晚</div><div class="what">四页循环和弯型页</div><div class="how">D-29 四页平级循环，D-30 弯型页 <code>49573f9</code>。</div></div>
<div class="ev"><div class="when">10-17 · 晚上</div><div class="what">弹窗和开始滑行</div><div class="how">D-31 手机弹窗 <code>93b6498</code>，IQ 改 0–200 <code>816f043</code>，弹窗动画徽章 <code>2234275</code>，D-32 开始滑行 <code>5658911</code>。</div></div>
<div class="ev"><div class="when">10-18 · 上午</div><div class="what">页面导轨 D-33</div><div class="how">01–05 号文档加入 REQ-24、US-24、TC-24 <code>eef9c65</code> 起；模拟器显示导轨 <code>30ba3c7</code>，自测覆盖 <code>258f6fa</code>，预览图 <code>c865ba8</code>。</div></div>
<div class="ev"><div class="when">10-18</div><div class="what">17 个 Sprint 全部完成</div><div class="how">项目负责人确认。开发板和真机部分在线下设备上完成。</div></div>
</div>

作者：Dylan Li 52 个提交，另 3 个用本机账户提交（`git log --format=%an`）。仓库没有打标签。
