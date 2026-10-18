<p class="lang"><strong>简体中文</strong> · <a href="followups.html">English</a></p>

<p class="eyebrow">Follow-up</p>

# 后续事项

项目已经交付。下面是交付范围之外、或者有意不做的事，供下一阶段参考。

## 有意不做的（已接受的风险）

来源：`需求文件夹/04-validation-and-decisions.zh_CN.md` 风险表

<div class="cards">
<div class="card"><span class="tag">D-12</span><h4>蓝牙不加密</h4><p>附近的人能听到音频。为了降低连接复杂度，有意不配对。</p></div>
<div class="card"><span class="tag">D-18</span><h4>没有功耗管理</h4><p>这次交付不做省电策略。</p></div>
<div class="card"><span class="tag">D-31</span><h4>摔倒弹窗 10 秒后关闭</h4><p>升级处理由手机负责。POPUP_END 原因 0 告诉手机：没人按键。</p></div>
<div class="card"><span class="tag">D-32</span><h4>开始滑行由手机判定</h4><p>手机误判时设备会跳页，但不取消任何操作。在实时数据页短按 DOWN 就能回报告页。</p></div>
</div>

## 文档里记录的设计限制

来源：`Firmware/docs/heygo-changes.zh_CN.md:329-331`

| 限制 | 说明 |
|---|---|
| 屏幕不显示电量 | 电量通过 DEVICE_STATUS 发给手机 |
| 录音时到达的 PLAY_START 会丢 | D-20 规定手机不能在这时发送 |
| 正文最多四行 | 更长的文字以三个点结尾，语音会播完整回答 |

## 下一阶段可以做的

| 方向 | 依据 |
|---|---|
| 队伍对讲 | 协议已定义 0x50–0x55，属于 P1 |
| P1、P2 需求 | REQ-04、09、13（P1），REQ-10、11、12、18、19（P2） |
| 文档同步 | `08-sprint-roadmap` 和 `04` 号文档里 Sprint 5–16、D-33 的状态写法，与"全部完成"对齐 |
| CI | 把 workflow 移到仓库根目录的 `.github/workflows/` |
