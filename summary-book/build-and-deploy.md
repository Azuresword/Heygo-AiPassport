<p align="right"><strong>English</strong> · <a href="build-and-deploy.zh_CN.html">简体中文</a></p>

# Checks and deployment

## Gate

1. Activate ESP-IDF 5.5.3.
2. In `Firmware`, run `./tools/validate.sh`.
3. Make sure that the last line is `Firmware build: PASS`.

The gate also runs these checks (`Firmware/docs/heygo-changes.md`, section 3):

| Check | What it finds |
|---|---|
| Host tests | 33 `test_*` files in `Firmware/tests/`. |
| Glyph check | Screen text that the font cannot show. |
| `host_render` | Screen overlap, memory pool leaks, and cut UTF-8 text. |
| `flow_sim --check` and `--run-selftest` | Differences between the simulator and the state machine. The self-test runs in headless Chrome. |

Last recorded result: host tests pass, and the simulator self-test is 117/117.

**These are host results. They do not prove that the device works.**

`validate.sh` lines 60 to 63 support macOS.
The Apple linker has no `--gc-sections`, so the script uses `-dead_strip`.

## CI

The workflows are in `Firmware/.github/workflows/`: build-firmware, firmware-checks, static-checks, and sync-main.

GitHub reads workflows only from `.github/workflows/` at the repository root.
This repository has no `.github/` at the root.
Thus, these workflows possibly do not run.
This is an inference. We did not examine the Actions page.

## Deployment

- GitHub Pages publishes the root of `main`. The root has a `.nojekyll` file.
- `index.html` opens `assets/sim/coach-flow.html`.
- This book is in `summary/`. After you change `summary-book/`, run `summary-book/build.sh`.
