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

  // D-29: three flat pages in a loop. DOWN goes forward, UP goes back.
  await tap("DOWN"); ok(st === "REPORT" && on() === "report", "2 DOWN: live data -> report");
  await tap("DOWN"); ok(st === "READY" && on() === "coach", "2 DOWN: report -> coach");
  await tap("DOWN"); ok(st === "LIVE", "2 DOWN: coach -> live data (loop)");
  await tap("UP");   ok(st === "READY", "2 UP: live data -> coach (loop)");
  await tap("UP");   ok(st === "REPORT", "2 UP: coach -> report");
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
  ok(st === "REPORT" && /AGENT_CANCEL/.test(bleLog().split("AGENT_END").pop()), "8 a short UP in the wait cancels the question and moves back one page");
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
