<p class="lang"><strong>English</strong> · <a href="memory.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Memory</p>

# Memory map

The numbers on this page come from a build on 2026-10-18 at commit `df81cc8`. `Firmware/` is the same as at `c865ba8`. The tools were `idf.py size`, `idf.py size-components`, and `idf.py size-files`. The LVGL pool numbers come from host measurements (`Firmware/docs/heygo-changes.md`).

<div class="tiles">
<div class="tile accent"><div class="k">App / factory</div><div class="v">23.4<small>%</small></div></div>
<div class="tile accent"><div class="k">Static DRAM</div><div class="v">45.9<small>%</small></div></div>
<div class="tile"><div class="k">LVGL pool peak</div><div class="v">86.6<small>%</small></div></div>
<div class="tile"><div class="k">PSRAM</div><div class="v">0</div></div>
</div>

## Flash: the 8 MB layout

<figure>
<img src="images/flash-map-en.svg" alt="Flash map">
<figcaption>Figure 12. Flash map. The top bar is the full 8 MB. The bottom bar shows the first 64 KB. Sources: <code>Firmware/partitions.csv</code> and the build log.</figcaption>
</figure>

| Region | Start | Size | Content |
|---|---|---|---|
| Bootloader | `0x0` | 32 KB | 21,024 bytes used |
| Partition table | `0x8000` | 4 KB | |
| `nvs` | `0x9000` | 24 KB | Key-value storage |
| `phy_init` | `0xF000` | 4 KB | Radio calibration data |
| `factory` | `0x10000` | 8,323,072 bytes | The only application partition. It uses all remaining flash. There are no OTA partitions. |

The application image is 1,945,536 bytes. The `factory` partition has 6,377,536 bytes (76.6%) free.

## Flash: the content of the application image

Read-only data is 1,015,120 bytes. Code is 822,896 bytes. The total is 1,838,016 bytes. By source:

{% include "./_partials/flash-parts-en.html" %}

**Table view**

| Source | Bytes | Basis |
|---|---|---|
| Chinese font 16 px | 516,932 | `heygo_sc_16.c.obj`, 3,871 glyphs |
| Other ESP-IDF parts | 438,960 | The total minus the other rows |
| LVGL | 331,660 | Code and read-only data of `liblvgl__lvgl.a` |
| Popup badges | 201,152 | `heygo_popup_icons.c.obj`, 4 kinds × 4 frames, 56×56 ARGB8888 |
| BLE controller and NimBLE | 168,668 | `libbtdm_app.a` and `libbt.a` |
| Other fonts and rider sprite | 127,789 | `heygo_sc_28`, `heygo_display_20/34/48`, and `heygo_board_sprite` |
| Heygo app code | 52,855 | `libmain.a` without fonts and images |

<div class="callout"><b>Result: fonts and images use almost half of the flash content.</b>
Fonts and sprites use 845,873 bytes, 46% of the flash content. The 16 px Chinese font is the largest. It must show any answer from the phone, so it has GB2312 level 1. The Heygo app code is only approximately 52 KB.</div>

## RAM: the 320 KB layout

The ESP32-C3 has 321,296 bytes of internal RAM for this use. It has no PSRAM. The chart shows the static allocation:

{% include "./_partials/dram-parts-en.html" %}

**Table view**

| Section | Bytes | Description |
|---|---|---|
| IRAM code | 96,852 | Code that must run from RAM, for example interrupts, flash operations, and FreeRTOS |
| `.bss` | 40,088 | LVGL 25,084 (with the 24 KiB pool) and the app 9,140 |
| `.data` | 10,516 | Global variables with initial values |
| Free at runtime | 173,840 | Heap for task stacks, the BLE stack, the LVGL draw buffer, and queues |

### Heap use at runtime

| Use | Bytes | Source |
|---|---|---|
| LVGL draw buffer | 19,200 | 240 × 40 lines × 2 bytes (RGB565), `bsp_display_lvgl.c:12` |
| Own task stacks | 11,264 | voice 4,096, coach 4,096, and ble_tx 3,072 |
| LVGL task stack | 7,168 | esp_lvgl_port default |
| NimBLE host task stack | 4,096 | `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE` |
| BLE packet queues | 3,440 | Document 07, section 10 |
| Audio down queue | 1,312 | 8 × 164 bytes |

The BLE controller and stack also allocate heap memory at runtime. A device measurement is necessary for that part. This page does not show it.

## LVGL pool: 24 KiB

`CONFIG_LV_MEM_SIZE_KILOBYTES=24`. After the management overhead, 21,656 bytes are available.

| Screen | Use | Source |
|---|---|---|
| First build of the Coach screen | +6,768 | `heygo-changes.md`, section 5 |
| Turns page | 16,824 | Section 11 |
| Popup page (longest text) | 13,288 | Section 12 |
| **Peak of a full run** | **18,760 / 21,656 (86.6%)** | Section 12, commit `5658911` |

These numbers come from a 64-bit host. A host pointer has 8 bytes. An ESP32-C3 pointer has 4 bytes. Thus, the device uses less pool memory. The pool is small, so the firmware keeps only one screen at a time.
