#!/usr/bin/env python3
"""gen_popup_icons - make the icons of the popup page (D-31).

Each popup kind has one round badge in its own color, with a dark picture on it.
Each badge has 4 frames. The device shows them in a loop, so the picture moves.

  fall    red     A rider who falls on the back. Three stars turn in the air.
  voice   lime    A megaphone. The sound waves come out one by one.
  issue   orange  A magnifier with "!". It swings from side to side.
  badge   gold    A cup. Small stars flash around it.

Output, in assets/sprites:
  heygo_popup_icons.h      The declarations.
  heygo_popup_icons.c      16 LVGL images, ARGB8888 (B, G, R, A in memory), 56 x 56 px.
  heygo_popup_icons.png    A preview at 2x scale.
  heygo_popup_icons_sheet.png  The frames at 1x with alpha, for the browser simulator:
                           one row for each kind, one column for each frame.

Run: python3 assets/tools/gen_popup_icons.py  (needs Pillow)
"""
import math
from pathlib import Path

from PIL import Image, ImageDraw

SIZE = 56          # Pixels on the screen.
SS = 8             # Supersampling.
FRAMES = 4
KINDS = ["fall", "voice", "issue", "badge"]
OUT = Path(__file__).resolve().parent.parent / "sprites"

DARK = (10, 10, 10, 255)
WHITE = (255, 255, 255, 255)
# The badge colors. main/ui/ui_theme.h has the same values: DANGER, ACCENT, WARN, GOLD.
COLOR = {"fall": (239, 68, 68, 255), "voice": (177, 254, 43, 255),
         "issue": (255, 159, 28, 255), "badge": (255, 212, 59, 255)}

S = SIZE * SS
C = S / 2


def u(v):
    """Screen pixels to canvas pixels."""
    return v * SS


def disc(d, color):
    r = u(26)
    d.ellipse((C - r, C - r, C + r, C + r), fill=color)


def line(d, pts, w, color=DARK):
    d.line([(x, y) for x, y in pts], fill=color, width=int(u(w)), joint="curve")
    r = u(w) / 2
    for x, y in (pts[0], pts[-1]):
        d.ellipse((x - r, y - r, x + r, y + r), fill=color)


def star(d, cx, cy, r, color, points=5, inner=0.45, turn=0.0):
    pts = []
    for i in range(points * 2):
        a = turn + math.pi * i / points - math.pi / 2
        rr = r if i % 2 == 0 else r * inner
        pts.append((cx + rr * math.cos(a), cy + rr * math.sin(a)))
    d.polygon(pts, fill=color)


def rot(px, py, cx, cy, deg):
    a = math.radians(deg)
    x, y = px - cx, py - cy
    return (cx + x * math.cos(a) - y * math.sin(a), cy + x * math.sin(a) + y * math.cos(a))


def fall(k):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    disc(d, COLOR["fall"])
    # The rider falls on the back: the head is low, the legs and the board are up.
    # The body rocks a few degrees each frame.
    tilt = [-6, 0, 6, 0][k]
    cx, cy = C, C + u(2)
    def p(x, y):
        return rot(cx + u(x), cy + u(y), cx, cy, tilt)
    hip = p(3, -1)
    line(d, [p(-7, 8), hip], 5.0)                           # body
    line(d, [hip, p(6, -9)], 4.0)                           # leg
    line(d, [hip, p(11, -4)], 4.0)                          # leg
    line(d, [p(1, -15), p(17, 1)], 3.6)                     # board, in the air
    line(d, [p(-4, 5), p(-14, 1)], 3.0)                     # arm
    line(d, [p(-4, 5), p(-1, 15)], 3.0)                     # arm
    head = p(-12, 12)
    hr = u(4.8)
    d.ellipse((head[0] - hr, head[1] - hr, head[0] + hr, head[1] + hr), fill=DARK)
    # Three white stars turn above the head: 40 degrees each frame, so the loop is smooth.
    sx, sy = C - u(10), C - u(10)
    for i in range(3):
        a = math.radians(i * 120 + k * 40)
        star(d, sx + u(7) * math.cos(a), sy + u(4) * math.sin(a), u(3.4), WHITE, turn=a)
    return img


def voice(k):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    disc(d, COLOR["voice"])
    # The megaphone points right.
    x0 = C - u(15)
    d.rounded_rectangle((x0, C - u(4.5), x0 + u(6), C + u(4.5)), radius=u(1.5), fill=DARK)
    d.polygon([(x0 + u(5), C - u(4.5)), (x0 + u(17), C - u(11)), (x0 + u(17), C + u(11)),
               (x0 + u(5), C + u(4.5))], fill=DARK)
    d.rounded_rectangle((x0 + u(15.5), C - u(11.5), x0 + u(18.5), C + u(11.5)), radius=u(1.5), fill=DARK)
    line(d, [(x0 + u(6), C + u(4)), (x0 + u(8.5), C + u(11))], 3.2)   # handle
    # The waves: frame 0 has none, frames 1 to 3 add one each.
    for i in range(k):
        r = u(5 + 5 * i)
        box = (x0 + u(18) - r, C - r, x0 + u(18) + r, C + r)
        d.arc(box, -40, 40, fill=DARK, width=int(u(2.6)))
    return img


def issue(k):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    disc(d, COLOR["issue"])
    swing = [-12, 0, 12, 0][k]
    cx, cy = C - u(3), C - u(3)
    def p(x, y):
        return rot(cx + u(x), cy + u(y), C + u(8), C + u(8), swing)
    lc = p(0, 0)
    r = u(10.5)
    d.ellipse((lc[0] - r, lc[1] - r, lc[0] + r, lc[1] + r), outline=DARK, width=int(u(3.4)))
    line(d, [p(8, 8), p(15, 15)], 5.0)                      # handle
    # The "!" stays upright in the lens.
    line(d, [(lc[0], lc[1] - u(5.5)), (lc[0], lc[1] + u(1.2))], 3.0)
    dr = u(1.7)
    d.ellipse((lc[0] - dr, lc[1] + u(4.6) - dr, lc[0] + dr, lc[1] + u(4.6) + dr), fill=DARK)
    return img


def badge(k):
    img = Image.new("RGBA", (S, S), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    disc(d, COLOR["badge"])
    # The cup.
    top = C - u(12)
    d.pieslice((C - u(10), top - u(10), C + u(10), top + u(18)), 0, 180, fill=DARK)
    d.rectangle((C - u(10), top, C + u(10), top + u(4)), fill=DARK)
    for sgn in (-1, 1):
        ex = C + sgn * u(11)
        d.arc((ex - u(5), top + u(1), ex + u(5), top + u(10)), 270 if sgn > 0 else 90,
              90 if sgn > 0 else 270, fill=DARK, width=int(u(2.4)))
    d.rectangle((C - u(2), top + u(13), C + u(2), top + u(19)), fill=DARK)
    d.rounded_rectangle((C - u(8), top + u(19), C + u(8), top + u(23)), radius=u(1.5), fill=DARK)
    star(d, C, top + u(5.5), u(4.2), COLOR["badge"])
    # Small four-point stars flash at the edge: each frame lights another pair.
    spots = [(-17, -12), (17, -8), (-15, 10), (16, 12)]
    for i, (x, y) in enumerate(spots):
        on = i in ((k, (k + 2) % 4))
        r = u(4.2 if on else 1.6)
        star(d, C + u(x), C + u(y), r, WHITE, points=4, inner=0.3)
    return img


DRAW = {"fall": fall, "voice": voice, "issue": issue, "badge": badge}


def frames():
    return {kind: [DRAW[kind](k).resize((SIZE, SIZE), Image.LANCZOS) for k in range(FRAMES)] for kind in KINDS}


def write(all_frames):
    OUT.mkdir(parents=True, exist_ok=True)
    rows = []
    for kind in KINDS:
        for k, img in enumerate(all_frames[kind]):
            data = []
            for r, g, b, a in img.getdata():
                data += [b, g, r, a]
            lines = [", ".join(f"0x{v:02x}" for v in data[i:i + 16]) for i in range(0, len(data), 16)]
            rows.append(f"    /* {kind} {k} */\n    " + ",\n    ".join(lines))
    n = len(KINDS) * FRAMES
    c = [
        "/* assets/sprites/heygo_popup_icons.c",
        " * Generated by assets/tools/gen_popup_icons.py. Do not edit.",
        f" * {n} images of the popup page, {SIZE} x {SIZE} px, ARGB8888: {FRAMES} frames for each kind,",
        " * in the order fall, voice, issue, badge. */",
        '#include "heygo_popup_icons.h"',
        "",
        f"static const uint8_t s_map[{n}][{SIZE * SIZE * 4}] = {{",
        ",\n".join("    {\n" + r + "\n    }" for r in rows),
        "};",
        "",
        "#define FRAME(k) { .header = { .magic = LV_IMAGE_HEADER_MAGIC, .cf = LV_COLOR_FORMAT_ARGB8888, \\",
        f"                         .w = {SIZE}, .h = {SIZE}, .stride = {SIZE * 4} }}, \\",
        f"                  .data_size = {SIZE * SIZE * 4}, .data = s_map[k] }}",
        "",
        "const lv_image_dsc_t heygo_popup_icon[HEYGO_POPUP_KINDS][HEYGO_POPUP_FRAMES] = {",
        ",\n".join("    { " + ", ".join(f"FRAME({i * FRAMES + k})" for k in range(FRAMES)) + " }"
                   for i in range(len(KINDS))),
        "};",
        "",
    ]
    (OUT / "heygo_popup_icons.c").write_text("\n".join(c), encoding="utf-8")
    h = [
        "/* assets/sprites/heygo_popup_icons.h",
        " * Declares the icons of the popup page. assets/tools/gen_popup_icons.py makes them.",
        " *",
        " *   heygo_popup_icon[kind - 1][frame]  4 kinds (fall, voice, issue, badge), 4 frames each,",
        " *                                     56 x 56 px. Show the frames in a loop. */",
        "#pragma once",
        "",
        '#include "lvgl.h"',
        "",
        f"#define HEYGO_POPUP_KINDS  {len(KINDS)}",
        f"#define HEYGO_POPUP_FRAMES {FRAMES}",
        f"#define HEYGO_POPUP_ICON   {SIZE}",
        "",
        "extern const lv_image_dsc_t heygo_popup_icon[HEYGO_POPUP_KINDS][HEYGO_POPUP_FRAMES];",
        "",
    ]
    (OUT / "heygo_popup_icons.h").write_text("\n".join(h), encoding="utf-8")
    # Preview: one row for each kind, on the screen background, at 2x.
    sc, gap = 2, 8
    cell = SIZE * sc + gap
    prev = Image.new("RGB", (FRAMES * cell + gap, len(KINDS) * cell + gap), (0, 0, 0))
    for row, kind in enumerate(KINDS):
        for k, img in enumerate(all_frames[kind]):
            tile = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 255))
            tile.alpha_composite(img)
            prev.paste(tile.convert("RGB").resize((SIZE * sc, SIZE * sc), Image.LANCZOS),
                       (gap + k * cell, gap + row * cell))
    prev.save(OUT / "heygo_popup_icons.png")
    sheet = Image.new("RGBA", (FRAMES * SIZE, len(KINDS) * SIZE), (0, 0, 0, 0))
    for row, kind in enumerate(KINDS):
        for k, img in enumerate(all_frames[kind]):
            sheet.paste(img, (k * SIZE, row * SIZE))
    sheet.save(OUT / "heygo_popup_icons_sheet.png")
    print(f"Wrote {n} frames to {OUT}")


if __name__ == "__main__":
    write(frames())
