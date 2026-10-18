<p class="lang"><strong>简体中文</strong> · <a href="product.html">English</a></p>

<p class="eyebrow">Product</p>

# 产品与页面

## 谁在用，什么时候用

一趟滑完后，坐缆车或休息时用。手机留在口袋里，开着蓝牙。用户不摘手套，只按三个实体键：UP、OK、DOWN。

来源：`需求文件夹/01-product-requirements.zh_CN.md:12-26`

## 四个页面，一个循环

四个页面平级。短按 DOWN 去下一页，短按 UP 回上一页，到头会绕回来（D-29、D-30）。任何页面长按 OK 都进教练页。

<figure>
<div class="figure-panel">
<div class="loop">
<div class="screen"><img src="../assets/mockups/10-ski-live.png" alt="实时数据页"><div class="cap">实时数据</div><div class="sub">LIVE RUN</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/14-report.png" alt="本趟报告页"><div class="cap">本趟报告</div><div class="sub">RUN REPORT</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/18-turns-heel.png" alt="弯型页"><div class="cap">弯型</div><div class="sub">TURN SHAPE</div></div>
<div class="arrow"><b>▶</b>DOWN</div>
<div class="screen"><img src="../assets/mockups/02-ready.png" alt="AI 教练页"><div class="cap">AI 教练</div><div class="sub">AI COACH</div></div>
</div>
<div class="loop-back">◀ 短按 DOWN 从教练页绕回实时数据页 · 短按 UP 反向</div>
</div>
<figcaption>图 1　四页循环。截图由 LVGL 在主机上渲染，来自 <code>assets/mockups/</code>。</figcaption>
</figure>

| 页面 | 显示什么 | 决策 |
|---|---|---|
| 实时数据 | 速度、落差、距离、转弯、时间。手机每 1 秒发一帧 | D-23 |
| 本趟报告 | Heygo IQ（0–200）、核心问题、一条小建议。每趟一份 | D-26、D-27 |
| 弯型 | 这一趟的线路，一页 8 个弯，逐弯显示速度、角度、半径 | D-30 |
| AI 教练 | 按住 OK 说话，松开发送，设备播放回答 | D-17 |

## 弯型页怎么看

弯型页用三个键逐弯浏览：OK 选下一弯，长按 UP 选上一弯，长按 DOWN 去下一页。设备存 3 组弯，不够时用 TURN_REQUEST 向手机要。

<figure>
<div class="figure-panel">
<div class="screens big">
<div class="screen"><img src="../assets/sim/turns-demo.gif" alt="弯型页动画"><div class="cap">逐弯浏览</div><div class="sub">turns-demo.gif</div></div>
</div>
</div>
<figcaption>图 2　弯型页动画，来自 <code>assets/sim/turns-demo.gif</code>。</figcaption>
</figure>

## 手机能主动做的两件事

<div class="cards">
<div class="card"><span class="tag">D-31 · POPUP 0x66</span><h4>弹窗</h4><p>立即打断任何状态。先播语音，再倒计时 10 秒。任意键关闭，回到弹窗前的页面。新弹窗替换旧弹窗。</p></div>
<div class="card"><span class="tag">D-32 · RUN_START 0x68</span><h4>开始滑行</h4><p>手机判定开始滑行，设备打开实时数据页。对话中或弹窗中，等结束后再打开。不取消任何操作。</p></div>
<div class="card"><span class="tag">D-33</span><h4>页面导轨</h4><p>屏幕右侧一条导轨，显示自己在循环中的位置。荧光绿滑块是当前页，▲▼ 标出 UP、DOWN 会去哪。</p></div>
</div>

<figure>
<div class="figure-panel">
<div class="screens">
<div class="screen"><img src="../assets/mockups/popup-anim.gif" alt="弹窗动画"><div class="cap">弹窗入场</div><div class="sub">popup-anim.gif</div></div>
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="摔倒提醒"><div class="cap">摔倒提醒</div><div class="sub">FALL ALERT</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="成就报告"><div class="cap">成就报告</div><div class="sub">ACHIEVED</div></div>
</div>
</div>
<figcaption>图 3　手机弹窗。每类弹窗有自己的颜色和动画徽章。</figcaption>
</figure>

来源：`需求文件夹/04-validation-and-decisions.zh_CN.md` D-29 至 D-33 行
