#!/usr/bin/env python3
"""turns_gif - make an animation of the Turns page with the device drawing code.

It uses the renderer of render.py (LVGL with the device settings), so each frame shows
the real pixels. The run has 44 turns, as `phone_sim.py --turns 44` sends.

  1. The page opens and draws the first page point by point.
  2. The rider replays turn 1.
  3. OK selects turn 2 and turn 3. The rider replays each of them.

Output: ../assets/sim/turns-demo.gif (480 x 640, 2x scale).
Run in Firmware: python3 tools/host_render/turns_gif.py  (needs Pillow)
"""
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import render  # noqa: E402

from PIL import Image  # noqa: E402

RUN, TURNS = 4, 44
OUT = render.ASSETS / "sim" / "turns-demo.gif"


def frames():
    """(name, shape command, duration in ms) for each frame."""
    out = []
    pts = 3 * 8 + 1
    for drawn in range(1, pts + 1):                       # The first page draws: one point each 40 ms.
        out.append((f"d{drawn:02}", render.shape(RUN, 0, TURNS, 0, drawn=drawn), 60))
    for sel in (0, 1, 2):                                # The rider replays turns 1, 2, and 3.
        for permille in range(0, 1001, 100):
            out.append((f"t{sel}_{permille:04}", render.shape(RUN, 0, TURNS, sel, permille=permille), 110))
        out[-1] = out[-1][:2] + (900,)                   # A short stop at the end of each turn.
    return out


def main():
    exe = render.build((render.FW / "sdkconfig").resolve())
    raw = render.BUILD / "gif"
    raw.mkdir(parents=True, exist_ok=True)
    seq = frames()
    cmds = ["screen turns"]
    for name, shape, _ in seq:
        cmds += [f"shape {shape}", f"snap {name}"]
    r = subprocess.run([str(exe), str(raw)], input="\n".join(cmds) + "\n", capture_output=True, text=True)
    if r.returncode or "ERR" in r.stdout:
        sys.exit(f"ERROR: render failed\n{r.stdout}\n{r.stderr}")
    imgs = [render.to_png(raw / f"{name}.rgb565").resize((480, 640), Image.NEAREST) for name, _, _ in seq]
    pal = [im.convert("P", palette=Image.ADAPTIVE, colors=64) for im in imgs]
    pal[0].save(OUT, save_all=True, append_images=pal[1:], duration=[d for _, _, d in seq], loop=0, optimize=True)
    print(f"Wrote {len(pal)} frames to {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
