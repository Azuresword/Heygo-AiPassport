**简体中文** · [English](README.md)

# 设备屏幕资源

本文件夹存放 P0 设备屏幕的文字、中文字体和界面效果图。
`../Firmware` 里的固件从本文件夹编译字体。

## 内容

| 路径 | 内容 |
| --- | --- |
| `ui/strings.json` | 设备屏幕显示的 71 条文字，有中文和英文。`popup.*` 和 `hint.popup` 属于弹窗页（D-31）。 |
| `fonts/heygo_sc_16.c` | 16 px 字体，用于状态、提示和手机发来的文字。3,869 个字形。 |
| `fonts/heygo_sc_28.c` | 28 px 粗体，用于状态词和标题。185 个字形。 |
| `fonts/heygo_display_20.c`、`fonts/heygo_display_34.c`、`fonts/heygo_display_48.c` | Staatliches 展示字体，20 px、34 px 和 48 px。只含 ASCII。48 px 用于显示速度。 |
| `fonts/heygo_fonts.h` | 四个字体的声明。 |
| `fonts/*.chars.txt` | 每个字体包含的字符。 |
| `fonts/LICENSE-OFL.txt`、`fonts/LICENSE-OFL-Staatliches.txt` | 源字体的许可证。 |
| `tools/gen_lv_font.py` | 从源字体生成字体文件。 |
| `tools/check_glyphs.py` | 确认每个要显示的字符都在对应字体里。 |
| `sprites/heygo_board_sprite.c`、`sprites/heygo_sprites.h` | 弯型页的小精灵图：16 帧，每帧 20 × 20 px，ARGB8888。`heygo_board_sprite.png` 是预览图。 |
| `sprites/heygo_popup_icons.c`、`sprites/heygo_popup_icons.h` | 弹窗徽章（D-31）：4 种类型各 4 帧，56 × 56 px，ARGB8888，由 `tools/gen_popup_icons.py` 生成。`heygo_popup_icons.png` 是预览图，模拟器使用 `heygo_popup_icons_sheet.png`。 |
| `tools/gen_sprite.py` | 生成小精灵图。 |
| `mockups/` | 主机界面检查生成的屏幕图片。`compare-A-B.png` 是两个设计方案的对比，目前采用 B 版。 |
| `sim/coach-flow.html` | Coach 流程的交互模拟器。 |

## 字体

共有三个源字体，许可证都是 SIL Open Font License 1.1：

- Noto Sans SC Regular，用于正文。
- Noto Sans SC Bold，用于状态词。
- Staatliches，Figma 里 Heygo 品牌手册使用的窄体展示字体。

`gen_lv_font.py` 每个源字体只下载一次，并校验它的 SHA-256 摘要。
源字体保存在 `~/.cache/heygo-fonts`，不放进仓库。

所有字体都使用 LVGL 9 的 `lv_font_fmt_txt` 格式，每像素 4 位，不压缩。

| 字体 | 源字体 | 字符集 |
| --- | --- | --- |
| `heygo_sc_16` | Noto Sans SC Regular | ASCII、中文标点、GB2312 一级字（3,755 个）和全部词汇表字符。 |
| `heygo_sc_28` | Noto Sans SC Bold | ASCII 和全部词汇表字符。 |
| `heygo_display_20`、`heygo_display_34`、`heygo_display_48` | Staatliches | 只含 ASCII。 |

16 px 字体要显示手机发来的转写和回答，所以需要常用字。
字符集以外的字符会显示成空框。

要重新生成字体，在本文件夹里运行以下命令：

```bash
python3 tools/gen_lv_font.py --name heygo_sc_16 --size 16 --charset text
python3 tools/gen_lv_font.py --name heygo_sc_28 --size 28 --charset ui --source sc-bold
python3 tools/gen_lv_font.py --name heygo_display_20 --size 20 --charset latin --source staatliches
python3 tools/gen_lv_font.py --name heygo_display_34 --size 34 --charset latin --source staatliches
python3 tools/gen_lv_font.py --name heygo_display_48 --size 48 --charset latin --source staatliches
python3 tools/check_glyphs.py
```

这些工具需要 Python 3、Pillow 和 fontTools。

## 修改文字

1. 在 `ui/strings.json` 里添加或修改文字。
2. 如果文字使用大字体，把 `size` 设为 28。
3. 如果文字里有新字符，重新生成字体。
4. 运行 `python3 tools/check_glyphs.py`。

固件门禁也会运行 `check_glyphs.py`。缺少字形时，门禁停止。

## 效果图

`mockups/` 里的图片由 LVGL 绘制，不是手绘稿。
主机界面检查用设备配置和这些字体绘制生成的界面。
要重新生成图片，在 `../Firmware` 里运行 `python3 tools/host_render/render.py`。

| 图片 | 状态 |
| --- | --- |
| `01-ski.png` | 实时数据页，还没有数据 |
| `02-ready.png` | 教练页，就绪 |
| `03-listening.png` | 设备正在录制问题 |
| `04-thinking.png` | 手机已收到问题，正在等回答 |
| `05-answering.png` | 设备正在播放回答 |
| `06-answer-max.png` | 超过四行的回答以三个点结尾 |
| `07-failed.png` | 语音识别失败 |
| `08-link-lost.png` | 与手机的连接断开 |
| `10-ski-live.png` | 滑行中的实时数据 |
| `11-ski-stale.png` | 5 秒没有数据包：速度显示"--" |
| `12-ski-report.png` | 一趟结束后：滑行页显示报告，含最长文本 |
| `13-report-empty.png` | 还没有报告时的报告页 |
| `14-report.png` | 有报告的报告页 |
| `15-report-max.png` | 最长文本和报告时间的报告页 |
| `16-turns-empty.png` | 还没有弯型时的弯型页 |
| `17-turns-drawing.png` | 打开时第一页正在画：小精灵在线头 |
| `18-turns-heel.png` | 弯型页：一个 HEEL SIDE 弯，小精灵在弯的中段 |
| `19-turns-schematic.png` | 弯型页：255 个弯的示意弯型的最后一组，显示"示意" |
| `20-turns-loading.png` | 弯型页：某一页还在从手机传来的路上 |
| `21-turns-short.png` | 只有一页的一趟，停在第 2 弯：第二行提示长按 UP |
| `22-popup-fall.png` | 红色的摔倒提醒，默认标题，正在播报语音（D-31） |
| `23-popup-voice.png` | 语音播报，正在等待语音 |
| `24-popup-issue.png` | 问题报告，倒计时中："8 秒后返回" |
| `25-popup-badge.png` | 成就报告，倒计时刚开始 |
| `26-popup-max.png` | 标题和正文都是最长的弹窗 |
| `popup-anim.gif` | 入场动画：先是实时数据页，然后四种弹窗依次作为新弹窗出现 |
| `contact-sheet.png` | 全部 25 张图，放大 2 倍 |

## 模拟器

用浏览器打开 `sim/coach-flow.html`，不需要服务器，也不需要联网。

- 按住设备按键 500 ms 算长按。键盘的 ↑、空格、↓ 对应三个按键。
- 流转图和触发矩阵显示当前状态和可走的转换。
- 手机面板会自动回应每个问题。可以选场景，也可以关掉自动应答，手动发送手机事件。
- 日志显示每次转换、对应的动作和每条蓝牙消息。
- 滑行数据按钮可以开始一趟、暂停发送和结束这一趟。LIVE_STATS 每秒一帧，结束后 2 秒发送 RUN_REPORT。
- 教练页空闲时短按 DOWN 打开报告页，短按 UP 返回。
- 弹窗按钮可以发送每种弹窗，可选带不带语音（D-31）。一个进度条显示 10 秒倒计时。流转图把三个弹窗状态放在单独的区域里。

浏览器用系统字体绘制屏幕。真实像素以 `mockups/` 里的图片为准。
不要手工修改这个页面。在 `../Firmware` 里运行 `python3 tools/flow/flow_sim.py`。

## 待定事项

- 手机可能发来 GB2312 一级字以外的字符。设备会把这个字符显示成空框。
