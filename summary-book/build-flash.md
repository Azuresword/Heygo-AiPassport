<p class="lang"><strong>English</strong> · <a href="build-flash.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Build</p>

# Firmware build

## From source files to firmware

<figure>
<img src="images/fw-build-en.svg" alt="Firmware build steps">
<figcaption>Figure 11. Generation, validation, and firmware packaging.</figcaption>
</figure>

The `284ab00` snapshot uses four types of generated output:

| Output | Input | Script | Files |
|---|---|---|---|
| State machine and screens | `flows/coach.flow.json` | `tools/flow/flowc.py` | Four `.c/.h` pairs in `main/generated/` |
| Simulator | The same flow graph | `tools/flow/flow_sim.py` | `assets/sim/coach-flow.html` |
| Fonts | Noto Sans SC and Staatliches, OFL 1.1, with character lists | `assets/tools/gen_lv_font.py` | `assets/fonts/heygo_*.c` |
| Sprites | Drawing definitions | `gen_sprite.py`, `gen_popup_icons.py` | `assets/sprites/*.c` |

The state machine and simulator share the same graph. This keeps their declared transitions aligned. Simulator tests do not establish equivalent BLE timing, audio behavior, scheduling, or hardware rendering. `flowc.py --check` and `flow_sim.py --check` reject stale generated files.

### Fonts

| Font | Glyphs | Use |
|---|---|---|
| `heygo_sc_16` | 3,872 | Body text, transcripts, and answers. Includes 3,755 GB2312 level 1 characters, ASCII, punctuation, and application vocabulary. |
| `heygo_sc_28` | 203 | Large state labels, ASCII, and application vocabulary. |
| `heygo_display_20/34/48` | 96 each | Staatliches for numbers and English labels. Includes 95 printable ASCII characters and the degree sign. |

Counts include the space character. The Chinese fonts support a fixed glyph set, not unrestricted Unicode text.

## Build

1. Install ESP-IDF 5.5.3.
2. Activate the environment with `. "$IDF_PATH/export.sh"`.
3. Open the `Firmware/` directory.
4. Run `./tools/validate.sh`.

The gate runs repository checks, host tests, generated-file checks, and glyph checks. It then builds firmware in a temporary directory with a separate `sdkconfig`. Next, it merges the image, checks image offsets and partitions, renders screens, and archives debug files.

Use `./tools/validate.sh --static` for checks without a firmware build. Use `./tools/validate.sh --firmware` for the firmware build and image checks.

| Output | Content | Location |
|---|---|---|
| Merged image | Bootloader, partition table, and application | `build/FoloToy-AI-Passport-full.bin` |
| Debug archive | Images, ELF, map, manifest, and flash arguments | `build/firmware/<full-image-sha256>/` |
| Application image | Application without bootloader or partition table | `build/firmware/<full-image-sha256>/FoloToy-AI-Passport.bin` |

The temporary build directory exists only during the gate. A separate `idf.py build` retains the build directory for `idf.py -p <port> flash`.

<a id="build-results-for-this-book"></a>

## Build snapshot

The full gate completed on 2026-10-18 at commit `284ab00`, with exit code 0. These results describe that snapshot and do not establish hardware acceptance.

<div class="tiles">
<div class="tile accent"><div class="k">Gate</div><div class="v" style="font-size:24px">PASS</div></div>
<div class="tile"><div class="k">App image</div><div class="v">1.86<small>MiB</small></div></div>
<div class="tile"><div class="k">Merged image</div><div class="v">1.92<small>MiB</small></div></div>
<div class="tile"><div class="k">Build steps</div><div class="v">1,995</div></div>
</div>

| Item | Result |
|---|---|
| Host tests | PASS: protocol, ADPCM, playback rules, keys, link core, and screen components |
| Simulator | `--check` PASS, self-test 128/128 |
| Glyphs | PASS, 72 vocabulary strings and 47 flow texts |
| Application layout | 1,948,640 bytes in the 8,323,072-byte `factory` partition at `0x10000` |
| Merged image | 2,014,176 bytes, flash address `0x0` |

Compiler settings use `-Og` debug optimization and assertions. Flash settings use DIO mode at 80 MHz.

## Flashing

| Case | Method | Data effect |
|---|---|---|
| New device or full refresh | Write `FoloToy-AI-Passport-full.bin` at `0x0`. | The merged file fills gaps between images. It can overwrite NVS and PHY initialization data. |
| Daily development | Run `idf.py -p <port> flash` from a retained build. | Segmented flashing preserves NVS when the partition layout remains compatible. |
| Full-chip erase | Run `idf.py erase-flash`. | Erases all user data. Use only for an intentional full erase. |

The application-only image requires a compatible bootloader and partition table. It is unsuitable for initial provisioning by itself.

Logs and flashing use the ESP32-C3 USB Serial/JTAG interface on GPIO18 and GPIO19. Firmware backup is not a prerequisite for flashing. The single factory partition provides no firmware rollback slot.
