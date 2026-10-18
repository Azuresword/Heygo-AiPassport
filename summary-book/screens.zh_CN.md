<p class="lang"><strong>简体中文</strong> · <a href="screens.html">English</a></p>

<p class="eyebrow">Screens</p>

# 界面一览

以下 25 张界面样例使用固件的 LVGL 界面代码在主机上渲染，覆盖正常、边界和异常状态。实际屏幕的色彩、刷新和显示效果需在设备上验证。

## 教练对话：一次提问的四个状态

<figure>
<div class="figure-panel">
<div class="loop">
<div class="screen"><img src="../assets/mockups/02-ready.png" alt="就绪"><div class="cap">就绪</div><div class="sub">READY</div></div>
<div class="arrow"><b>▶</b>按住 OK</div>
<div class="screen"><img src="../assets/mockups/03-listening.png" alt="正在听"><div class="cap">正在听</div><div class="sub">LISTENING</div></div>
<div class="arrow"><b>▶</b>松开</div>
<div class="screen"><img src="../assets/mockups/04-thinking.png" alt="思考中"><div class="cap">思考中</div><div class="sub">THINKING</div></div>
<div class="arrow"><b>▶</b>回答</div>
<div class="screen"><img src="../assets/mockups/05-answering.png" alt="回答中"><div class="cap">回答中</div><div class="sub">ANSWERING</div></div>
</div>
</div>
<figcaption>图 4　一次教练提问。对应的蓝牙消息见 <a href="architecture.zh_CN.html">架构与协议</a> 图 7。</figcaption>
</figure>

识别失败后可长按 OK 重试。连接中断时，需先恢复手机连接。

<div class="screens">
<div class="screen"><img src="../assets/mockups/07-failed.png" alt="没听清"><div class="cap">没听清</div><div class="sub">FAILED</div></div>
<div class="screen"><img src="../assets/mockups/08-link-lost.png" alt="手机未连接"><div class="cap">手机未连接</div><div class="sub">LINK LOST</div></div>
<div class="screen"><img src="../assets/mockups/06-answer-max.png" alt="长回答"><div class="cap">长回答</div><div class="sub">四行截断，语音播完整</div></div>
</div>

## 滑行数据和报告

<div class="screens">
<div class="screen"><img src="../assets/mockups/01-ski.png" alt="等待数据"><div class="cap">等待数据</div><div class="sub">还没开始滑</div></div>
<div class="screen"><img src="../assets/mockups/10-ski-live.png" alt="滑行中"><div class="cap">滑行中</div><div class="sub">第 4 趟 · 38 km/h</div></div>
<div class="screen"><img src="../assets/mockups/11-ski-stale.png" alt="数据已过期"><div class="cap">数据已过期</div><div class="sub">5 秒没收到</div></div>
<div class="screen"><img src="../assets/mockups/14-report.png" alt="本趟报告"><div class="cap">本趟报告</div><div class="sub">Heygo IQ 82</div></div>
<div class="screen"><img src="../assets/mockups/15-report-max.png" alt="报告上限"><div class="cap">上限值</div><div class="sub">IQ 200 · 136 弯</div></div>
</div>

## 弯型

<div class="screens">
<div class="screen"><img src="../assets/mockups/17-turns-drawing.png" alt="画线中"><div class="cap">画线中</div><div class="sub">TOE SIDE</div></div>
<div class="screen"><img src="../assets/mockups/18-turns-heel.png" alt="后刃弯"><div class="cap">后刃弯</div><div class="sub">HEEL SIDE</div></div>
<div class="screen"><img src="../assets/mockups/19-turns-schematic.png" alt="示意"><div class="cap">示意线路</div><div class="sub">255 弯</div></div>
<div class="screen"><img src="../assets/mockups/20-turns-loading.png" alt="加载中"><div class="cap">加载中</div><div class="sub">等 TURN_GROUP</div></div>
<div class="screen"><img src="../assets/mockups/21-turns-short.png" alt="短趟"><div class="cap">短趟</div><div class="sub">5 弯</div></div>
</div>

## 手机弹窗

<div class="screens">
<div class="screen"><img src="../assets/mockups/22-popup-fall.png" alt="摔倒提醒"><div class="cap">摔倒提醒</div><div class="sub">红色</div></div>
<div class="screen"><img src="../assets/mockups/23-popup-voice.png" alt="语音播报"><div class="cap">语音播报</div><div class="sub">绿色</div></div>
<div class="screen"><img src="../assets/mockups/24-popup-issue.png" alt="问题报告"><div class="cap">问题报告</div><div class="sub">橙色</div></div>
<div class="screen"><img src="../assets/mockups/25-popup-badge.png" alt="成就报告"><div class="cap">成就报告</div><div class="sub">黄色</div></div>
</div>

## 全部界面

<figure>
<div class="figure-panel"><img src="../assets/mockups/contact-sheet.png" alt="25 张界面总览"></div>
<figcaption>图 5　25 种界面状态总览。</figcaption>
</figure>
