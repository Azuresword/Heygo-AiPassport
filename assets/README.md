[简体中文](README.zh_CN.md) · **English**

# Assets for the device screen

This folder holds the screen text, the Chinese fonts, and the screen mockups for the P0 device.
The firmware in `../Firmware` compiles the fonts from this folder.

## Contents

| Path | Content |
| --- | --- |
| `ui/strings.json` | The 33 strings that the P0 screen shows, in Chinese and English. |
| `fonts/heygo_sc_16.c` | 16 px font for status, hints, and phone text. 3,869 glyphs. |
| `fonts/heygo_sc_28.c` | 28 px font for state words and titles. 176 glyphs. |
| `fonts/heygo_fonts.h` | Declarations of the two fonts. |
| `fonts/*.chars.txt` | The characters in each font. |
| `fonts/LICENSE-OFL.txt` | The license of the source font. |
| `tools/gen_lv_font.py` | Makes a font file from the source font. |
| `tools/check_glyphs.py` | Makes sure that each shown character exists in its font. |
| `mockups/` | Screen images from the host screen check. |
| `sim/coach-flow.html` | Interactive simulator of the Coach flow. |

## Fonts

The source font is Noto Sans SC Regular, under the SIL Open Font License 1.1.
`gen_lv_font.py` downloads it once and checks its SHA-256 digest.
It keeps the source font in `~/.cache/heygo-fonts`, outside the repository.

Both fonts use the LVGL 9 format `lv_font_fmt_txt` with 4 bits for each pixel and no compression.

| Font | Character set |
| --- | --- |
| `heygo_sc_16` | ASCII, Chinese punctuation, GB2312 level 1 (3,755 characters), and all vocabulary characters. |
| `heygo_sc_28` | ASCII and all vocabulary characters. |

The 16 px font needs common characters because it shows transcripts and answers from the phone.
A character outside this set shows as an empty box.

To make the fonts again, run these commands from this folder:

```bash
python3 tools/gen_lv_font.py --name heygo_sc_16 --size 16 --charset text
python3 tools/gen_lv_font.py --name heygo_sc_28 --size 28 --charset ui
python3 tools/check_glyphs.py
```

The tools need Python 3, Pillow, and fontTools.

## Text changes

1. Add or change the string in `ui/strings.json`.
2. Set `size` to 28 if the string uses the large font.
3. Make the fonts again if the string has a new character.
4. Run `python3 tools/check_glyphs.py`.

The firmware gate also runs `check_glyphs.py`. A missing glyph stops the gate.

## Mockups

The images in `mockups/` come from LVGL. They are not drawings.
The host screen check draws the generated screens with the device settings and these fonts.
To make the images again, run `python3 tools/host_render/render.py` from `../Firmware`.

| Image | State |
| --- | --- |
| `01-ski.png` | Ski mode |
| `02-ready.png` | Rest mode, ready |
| `03-listening.png` | The device records a question |
| `04-thinking.png` | The phone has the question and waits for the answer |
| `05-answering.png` | The device plays the answer |
| `06-answer-max.png` | An answer longer than four lines ends with three dots |
| `07-failed.png` | Speech recognition failed |
| `08-link-lost.png` | The phone link is lost |
| `09-exit.png` | The exit question |
| `contact-sheet.png` | All nine images at 2x scale |

## Simulator

Open `sim/coach-flow.html` in a browser. It needs no server and no network.

- Hold a device button for 500 ms to make a hold. Use the keys Up, Space, and Down for the same buttons.
- The diagram and the trigger matrix show the current state and the possible transitions.
- The phone panel answers each question automatically. Select a scenario, or turn off the automatic answer and send each phone event by hand.
- The logs show each transition, its actions, and each BLE message.

The browser draws the screen with system fonts. The images in `mockups/` show the real pixels.
Do not edit the page by hand. Run `python3 tools/flow/flow_sim.py` from `../Firmware`.

## Open items

- The phone can send a character outside GB2312 level 1. The device then shows an empty box for it.
