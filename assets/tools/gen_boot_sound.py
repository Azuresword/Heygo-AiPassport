#!/usr/bin/env python3
"""gen_boot_sound - make the boot sound of the device (D-36).

Three original designs. Each one is clean, short, and synthetic, with a fast build-up and
a precise hit: the "tech and sport" feel of action-camera brands. None copies a real
product sound. All follow the boot animation of main/ui/boot_screen.c:

  A "Dash"  A tonal riser (0 - 0.42 s), an impact when the snowboard lands, two glassy
              blips as the letters land, and a bright chord that locks in with the tagline (1.0 s).
  B "Carve"   Two fast carve whooshes, three metallic ticks on the letters, and a synth
              stab whose filter opens with the tagline.
  C "Pulse"   No noise. Three pure tones that glide up into place, and a held top note
              with a soft shimmer.

The speaker is small: each design has no energy below 300 Hz, a peak of -4 dBFS, and a
fade at the end, so the speaker makes no click and does not rattle.
A fixed random seed makes each run give the same files.

Output, in assets/audio:
  boot_sound_a.wav, boot_sound_b.wav, boot_sound_c.wav   The three designs, 16 kHz mono.
  boot_sound.wav           The design in the firmware (PICK), for listening.
  heygo_boot_sound.c/.h    The same design as 16-bit PCM for the device.

Run: python3 assets/tools/gen_boot_sound.py [--pick A|B|C]  (needs numpy)
"""
import argparse
import math
import wave
from pathlib import Path

import numpy as np

RATE = 16000
SECONDS = 2.0
N = int(RATE * SECONDS)
PEAK = 10 ** (-4 / 20)          # -4 dBFS.
OUT = Path(__file__).resolve().parent.parent / "audio"
RNG = np.random.default_rng(36)


# ---------------------------------------------------------------- building blocks

def seg(start, length):
    """The sample range of a part, and its local time axis."""
    s, e = int(start * RATE), min(N, int((start + length) * RATE))
    return slice(s, e), np.arange(e - s) / RATE


def env_ad(t, attack, decay):
    """A fast attack, then an exponential decay."""
    return np.minimum(1.0, t / max(attack, 1e-4)) * np.exp(-t / decay)


def phase(freq):
    """The phase of a tone whose frequency changes in time (an array in Hz)."""
    return 2 * np.pi * np.cumsum(freq) / RATE


def saw(freq, harmonics=24):
    """A band-limited saw wave: only harmonics below the Nyquist frequency."""
    ph = phase(freq)
    out = np.zeros_like(ph)
    for k in range(1, harmonics + 1):
        out += np.where(k * freq < RATE / 2 - 200, np.sin(k * ph) / k, 0.0)
    return out * 0.6


def svf(x, fc, q):
    """A state-variable filter with a moving cutoff (TPT form, stable at each cutoff).
    q is the damping, 1 / Q: a small value gives a narrow band. Returns low-pass and band-pass."""
    fc = np.broadcast_to(fc, x.shape)
    low = np.zeros_like(x)
    band = np.zeros_like(x)
    ic1 = ic2 = 0.0
    for i in range(len(x)):
        g = math.tan(math.pi * min(fc[i], RATE / 2.05) / RATE)
        a1 = 1 / (1 + g * (g + q))
        a2 = g * a1
        a3 = g * a2
        v3 = x[i] - ic2
        v1 = a1 * ic1 + a2 * v3
        v2 = ic2 + a2 * ic1 + a3 * v3
        ic1, ic2 = 2 * v1 - ic1, 2 * v2 - ic2
        low[i], band[i] = v2, v1
    return low, band


def fm(t, fc, ratio, index, idecay):
    """FM: a glassy, metallic tone. The index falls, so the tone gets pure."""
    return np.sin(2 * np.pi * fc * t + index * np.exp(-t / idecay) * np.sin(2 * np.pi * fc * ratio * t))


def high_pass(x, fc=300.0):
    """Two one-pole stages: 12 dB per octave below fc."""
    for _ in range(2):
        rc = 1 / (2 * np.pi * fc)
        a = rc / (rc + 1 / RATE)
        y = np.zeros_like(x)
        for i in range(1, len(x)):
            y[i] = a * (y[i - 1] + x[i] - x[i - 1])
        x = y
    return x


def room(x, mix=0.12):
    """A small, bright room: three short echoes that fall fast."""
    y = x.copy()
    for d, g in ((0.023, 0.35), (0.041, 0.22), (0.067, 0.12)):
        k = int(d * RATE)
        y[k:] += g * x[:-k]
    return (1 - mix) * x + mix * y


def master(x, drive):
    """High-pass, a soft limiter for punch, the end fade, and the peak level.
    A high drive adds punch, and also harmonics. Pure tones need a low drive."""
    x = high_pass(x)
    x = np.tanh(drive * x / (np.max(np.abs(x)) + 1e-9))
    fade = int(0.12 * RATE)
    x[-fade:] *= np.linspace(1, 0, fade)
    x[:16] *= np.linspace(0, 1, 16)
    return x / np.max(np.abs(x)) * PEAK


# ---------------------------------------------------------------- the three designs
# The times come from boot_screen.c: the board lands at 0.42 s, "g" 0.50, "o" 0.58,
# "H" 0.74, "e" 0.82, "y" 0.90. The tagline rises at 1.0 s, the line grows at 1.15 s.

def impact(x, at, amp=0.8):
    """A short punch: a tone that falls from 1,400 Hz to 380 Hz, and a click."""
    s, t = seg(at, 0.25)
    f = 380 + 1020 * np.exp(-t / 0.018)
    x[s] += amp * np.sin(phase(f)) * env_ad(t, 0.001, 0.07)
    _, click = svf(RNG.uniform(-1, 1, len(t)), 4500, 0.7)
    x[s] += 0.5 * click * env_ad(t, 0.0005, 0.006)


def design_a():
    """Dash."""
    x = np.zeros(N)
    # The riser: a saw that glides up from 220 Hz to 1,760 Hz, through a filter that opens.
    s, t = seg(0.0, 0.42)
    k = (t / 0.42) ** 2.2                                   # It speeds up, as a run starts.
    f = 220 * 8 ** k
    _, band = svf(saw(f, 18), 600 + 5200 * k, 0.25)
    air = svf(svf(RNG.uniform(-1, 1, len(t)), 2000 + 4000 * k, 0.3)[1], 2000 + 4000 * k, 0.3)[1]
    x[s] += (0.55 * band + 0.05 * air) * np.minimum(1, t / 0.05) * (0.4 + 0.6 * k)
    impact(x, 0.42)
    # Two glassy blips: B5, then F#6.
    for at, fc in ((0.50, 987.8), (0.66, 1480.0)):
        s, t = seg(at, 0.3)
        x[s] += 0.32 * fm(t, fc, 2.0, 2.2, 0.03) * env_ad(t, 0.001, 0.07)
    # The lock-in: E5, B5, E6, and a high E7 sparkle. Two voices a little apart make it wide.
    s, t = seg(1.0, 1.0)
    for fc, a in ((659.3, 0.34), (987.8, 0.26), (1318.5, 0.22), (2637.0, 0.07)):
        for cents in (-4, 4):
            x[s] += a * 0.5 * fm(t, fc * 2 ** (cents / 1200), 1.0, 0.8, 0.05) * env_ad(t, 0.003, 0.55)
    s, t = seg(1.0, 0.4)
    x[s] += 0.25 * svf(RNG.uniform(-1, 1, len(t)), 7000, 0.6)[1] * env_ad(t, 0.0008, 0.05)
    return master(room(x), 1.3)


def design_b():
    """Carve."""
    x = np.zeros(N)
    # Two carves: band noise up, then a shorter one up again, as heel and toe edges.
    for start, length, f0, f1, amp in ((0.0, 0.26, 700, 2600, 0.7), (0.20, 0.22, 1000, 4200, 0.8)):
        s, t = seg(start, length)
        k = t / length
        fc = f0 * (f1 / f0) ** k
        band = svf(svf(RNG.uniform(-1, 1, len(t)), fc, 0.14)[1], fc, 0.14)[1]   # Two stages: a narrow band.
        x[s] += amp * band * np.sin(np.pi * k) ** 1.5
    impact(x, 0.42, 0.7)
    # Three metallic ticks on "H", "e", "y".
    for at, fc in ((0.74, 1760.0), (0.82, 2217.5), (0.90, 2637.0)):
        s, t = seg(at, 0.15)
        x[s] += 0.3 * fm(t, fc, 3.5, 3.0, 0.012) * env_ad(t, 0.0005, 0.03)
    # The stab: a saw chord in E with a filter that opens fast and closes slowly.
    s, t = seg(1.0, 1.0)
    chord = sum(saw(np.full(len(t), f), 12) * a for f, a in ((329.6, 0.5), (493.9, 0.4), (659.3, 0.35), (987.8, 0.2)))
    low, _ = svf(chord, 900 + 5000 * np.exp(-t / 0.12), 0.35)
    x[s] += 0.9 * low * env_ad(t, 0.002, 0.45)
    return master(room(x, 0.15), 1.4)


def design_c():
    """Pulse."""
    x = np.zeros(N)
    # Three pure tones. Each glides up into its note in 40 ms: E5 on the board, B5 on "o", E6 on "y".
    for at, fc, length in ((0.42, 659.3, 0.5), (0.58, 987.8, 0.5), (0.90, 1318.5, 0.5)):
        s, t = seg(at, length)
        f = fc * (1 - 0.12 * np.exp(-t / 0.012))
        x[s] += 0.35 * (np.sin(phase(f)) + 0.15 * np.sin(2 * phase(f))) * env_ad(t, 0.002, 0.16)
    # A soft rise into the first tone.
    s, t = seg(0.0, 0.42)
    x[s] += 0.12 * np.sin(phase(330 * 2 ** (t / 0.42))) * (t / 0.42) ** 2
    # The held top note with a slow vibrato, and a shimmer one octave up.
    s, t = seg(1.0, 1.0)
    vib = 1 + 0.004 * np.sin(2 * np.pi * 5.5 * t) * np.minimum(1, t / 0.3)
    x[s] += 0.4 * np.sin(phase(1318.5 * vib)) * env_ad(t, 0.004, 0.6)
    x[s] += 0.12 * np.sin(phase(2637.0 * vib)) * env_ad(t, 0.02, 0.35) * (0.6 + 0.4 * np.sin(2 * np.pi * 12 * t))
    x[s] += 0.2 * np.sin(phase(np.full(len(t), 659.3))) * env_ad(t, 0.004, 0.4)
    return master(room(x, 0.2), 0.7)


DESIGNS = {"A": design_a, "B": design_b, "C": design_c}


def write_wav(path, pcm):
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(pcm.astype("<i2").tobytes())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--pick", choices=sorted(DESIGNS), default="A", help="the design in the firmware")
    pick = ap.parse_args().pick
    OUT.mkdir(parents=True, exist_ok=True)
    pcms = {}
    for name, make in DESIGNS.items():
        pcm = np.round(make() * 32767).astype(np.int16)
        pcms[name] = pcm
        write_wav(OUT / f"boot_sound_{name.lower()}.wav", pcm)
        rms = 20 * math.log10(np.sqrt(np.mean((pcm / 32767.0) ** 2)))
        print(f"boot_sound_{name.lower()}.wav  peak {np.max(np.abs(pcm))} of 32767, RMS {rms:.1f} dBFS")
    pcm = pcms[pick]
    write_wav(OUT / "boot_sound.wav", pcm)
    c = ["/* assets/audio/heygo_boot_sound.c",
         " * Generated by assets/tools/gen_boot_sound.py. Do not edit.",
         f" * The boot sound (D-36), design {pick}: {SECONDS:.1f} s, {RATE} Hz, 16-bit, mono PCM. Peak -4 dBFS. */",
         '#include "heygo_boot_sound.h"', "",
         "const int16_t heygo_boot_sound[HEYGO_BOOT_SOUND_SAMPLES] = {"]
    for i in range(0, N, 16):
        c.append("    " + ", ".join(str(int(v)) for v in pcm[i:i + 16]) + ",")
    c += ["};", ""]
    (OUT / "heygo_boot_sound.c").write_text("\n".join(c))
    h = ["/* assets/audio/heygo_boot_sound.h",
         " * Generated by assets/tools/gen_boot_sound.py. Do not edit.",
         " * The boot sound (D-36), 16 kHz, 16-bit, mono PCM. main/audio/voice.c plays it. */",
         "#pragma once", "", "#include <stdint.h>", "",
         f"#define HEYGO_BOOT_SOUND_DESIGN  \"{pick}\"",
         f"#define HEYGO_BOOT_SOUND_RATE    {RATE}",
         f"#define HEYGO_BOOT_SOUND_SAMPLES {N}", "",
         "extern const int16_t heygo_boot_sound[HEYGO_BOOT_SOUND_SAMPLES];", ""]
    (OUT / "heygo_boot_sound.h").write_text("\n".join(h))
    print(f"Wrote heygo_boot_sound.c and boot_sound.wav with design {pick}: {N} samples ({N * 2} bytes)")


if __name__ == "__main__":
    main()
