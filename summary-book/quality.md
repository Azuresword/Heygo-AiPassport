<p class="lang"><strong>English</strong> · <a href="quality.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Quality</p>

# Quality and deployment

## Validation process

At each sprint end, run `./tools/validate.sh` from `Firmware/`. The full gate must exit with code 0.

<figure>
<img src="images/gate-en.svg" alt="Firmware validation process">
<figcaption>Figure 8. Firmware validation checks.</figcaption>
</figure>

| Check | Coverage |
|---|---|
| Host tests | 33 test files cover the link core, keys, turns, popups, ADPCM, and other modules. |
| Glyph check | Checks that screen text has matching font glyphs. |
| `host_render` | Checks screen overlap, pool leaks, and UTF-8 truncation. Generates screen examples. |
| `flow_sim` | Checks that generated files match the flow graph. Runs interaction checks in headless Chrome. |
| Firmware build | Checks compilation, linking, partition layout, and the merged image. |

## Baseline test results

The full gate at version `284ab00` exited with code 0 on 2026-10-18.

<div class="tiles">
<div class="tile accent"><div class="k">Full gate</div><div class="v" style="font-size:24px">PASS</div></div>
<div class="tile accent"><div class="k">Simulator self-test</div><div class="v">128<small>/128</small></div></div>
<div class="tile"><div class="k">Text checked for glyphs</div><div class="v">72<small>+47</small></div></div>
<div class="tile"><div class="k">Merged image</div><div class="v">1.92<small>MiB</small></div></div>
</div>

The glyph check covered 72 screen strings and 47 flow strings. [Firmware build](build-flash.md) describes the image layout and build settings.

## Device acceptance

The project owner recorded board flashing, phone app integration tests, and U1 usability testing with 5 testers as complete. The repository contains no detailed records for these tests. This report therefore does not list them as reproducible device test passes.

Host tests cover logic and rendering. BLE timing, audio quality, device memory, and outdoor usability require device test records.

## Website deployment

| Content | Address |
|---|---|
| Simulator | <https://azuresword.github.io/Heygo-AiPassport/> |
| Technical report | <https://azuresword.github.io/Heygo-AiPassport/summary/> |

GitHub Pages publishes the root of `main`. The root `.nojekyll` file enables direct static publication. The simulator entry redirects to `assets/sim/coach-flow.html`. The report is in `summary/`.

## Automation scope

GitHub Actions currently lists only the Pages deployment workflow. Firmware workflows are in `Firmware/.github/workflows/`. GitHub discovers workflows only in `.github/workflows/` at the repository root. Firmware validation therefore runs in the development environment. A successful Pages deployment does not establish that firmware validation passed.
