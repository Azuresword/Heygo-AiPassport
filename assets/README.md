[简体中文](README.zh_CN.md) · **English**

# Assets for the device screen

This folder holds the screen text, the Chinese fonts, and the screen mockups for the P0 device.
The firmware in `../Firmware` compiles the fonts from this folder.

## Contents

| Path | Content |
| --- | --- |
| `ui/strings.json` | The 74 strings that the device screen shows, in Chinese and English. The `popup.*` strings and `hint.popup` belong to the popup page (D-31). The `boot.*` strings belong to the boot screen (D-35). |
| `fonts/heygo_sc_16.c` | 16 px font for status, hints, and phone text. 3,869 glyphs. |
| `fonts/heygo_sc_28.c` | 28 px bold font for state words and titles. 185 glyphs. |
| `fonts/heygo_display_20.c`, `fonts/heygo_display_34.c`, `fonts/heygo_display_48.c` | Staatliches headline font, 20 px, 34 px, and 48 px. ASCII only. The 48 px font shows the speed. |
| `fonts/heygo_fonts.h` | Declarations of the four fonts. |
| `fonts/*.chars.txt` | The characters in each font. |
| `fonts/LICENSE-OFL.txt`, `fonts/LICENSE-OFL-Staatliches.txt` | The licenses of the source fonts. |
| `tools/gen_lv_font.py` | Makes a font file from the source font. |
| `tools/check_glyphs.py` | Makes sure that each shown character exists in its font. |
| `sprites/heygo_board_sprite.c`, `sprites/heygo_sprites.h` | The rider sprite of the Turns page: 16 frames of 20 x 20 px, ARGB8888. `heygo_board_sprite.png` is a preview. |
| `sprites/heygo_popup_icons.c`, `sprites/heygo_popup_icons.h` | The popup badges (D-31): 4 kinds of 4 frames, 56 x 56 px, ARGB8888. `tools/gen_popup_icons.py` makes them. `heygo_popup_icons.png` is a preview. The simulator uses `heygo_popup_icons_sheet.png`. |
| `tools/gen_sprite.py` | Makes the rider sprite. |
| `brand/logo_*.png`, `brand/logo_parts.json` | The six white parts of the Heygo logo, and their places, from the Figma file "Logo" (D-35). `tools/fetch_figma_logo.py` exports them. A build does not need Figma. |
| `sprites/heygo_logo.c`, `sprites/heygo_logo.h` | The boot logo in six A8 parts, 200 x 79 px together, 12,288 bytes. `tools/gen_boot_logo.py` makes them from `brand/`. |
| `mockups/27-boot-*.png` to `mockups/29-boot-*.png`, `mockups/boot-anim.gif` | The boot screen at 0.3 s, 1.0 s, and 1.7 s, and its animation. The host renderer draws them. |
| `mockups/` | Screen images from the host screen check. `compare-A-B.png` shows the two design options. Version B is in use. |
| `sim/coach-flow.html` | Interactive simulator of the Coach flow. |
| `sim/turns-demo.gif` | An animation of the Turns page. `Firmware/tools/host_render/turns_gif.py` makes it with the device drawing code. |

## Fonts

Three source fonts exist. All use the SIL Open Font License 1.1:

- Noto Sans SC Regular for body text.
- Noto Sans SC Bold for state words.
- Staatliches, the condensed headline face of the Heygo brand book in Figma.

`gen_lv_font.py` downloads each source font once and checks its SHA-256 digest.
It keeps the source fonts in `~/.cache/heygo-fonts`, outside the repository.

All fonts use the LVGL 9 format `lv_font_fmt_txt` with 4 bits for each pixel and no compression.

| Font | Source | Character set |
| --- | --- | --- |
| `heygo_sc_16` | Noto Sans SC Regular | ASCII, Chinese punctuation, GB2312 level 1 (3,755 characters), and all vocabulary characters. |
| `heygo_sc_28` | Noto Sans SC Bold | ASCII and all vocabulary characters. |
| `heygo_display_20`, `heygo_display_34`, `heygo_display_48` | Staatliches | ASCII only. |

The 16 px font needs common characters because it shows transcripts and answers from the phone.
A character outside this set shows as an empty box.

To make the fonts again, run these commands from this folder:

```bash
python3 tools/gen_lv_font.py --name heygo_sc_16 --size 16 --charset text
python3 tools/gen_lv_font.py --name heygo_sc_28 --size 28 --charset ui --source sc-bold
python3 tools/gen_lv_font.py --name heygo_display_20 --size 20 --charset latin --source staatliches
python3 tools/gen_lv_font.py --name heygo_display_34 --size 34 --charset latin --source staatliches
python3 tools/gen_lv_font.py --name heygo_display_48 --size 48 --charset latin --source staatliches
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
| `01-ski.png` | The live data page with no data yet |
| `02-ready.png` | The Coach page, ready |
| `03-listening.png` | The device records a question |
| `04-thinking.png` | The phone has the question and waits for the answer |
| `05-answering.png` | The device plays the answer |
| `06-answer-max.png` | An answer longer than four lines ends with three dots |
| `07-failed.png` | Speech recognition failed |
| `08-link-lost.png` | The phone link is lost |
| `10-ski-live.png` | Ski data during a run |
| `11-ski-stale.png` | 5 seconds with no packet: the speed shows "--" |
| `12-ski-report.png` | After a run: the report on the Ski page, with the longest texts |
| `13-report-empty.png` | The Report page before the first report |
| `14-report.png` | The Report page with a report |
| `15-report-max.png` | The Report page with the longest texts and the report age |
| `16-turns-empty.png` | The Turns page before the first shape |
| `17-turns-drawing.png` | The Turns page while the first page draws: the rider at the head of the line |
| `18-turns-heel.png` | The Turns page: a heel side turn, the rider in the middle of the turn |
| `19-turns-schematic.png` | The Turns page: the last group of a schematic run of 255 turns, with the schematic tag |
| `20-turns-loading.png` | The Turns page while a page is still on its way from the phone |
| `21-turns-short.png` | A run of one page, on turn 2: the second row names the UP hold |
| `22-popup-fall.png` | A fall alert in red, with the default title, while its audio plays (D-31) |
| `23-popup-voice.png` | A voice message that waits for its audio |
| `24-popup-issue.png` | A problem report during the countdown: "8 秒后返回" (back in 8 seconds) |
| `25-popup-badge.png` | An achievement at the start of the countdown |
| `26-popup-max.png` | A popup with the longest title and body |
| `popup-anim.gif` | The entry animation: the live data page, then the four kinds, each as a new popup |
| `contact-sheet.png` | All 25 images at 2x scale |

## Simulator

Open `sim/coach-flow.html` in a browser. It needs no server and no network.

- Hold a device button for 500 ms to make a hold. Use the keys Up, Space, and Down for the same buttons.
- The diagram and the trigger matrix show the current state and the possible transitions.
- The phone panel answers each question automatically. Select a scenario, or turn off the automatic answer and send each phone event by hand.
- The logs show each transition, its actions, and each BLE message.
- The ski data buttons start a run, pause the packets, and end the run. LIVE_STATS comes once each second, and RUN_REPORT comes 2 seconds after the end.
- The start of a run sends RUN_START first (D-32). The device opens the live data page at once, or after the talk with the coach or the popup ends.
- A short DOWN press on the idle Coach page opens the Report page. A short UP press returns.
- The popup buttons send a popup of each kind, with or without audio (D-31). A bar shows the 10-second countdown. The diagram shows the three popup states in a zone of their own.

The browser draws the screen with system fonts. The images in `mockups/` show the real pixels.
Do not edit the page by hand. Run `python3 tools/flow/flow_sim.py` from `../Firmware`.

## Open items

- The phone can send a character outside GB2312 level 1. The device then shows an empty box for it.
