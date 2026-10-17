#!/usr/bin/env python3
"""analyze_log - turn a firmware monitor log into device test numbers.

It reads the lines that the Coach task and the voice task write, and prints one row
for each question, then a summary for the TC-06 and TC-07 limits.

It also reads the ski data lines. For each LIVE_STATS, RUN_REPORT, and SHOW_PAGE,
it finds the first screen change after the packet and gives the time between them.
The summary compares the 95th percentile with the 300 ms limit of TC-21.

Usage:
  python3 tools/phone_sim/analyze_log.py monitor.log [--csv out.csv]
  idf.py monitor | tee monitor.log      # How to keep a log.
"""

import argparse
import csv
import re
import sys

LINE = re.compile(r"^[IWE] \((\d+)\) (\w+): (.*)$")
STEP = re.compile(r"^(\w+) --(\w+)--> (\w+)$")
CAPTURE = re.compile(r"^capture: (\d+) blocks \((\d+) ms\), peak (\d+)")
STOP = re.compile(r"^(capture|playback) stop: (\d+) ms")

RX = re.compile(r"^rx (LIVE_STATS|RUN_REPORT)\b(.*)$")
PAGE_ASK = re.compile(r"^phone asks for page (\d+)")
SCREEN = re.compile(r'^(ski|report)\.(\w+) = "(.*)"$|^show (\w+) screen$')

CAPTURE_STOP_LIMIT_MS = 100    # TC-06
SCREEN_UPDATE_LIMIT_MS = 300   # TC-21 and TC-03
PLAYBACK_STOP_LIMIT_MS = 200   # TC-07
DMA_TAIL_MS = 90               # Sound that the I2S DMA still holds after the stop.


def parse(lines):
    """Return a list of questions. Each question is a dict."""
    questions, cur = [], None
    for raw in lines:
        m = LINE.match(raw.strip())
        if not m:
            continue
        t, tag, msg = int(m.group(1)), m.group(2), m.group(3)
        if tag == "coach":
            s = STEP.match(msg)
            if not s:
                continue
            frm, ev, to = s.groups()
            if to == "CAPTURE":
                cur = {"start_ms": t, "trigger": f"{frm}/{ev}", "outcome": "", "blocks": None}
                questions.append(cur)
            elif cur is None:
                continue
            elif frm == "CAPTURE":
                cur["end_event"] = ev
                cur["hold_ms"] = t - cur["start_ms"]
                cur["release_ms"] = t
                if to == "READY":   # Canceled or lost during capture.
                    cur["outcome"] = ev
            elif frm == "WAIT_ANSWER" and to == "PLAYBACK":
                cur["answer_start_ms"] = t - cur.get("release_ms", t)
            elif frm in ("WAIT_ANSWER", "PLAYBACK") and to == "READY":
                cur["outcome"] = ev
                if frm == "PLAYBACK":
                    cur["play_ms"] = t - cur["release_ms"] - cur.get("answer_start_ms", 0)
                if ev == "UP_HOLD" and frm == "PLAYBACK":
                    cur["cancel_ms"] = t
        elif tag == "voice":
            c = CAPTURE.match(msg)
            if c and cur is not None:
                cur["blocks"], cur["audio_ms"], cur["peak"] = (int(x) for x in c.groups())
                continue
            st = STOP.match(msg)
            if st and cur is not None:
                cur[f"{st.group(1)}_stop_ms"] = int(st.group(2))
    return questions


def summary(questions):
    out = []
    caps = [q["capture_stop_ms"] for q in questions if "capture_stop_ms" in q]
    plays = [q["playback_stop_ms"] for q in questions if "playback_stop_ms" in q]
    empty = [q for q in questions if q.get("blocks") == 0]
    quiet = [q for q in questions if q.get("peak") is not None and q["peak"] < 300]
    out.append(f"questions: {len(questions)}")
    if caps:
        bad = sum(v > CAPTURE_STOP_LIMIT_MS for v in caps)
        out.append(f"capture stop: max {max(caps)} ms, {bad} over {CAPTURE_STOP_LIMIT_MS} ms (TC-06)")
    if plays:
        worst = max(plays) + DMA_TAIL_MS
        bad = sum(v + DMA_TAIL_MS > PLAYBACK_STOP_LIMIT_MS for v in plays)
        out.append(f"playback stop: max {max(plays)} ms + up to {DMA_TAIL_MS} ms DMA = {worst} ms, "
                   f"{bad} over {PLAYBACK_STOP_LIMIT_MS} ms (TC-07)")
    out.append(f"questions with 0 blocks: {len(empty)}")
    out.append(f"questions with peak below 300: {len(quiet)}")
    outcomes = {}
    for q in questions:
        outcomes[q["outcome"] or "(open)"] = outcomes.get(q["outcome"] or "(open)", 0) + 1
    out.append("outcomes: " + ", ".join(f"{k} {v}" for k, v in sorted(outcomes.items())))
    return out


def parse_data(lines):
    """Return one dict for each ski data packet: its kind, receive time, and screen time.

    The screen time is the first screen line after the packet and before the next packet.
    It stays None when the screen did not change, for example on another page.
    """
    packets = []
    for raw in lines:
        m = LINE.match(raw.strip())
        if not m:
            continue
        t, tag, msg = int(m.group(1)), m.group(2), m.group(3)
        kind = None
        if tag == "ble_link":
            r = RX.match(msg)
            if r:
                kind = r.group(1)
            elif PAGE_ASK.match(msg):
                kind = "SHOW_PAGE"
        if kind:
            packets.append({"kind": kind, "rx_ms": t, "screen_ms": None, "first": ""})
            continue
        if tag == "screen" and packets and packets[-1]["screen_ms"] is None:
            s = SCREEN.match(msg)
            if s:
                packets[-1]["screen_ms"] = t
                packets[-1]["first"] = f"{s.group(1)}.{s.group(2)}" if s.group(1) else f"show {s.group(4)}"
    for p in packets:
        p["delay_ms"] = None if p["screen_ms"] is None else p["screen_ms"] - p["rx_ms"]
    return packets


def p95(values):
    """The 95th percentile as doc 04 defines it: sort, then take item ceil(0.95 n)."""
    s = sorted(values)
    return s[max(0, -(-95 * len(s) // 100) - 1)]


def summary_data(packets):
    out = []
    for kind in ("LIVE_STATS", "RUN_REPORT", "SHOW_PAGE"):
        group = [p for p in packets if p["kind"] == kind]
        if not group:
            continue
        delays = [p["delay_ms"] for p in group if p["delay_ms"] is not None]
        line = f"{kind}: {len(group)} packets, {len(group) - len(delays)} with no screen change"
        if delays:
            worst = p95(delays)
            verdict = "PASS" if worst <= SCREEN_UPDATE_LIMIT_MS else "FAIL"
            line += f", screen delay p95 {worst} ms, max {max(delays)} ms ({verdict} against {SCREEN_UPDATE_LIMIT_MS} ms)"
        out.append(line)
    return out


FIELDS = ["start_ms", "trigger", "end_event", "hold_ms", "blocks", "audio_ms", "peak",
          "capture_stop_ms", "answer_start_ms", "play_ms", "playback_stop_ms", "outcome"]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log")
    ap.add_argument("--csv")
    args = ap.parse_args(argv)
    with open(args.log, encoding="utf-8", errors="replace") as f:
        lines = f.readlines()
    questions = parse(lines)
    packets = parse_data(lines)
    print(" | ".join(FIELDS))
    for q in questions:
        print(" | ".join(str(q.get(k, "")) for k in FIELDS))
    print()
    for line in summary(questions):
        print(line)
    if packets:
        print()
        print("ski data: kind | rx_ms | screen_ms | delay_ms | first screen change")
        for p in packets:
            print(f"{p['kind']} | {p['rx_ms']} | {p['screen_ms']} | {p['delay_ms']} | {p['first']}")
        print()
        for line in summary_data(packets):
            print(line)
    if args.csv:
        with open(args.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=FIELDS, extrasaction="ignore")
            w.writeheader()
            w.writerows(questions)
    return 0


if __name__ == "__main__":
    sys.exit(main())
