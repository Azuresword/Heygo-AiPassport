<p class="lang"><strong>简体中文</strong> · <a href="scrum.html">English</a></p>

<p class="eyebrow">Scrum</p>

# 用 Scrum 落地

本项目把 Scrum 压成 4 小时一个 Sprint。每个 Sprint 只有一个目标。完成与否看证据，不看感觉。

本章按项目实际的做法梳理 Scrum 流程，每一步都给出仓库里的出处。

<div class="tiles">
<div class="tile accent"><div class="k">Sprint</div><div class="v">18</div></div>
<div class="tile"><div class="k">每个 Sprint</div><div class="v">4<small>小时</small></div></div>
<div class="tile"><div class="k">用户故事</div><div class="v">25</div></div>
<div class="tile"><div class="k">需求条目</div><div class="v">25</div></div>
<div class="tile"><div class="k">决策</div><div class="v">34</div></div>
</div>

## 1. 全景

<figure>
<img src="images/scrum-zh.svg" alt="本项目的 Scrum 流程">
<figcaption>图 13　Scrum 流程。上面一行是交付主线，下面一行是变更回路。来源：<code>需求文件夹/03-scrum-backlog.zh_CN.md</code>、<code>08-sprint-roadmap.zh_CN.md</code>。</figcaption>
</figure>

整个流程有两条线：

1. **交付主线。** 产品目标决定待办列表。待办列表的条目排进 Sprint。每个 Sprint 交付一个增量和一份证据。
2. **变更回路。** 用户的反馈先写成一条决策（D-xx），再改需求文档，最后重排待办列表。

## 2. 产品目标和待办列表

### 产品目标

> 滑雪者滑行时在设备上看实时数据，休息时用语音向 AI 教练提问并听到回答。手机留在口袋里，手套不用摘。

来源：`03-scrum-backlog.zh_CN.md` 第 1 节

### 四层追溯

每项功能都能从需求一路追到 Sprint。四层编号放在四份文档里：

| 层 | 编号 | 文档 | 例子：页面导轨 |
|---|---|---|---|
| 需求 | REQ-xx | `01-product-requirements` | REQ-24 显示用户在页面循环中的位置 |
| 用户故事 | US-xx | `03-scrum-backlog` | US-24 侧边条，故事点 2–3 |
| 验收用例 | TC-xx | `04-validation-and-decisions` | TC-24 40 次翻页，滑块每次 200 ms 内到位 |
| Sprint | 0–17 | `08-sprint-roadmap`、`03` | Sprint 16 模拟器、固件和验收 |

这样做的作用：评审一个增量时，可以直接查到它对应哪条需求、用哪个用例验收。

### 故事点的用法

- 故事点比较不确定性和工作量，不代表小时数。
- 团队没有速度数据，所以计划不假定速度。
- P0 范围在 0.4 版缩小后，5 个 P0 故事标为"待重新估算"。

来源：`03-scrum-backlog.zh_CN.md` 第 2、4 节

## 3. 4 小时 Sprint

### 固定结构

<div class="figure-panel">
<div class="timebox"><div class="f15">计划 15</div><div class="f200 dev">开发 200 分钟</div><div class="f15">评审 15</div><div class="f10">回顾</div></div>
</div>

| 事件 | 时长 | 产出 |
|---|---|---|
| Sprint 计划 | 15 分钟 | 确认 Sprint 目标和任务。检查前提条件 |
| 开发 | 200 分钟 | 可用的增量，通过本 Sprint 的完成检查 |
| Sprint 评审 | 15 分钟 | 演示增量，按固定格式报告证据 |
| Sprint 回顾 | 10 分钟 | 记录一项改进，在下一个 Sprint 执行 |

### 四条规则

1. 前提条件不满足时，Sprint 不开始。团队改做下一个前提条件已满足的 Sprint。
2. 开发超时，就按砍线去掉任务。去掉的任务回到待办列表。
3. Sprint 目标没有达成时，由产品负责人决定下一步。团队不延长 Sprint。
4. 每个 Sprint 结束时，`./tools/validate.sh` 的退出码必须是 0。

来源：`08-sprint-roadmap.zh_CN.md` 第 1 节

### 例子：Sprint 0

| 顺序 | 任务 | 时间盒 | 完成检查 |
|---|---|---|---|
| 1 | T-1 模式与音频状态机 | 60 分钟 | 每条转移各有一项主机测试 |
| 2 | T-2 P0 消息的包层 | 75 分钟 | 主机测试覆盖组包、解包、`xid` 和错误码 |
| 3 | T-3 IMA ADPCM 编解码 | 60 分钟 | 主机测试覆盖块大小、往返、丢块和满幅输入 |
| 4 | T-4 门禁与构建 | 45 分钟 | 门禁和构建通过，报告写明静态 RAM |

**砍线：** T-1 和 T-2 合计超过 150 分钟时，去掉 T-3。T-4 一定要做。

**为什么可以砍 T-3：** 07 号协议有裸 PCM 回退。没有 ADPCM，链路照样能测。

**结果：** 四项任务都在提交 `a917aaa` 交付，没有执行砍线。按完成定义，Sprint 0 不关闭任何故事，因为每个 P0 故事还需要设备测试和真实的手机应用。

来源：`06-sprint-0-plan.zh_CN.md` 第 2、4、7 节

## 4. 完成定义和证据

### 完成定义

每个增量切片都要满足下面适用的检查：

- 用户在实体 AI Passport 上完成操作。
- 切片需要时，使用真实的手机应用。
- 主机测试、仓库检查和固件构建全部通过。
- 设备测试在记录的条件下通过。
- 中文显示、按键动作、模式切换和取消行为通过各自的测试。
- 时延满足切片的限制。
- 中英文文档描述相同的行为。
- 团队记录每一项剩余缺口。

**关键约束：** 模拟器或桩程序不能代替设备测试，也不能代替真实服务测试。

来源：`03-scrum-backlog.zh_CN.md` 第 9 节

### 证据格式

每次 Sprint 评审都用这个格式报告：

```text
Sprint: N
Sprint Goal: 达成 / 未达成
Build: PASS / FAIL / NOT RUN
Host tests: PASS / FAIL / NOT RUN
Device tests: PASS / FAIL / PARTIAL / NOT RUN（写明手机是真实应用还是 phone_sim）
Unverified: 每一项未验证的内容
```

来源：`08-sprint-roadmap.zh_CN.md` 第 5 节

这个格式把"构建通过""主机通过""设备通过"分成三栏。Sprint 0 的计划把一条风险写得很直接："纯主机的结果被当成产品结果，状态报告失真。"证据格式就是这条风险的对策（`06-sprint-0-plan.zh_CN.md` 第 6 节）。

### 测试条件

| 条件 | 内容 |
|---|---|
| N1 室内参考链路 | 一台 AI Passport、一部手机、真实的手机应用。往返时延、抖动、丢包率取第 95 百分位 |
| N2 故障 | 逐项测：链路中断 30 秒、应用切到后台、权限失败、MTU 低于 172、时钟偏移 5 分钟 |
| U1 可用性 | 5 名成年人，徒手和戴手套各做一遍同一份任务脚本，过程中不提供帮助 |

来源：`04-validation-and-decisions.zh_CN.md` 第 2 节

## 5. 代码 Sprint 和验收 Sprint 分开

每个故事都有两个 Sprint：一个写代码，一个验收。

| 故事 | 代码 Sprint | 验收 Sprint |
|---|---|---|
| US-06 语音采集、US-07 语音播放 | 0–1 | 6 |
| US-08 按键 Agent 交流 | 1 | 8 |
| US-15 手套操作与中文显示 | 1 | 9 |
| US-02 数据同步 | 2 | 7 |
| US-21 实时数据页 | 3 | 7、9 |
| US-03 每趟报告 | 4 | 7 |
| US-05 弯型 | 11 | 12 |
| US-22 手机弹窗 | 13 | 14 |

来源：`03-scrum-backlog.zh_CN.md` 第 4 节

这样分的原因：

1. 开发板当时不在手边。代码 Sprint 只在主机上就能完成，不用等硬件。
2. 验收 Sprint 有自己的前提条件：开发板、支持协议 0.2 的手机应用、5 名测试者。前提条件不满足，验收 Sprint 就不开始。
3. 代码和验收分开记录，状态就不会混。"代码已完成"和"验收通过"是两件事。

## 6. 变更怎样进入 Sprint

### 四步变更流程

1. 记录反馈和受影响的需求。
2. 产品负责人重新为待办列表排序。
3. 开发人员更新估算。
4. 把每项已接受的变更记入决策记录。

来源：`03-scrum-backlog.zh_CN.md` 第 10 节

34 条决策都按这个流程进入项目。其中 6 条后来被替代，5 条被同一条 D-29（四页平级循环）替代。决策的理由写在 `05-decision-amendments`。

### 实例：D-33 和 D-34 的落地顺序

<figure>
<img src="images/change-zh.svg" alt="D-33 和 D-34 的落地时间线">
<figcaption>图 14　一条决策变成固件的过程。空心圆是用户评审后的修改。来源：<code>git log</code>。</figcaption>
</figure>

两条决策都是先改文档，再改代码，最后重绘界面图：

1. **先改文档。** 01–05 号需求文档中英成对更新。
2. **再改代码。** D-33 先做模拟器（10:47），用户评审后才写固件（11:49）。D-34 的流程图、固件和模拟器在同一批提交里完成（11:22）。
3. **用户评审随时插入。** D-33 改了两轮。第一轮（11:39）去掉箭头，在固件之前。第二轮（12:05）改成同一种颜色，在固件之后，只改了 `page_rail.c`（`ea4815a`）。
4. **最后重绘界面图。** `host_render` 重绘 25 张界面图，让文档里的图和固件一致。

| 决策 | 文档 | 固件 | 用时 |
|---|---|---|---|
| D-33 页面导轨 | 10:45 | 11:49 | 64 分钟，含一轮评审 |
| D-34 链路状态 | 11:08 | 11:22 | 14 分钟 |

用时是首尾提交的时间差，不是实测的工作时间。

## 7. 支撑 Scrum 的工程手段

Scrum 规定了节奏。下面这些工程手段让 4 小时的 Sprint 真正跑得起来：

<div class="cards">
<div class="card"><span class="tag">单一来源</span><h4>一个流程图</h4><p><code>coach.flow.json</code> 同时生成固件的转移表和浏览器模拟器。改一处，两边一起变。</p></div>
<div class="card"><span class="tag">评审</span><h4>在线模拟器</h4><p>模拟器发布在 GitHub Pages。评审一个增量，打开网页就能按键试，不需要开发板。</p></div>
<div class="card"><span class="tag">完成定义</span><h4>门禁自动检查</h4><p><code>validate.sh</code> 跑主机测试、字形、界面检查、模拟器自测和固件构建。完成定义里能自动查的部分都交给它。</p></div>
<div class="card"><span class="tag">增量</span><h4>一个增量一个分支</h4><p><code>feat/report-live-data</code>、<code>feat/turn-view</code>、<code>feat/app-popups</code>、<code>feat/run-start</code>。增量完成后合入 <code>main</code>。</p></div>
<div class="card"><span class="tag">文档</span><h4>中英成对，遵循 asd-ste100</h4><p>D-06 规定所有文档中英成对。英文用 <code>ste-lint.py</code> 检查。手机应用团队读同一套协议文档。</p></div>
<div class="card"><span class="tag">测试</span><h4>纯 C 主机测试</h4><p>状态机、协议、ADPCM 都是纯 C。33 个主机测试在没有硬件时也能证明逻辑。</p></div>
</div>

### 模拟器跟着 Sprint 一起长

每个和界面有关的 Sprint 都有一项"更新流程模拟器"的任务，所以自测项数随增量增加：

{% include "./_partials/selftest-zh.html" %}

来源：`08-sprint-roadmap.zh_CN.md` 第 3、4 节的 Sprint 结果，`Firmware/docs/heygo-changes.zh_CN.md` 第 12、13 节，本书在 `c865ba8` 和 `284ab00` 上的实测。

## 8. 角色

| 角色 | 文档里的人员 | 实际情况 |
|---|---|---|
| 产品负责人 | 未指定 | 所有范围决策都由用户做出，包括 D-17 |
| Scrum Master | 未指定 | — |
| 开发人员 | 未指定 | 一名固件开发人员，手机应用团队并行开发 |

来源：`03-scrum-backlog.zh_CN.md` 第 2、3 节

## 9. 回头看：哪些有效，哪些可以改进

### 有效的做法

- **4 小时时间盒配砍线。** 超时就砍任务，不延长 Sprint。Sprint 0 和 Sprint 2 都定了砍线，结果都写着"没有执行砍线"。
- **证据分栏。** 构建、主机、设备、未验证四栏分开写，主机结果不会被当成产品结果。
- **决策记录。** 34 条决策都有编号、状态和理由，替代关系也写清楚了。
- **先模拟器后固件。** 用户先在浏览器里评审外观。D-33 的第一轮修改（去掉箭头）发生在写固件之前，固件不用为这一轮返工。

### 可以改进的地方

| 缺口 | 证据 | 建议 |
|---|---|---|
| 没有记录实际用时 | Sprint 0、2、3、4、11、13、15 的结果都写明没有记录实际用时 | 每个 Sprint 记录开始和结束时间，积累速度数据 |
| 回顾的改进项没有书面记录 | 文档里找不到回顾记录 | 每次回顾写一行改进项，放进下一个 Sprint 的计划 |
| 路线图状态列没有同步 | `08-sprint-roadmap` 里 Sprint 5–15 仍写"未开始" | Sprint 评审后同步状态列 |
| 角色没有指定 | `03` 第 3 节三个角色都写"未指定" | 写明产品负责人，排序和验收就有明确的人负责 |
| 故事点待重新估算 | 5 个 P0 故事写"待重新估算" | 下一次计划会上补齐 |
