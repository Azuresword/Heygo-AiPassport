#!/usr/bin/env python3
"""Check tools/phone_sim/heygo_link.py byte for byte against the firmware C code.

The test compiles main/protocol/ble_proto.c and main/audio/ima_adpcm.c into one
shared library and calls it with ctypes. It needs no BLE hardware.
"""

import ctypes
import os
import random
import subprocess
import sys
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "phone_sim"))
import heygo_link as hl  # noqa: E402


def build_lib():
    out = os.path.join(tempfile.mkdtemp(), "libheygo.so")
    cc = os.environ.get("CC", "cc")
    subprocess.run([cc, "-shared", "-fPIC", "-O1", "-std=c11", "-I", os.path.join(ROOT, "main", "protocol"),
                    "-I", os.path.join(ROOT, "main", "audio"),
                    os.path.join(ROOT, "main", "protocol", "ble_proto.c"),
                    os.path.join(ROOT, "main", "audio", "ima_adpcm.c"), "-o", out], check=True)
    return ctypes.CDLL(out)


LIB = build_lib()
U8P = ctypes.POINTER(ctypes.c_uint8)


def c_build(fn, *args):
    buf = (ctypes.c_uint8 * 256)()
    n = getattr(LIB, fn)(buf, ctypes.c_size_t(256), *args)
    return bytes(buf[:n])


class PacketTest(unittest.TestCase):
    def test_hello_ack(self):
        c = c_build("bp_build_hello_ack", ctypes.c_uint8(0), ctypes.c_uint8(2), ctypes.c_uint32(1760000000),
                    ctypes.c_char_p(b"phone_sim"))
        self.assertEqual(hl.hello_ack(1760000000, "phone_sim"), c)

    def test_phone_status(self):
        c = c_build("bp_build_phone_status", ctypes.c_uint32(1760000123), ctypes.c_bool(True),
                    ctypes.c_bool(False), ctypes.c_bool(True))
        self.assertEqual(hl.phone_status(1760000123, True, False, True), c)

    def test_agent_state(self):
        c = c_build("bp_build_agent_state", ctypes.c_uint8(0x05), ctypes.c_uint8(4), ctypes.c_uint8(0),
                    ctypes.c_char_p("回答中".encode()))
        self.assertEqual(hl.agent_state(0x05, 4, 0, "回答中"), c)

    def test_play_start_and_end(self):
        c = c_build("bp_build_play_start", ctypes.c_uint8(0x05), ctypes.c_uint8(0), ctypes.c_uint8(0),
                    ctypes.c_char_p(None))
        self.assertEqual(hl.play_start(0x05), c)
        self.assertEqual(hl.play_end(0x05), c_build("bp_build_play_end", ctypes.c_uint8(0x05)))

    def test_audio_down(self):
        block = bytes(range(164))
        cblock = (ctypes.c_uint8 * 164)(*block)
        c = c_build("bp_build_audio", ctypes.c_uint8(hl.AUDIO_DOWN), ctypes.c_uint8(0x05), ctypes.c_uint8(7),
                    cblock, ctypes.c_size_t(164))
        self.assertEqual(hl.audio_down(0x05, 7, block), c)
        self.assertEqual(len(c), 168)

    def test_read_hello(self):
        c = c_build("bp_build_hello", ctypes.c_uint8(1), ctypes.c_uint16(247), ctypes.c_char_p(b"heygo 0.2"))
        _, _, _, _, payload = hl.parse(c)
        self.assertEqual(hl.read_hello(payload), {"major": 0, "minor": 2, "codec_mask": 1, "mtu": 247, "fw": "heygo 0.2"})

    def test_live_stats(self):
        class Live(ctypes.Structure):
            _fields_ = [("run_state", ctypes.c_uint8), ("run_no", ctypes.c_uint8), ("speed_dkmh", ctypes.c_uint16),
                        ("drop_m", ctypes.c_uint16), ("distance_m", ctypes.c_uint16), ("turns", ctypes.c_uint16),
                        ("time_s", ctypes.c_uint16)]
        for vals in [(1, 4, 383, 312, 1850, 24, 151), (3, 0xFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF)]:
            c = c_build("bp_build_live_stats", ctypes.byref(Live(*vals)))
            self.assertEqual(hl.live_stats(*vals), c)
            self.assertEqual(len(c), 16)

    def test_run_report(self):
        for vals in [(4, 1850, 24, 82, "重心偏后", "下一趟把双手放在身体前方"), (1, 0xFFFF, 0xFFFF, 0xFF, "", ""),
                     (2, 900, 12, 200, "", ""), (3, 900, 12, 127, "", "")]:
            run_no, dist, turns, iq, problem, tip = vals
            c = c_build("bp_build_run_report", ctypes.c_uint8(run_no), ctypes.c_uint16(dist), ctypes.c_uint16(turns),
                        ctypes.c_uint8(iq), ctypes.c_char_p(problem.encode()), ctypes.c_char_p(tip.encode()))
            self.assertEqual(hl.run_report(*vals), c)
        with self.assertRaises(ValueError):
            hl.run_report(1, 0, 0, 50, tip="雪" * 21)
        with self.assertRaises(ValueError):
            hl.run_report(1, 0, 0, 201)

    def test_show_page(self):
        self.assertEqual(hl.show_page(), c_build("bp_build_show_page", ctypes.c_uint8(3)))
        with self.assertRaises(ValueError):
            hl.show_page(1)

    def test_turn_shape(self):
        class Turn(ctypes.Structure):
            _fields_ = [(n, ctypes.c_uint8) for n in ("first", "last", "side", "speed", "edge", "radius")]
        for n_turns, src in [(255, hl.SRC_MEASURED), (5, hl.SRC_SCHEMATIC), (0, hl.SRC_MEASURED)]:
            c = c_build("bp_build_turn_shape", ctypes.c_uint8(4), ctypes.c_uint8(src), ctypes.c_uint8(n_turns))
            self.assertEqual(hl.turn_shape(4, src, n_turns), c)
        # Each group of a 44-turn run, and of the largest run.
        for n_turns in (44, 255, 3):
            for g in range((n_turns + 7) // 8):
                turns, pts = hl.demo_turn_group(n_turns, g)
                self.assertEqual(len(turns), min(8, n_turns - 8 * g))
                ctr = (Turn * len(turns))(*[Turn(*t) for t in turns])
                flat = (ctypes.c_uint8 * (2 * len(pts)))(*[v for p in pts for v in p])
                c = c_build("bp_build_turn_group", ctypes.c_uint8(4), ctypes.c_uint8(g), ctypes.c_uint8(len(turns)),
                            ctr, ctypes.c_uint8(len(pts)), flat)
                self.assertTrue(c, "the C builder accepts the demo group")
                self.assertEqual(hl.turn_group(4, g, turns, pts), c)
                self.assertLessEqual(len(c) + 3, 172, "a group fits the MTU of voice")
        turns, pts = hl.demo_turn_group(44, 1)
        self.assertEqual(turns[0][2], hl.SIDE_TOE)
        self.assertTrue(min(x for x, _ in pts) < 5 and max(x for x, _ in pts) > 250, "a group fills the side axis")
        self.assertEqual((pts[0][1], pts[-1][1]), (0, 255), "a group fills the downhill axis")
        c = c_build("bp_build_turn_request", ctypes.c_uint8(4), ctypes.c_uint8(2), ctypes.c_uint8(3))
        self.assertEqual(hl.turn_request(4, 2, 3), c)
        self.assertEqual(hl.read_turn_request(hl.parse(c)[4]), (4, 2, 3))
        with self.assertRaises(ValueError):
            hl.turn_shape(1, 0, 256)
        with self.assertRaises(ValueError):
            hl.turn_group(1, 0, [(0, 1, 0, 1, 91, 1)], [(0, 0), (1, 1)])
        with self.assertRaises(ValueError):
            hl.demo_turn_group(8, 1)

    def test_short_packet(self):
        with self.assertRaises(ValueError):
            hl.parse(b"\x01\x00")


class CodecTest(unittest.TestCase):
    def c_encode_stream(self, samples):
        class State(ctypes.Structure):
            _fields_ = [("predictor", ctypes.c_int16), ("index", ctypes.c_uint8)]
        st = State()
        LIB.ima_state_init(ctypes.byref(st))
        blocks = []
        for i in range(0, len(samples), 320):
            chunk = samples[i:i + 320]
            pcm = (ctypes.c_int16 * 320)(*chunk)
            out = (ctypes.c_uint8 * 164)()
            LIB.ima_encode_block(ctypes.byref(st), pcm, ctypes.c_size_t(len(chunk)), out)
            blocks.append(bytes(out))
        return blocks

    def signal(self, n, seed=7):
        rnd = random.Random(seed)
        out, v = [], 0
        for i in range(n):
            v = int(0.9 * v + rnd.randint(-6000, 6000))
            out.append(max(-32768, min(32767, v + (20000 if (i // 40) % 2 else -20000) * (i % 1000 < 50))))
        return out

    def test_encode_matches_c(self):
        samples = self.signal(320 * 12 + 77)    # A short last block, filled with silence.
        self.assertEqual(hl.encode_stream(samples), self.c_encode_stream(samples))

    def test_decode_matches_c(self):
        for block in self.c_encode_stream(self.signal(320 * 5, seed=3)):
            pcm = (ctypes.c_int16 * 320)()
            n = LIB.ima_decode_block((ctypes.c_uint8 * 164)(*block), pcm)
            self.assertEqual(n, 320)
            self.assertEqual(hl.decode_block(block), list(pcm))

    def test_bad_block(self):
        bad = bytearray(164)
        bad[2] = 89
        with self.assertRaises(ValueError):
            hl.decode_block(bytes(bad))


if __name__ == "__main__":
    unittest.main(verbosity=1)
