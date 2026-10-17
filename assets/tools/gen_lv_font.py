#!/usr/bin/env python3
"""gen_lv_font - turn the source font into an LVGL 9 font in C.

The host has no node, so lv_font_conv cannot run. This tool replaces it for
the plain, uncompressed format that LVGL 9 reads with lv_font_fmt_txt.

Two character sets exist:

  text  ASCII, Chinese punctuation, GB2312 level 1 (3755 characters), and
        every vocabulary character. The device shows transcripts and answers
        from the phone in this font, so it needs common characters, not only
        the vocabulary.
  ui    ASCII and every vocabulary character. Large state words use it.

The source font is Noto Sans SC Regular under the SIL Open Font License 1.1.
The tool downloads it once, checks its SHA-256, and keeps it outside the
repository.
"""

import argparse
import hashlib
import json
import pathlib
import sys
import urllib.request

from fontTools.ttLib import TTFont
from PIL import Image, ImageDraw, ImageFont

FONT_URL = "https://raw.githubusercontent.com/notofonts/noto-cjk/main/Sans/SubsetOTF/SC/NotoSansSC-Regular.otf"
FONT_SHA256 = "faa6c9df652116dde789d351359f3d7e5d2285a2b2a1f04a2d7244df706d5ea9"
FONT_CACHE = pathlib.Path.home() / ".cache" / "heygo-fonts" / "NotoSansSC-Regular.otf"

ASSETS = pathlib.Path(__file__).resolve().parents[1]
VOCAB = ASSETS / "ui" / "strings.json"

ASCII = [chr(c) for c in range(0x20, 0x7F)]
PUNCT = list("，。？！：；、“”‘’（）《》—…·％")

BITMAP_INDEX_MAX = (1 << 20) - 1   # LV_FONT_FMT_TXT_LARGE == 0
ADV_W_MAX = (1 << 12) - 1


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def ensure_font():
    if not FONT_CACHE.exists():
        FONT_CACHE.parent.mkdir(parents=True, exist_ok=True)
        tmp = FONT_CACHE.with_suffix(".part")
        print(f"downloading {FONT_URL}")
        urllib.request.urlretrieve(FONT_URL, tmp)
        tmp.rename(FONT_CACHE)
    got = sha256(FONT_CACHE)
    if got != FONT_SHA256:
        sys.exit(f"font checksum mismatch: expected {FONT_SHA256}, got {got}. Delete {FONT_CACHE} and run again.")
    return FONT_CACHE


def vocab_chars():
    data = json.loads(VOCAB.read_text(encoding="utf-8"))
    out = set()
    for entry in data["strings"].values():
        text = entry["zh"].replace("{n}", "0123456789")
        out.update(text)
    return out


def gb2312_level1():
    chars = []
    for hi in range(0xB0, 0xD8):
        for lo in range(0xA1, 0xFF):
            try:
                chars.append(bytes([hi, lo]).decode("gb2312"))
            except UnicodeDecodeError:
                pass
    return chars


def charset(name):
    chars = set(ASCII) | vocab_chars()
    if name == "text":
        chars |= set(PUNCT) | set(gb2312_level1())
    return sorted(chars, key=ord)


def font_copyright(path):
    name = TTFont(path, lazy=True)["name"]
    rec = name.getDebugName(0) or ""
    return " ".join(rec.split())


def render(font, ch, size):
    """Return (box_w, box_h, ofs_x, ofs_y, adv_w16, pixels) for one glyph.

    ofs_y follows lv_font_conv: the distance from the baseline up to the
    bottom of the bounding box.
    """
    canvas = size * 3
    base_x, base_y = size, size * 2
    img = Image.new("L", (canvas, canvas), 0)
    ImageDraw.Draw(img).text((base_x, base_y), ch, font=font, fill=255, anchor="ls")
    adv = int(round(font.getlength(ch) * 16))
    bbox = img.getbbox()
    if bbox is None:
        return 0, 0, 0, 0, adv, []
    x0, y0, x1, y1 = bbox
    crop = img.crop(bbox)
    pixels = [(v * 15 + 127) // 255 for v in crop.getdata()]
    return x1 - x0, y1 - y0, x0 - base_x, base_y - y1, adv, pixels


def pack4(pixels):
    """Pack 4-bit pixels into a continuous stream, high nibble first."""
    out = bytearray((len(pixels) + 1) // 2)
    for i, v in enumerate(pixels):
        out[i // 2] |= (v << 4) if i % 2 == 0 else v
    return out


def c_array(data, per_line=24):
    lines = []
    for i in range(0, len(data), per_line):
        lines.append("    " + ",".join(f"0x{b:02x}" for b in data[i:i + per_line]) + ",")
    return "\n".join(lines)


def generate(name, size, chars, font_path):
    cmap = TTFont(font_path, lazy=True).getBestCmap()
    missing = [c for c in chars if ord(c) not in cmap and c != " "]
    if missing:
        sys.exit(f"{name}: the source font lacks {len(missing)} characters: {''.join(missing)}")

    ascii_part = [c for c in chars if 0x20 <= ord(c) <= 0x7E]
    if [ord(c) for c in ascii_part] != list(range(0x20, 0x7F)):
        sys.exit(f"{name}: the ASCII range must be complete")
    other = [c for c in chars if ord(c) > 0x7E]
    ordered = ascii_part + other
    start = ord(other[0])
    span = ord(other[-1]) - start + 1
    if span > 0xFFFF:
        sys.exit(f"{name}: the sparse range spans {span} code points, above 65535")

    font = ImageFont.truetype(str(font_path), size)
    ascent, descent = font.getmetrics()

    bitmap = bytearray()
    dsc = ["    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id 0 reserved */,"]
    for ch in ordered:
        w, h, ox, oy, adv, pixels = render(font, ch, size)
        if len(bitmap) > BITMAP_INDEX_MAX or adv > ADV_W_MAX or w > 255 or h > 255:
            sys.exit(f"{name}: glyph U+{ord(ch):04X} exceeds an LVGL field limit")
        if not (-128 <= ox <= 127 and -128 <= oy <= 127):
            sys.exit(f"{name}: glyph U+{ord(ch):04X} offset exceeds int8")
        dsc.append(f"    {{.bitmap_index = {len(bitmap)}, .adv_w = {adv}, .box_w = {w}, .box_h = {h}, "
                   f".ofs_x = {ox}, .ofs_y = {oy}}}, /* U+{ord(ch):04X} */")
        bitmap += pack4(pixels)
    if not bitmap:
        bitmap = bytearray(1)

    unicode_list = [ord(c) - start for c in other]
    copyright_text = font_copyright(font_path)
    src = f"""/* Generated by assets/tools/gen_lv_font.py. Do not edit by hand.
 *
 * Font: {name}. Size: {size} px. Bpp: 4. Glyphs: {len(ordered)}. Bitmap bytes: {len(bitmap)}.
 * Source: Noto Sans SC Regular, SHA-256 {FONT_SHA256}.
 * {copyright_text}
 * License: SIL Open Font License 1.1. See assets/fonts/LICENSE-OFL.txt.
 */

#include "lvgl.h"

static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {{
{c_array(bitmap)}
}};

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {{
{chr(10).join(dsc)}
}};

static const uint16_t unicode_list_1[] = {{
{chr(10).join("    " + ",".join(str(v) for v in unicode_list[i:i + 16]) + "," for i in range(0, len(unicode_list), 16))}
}};

static const lv_font_fmt_txt_cmap_t cmaps[] = {{
    {{
        .range_start = 32, .range_length = 95, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0,
        .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }},
    {{
        .range_start = {start}, .range_length = {span}, .glyph_id_start = 96,
        .unicode_list = unicode_list_1, .glyph_id_ofs_list = NULL, .list_length = {len(unicode_list)},
        .type = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY
    }}
}};

static const lv_font_fmt_txt_dsc_t font_dsc = {{
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 2,
    .bpp = 4,
    .kern_classes = 0,
    .bitmap_format = 0,
}};

const lv_font_t lv_font_{name} = {{
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
    .line_height = {ascent + descent},
    .base_line = {descent},
    .subpx = LV_FONT_SUBPX_NONE,
    .underline_position = -2,
    .underline_thickness = 1,
    .dsc = &font_dsc,
}};
"""
    return src, "".join(ordered), len(bitmap)


def main(argv=None):
    ap = argparse.ArgumentParser(description="Generate an LVGL 9 font from the source font.")
    ap.add_argument("--name", required=True, help="font name, for example heygo_sc_16")
    ap.add_argument("--size", type=int, required=True, help="pixel size")
    ap.add_argument("--charset", choices=["text", "ui"], required=True)
    ap.add_argument("--out-dir", default=str(ASSETS / "fonts"))
    args = ap.parse_args(argv)

    font_path = ensure_font()
    chars = charset(args.charset)
    src, glyphs, nbytes = generate(args.name, args.size, chars, font_path)
    out = pathlib.Path(args.out_dir)
    (out / f"{args.name}.c").write_text(src, encoding="utf-8")
    (out / f"{args.name}.chars.txt").write_text(glyphs + "\n", encoding="utf-8")
    print(f"{args.name}: {len(glyphs)} glyphs, {nbytes} bitmap bytes -> {out / (args.name + '.c')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
