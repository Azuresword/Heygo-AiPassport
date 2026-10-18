#!/usr/bin/env python3
"""gen_boot_sound - make the boot sound of the device (D-36).

The sound follows the boot animation of main/ui/boot_screen.c, on the same time line:

  0.00 - 0.45 s  A carve: band-pass noise that sweeps up, as the snowboard carves in.
  0.18 - 0.70 s  Five soft plucks on a rising scale, one for each logo letter:
                 "g", "o", "H", "e", "y".
  1.00 - 2.00 s  A bell chord in E when the tagline rises, and a high shimmer when
                 the lime line grows (1.15 s).

The speaker is small, so the sound has no energy below 250 Hz. The peak is -6 dBFS.
The sound ends with a fade, so the speaker makes no click. A fixed random seed makes
each run give the same file.

Output:
  assets/audio/heygo_boot_sound.c   16 kHz, 16-bit, mono PCM for the device.
  assets/audio/heygo_boot_sound.h   The declaration.
  assets/audio/boot_sound.wav       The same sound, for listening and for the simulator.

Run: python3 assets/tools/gen_boot_sound.py  (Python 3 only, no other package)
"""
import math
import random
import struct
import wave
from pathlib import Path

RATE = 16000
SECONDS = 2.0
N = int(RATE * SECONDS)
PEAK = 0.5                  # -6 dBFS.
OUT = Path(__file__).resolve().parent.parent / "audio"


def carve(buf):
    """Band-pass noise. The center goes from 900 Hz to 3,200 Hz in 0.45 s."""
    rng = random.Random(35)
    q = 0.12                 # 1 / Q. A small value keeps the band narrow, so the sweep is heard.
    low = [0.0, 0.0]
    band = [0.0, 0.0]
    end = int(0.45 * RATE)
    for i in range(end):
        t = i / RATE
        fc = 900 * (3200 / 900) ** (t / 0.45)
        f = 2 * math.sin(math.pi * fc / RATE)
        x = rng.uniform(-1, 1)
        # Two state-variable band-pass stages in a row: the sides fall twice as fast.
        for k in range(2):
            low[k] += f * band[k]
            high = x - low[k] - q * band[k]
            band[k] += f * high
            x = band[k] * q
        env = min(1.0, t / 0.08) * max(0.0, 1 - max(0.0, t - 0.28) / 0.17)
        buf[i] += 2.0 * x * env


def pluck(buf, start, freq, amp=0.30, decay=0.08):
    """A short, soft note with a few partials."""
    s = int(start * RATE)
    for i in range(int(0.35 * RATE)):
        if s + i >= N:
            break
        t = i / RATE
        env = min(1.0, t / 0.004) * math.exp(-t / decay)
        v = (math.sin(2 * math.pi * freq * t) + 0.35 * math.sin(2 * math.pi * 2 * freq * t)
             + 0.12 * math.sin(2 * math.pi * 3 * freq * t))
        buf[s + i] += amp * env * v


def bell(buf, start, freq, amp, decay=0.5):
    """A bell tone: partials that are not quite harmonic, the higher ones die first."""
    s = int(start * RATE)
    partials = [(1.0, 1.0, 1.0), (2.0, 0.45, 0.6), (3.01, 0.22, 0.4), (4.2, 0.12, 0.25)]
    for i in range(N - s):
        t = i / RATE
        attack = min(1.0, t / 0.006)
        v = 0.0
        for ratio, a, dk in partials:
            v += a * math.exp(-t / (decay * dk)) * math.sin(2 * math.pi * freq * ratio * t)
        buf[s + i] += amp * attack * v


def shimmer(buf, start, freq=2637.0, amp=0.05, length=0.6):
    """A high tone with a fast tremolo, under the lime line."""
    s = int(start * RATE)
    for i in range(int(length * RATE)):
        if s + i >= N:
            break
        t = i / RATE
        env = math.sin(math.pi * t / length) ** 2
        buf[s + i] += amp * env * (0.6 + 0.4 * math.sin(2 * math.pi * 14 * t)) * math.sin(2 * math.pi * freq * t)


def reverb(buf):
    """A small room: four comb filters and two all-pass filters (Schroeder)."""
    out = [0.0] * N
    for d, g in ((557, 0.72), (617, 0.70), (691, 0.68), (751, 0.66)):
        line = [0.0] * d
        for i in range(N):
            y = line[i % d]
            line[i % d] = buf[i] + g * y
            out[i] += y / 4
    for d, g in ((113, 0.6), (37, 0.6)):
        line = [0.0] * d
        for i in range(N):
            y = line[i % d]
            v = out[i] + g * y
            line[i % d] = v
            out[i] = y - g * v
    return [b + 0.18 * w for b, w in zip(buf, out)]


def high_pass(buf, fc=250.0):
    """One pole. The small speaker cannot play low tones, and they make it rattle."""
    rc = 1 / (2 * math.pi * fc)
    a = rc / (rc + 1 / RATE)
    out, prev_x, prev_y = [], 0.0, 0.0
    for x in buf:
        y = a * (prev_y + x - prev_x)
        out.append(y)
        prev_x, prev_y = x, y
    return out


def main():
    buf = [0.0] * N
    carve(buf)
    # A rising E major pentatonic, one note for each letter, at the times of boot_screen.c.
    for start, freq in ((0.18, 659.3), (0.26, 740.0), (0.42, 830.6), (0.50, 987.8), (0.58, 1108.7)):
        pluck(buf, start, freq)
    for freq, amp in ((659.3, 0.30), (987.8, 0.22), (1318.5, 0.18), (1480.0, 0.10)):
        bell(buf, 1.0, freq, amp)
    shimmer(buf, 1.15)
    buf = high_pass(reverb(buf))
    # The last 150 ms fade out, so the end makes no click.
    fade = int(0.15 * RATE)
    for i in range(fade):
        buf[N - fade + i] *= 1 - i / fade
    peak = max(abs(v) for v in buf)
    pcm = [round(v / peak * PEAK * 32767) for v in buf]

    OUT.mkdir(parents=True, exist_ok=True)
    with wave.open(str(OUT / "boot_sound.wav"), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(struct.pack(f"<{N}h", *pcm))
    c = ["/* assets/audio/heygo_boot_sound.c",
         " * Generated by assets/tools/gen_boot_sound.py. Do not edit.",
         f" * The boot sound (D-36): {SECONDS:.1f} s, {RATE} Hz, 16-bit, mono PCM. Peak -6 dBFS. */",
         '#include "heygo_boot_sound.h"', "",
         f"const int16_t heygo_boot_sound[HEYGO_BOOT_SOUND_SAMPLES] = {{"]
    for i in range(0, N, 16):
        c.append("    " + ", ".join(str(v) for v in pcm[i:i + 16]) + ",")
    c += ["};", ""]
    (OUT / "heygo_boot_sound.c").write_text("\n".join(c))
    h = ["/* assets/audio/heygo_boot_sound.h",
         " * Generated by assets/tools/gen_boot_sound.py. Do not edit.",
         " * The boot sound (D-36), 16 kHz, 16-bit, mono PCM. main/audio/voice.c plays it. */",
         "#pragma once", "", "#include <stdint.h>", "",
         f"#define HEYGO_BOOT_SOUND_RATE    {RATE}",
         f"#define HEYGO_BOOT_SOUND_SAMPLES {N}", "",
         "extern const int16_t heygo_boot_sound[HEYGO_BOOT_SOUND_SAMPLES];", ""]
    (OUT / "heygo_boot_sound.h").write_text("\n".join(h))
    rms = math.sqrt(sum(v * v for v in pcm) / N) / 32767
    print(f"Wrote boot_sound.wav and heygo_boot_sound.c: {N} samples ({N * 2} bytes), "
          f"peak {max(abs(v) for v in pcm)} of 32767, RMS {20 * math.log10(rms):.1f} dBFS")


if __name__ == "__main__":
    main()
