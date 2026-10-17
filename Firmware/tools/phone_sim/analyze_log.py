#!/usr/bin/env python3
"""analyze_log - turn a firmware monitor log into Sprint 1 test numbers.

It reads the lines that the Coach task and the voice task write, and prints one row
for each question, then a summary for the TC-06 and TC-07 limits.

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

CAPTURE_STOP_LIMIT_MS = 100    # TC-06
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


FIELDS = ["start_ms", "trigger", "end_event", "hold_ms", "blocks", "audio_ms", "peak",
          "capture_stop_ms", "answer_start_ms", "play_ms", "playback_stop_ms", "outcome"]


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("log")
    ap.add_argument("--csv")
    args = ap.parse_args(argv)
    with open(args.log, encoding="utf-8", errors="replace") as f:
        questions = parse(f)
    print(" | ".join(FIELDS))
    for q in questions:
        print(" | ".join(str(q.get(k, "")) for k in FIELDS))
    print()
    for line in summary(questions):
        print(line)
    if args.csv:
        with open(args.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=FIELDS, extrasaction="ignore")
            w.writeheader()
            w.writerows(questions)
    return 0


if __name__ == "__main__":
    sys.exit(main())
