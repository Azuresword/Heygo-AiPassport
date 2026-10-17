#!/usr/bin/env python3
"""flow_sim - make an interactive HTML simulator from a flow graph.

The page shows the device screen, the three buttons, the state diagram, the
trigger matrix, and the BLE messages. It reads all states, transitions, screen
layouts, and screen texts from the flow graph and assets/ui/strings.json, so it
cannot disagree with the generated firmware code.

Usage:
  python3 tools/flow/flow_sim.py [FLOW] [--out FILE] [--check]

  FLOW     Flow graph. Default: flows/coach.flow.json.
  --out    Output file. Default: ../assets/sim/<app>-flow.html.
  --check  Do not write. Exit 1 when the output file differs from a new build.
  --selftest
           Add flow_sim.selftest.js to the page. Use it only for test builds.
  --run-selftest
           Build a test page, run it in headless Chrome, and report the result.
           Without Chrome the step reports SKIPPED.
"""

import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
FW = HERE.parent.parent
REPO = FW.parent
sys.path.insert(0, str(HERE))
import flowc  # noqa: E402

HOLD_MS = 500  # D-15 and document 01: a hold starts at 500 ms.


def theme_colors():
    text = (FW / "main" / "ui" / "ui_theme.h").read_text()
    found = dict(re.findall(r"#define UI_HEX_(\w+)\s+0x([0-9A-Fa-f]{6})", text))
    return {k.lower(): "#" + v.upper() for k, v in found.items()}


def check_effects(flow, strings):
    errors = []
    for aid, effect in flow.get("ui_effects", {}).items():
        if aid == "comment":
            continue
        for sid, value in effect.items():
            key = value.get("key") if isinstance(value, dict) else (
                value if isinstance(value, str) else None
            )
            if key and key not in strings:
                errors.append(f"ui_effects {aid}.{sid}: unknown string key {key!r}")
    return errors


def static_issues(flow):
    issues = []
    table = {(t["from"], t["on"]) for t in flow["transitions"]}
    busy = [s for s in ("CAPTURE", "WAIT_ANSWER", "PLAYBACK") if (s, "DOWN_HOLD") not in table]
    if busy:
        issues.append("在 " + "、".join(busy) + " 状态下长按 DOWN 会被忽略。"
                      "这是固件侧的选择，需求没有定义这种情况。")
    issues.append("手机事件到状态机事件的对应关系（PLAY_START → ANSWER_START 等）是模拟器的假设。"
                  "固件里连接这两层的代码还没有写。")
    return issues


def build(flow_path, selftest=False):
    flow = json.loads(flow_path.read_text(encoding="utf-8"))
    flowc.validate(flow, str(flow_path))
    strings = {k: v["zh"] for k, v in json.loads(
        (REPO / "assets" / "ui" / "strings.json").read_text(encoding="utf-8"))["strings"].items()}
    errors = check_effects(flow, strings)
    if errors:
        sys.exit("ERROR:\n  " + "\n  ".join(errors))
    colors = theme_colors()
    data = {"flow": flow, "strings": strings, "colors": colors, "hold_ms": HOLD_MS,
            "static_issues": static_issues(flow)}
    blob = json.dumps(data, ensure_ascii=False, separators=(",", ":")).replace("</", "<\\/")
    rel = flow_path.resolve().relative_to(FW).as_posix()
    sha = hashlib.sha256(flow_path.read_bytes()).hexdigest()[:12]
    counts = f"{len(flow['states'])} 个状态 · {len(flow['transitions'])} 条转换"
    html = (HERE / "flow_sim.template.html").read_text(encoding="utf-8")
    for key, value in {
        "__TITLE__": f"{flow.get('title', flow['app'])} · 交互模拟器",
        "__SOURCE__": f"Firmware/{rel}", "__COUNTS__": counts, "__SHA__": sha,
        "__HOLD__": str(HOLD_MS), "__ACCENT__": colors["accent"], "__DANGER__": colors["danger"],
        "__OK__": colors["ok"], "__BG__": colors["bg"], "__INK__": colors["ink"],
        "__MUTED__": colors["muted"], "__SURFACE__": colors["surface"],
        "__SURFACE_HI__": colors["surface_hi"], "__DATA__": blob,
    }.items():
        html = html.replace(key, value)
    if selftest:
        js = (HERE / "flow_sim.selftest.js").read_text(encoding="utf-8")
        html = html.replace("</body>", f"<script>\n{js}</script>\n</body>")
    return flow["app"], html


CHROME_PATHS = ["/Applications/Google Chrome.app/Contents/MacOS/Google Chrome",
                "google-chrome", "chromium", "chromium-browser"]


def run_selftest(flow_path):
    chrome = next((c for c in CHROME_PATHS if shutil.which(c) or Path(c).exists()), None)
    if not chrome:
        print("flow simulator self-test: SKIPPED (no Chrome)")
        return 0
    _, html = build(flow_path, selftest=True)
    with tempfile.TemporaryDirectory() as tmp:
        page = Path(tmp) / "sim.html"
        page.write_text(html, encoding="utf-8")
        # Virtual time runs the 60 seconds of timers in the test at once.
        r = subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-first-run",
                            f"--user-data-dir={tmp}/profile", "--virtual-time-budget=120000",
                            "--dump-dom", page.as_uri()], capture_output=True, text=True, timeout=120)
    m = re.search(r'<pre id="selftest">(.*?)</pre>', r.stdout, re.S)
    if not m:
        print("flow simulator self-test: FAIL (no result in the page)")
        return 1
    lines = m.group(1).replace("&gt;", ">").replace("&lt;", "<").replace("&amp;", "&").splitlines()
    for line in lines:
        if line.startswith("FAIL"):
            print("  " + line)
    print("flow simulator self-test: " + lines[-1].replace("RESULT ", ""))
    return 0 if lines[-1].startswith("RESULT PASS") else 1


def main(argv=None):
    ap = argparse.ArgumentParser()
    ap.add_argument("flow", nargs="?", type=Path, default=FW / "flows" / "coach.flow.json")
    ap.add_argument("--out", type=Path)
    ap.add_argument("--check", action="store_true")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--run-selftest", action="store_true")
    args = ap.parse_args(argv)
    if args.run_selftest:
        return run_selftest(args.flow)
    app, html = build(args.flow, args.selftest)
    out = args.out or REPO / "assets" / "sim" / f"{app}-flow.html"
    if args.check:
        if not out.exists() or out.read_text(encoding="utf-8") != html:
            print(f"flow simulator: STALE ({out.name}). Run: python3 tools/flow/flow_sim.py")
            return 1
        print(f"flow simulator: PASS ({out.name} matches the flow graph)")
        return 0
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(html, encoding="utf-8")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
