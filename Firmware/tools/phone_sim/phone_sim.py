#!/usr/bin/env python3
"""phone_sim - a stand-in for the phone application, for device tests.

It connects to an AI Passport over BLE and speaks protocol 0.2 of doc 07.
It saves each question that the device sends as a WAV file, and then answers.
With --runs N, it also sends N simulated runs of ski data: LIVE_STATS once each
second during a run, then one RUN_REPORT after the run ends.

Answer modes:
  echo     Play the question back. This tests the full audio path. (Default.)
  wav      Play a WAV file: 16 kHz, mono, 16-bit. Give it with --wav.
  fail     Send AGENT_STATE failed, recognition. The device asks again.
  silent   Send nothing. The device shows a timeout after 10 seconds.

Usage:
  pip install bleak
  python3 tools/phone_sim/phone_sim.py [--mode echo|wav|fail|silent] [--wav FILE] [--name HeygoPP-XXXX]
                                      [--runs N] [--run-s SECONDS]

To make a WAV file on macOS:
  say -o /tmp/a.aiff "上一趟重心偏后" && afconvert -f WAVE -d LEI16@16000 -c 1 /tmp/a.aiff /tmp/a.wav
"""

import argparse
import asyncio
import os
import struct
import sys
import time
import wave

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import heygo_link as hl  # noqa: E402

STATUS_INTERVAL_S = 3.0      # Doc 07: at most every 5 s.
BLOCK_S = 0.020
AHEAD_BLOCKS = 4             # Doc 07: never more than 8 blocks ahead of playback.


def log(msg):
    print(time.strftime("%H:%M:%S"), msg, flush=True)


class Phone:
    def __init__(self, client, args):
        self.client = client
        self.args = args
        self.xid = 0
        self.up_blocks = []
        self.up_missing = 0
        self.expect_seq = 0
        self.t_end = 0.0
        self.answer_task = None
        self.canceled = set()
        self.acked = asyncio.Event()

    async def send(self, data):
        await self.client.write_gatt_char(hl.NUS_RX, data, response=False)

    def on_notify(self, _handle, data):
        try:
            ptype, _flags, xid, seq, payload = hl.parse(bytes(data))
        except ValueError as e:
            log(f"bad packet: {e}")
            return
        name = hl.TYPE_NAMES.get(ptype, f"0x{ptype:02x}")
        if ptype == hl.HELLO:
            h = hl.read_hello(payload)
            log(f"<- HELLO {h['major']}.{h['minor']} mask={h['codec_mask']} mtu={h['mtu']} fw={h['fw']!r}")
            asyncio.ensure_future(self.send(hl.hello_ack(int(time.time()))))
            log(f"-> HELLO_ACK {hl.PROTO_MAJOR}.{hl.PROTO_MINOR}")
            self.acked.set()
        elif ptype == hl.DEVICE_STATUS:
            log(f"<- DEVICE_STATUS {hl.read_device_status(payload)}")
        elif ptype == hl.CONTEXT:
            log(f"<- CONTEXT mode={payload[0]} page={payload[1]}")
        elif ptype == hl.AGENT_START:
            self.xid, self.up_blocks, self.up_missing, self.expect_seq = xid, [], 0, 0
            log(f"<- AGENT_START xid=0x{xid:02x} codec={payload[0]} rate={payload[1]} kHz")
            asyncio.ensure_future(self.send(hl.agent_state(xid, hl.ACCEPTED)))
        elif ptype == hl.AUDIO_UP:
            if xid != self.xid:
                return
            if seq != self.expect_seq:
                self.up_missing += (seq - self.expect_seq) & 0xFF
            self.expect_seq = (seq + 1) & 0xFF
            self.up_blocks.append(payload)
        elif ptype == hl.AGENT_END:
            self.t_end = time.monotonic()
            reason = "20-second limit" if payload and payload[0] == 1 else "release"
            log(f"<- AGENT_END xid=0x{xid:02x} ({reason}): {len(self.up_blocks)} blocks, "
                f"{len(self.up_blocks) * 20} ms, {self.up_missing} missing")
            self.answer_task = asyncio.ensure_future(self.answer(xid, list(self.up_blocks)))
        elif ptype == hl.AGENT_CANCEL:
            log(f"<- AGENT_CANCEL xid=0x{xid:02x}")
            self.canceled.add(xid)
        elif ptype == hl.ERROR:
            log(f"<- ERROR code={payload[0]} type=0x{payload[1]:02x} xid=0x{payload[2]:02x}")
        else:
            log(f"<- {name} xid=0x{xid:02x}")

    def save_question(self, xid, blocks):
        pcm = []
        for b in blocks:
            pcm.extend(hl.decode_block(b))
        path = os.path.join(self.args.out, f"question-{time.strftime('%H%M%S')}-x{xid:02x}.wav")
        with wave.open(path, "wb") as w:
            w.setnchannels(1)
            w.setsampwidth(2)
            w.setframerate(hl.SAMPLE_RATE)
            w.writeframes(struct.pack(f"<{len(pcm)}h", *pcm))
        log(f"saved {path}")
        return pcm

    def answer_blocks(self, question_pcm):
        if self.args.mode == "echo":
            return hl.encode_stream(question_pcm)
        with wave.open(self.args.wav, "rb") as w:
            if (w.getframerate(), w.getnchannels(), w.getsampwidth()) != (16000, 1, 2):
                raise SystemExit("--wav needs 16 kHz, mono, 16-bit")
            raw = w.readframes(w.getnframes())
        return hl.encode_stream(list(struct.unpack(f"<{len(raw) // 2}h", raw)))

    async def answer(self, xid, blocks):
        if not blocks:
            # No audio: an empty playback would hide the fault. Report a failure instead.
            log("question has 0 audio blocks: send AGENT_STATE failed (recognition)")
            await self.send(hl.agent_state(xid, hl.FAILED, hl.FAIL_RECOGNITION))
            return
        pcm = self.save_question(xid, blocks)
        peak = max(abs(v) for v in pcm)
        log(f"question level: peak {peak} of 32767" + ("  (near silence: check the microphone path)" if peak < 300 else ""))
        await self.send(hl.agent_state(xid, hl.RECOGNIZING, text="（模拟）收到问题"))
        if self.args.mode == "silent":
            log("silent mode: no answer")
            return
        await asyncio.sleep(0.5)
        if self.args.mode == "fail":
            await self.send(hl.agent_state(xid, hl.FAILED, hl.FAIL_RECOGNITION))
            log("-> AGENT_STATE failed (recognition)")
            return
        down = self.answer_blocks(pcm)
        await self.send(hl.agent_state(xid, hl.ANSWERING, text="（模拟）回放你的问题" if self.args.mode == "echo" else "（模拟）回答"))
        await self.send(hl.play_start(xid))
        log(f"-> PLAY_START, {len(down)} blocks, first answer {1000 * (time.monotonic() - self.t_end):.0f} ms after AGENT_END")
        t0 = time.monotonic()
        for seq, block in enumerate(down):
            if xid in self.canceled:
                log("answer stopped: canceled")
                return
            # Real-time pace, a few blocks ahead of the speaker.
            due = t0 + (seq - AHEAD_BLOCKS) * BLOCK_S
            delay = due - time.monotonic()
            if delay > 0:
                await asyncio.sleep(delay)
            await self.send(hl.audio_down(xid, seq, block))
        await self.send(hl.play_end(xid))
        await self.send(hl.agent_state(xid, hl.DONE))
        log(f"-> PLAY_END after {len(down)} blocks")

    async def ski_loop(self, runs, run_s):
        """Send simulated runs. Run 2 has no speed, so the device must show "--" for it."""
        await self.acked.wait()
        await asyncio.sleep(1.0)
        for run_no in range(1, runs + 1):
            drop = dist = turns = 0
            log(f"run {run_no}: start, {run_s} s")
            for t in range(run_s):
                stopped = run_s >= 20 and run_s // 2 <= t < run_s // 2 + 3     # A short stop mid-run.
                speed = 0 if stopped else 250 + (t * 37) % 200                    # 25.0 to 44.9 km/h
                if not stopped:
                    drop += 4
                    dist += speed // 36                                           # meters in one second
                    turns += 1 if t % 3 == 0 else 0
                state = hl.RUN_STOPPED if stopped else hl.RUN_SKIING
                shown = hl.U16_UNKNOWN if run_no == 2 else speed
                await self.send(hl.live_stats(state, run_no, shown, drop, dist, turns, t + 1))
                await asyncio.sleep(1.0)
            await self.send(hl.live_stats(hl.RUN_ENDED, run_no, hl.U16_UNKNOWN, drop, dist, turns, run_s))
            log(f"run {run_no}: end, drop {drop} m, {dist} m, {turns} turns")
            await asyncio.sleep(2.0)                                              # The phone needs time for the report.
            iq = 60 + (run_no * 11) % 40
            await self.send(hl.run_report(run_no, dist, turns, iq, "重心偏后", "下一趟把双手放在身体前方"))
            log(f"-> RUN_REPORT run {run_no}: iq {iq}")
            await asyncio.sleep(10.0)                                             # The lift: no LIVE_STATS.

    async def status_loop(self):
        while self.client.is_connected:
            await self.send(hl.phone_status(int(time.time())))
            await asyncio.sleep(STATUS_INTERVAL_S)


async def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mode", choices=["echo", "wav", "fail", "silent"], default="echo")
    ap.add_argument("--wav", help="answer file for --mode wav")
    ap.add_argument("--name", help="device name; default: the first HeygoPP-* device")
    ap.add_argument("--out", default=".", help="folder for the question WAV files")
    ap.add_argument("--runs", type=int, default=0, help="number of simulated runs of ski data (default 0)")
    ap.add_argument("--run-s", type=int, default=30, help="seconds of each simulated run (default 30)")
    args = ap.parse_args()
    if args.mode == "wav" and not args.wav:
        ap.error("--mode wav needs --wav FILE")
    try:
        from bleak import BleakClient, BleakScanner
    except ImportError:
        raise SystemExit("phone_sim needs bleak: pip install bleak")

    log("scanning ...")
    dev = await BleakScanner.find_device_by_filter(
        lambda d, ad: (d.name or ad.local_name or "").startswith(args.name or hl.NAME_PREFIX), timeout=15)
    if not dev:
        raise SystemExit("no HeygoPP device found")
    log(f"connecting to {dev.name} ({dev.address})")
    async with BleakClient(dev) as client:
        log(f"connected, MTU {client.mtu_size}")
        if client.mtu_size < 172:
            log("warning: MTU below 172. The device disables voice.")
        phone = Phone(client, args)
        await client.start_notify(hl.NUS_TX, phone.on_notify)
        if args.runs > 0:
            asyncio.ensure_future(phone.ski_loop(args.runs, max(1, args.run_s)))
        await phone.status_loop()
    log("disconnected")


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
