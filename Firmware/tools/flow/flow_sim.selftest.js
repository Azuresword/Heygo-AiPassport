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

  ok(st === "SKI" && document.querySelector(".scr.on").dataset.id === "ski", "1 starts in SKI on the ski screen");

  await hold("OK");
  ok(st === "READY", "2 an OK hold enters READY");
  ok(!mic, "2 the press that enters Rest mode does not start capture (D-15)");
  ok(shown("coach.status") === "已连接", "2 status shows the link");

  press("OK", key("OK")); await sleep(600);
  ok(st === "CAPTURE" && mic, "3 an OK hold starts capture");
  ok(shown("coach.state") === "正在听", "3 the screen shows 正在听");
  ok(widgetEls["coach.icon"].classList.contains("icon-listening"), "3 the listening state has its own icon");
  await sleep(1000); release("OK"); await sleep(50);
  ok(st === "WAIT_ANSWER" && !mic, "3 release stops capture");
  ok(/AGENT_END.*reason=0/.test(bleLog()), "3 AGENT_END has reason 0");
  await sleep(1000);
  ok(shown("coach.body") === "上一趟我的转弯怎么样？", "4 the transcript shows");
  await sleep(1500);
  ok(st === "PLAYBACK" && spk, "4 the answer plays");
  await sleep(6000);
  ok(st === "READY" && !spk, "4 playback ends in READY");
  ok(shown("coach.body").startsWith("重心有点偏后"), "4 the answer text stays after playback (ui_show_done)");

  press("OK", key("OK")); await sleep(20600);
  ok(st === "WAIT_ANSWER" && !mic, "5 capture stops at the 20-second limit");
  ok(shown("coach.body") === "已到 20 秒上限", "5 the screen shows the limit message");
  ok(/AGENT_END.*reason=1/.test(bleLog()), "5 AGENT_END has reason 1");
  release("OK"); await sleep(50);
  ok(st === "WAIT_ANSWER", "5 the late release changes nothing");
  await sleep(2600);
  ok(st === "PLAYBACK", "6 the answer plays");
  await hold("UP");
  ok(st === "READY" && !spk, "6 an UP hold stops playback");
  ok(/AGENT_CANCEL/.test(bleLog()), "6 the device sends AGENT_CANCEL");
  await sleep(8000);
  ok(st === "READY", "6 no canceled answer resumes");

  document.getElementById("scenario").value = "silent";
  await hold("OK", 1200);
  ok(st === "WAIT_ANSWER", "7 the question ends");
  await sleep(10200);
  ok(st === "READY" && shown("coach.body") === "超时，请重试", "7 no answer gives a timeout after 10 seconds");

  document.getElementById("scenario").value = "fail";
  await hold("OK", 1200);
  await sleep(2000);
  ok(st === "READY" && shown("coach.body") === "没听清，请重说", "8 a failed recognition asks again");

  document.getElementById("scenario").value = "normal";
  await hold("OK", 1200); await sleep(2600);
  ok(st === "PLAYBACK", "9 the answer plays");
  press("OK", key("OK")); await sleep(600);
  ok(st === "CAPTURE" && mic && !spk, "9 an OK hold in playback stops the speaker before capture");
  release("OK"); await sleep(50);
  await hold("UP");

  setLink(false);
  ok(st === "READY" && widgetEls["coach.status"].style.color === "rgb(239, 68, 68)", "10 link loss keeps READY and shows the status in the danger color");
  ok(shown("coach.status") === "未连接", "10 status shows 未连接");
  ok(widgetEls["coach.icon"].classList.contains("icon-link"), "10 link loss has its own icon");
  await hold("OK");
  ok(st === "READY" && !mic, "10 without the link, an OK hold starts no capture (ASK_BLOCKED)");
  ok(shown("coach.state") === "失败" && shown("coach.body") === "手机未连接", "10 the screen names the cause");
  ok(!document.getElementById("issues").textContent.includes("蓝牙断开时长按 OK"), "10 the missing-guard issue no longer appears");
  await hold("UP");
  setLink(true);

  await hold("DOWN");
  ok(st === "EXIT_Q", "11 a DOWN hold opens the exit question");
  await tap("UP");
  ok(st === "READY", "11 another key returns to READY");
  await hold("DOWN"); await tap("OK");
  ok(st === "SKI", "11 a short OK returns to Ski mode");

  press("UP", key("UP")); press("OK", key("OK")); await sleep(600); release("OK"); release("UP"); await sleep(50);
  ok(document.getElementById("toast").textContent.includes("一次只能按一个键"), "12 a second key is refused");

  ok(!both, "13 capture and playback never run together");
  const pre = document.createElement("pre");
  pre.id = "selftest";
  pre.textContent = out.join("\n") + `\nRESULT ${fails ? "FAIL" : "PASS"} ${out.length - fails}/${out.length}`;
  document.body.appendChild(pre);
})();
