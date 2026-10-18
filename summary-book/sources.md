<p class="lang"><strong>English</strong> · <a href="sources.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Sources</p>

# Sources

## Origin of the data

| Content | Source |
|---|---|
| Sprint status | The project owner confirmed it on 2026-10-18. |
| Requirements, decisions, protocol, and sprint goals | `需求文件夹/01` to `08`, at `main` commit `c865ba8` |
| Hardware | `Firmware/docs/README.md`, hardware section |
| Gate and known limits | `Firmware/docs/heygo-changes.md` |
| Commits, times, and authors | `git rev-list` and `git log` |
| Simulator self-test 128/128 | `flow_sim.py --run-selftest` in the full gate at `5100fd2` on 2026-10-18 |
| Full gate, image sizes, and memory sections | `./tools/validate.sh`, `idf.py size`, `size-components`, and `size-files` at `5100fd2` on 2026-10-18 |
| Tasks, queues, and stacks | `main/coach_app.c`, `main/audio/voice.c`, `main/link/ble_link.c`, esp_lvgl_port, and the generated `sdkconfig` |
| Screens and animations | `assets/mockups/` and `assets/sim/` |
| Architecture, sequence, gate, and commit figures | `summary-book/figures.py` draws them from the sources above. |

## Inferences

| Statement | Basis |
|---|---|
| CI possibly does not run. | GitHub reads only `.github/workflows/` at the root. We did not examine the Actions page. |
| "41 hours" | The time between the first and the last commit. |

## Scope

This book is about AI Passport only.
