<p class="lang"><strong>English</strong> · <a href="memory.zh_CN.html">简体中文</a></p>

<p class="eyebrow">Memory</p>

# Memory map

Build snapshot: 2026-10-18, commit `284ab00`. Flash and static RAM values describe this build. LVGL pool values describe host rendering, not device measurements. KiB and MiB denote 1,024 bytes and 1,048,576 bytes.

<div class="tiles">
<div class="tile accent"><div class="k">App / factory</div><div class="v">23.4<small>%</small></div></div>
<div class="tile accent"><div class="k">Static internal RAM</div><div class="v">47.2<small>%</small></div></div>
<div class="tile"><div class="k">Host LVGL pool peak</div><div class="v">76.6<small>%</small></div></div>
<div class="tile"><div class="k">PSRAM</div><div class="v">0</div></div>
</div>

<a id="flash-the-8-mb-layout"></a>

## Flash: the 8 MiB layout

<figure>
<img src="images/flash-map-en.svg" alt="Flash map">
<figcaption>Figure 12. Flash layout. The top bar shows 8 MiB. The bottom bar shows the first 64 KiB.</figcaption>
</figure>

| Region | Start | Size | Content |
|---|---|---|---|
| Bootloader area | `0x0` | 32 KiB | Bootloader image: 21,024 bytes |
| Partition table area | `0x8000` | 4 KiB | Partition table image: 3,072 bytes |
| `nvs` | `0x9000` | 24 KiB | Key-value storage |
| `phy_init` | `0xF000` | 4 KiB | PHY initialization data |
| `factory` | `0x10000` | 8,323,072 bytes | The only application partition. It occupies the remaining flash. No OTA partition exists. |

The application image occupies 1,948,640 bytes, or 23.4% of `factory`. The partition has 6,374,432 bytes (76.6%) of unused capacity.

## Flash: the content of the application image

Flash-resident code occupies 825,880 bytes. Read-only data occupies 1,014,976 bytes, plus a 256-byte application descriptor. These sections total 1,841,112 bytes. The complete application image also includes RAM-loaded sections, image headers, and padding.

{% include "./_partials/flash-parts-en.html" %}

| Component | Bytes | Content |
|---|---|---|
| Chinese font 16 px | 516,932 | `heygo_sc_16`, 3,872 glyphs |
| Other ESP-IDF parts | 439,142 | Remaining SDK components and section overhead |
| LVGL | 332,036 | Flash-resident code and read-only data |
| Popup badges | 201,152 | Four kinds × four frames, 56 × 56 ARGB8888 |
| BLE controller and NimBLE | 168,668 | Bluetooth controller and host stack |
| Other fonts and rider sprite | 127,789 | `heygo_sc_28`, `heygo_display_20/34/48`, and `heygo_board_sprite` |
| Heygo app code | 55,393 | Application code and constants, excluding fonts and images |

<div class="callout"><b>Fonts and images occupy 45.9% of these flash sections.</b>
They total 845,873 bytes. The 16 px Chinese font is the largest asset. It includes GB2312 level 1 for common Chinese transcripts and answers. Characters outside its glyph set need additional coverage. App code and constants occupy approximately 54.1 KiB.</div>

<a id="ram-the-320-kb-layout"></a>

## RAM: shared instruction and data memory

The linker reports a 321,296-byte region shared by instruction RAM (IRAM) and data RAM (DRAM). Static allocation uses 151,576 bytes, or 47.2%. This percentage includes code in IRAM. It is not a DRAM-only measurement or the chip's total physical SRAM capacity.

{% include "./_partials/dram-parts-en.html" %}

| Section | Bytes | Description |
|---|---|---|
| IRAM code | 96,852 | Code in RAM, including interrupt, flash, and FreeRTOS routines |
| `.bss` | 44,200 | Zero-initialized data. LVGL uses 29,180 bytes, including its 28 KiB pool. The app uses 9,156 bytes. |
| `.data` | 10,524 | Data with initial values |
| Link-time remainder | 169,720 | Capacity before runtime allocations. This is not measured free heap. |

### Heap use at runtime

The following configured allocations consume part of the remaining RAM. Queue figures count item storage only. They exclude FreeRTOS control structures and allocator overhead.

| Use | Bytes | Calculation or configuration |
|---|---|---|
| LVGL draw buffer | 19,200 | 240 × 40 lines × 2 bytes, RGB565 |
| App task stacks | 11,264 | voice 4,096 + coach 4,096 + ble_tx 3,072 |
| LVGL task stack | 7,168 | esp_lvgl_port default |
| NimBLE host task stack | 4,096 | `CONFIG_BT_NIMBLE_HOST_TASK_STACK_SIZE` |
| BLE TX queues | 3,936 | Two queues × 8 items × 246 bytes, including each item's length field |
| Audio down queue | 1,312 | 8 × 164-byte ADPCM blocks |

This table is not a complete heap budget. BLE buffers, I2S DMA, the coach event queue, timers, task control blocks, and other allocations also consume RAM. Device tests must measure free heap, minimum free heap, and the largest available block.

## LVGL pool: 28 KiB

`CONFIG_LV_MEM_SIZE_KILOBYTES=28` reserves 28,672 bytes for LVGL. The host allocator reports 25,752 usable bytes after management overhead.

<div class="callout"><b>The page rail required a larger pool.</b>
The 24 KiB configuration stalled during speed-digit rendering, with a largest free block of 1,936 bytes. Increasing the pool to 28 KiB adds 4,096 bytes of static RAM. LVGL <code>.bss</code> increased from 25,084 to 29,180 bytes.</div>

| Measurement | Value | Conditions |
|---|---|---|
| Usable pool | 25,752 bytes | 28 KiB configuration, 64-bit host |
| Full-run peak | **19,728 / 25,752 bytes (76.6%)** | Host rendering |
| Reported largest free block | At least 6,032 bytes | Page samples |
| Rail allocation | One persistent LVGL object | Shared across pages |
| Peak before the rail | 18,760 / 21,656 bytes (86.6%) | Earlier 24 KiB configuration, commit `5658911` |

The page samples do not establish the largest free block at the instant of peak allocation. Host pointers use eight bytes, while ESP32-C3 pointers use four bytes. Device allocation sizes can differ. Host measurements do not establish the device's memory margin.

Each page change logs LVGL usage, the largest free pool block, current free heap, and minimum free heap. These measurements support device memory tests.
