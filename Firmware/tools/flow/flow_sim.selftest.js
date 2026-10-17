// Self-test for the flow simulator. flow_sim.py adds this file only with --selftest.
// Headless Chrome runs it with a virtual time budget and reads the result from the DOM.
(async () => {
  const out = [];
  let fails = 0;
  const sleep = ms => new Promise(r => setTimeout(r, ms));
  const ok = (cond, msg) => { out.push((cond ? "PASS " : "FAIL ") + msg); if (!cond) fails++; };
  const key = k => document.querySelector(`.key[data-key="${k}"]`);
  const hold = async (k, ms = 600) => { press(k, key(k)); await sleep(ms); release(k); await sleep(50); };
  const tap = async k => { press(k, key(k)); await sleep(120); release(k); await sleep(50); };
  const shown = id => widgetEls[id].textContent;
  const fsmLog = () => document.getElementById("fsmLog").textContent;
  const bleLog = () => document.getElementById("bleLog").textContent;

  // Check the audio rule after every transition.
  const realFire = fire;
  let both = false;
  fire = (ev, src) => { const r = realFire(ev, src); if (mic && spk) both = true; return r; };

  const on = () => document.querySelector(".scr.on").dataset.id;
  ok(st === "LIVE" && on() === "ski", "1 starts on the live data page (D-29)");

  // D-29 and D-30: four flat pages in a loop. DOWN goes forward, UP goes back.
  await tap("DOWN"); ok(st === "REPORT" && on() === "report", "2 DOWN: live data -> report");
  await tap("DOWN"); ok(st === "TURNS" && on() === "turns", "2 DOWN: report -> turns (D-30)");
  await tap("DOWN"); ok(st === "READY" && on() === "coach", "2 DOWN: turns -> coach");
  await tap("DOWN"); ok(st === "LIVE", "2 DOWN: coach -> live data (loop)");
  await tap("UP");   ok(st === "READY", "2 UP: live data -> coach (loop)");
  await tap("UP");   ok(st === "TURNS", "2 UP: coach -> turns");
  await tap("UP");   ok(st === "REPORT", "2 UP: turns -> report");
  await tap("OK");   ok(st === "REPORT", "2 a short OK changes no page");
  await hold("UP");  ok(st === "REPORT", "2 an UP hold does nothing");

  // An OK hold opens the Coach page from every page. That press never starts capture.
  await hold("OK");
  ok(st === "READY" && !mic, "3 an OK hold on the Report page opens the Coach page, with no capture");
  await tap("DOWN"); await hold("OK");
  ok(st === "READY" && !mic, "3 an OK hold on the live data page opens the Coach page, with no capture");
  ok(shown("coach.status") === "已连接", "3 status shows the link");

  press("OK", key("OK")); await sleep(600);
  ok(st === "CAPTURE" && mic, "4 an OK hold on the Coach page starts capture");
  ok(shown("coach.state") === "正在听", "4 the screen shows 正在听");
  await sleep(1000); release("OK"); await sleep(50);
  ok(st === "WAIT_ANSWER" && !mic, "4 release stops capture");
  ok(/AGENT_END.*reason=0/.test(bleLog()), "4 AGENT_END has reason 0");
  await sleep(1000);
  ok(shown("coach.body") === "上一趟我的转弯怎么样？", "5 the transcript shows");
  await sleep(1500);
  ok(st === "PLAYBACK" && spk, "5 the answer plays");
  await sleep(6000);
  ok(st === "READY" && !spk, "5 playback ends in READY");
  ok(shown("coach.body").startsWith("重心有点偏后"), "5 the answer text stays after playback (ui_show_done)");

  press("OK", key("OK")); await sleep(20600);
  ok(st === "WAIT_ANSWER" && !mic, "6 capture stops at the 20-second limit");
  ok(shown("coach.body") === "已到 20 秒上限", "6 the screen shows the limit message");
  ok(/AGENT_END.*reason=1/.test(bleLog()), "6 AGENT_END has reason 1");
  release("OK"); await sleep(50);
  ok(st === "WAIT_ANSWER", "6 the late release changes nothing");
  await sleep(2600);
  ok(st === "PLAYBACK", "7 the answer plays");
  await tap("DOWN");
  ok(st === "LIVE" && !spk, "7 a short DOWN in playback stops the speaker and moves to the next page");
  ok(/AGENT_CANCEL/.test(bleLog()), "7 the device sends AGENT_CANCEL");
  await sleep(8000);
  ok(st === "LIVE", "7 no canceled answer moves the page back");

  await hold("OK");
  document.getElementById("scenario").value = "silent";
  await hold("OK", 1200);
  ok(st === "WAIT_ANSWER", "8 the question ends");
  await tap("UP");
  ok(st === "TURNS" && /AGENT_CANCEL/.test(bleLog().split("AGENT_END").pop()), "8 a short UP in the wait cancels the question and moves back one page");
  await hold("OK"); await hold("OK", 1200);
  await sleep(10200);
  ok(st === "READY" && shown("coach.body") === "超时，请重试", "8 no answer gives a timeout after 10 seconds");

  document.getElementById("scenario").value = "fail";
  await hold("OK", 1200);
  await sleep(2000);
  ok(st === "READY" && shown("coach.body") === "没听清，请重说", "9 a failed recognition asks again");

  document.getElementById("scenario").value = "normal";
  await hold("OK", 1200); await sleep(2600);
  ok(st === "PLAYBACK", "10 the answer plays");
  press("OK", key("OK")); await sleep(600);
  ok(st === "CAPTURE" && mic && !spk, "10 an OK hold in playback stops the speaker before capture");
  release("OK"); await sleep(50);
  await tap("UP"); await hold("OK");

  setLink(false);
  ok(st === "READY" && widgetEls["coach.status"].style.color === "rgb(239, 68, 68)", "11 link loss keeps READY and shows the status in the danger color");
  ok(shown("coach.status") === "未连接", "11 status shows 未连接");
  await hold("OK");
  ok(st === "READY" && !mic, "11 without the link, an OK hold starts no capture (ASK_BLOCKED)");
  ok(shown("coach.state") === "失败" && shown("coach.body") === "手机未连接", "11 the screen names the cause");
  setLink(true);

  press("UP", key("UP")); press("OK", key("OK")); await sleep(600); release("OK"); release("UP"); await sleep(50);
  ok(document.getElementById("toast").textContent.includes("一次只能按一个键"), "12 a second key is refused");

  ok(!both, "13 capture and playback never run together");

  // Increment 2: ski data. Doc 07 section 6.
  await tap("DOWN");
  ok(st === "LIVE", "14 back on the live data page");
  ok(shown("ski.sub") === "等待手机数据" && shown("ski.big") === "--" && shown("ski.drop") === "--", "14 the live data page waits for data");
  skiStart(); await sleep(2100);
  ok(/^\d+$/.test(shown("ski.big")) && shown("ski.dest") === "LIVE RUN", "14 LIVE_STATS shows the speed");
  ok(shown("ski.sub").startsWith("第 1 趟") && shown("ski.time").startsWith("00:0") && /m$/.test(shown("ski.dist")), "14 each number sits in its own labeled cell");
  ok(st === "LIVE", "14 data does not change the state");
  document.getElementById("noSpeed").checked = true; await sleep(1100);
  ok(shown("ski.big") === "--", "14 an unknown speed shows -- and never 0");
  document.getElementById("noSpeed").checked = false;
  // The screen refreshes once a second, so the mark shows between 5 s and 6 s.
  skiPaused = true; await sleep(6200);
  ok(shown("ski.big") === "--" && shown("ski.sub").includes("数据已过期"), "15 5 s with no packet marks the data out of date");
  skiPaused = false; await sleep(1100);
  ok(!shown("ski.sub").includes("数据已过期"), "15 a new packet clears the mark");
  skiEnd(); await sleep(100);
  ok(shown("ski.dest") === "RUN ENDED" && shown("ski.sub").includes("报告生成中"), "16 the end packet waits for the report");
  await sleep(2100);
  ok(shown("ski.dest") === "REPORT" && shown("ski.unit") === "HEYGO IQ" && shown("ski.problem") === "重心偏后" && widgetEls["ski.drop"].style.display === "none", "16 the report replaces the metric cells on the live data page");
  await tap("DOWN");
  ok(st === "REPORT" && shown("report.problem") === "重心偏后" && shown("report.title").startsWith("第 1 趟"), "17 the Report page shows the run report");

  // D-30: the Turns page between the Report page and the Coach page. The device asks for groups of 8 turns.
  await sleep(1700);   // The App sends TURN_SHAPE 1.5 s after RUN_REPORT.
  ok(/TURN_SHAPE run=1/.test(bleLog()) && /TURN_REQUEST run=1 第 1–1 组/.test(bleLog()) && /TURN_GROUP run=1 第 1 组/.test(bleLog()),
     "19 the shape follows the report, and the device asks for the first group");
  const nTurns = dev.n_turns, nPts = dev.slots.get(0).points.length;
  const pts = id => widgetEls["turns." + id].querySelector("polyline").getAttribute("points").split(" ").map(p => p.split(",").map(Number));
  const rider = () => widgetEls["turns.rider"];
  const keys = () => [shown("turns.key_up"), shown("turns.key_ok"), shown("turns.key_down")].join("|");
  await tap("DOWN");
  ok(st === "TURNS" && on() === "turns", "19 a short DOWN on the Report page opens the Turns page");
  await sleep(POINT_MS * 4);
  ok(turnDrawnPts > 1 && turnDrawnPts < nPts && pts("path").length === turnDrawnPts * 4, "19 the first page draws point by point");
  const head = pts("path")[pts("path").length - 1];
  ok(parseInt(rider().style.left) === 10 + head[0] - 10 && parseInt(rider().style.top) === 108 + head[1] - 10, "19 the rider sits at the head of the line");
  ok(keys() === "▲报告|●跳过|▼教练" && shown("turns.key_hold") === "长按●问教练", "19 while drawing: UP and DOWN change the page, OK skips");
  await tap("OK");
  ok(pts("path").length === nPts * 4 && shown("turns.counter") === `TURN 1/${nTurns}` && keys() === "▲报告|●下一弯|▼教练",
     "19 OK shows the whole page at once");
  ok(widgetEls["turns.track"].style.display === "none", "19 one page: no bar");
  const box = el => el.getBoundingClientRect();
  ok(box(widgetEls["turns.path"].querySelector("polyline")).width > 100 && box(widgetEls["turns.path"].querySelector("svg")).width === 240,
     "19 the browser draws the line");
  await sleep(1800);
  const end1 = pts("turn")[pts("turn").length - 1];
  ok(parseInt(rider().style.left) === 10 + end1[0] - 10 && /rotate\(/.test(rider().style.transform), "19 the rider replays turn 1 to its end");
  await tap("OK");
  ok(shown("turns.counter") === `TURN 2/${nTurns}` && shown("turns.side") === "HEEL SIDE" && shown("turns.key_hold") === "长按▲上一弯",
     "19 OK selects the next turn, and the hint names the UP hold");
  await hold("UP");
  ok(st === "TURNS" && shown("turns.counter") === `TURN 1/${nTurns}`, "19 an UP hold selects the previous turn");
  await hold("OK");
  ok(st === "READY" && !mic, "19 an OK hold on the Turns page opens the Coach page, with no capture");
  await tap("UP");
  ok(st === "TURNS" && turnSel === 0, "19 a short UP on the Coach page opens the Turns page at turn 1");

  // A long run: 44 turns make 6 pages. The device keeps 3 groups and asks for the others.
  phoneSendShape(1, 44, 0); await sleep(400);
  ok(/TURN_REQUEST run=1 第 1–3 组/.test(bleLog()) && [0, 1, 2].every(g => dev.slots.has(g)), "20 the device asks for groups 1 to 3 (24 turns)");
  await sleep(POINT_MS * 30);
  ok(shown("turns.title") === "第 1 趟 · 44 弯" && shown("turns.key_hold") === "长按▼下一页", "20 turn 1 of a long run: the DOWN hold goes one page on");
  ok(widgetEls["turns.thumb"].style.display !== "none" && parseInt(widgetEls["turns.thumb"].style.left) === 20 &&
     parseInt(widgetEls["turns.thumb"].style.width) === 33, "20 the bar marks page 1 of 6");
  for (let i = 0; i < 8; i++) await tap("OK");
  ok(shown("turns.counter") === "TURN 9/44" && parseInt(widgetEls["turns.thumb"].style.left) === 20 + 33 &&
     shown("turns.key_hold") === "长按 ▲上一弯 ▼下一页", "20 OK past turn 8 moves the page at once");
  await hold("UP");
  ok(shown("turns.counter") === "TURN 8/44" && parseInt(widgetEls["turns.thumb"].style.left) === 20, "20 an UP hold goes back across the page edge");
  await hold("DOWN"); await hold("DOWN"); await sleep(400);
  ok(shown("turns.counter") === "TURN 17/44" && /TURN_REQUEST run=1 第 4–4 组/.test(bleLog()) && !dev.slots.has(0),
     "20 two DOWN holds go to page 3, which drops group 1 and asks for group 4");
  await hold("DOWN"); await hold("DOWN"); await hold("DOWN"); await sleep(400);
  ok(shown("turns.counter") === "TURN 41/44" && [3, 4, 5].every(g => dev.slots.has(g)), "20 DOWN holds reach the last page");
  setLink(false);
  const before = bleLog().length;
  await hold("DOWN"); await sleep(300);
  ok(turnSel === 0 && shown("turns.msg") === "蓝牙断开" && bleLog().length === before, "20 the DOWN hold wraps to page 1. With no link, it names the link and asks for nothing");
  setLink(true); await sleep(400);
  ok(/TURN_REQUEST run=1 第 1–3 组/.test(bleLog().slice(before)) && shown("turns.msg") === "", "20 after the reconnect, the device asks again");
  // The App team request of 2026-10-17: the Turns page shows the run of the last report.
  deviceOnReport(9); await sleep(50);
  ok(shown("turns.title") === "第 9 趟" && shown("turns.msg") === "加载中" && widgetEls["turns.path"].style.display === "none",
     "21 a report of a new run clears the old shape, and the page waits for the new one");
  ok(keys() === "▲报告||▼教练" && shown("turns.key_hold") === "长按●问教练", "21 while it waits, OK and the holds have no turn action");
  setLink(false); await sleep(50);
  ok(shown("turns.msg") === "蓝牙断开", "21 with no link, the page names the link");
  setLink(true); await sleep(50);
  phoneSendShape(9, 0, 0); await sleep(50);
  ok(shown("turns.title") === "第 9 趟" && shown("turns.msg") === "暂无弯型数据", "21 a shape of 0 turns: no turn data, never 0 弯");
  phoneSendShape(9, 12, 0); await sleep(600);
  ok(shown("turns.title") === "第 9 趟 · 12 弯" && shown("turns.msg") === "", "21 a later shape of the same run gives the turns");
  await tap("UP");
  ok(st === "REPORT", "20 a short UP on the Turns page returns to the Report page");

  // D-28: SHOW_PAGE opens the Report page from LIVE and READY only.
  await tap("UP");
  showReport(); await sleep(50);
  ok(st === "REPORT", "18 SHOW_PAGE opens the Report page from the live data page");
  await hold("OK");
  showReport(); await sleep(50);
  ok(st === "REPORT", "18 SHOW_PAGE opens the Report page from the idle Coach page");
  await hold("OK");
  press("OK", key("OK")); await sleep(700);
  showReport(); await sleep(50);
  ok(st === "CAPTURE" && mic, "18 SHOW_PAGE during capture changes nothing");
  release("OK"); await sleep(3500);
  ok(st === "PLAYBACK" && spk, "18 the answer plays");
  showReport(); await sleep(50);
  ok(st === "PLAYBACK" && spk, "18 SHOW_PAGE during playback changes nothing");
  await tap("DOWN");
  const pre = document.createElement("pre");
  pre.id = "selftest";
  pre.textContent = out.join("\n") + `\nRESULT ${fails ? "FAIL" : "PASS"} ${out.length - fails}/${out.length}`;
  document.body.appendChild(pre);
})();
