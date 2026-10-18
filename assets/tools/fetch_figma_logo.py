#!/usr/bin/env python3
"""fetch_figma_logo - get the parts of the Heygo logo from Figma for the boot screen (D-35).

The source is the final white logo in the Logo file: page "Logo定稿", frame "2".
The logo has six parts. The boot animation moves each part on its own:

  h, e, y   The letters "Hey".
  g, o      The letters "go".
  board     The snowboard under "go".

The script exports each part at 4 times the screen size, and then reduces it, so the
edges stay smooth. The wordmark is WIDTH px wide on the screen.

Output, in assets/brand:
  logo_<part>.png   The white part with alpha, at screen size.
  logo_parts.json   The size of the wordmark, and the place of each part in it.

The PNG files are in the repository. A build does not need Figma.
Run again only when the logo changes in Figma:
  python3 assets/tools/fetch_figma_logo.py  (needs Pillow, and a Figma token in
  FIGMA_TOKEN or ~/.config/figma/token)
"""
import io
import json
import os
import sys
import urllib.request
from pathlib import Path

from PIL import Image

FILE_KEY = "VlZUEzGoyj2N4gvjXr5zxE"     # figma.com/design/VlZUEzGoyj2N4gvjXr5zxE/Logo
GROUP = "665:23"                        # The wordmark with the board, in frame "2".
PARTS = {"h": "665:25", "e": "665:26", "y": "665:27", "g": "665:29", "o": "665:34", "board": "665:35"}
WIDTH = 200                             # The wordmark width on the screen, in px.
OVER = 4                                # Export at 4x, then reduce.
OUT = Path(__file__).resolve().parent.parent / "brand"


def token():
    t = os.environ.get("FIGMA_TOKEN")
    if not t:
        path = Path.home() / ".config" / "figma" / "token"
        if path.exists():
            t = path.read_text().strip()
    if not t:
        sys.exit("ERROR: no Figma token. Set FIGMA_TOKEN or write ~/.config/figma/token.")
    return t


def api(path, tok):
    req = urllib.request.Request("https://api.figma.com/v1/" + path, headers={"X-Figma-Token": tok})
    with urllib.request.urlopen(req, timeout=60) as r:
        return json.load(r)


def main():
    tok = token()
    ids = ",".join([GROUP] + list(PARTS.values()))
    nodes = api(f"files/{FILE_KEY}/nodes?ids={ids}", tok)["nodes"]
    box = {k: v["document"]["absoluteBoundingBox"] for k, v in nodes.items()}
    g = box[GROUP]
    scale = WIDTH / g["width"]
    urls = api(f"images/{FILE_KEY}?ids={','.join(PARTS.values())}&format=png&scale={scale * OVER:.5f}", tok)["images"]
    OUT.mkdir(parents=True, exist_ok=True)
    meta = {"source": f"figma.com/design/{FILE_KEY}/Logo, node {GROUP}",
            "width": WIDTH, "height": round(g["height"] * scale), "parts": {}}
    for name, nid in PARTS.items():
        with urllib.request.urlopen(urls[nid], timeout=60) as r:
            big = Image.open(io.BytesIO(r.read())).convert("RGBA")
        b = box[nid]
        w, h = max(1, round(b["width"] * scale)), max(1, round(b["height"] * scale))
        img = big.resize((w, h), Image.LANCZOS)
        img.save(OUT / f"logo_{name}.png")
        meta["parts"][name] = {"x": round((b["x"] - g["x"]) * scale), "y": round((b["y"] - g["y"]) * scale),
                               "w": w, "h": h}
        print(f"logo_{name}.png  {w} x {h} at {meta['parts'][name]['x']}, {meta['parts'][name]['y']}")
    (OUT / "logo_parts.json").write_text(json.dumps(meta, indent=2) + "\n")
    print(f"Wrote {len(PARTS)} parts to {OUT} (wordmark {meta['width']} x {meta['height']} px)")


if __name__ == "__main__":
    main()
