<p class="lang"><strong>简体中文</strong> · <a href="product.html">English</a></p>

<p class="eyebrow">Product</p>

# 产品与页面

## 使用场景

设备用于滑行结束、乘坐缆车或休息时查看数据和提问。手机保持蓝牙连接，放在口袋内。用户通过 UP、OK、DOWN 三个实体键操作。

## 四个页面，一个循环

四个页面平级循环。短按 DOWN 切换到下一页，短按 UP 切换到上一页。在实时数据、报告或弯型页长按 OK，进入教练页。在教练页再次长按 OK，开始录音；松开 OK 后发送。

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
<figcaption>图 1　四页循环与按键方向。</figcaption>
</figure>

| 页面 | 内容 |
|---|---|
| 实时数据 | 速度、落差、距离、转弯、时间。手机每 1 秒发一帧 |
| 本趟报告 | Heygo IQ（0–200）、核心问题、一条小建议。每趟一份 |
| 弯型 | 这一趟的线路，一页 8 个弯，逐弯显示速度、角度、半径 |
| AI 教练 | 按住 OK 说话，松开发送，设备播放回答 |

## 弯型浏览

短按 OK 选择下一弯，长按 UP 选择上一弯，长按 DOWN 切换下一组。每组最多 8 个弯。设备缓存 3 组，通过 TURN_REQUEST 向手机请求缺失的组。

<figure>
<div class="figure-panel">
<div class="screens big">
<div class="screen"><img src="../assets/sim/turns-demo.gif" alt="弯型页动画"><div class="cap">逐弯浏览</div><div class="sub">8 弯一组 · 按需加载</div></div>
</div>
</div>
<figcaption>图 2　弯型选择与分组切换。</figcaption>
</figure>

## 手机事件

<div class="cards">
<div class="card"><span class="tag">POPUP 0x66</span><h4>弹窗</h4><p>弹窗立即显示，并取消正在进行的教练对话。语音结束后倒计时 10 秒，无语音时直接倒计时。任意键关闭后返回原页面，教练页返回就绪状态。新弹窗替换旧弹窗。</p></div>
<div class="card"><span class="tag">RUN_START 0x68</span><h4>开始滑行</h4><p>手机检测到开始滑行后，通知设备显示实时数据页。对话或弹窗尚未结束时，设备延后切页。</p></div>
</div>

## 导航与链路反馈

<div class="cards">
<div class="card"><span class="tag">导航</span><h4>页面导轨</h4><p>右侧导轨用四个位置表示页面顺序，荧光绿滑块标记当前页。切页动画持续 180 ms。弹窗期间隐藏导轨。</p></div>
<div class="card"><span class="tag">连接</span><h4>每页显示链路状态</h4><p>五个屏幕共用一个状态栏：页面圆点、页面名、连接圆点和连接文字。连接文字是"已连接"、"未连接"或"无语音"。</p></div>
</div>

<figure>
<div class="figure-panel">
<div class="screens">
<div class="screen"><img src="../assets/mockups/popup-anim.gif" alt="弹窗动画"><div class="cap">弹窗入场</div><div class="sub">语音结束后倒计时</div></div>
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="摔倒提醒"><div class="cap">摔倒提醒</div><div class="sub">FALL ALERT</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="成就报告"><div class="cap">成就报告</div><div class="sub">ACHIEVED</div></div>
</div>
</div>
<figcaption>图 3　手机弹窗。每类弹窗有自己的颜色和动画徽章。</figcaption>
</figure>
