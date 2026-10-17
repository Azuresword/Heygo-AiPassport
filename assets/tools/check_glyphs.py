#!/usr/bin/env python3
"""check_glyphs - prove that every shown character exists in its font.

A successful build proves nothing about Chinese glyphs. LVGL shows an empty
box for a missing glyph and reports no error. This check fails instead.

Checks:
  1. Every vocabulary string fits in heygo_sc_16.
  2. Every vocabulary string with size 28 fits in heygo_sc_28.
  3. In each Firmware flow graph, every label text and every string subject
     bound to a label fits in the font of that label.
"""

import json
import pathlib
import sys

ASSETS = pathlib.Path(__file__).resolve().parents[1]
REPO = ASSETS.parent
FLOWS = REPO / "Firmware" / "flows"


def font_chars(name):
    path = ASSETS / "fonts" / f"{name}.chars.txt"
    if not path.exists():
        return None
    return set(path.read_text(encoding="utf-8").rstrip("\n"))


def missing(text, chars):
    return sorted({c for c in text if c not in chars and c not in "\n"}, key=ord)


def main():
    errors = []
    # Every generated font has a glyph list. A flow label can use any of them.
    names = sorted(p.name[:-len(".chars.txt")] for p in (ASSETS / "fonts").glob("*.chars.txt"))
    fonts = {name: font_chars(name) for name in sorted(set(names) | {"heygo_sc_16", "heygo_sc_28"})}
    for name, chars in fonts.items():
        if chars is None:
            errors.append(f"{name}: no glyph list. Run tools/gen_lv_font.py first.")
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1

    vocab = json.loads((ASSETS / "ui" / "strings.json").read_text(encoding="utf-8"))["strings"]
    for key, entry in vocab.items():
        text = entry["zh"].replace("{n}", "0123456789")
        for c in missing(text, fonts["heygo_sc_16"]):
            errors.append(f"vocabulary {key}: heygo_sc_16 lacks {c!r} (U+{ord(c):04X})")
        if entry.get("size") == 28:
            for c in missing(text, fonts["heygo_sc_28"]):
                errors.append(f"vocabulary {key}: heygo_sc_28 lacks {c!r} (U+{ord(c):04X})")

    checked = 0
    for flow in sorted(FLOWS.glob("*.flow.json")):
        g = json.loads(flow.read_text(encoding="utf-8"))
        widget_font = {}
        for scr in g.get("screens", []):
            for w in scr.get("widgets", []):
                font = w.get("font")
                widget_font[(scr["id"], w["id"])] = font
                if w["type"] == "label" and font in fonts and w.get("text"):
                    checked += 1
                    for c in missing(w["text"], fonts[font]):
                        errors.append(f"{flow.name} {scr['id']}.{w['id']}: {font} lacks {c!r} (U+{ord(c):04X})")
        subjects = {s["id"]: s for s in g.get("subjects", [])}
        first = g["screens"][0]["id"] if g.get("screens") else None
        for b in g.get("bindings", []):
            wid, prop = b["target"].rsplit(".", 1)
            subj = subjects.get(b["subject"])
            font = widget_font.get((first, wid))
            if prop != "text" or not subj or subj["type"] != "string" or font not in fonts:
                continue
            checked += 1
            for c in missing(subj.get("initial", ""), fonts[font]):
                errors.append(f"{flow.name} subject {subj['id']}: {font} lacks {c!r} (U+{ord(c):04X})")

    if errors:
        print("\n".join(errors), file=sys.stderr)
        print(f"glyph coverage: FAIL ({len(errors)} missing)", file=sys.stderr)
        return 1
    print(f"glyph coverage: PASS ({len(vocab)} vocabulary strings, {checked} flow texts)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
