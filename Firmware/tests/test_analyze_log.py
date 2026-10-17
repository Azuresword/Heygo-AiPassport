#!/usr/bin/env python3
"""Check tools/phone_sim/analyze_log.py with lines from the first device test (2026-10-17)."""

import os
import sys
import unittest

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "tools", "phone_sim"))
import analyze_log as al  # noqa: E402

# Lines from the monitor log of build 11b10cc. NimBLE lines stay in, as in a real log.
LOG = """\
I (72674) coach: SKI --OK_HOLD--> READY
I (75544) coach: READY --OK_HOLD--> CAPTURE
I (75546) NimBLE: GATT procedure initiated: notify;
I (78149) coach: CAPTURE --OK_RELEASE--> WAIT_ANSWER
I (78154) voice: capture: 134 blocks (2680 ms), peak 9114 of 32767
I (78779) coach: WAIT_ANSWER --ANSWER_START--> PLAYBACK
I (81438) coach: PLAYBACK --ANSWER_END--> READY
I (443324) coach: READY --OK_HOLD--> CAPTURE
I (443444) coach: CAPTURE --OK_RELEASE--> WAIT_ANSWER
I (443449) voice: capture: 10 blocks (200 ms), peak 1234 of 32767
I (450729) coach: WAIT_ANSWER --ANSWER_FAILED--> READY
I (478379) coach: READY --OK_HOLD--> CAPTURE
I (480789) coach: CAPTURE --OK_RELEASE--> WAIT_ANSWER
I (480799) voice: capture: 125 blocks (2500 ms), peak 10985 of 32767
I (491692) coach: WAIT_ANSWER --AGENT_TIMEOUT--> READY
I (500000) coach: READY --OK_HOLD--> CAPTURE
I (500100) coach: CAPTURE --OK_RELEASE--> WAIT_ANSWER
I (500104) voice: capture stop: 4 ms
I (500105) voice: capture: 0 blocks (0 ms), peak 0 of 32767
I (500600) coach: WAIT_ANSWER --ANSWER_START--> PLAYBACK
I (501000) coach: PLAYBACK --UP_HOLD--> READY
I (501001) voice: playback stop: 21 ms
I (510000) coach: READY --OK_HOLD--> CAPTURE
I (511000) coach: CAPTURE --UP_HOLD--> READY
I (511006) voice: capture stop: 6 ms
"""


class AnalyzeTest(unittest.TestCase):
    def setUp(self):
        self.q = al.parse(LOG.splitlines())

    def test_one_row_for_each_question(self):
        self.assertEqual(len(self.q), 5)

    def test_first_question(self):
        q = self.q[0]
        self.assertEqual(q["hold_ms"], 2605)
        self.assertEqual(q["blocks"], 134)
        self.assertEqual(q["peak"], 9114)
        self.assertEqual(q["answer_start_ms"], 630)
        self.assertEqual(q["play_ms"], 2659)
        self.assertEqual(q["outcome"], "ANSWER_END")

    def test_outcomes(self):
        self.assertEqual([q["outcome"] for q in self.q], ["ANSWER_END", "ANSWER_FAILED", "AGENT_TIMEOUT", "UP_HOLD", "UP_HOLD"])

    def test_summary_flags_faults(self):
        text = "\n".join(al.summary(self.q))
        self.assertIn("questions with 0 blocks: 1", text)
        self.assertIn("questions with peak below 300: 1", text)
        self.assertIn("capture stop: max 6 ms, 0 over 100 ms", text)
        self.assertIn("playback stop: max 21 ms + up to 90 ms DMA = 111 ms, 0 over 200 ms", text)


if __name__ == "__main__":
    unittest.main(verbosity=1)
