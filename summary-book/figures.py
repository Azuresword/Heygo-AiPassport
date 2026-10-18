#!/usr/bin/env python3
"""Make the SVG figures of the summary book.

Run this script before build.sh when a figure changes.
It writes images/*.svg and the HTML parts in _partials/.
The pages include the parts with {% include %}.
"""
import pathlib
import re

ROOT = pathlib.Path(__file__).resolve().parent
IMG = ROOT / "images"
IMG.mkdir(exist_ok=True)

BG, PANEL, PANEL2, LINE = "#0e1013", "#161a1f", "#1d2228", "#2a3038"
INK, INK2, INK3, GREEN = "#e8eaed", "#a9b0b8", "#6f7782", "#b1fe2b"
SANS = "-apple-system, 'PingFang SC', 'Noto Sans SC', 'Helvetica Neue', Arial, sans-serif"
MONO = "'SF Mono', Menlo, Consolas, monospace"


def esc(s):
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def svg(w, h, body, title):
    return (
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w}" height="{h}" '
        f'role="img" aria-label="{esc(title)}" font-family="{SANS}">'
        f"<title>{esc(title)}</title>"
        f'<defs><marker id="ah" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M0,0 L10,5 L0,10 z" fill="{GREEN}"/></marker>'
        f'<marker id="ag" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M0,0 L10,5 L0,10 z" fill="{INK2}"/></marker></defs>'
        f'<rect width="{w}" height="{h}" rx="14" fill="{PANEL}"/>{body}</svg>'
    )


def text(x, y, s, size=13, fill=INK, weight=400, anchor="start", mono=False, ls=0):
    fam = f' font-family="{MONO}"' if mono else ""
    sp = f' letter-spacing="{ls}"' if ls else ""
    return (f'<text x="{x}" y="{y}" font-size="{size}" fill="{fill}" font-weight="{weight}" '
            f'text-anchor="{anchor}"{fam}{sp}>{esc(s)}</text>')


def box(x, y, w, h, fill=PANEL2, stroke=LINE, rx=12, dash=False):
    d = ' stroke-dasharray="5 5"' if dash else ""
    return f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{rx}" fill="{fill}" stroke="{stroke}"{d}/>'


# ---------------------------------------------------------------- architecture
ARCH = {
    "en": dict(
        title="System architecture: device, BLE link, phone app, and HeygoAI",
        dev="DEVICE", devsub="ESP32-C3 · 8 MiB flash · no PSRAM",
        devitems=["3 keys · UP / OK / DOWN", "Microphone + speaker · ES8311", "Screen 240×320 · ST7789P3", "Fuel gauge · CW2017"],
        devrole="Input, display, recording, playback",
        link="BLE · NUS", linkitems=["Protocol 0.2", "IMA ADPCM 16 kHz", "20 ms audio blocks", "No pairing (D-12)"],
        phone="PHONE APP", phoneitems=["Speech to text", "Text to speech", "Run data · reports · turns", "Popups · run start"],
        phonerole="Analysis, speech, AI service access",
        cloud="HEYGOAI", clouditems=["AI coach answers"],
        up="audio up · events", down="audio down · data · popups",
    ),
    "zh": dict(
        title="系统架构：设备、蓝牙链路、手机应用和 HeygoAI",
        dev="设备 DEVICE", devsub="ESP32-C3 · 8 MiB Flash · 无 PSRAM",
        devitems=["三键 · UP / OK / DOWN", "麦克风 + 喇叭 · ES8311", "屏幕 240×320 · ST7789P3", "电量计 · CW2017"],
        devrole="按键、显示、录音和播放",
        link="BLE · NUS", linkitems=["协议 0.2", "IMA ADPCM 16 kHz", "20 ms 一块音频", "不配对（D-12）"],
        phone="手机应用 PHONE", phoneitems=["语音识别", "语音合成", "滑行数据 · 报告 · 弯型", "弹窗 · 开始滑行"],
        phonerole="数据分析、语音处理、AI 服务接入",
        cloud="HEYGOAI", clouditems=["AI 教练回答"],
        up="上行：音频 · 事件", down="下行：音频 · 数据 · 弹窗",
    ),
}


def arch(lang):
    L = ARCH[lang]
    b = []
    # device
    b.append(box(24, 24, 270, 262, fill=PANEL2, stroke="#4b6a12"))
    b.append(text(44, 54, L["dev"], 12, GREEN, 700, mono=True, ls=2))
    b.append(text(44, 76, L["devsub"], 12, INK3))
    for i, s in enumerate(L["devitems"]):
        y = 100 + i * 38
        b.append(box(44, y, 230, 30, fill=BG, rx=8))
        b.append(text(58, y + 20, s, 13, INK))
    b.append(text(44, 272, L["devrole"], 12, INK2))
    # link
    b.append(f'<line x1="300" y1="118" x2="474" y2="118" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    b.append(f'<line x1="474" y1="190" x2="300" y2="190" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    b.append(text(387, 108, L["up"], 11, INK2, anchor="middle"))
    b.append(text(387, 210, L["down"], 11, INK2, anchor="middle"))
    b.append(box(322, 132, 130, 44, fill=BG, stroke=GREEN, rx=22))
    b.append(text(387, 159, L["link"], 13, GREEN, 700, "middle", mono=True))
    for i, s in enumerate(L["linkitems"]):
        b.append(text(387, 240 + i * 17, s, 11, INK3, anchor="middle", mono=True))
    # phone
    b.append(box(480, 24, 250, 262))
    b.append(text(500, 54, L["phone"], 12, INK, 700, mono=True, ls=2))
    for i, s in enumerate(L["phoneitems"]):
        y = 76 + i * 38
        b.append(box(500, y, 210, 30, fill=BG, rx=8))
        b.append(text(514, y + 20, s, 13, INK))
    b.append(text(500, 272, L["phonerole"], 12, INK2))
    # cloud
    b.append(f'<line x1="736" y1="155" x2="786" y2="155" stroke="{INK2}" stroke-width="1.5" marker-end="url(#ag)" marker-start="url(#ag)"/>')
    b.append(box(790, 110, 150, 90, fill=PANEL, stroke="#3a424c", dash=True))
    b.append(text(865, 145, L["cloud"], 12, INK, 700, "middle", mono=True, ls=2))
    b.append(text(865, 170, L["clouditems"][0], 12, INK2, anchor="middle"))
    return svg(964, 310, "".join(b), L["title"])


# ---------------------------------------------------------------- sequence
SEQ = {
    "en": dict(
        title="One coach question: BLE messages and device states",
        dev="DEVICE", phone="PHONE APP",
        steps=[
            ("d", "AGENT_START 0x20", "Long press OK ≥ 500 ms", "LISTENING"),
            ("d", "AUDIO_UP 0x30 × n", "One block each 20 ms", ""),
            ("d", "AGENT_END 0x21", "User releases OK", "THINKING"),
            ("p", "AGENT_STATE 0x23", "Speech to text, AI answer", ""),
            ("p", "PLAY_START 0x32 · ch 0", "Only after AGENT_END (D-20)", "ANSWERING"),
            ("p", "AUDIO_DOWN 0x31 × n", "Real-time pace", ""),
            ("p", "PLAY_END 0x33", "Finish queued audio, then READY", ""),
        ],
    ),
    "zh": dict(
        title="一次教练提问：蓝牙消息和设备状态",
        dev="设备", phone="手机应用",
        steps=[
            ("d", "AGENT_START 0x20", "长按 OK 达到 500 ms", "正在听"),
            ("d", "AUDIO_UP 0x30 × n", "每 20 ms 一块", ""),
            ("d", "AGENT_END 0x21", "松开 OK", "思考中"),
            ("p", "AGENT_STATE 0x23", "转写、问 AI", ""),
            ("p", "PLAY_START 0x32 · 通道 0", "收到 AGENT_END 之后才发（D-20）", "回答中"),
            ("p", "AUDIO_DOWN 0x31 × n", "按实时节奏", ""),
            ("p", "PLAY_END 0x33", "队列播放完毕后回到就绪", ""),
        ],
    ),
}


def seq(lang):
    L = SEQ[lang]
    W, xd, xp, top, gap = 964, 300, 760, 92, 52
    h = top + gap * len(L["steps"]) + 30
    b = []
    for x, name, col in ((xd, L["dev"], GREEN), (xp, L["phone"], INK)):
        b.append(box(x - 90, 22, 180, 40, fill=PANEL2, stroke="#4b6a12" if col == GREEN else LINE, rx=10))
        b.append(text(x, 48, name, 13, col, 700, "middle", mono=True, ls=1))
        b.append(f'<line x1="{x}" y1="64" x2="{x}" y2="{h - 16}" stroke="{LINE}" stroke-width="2" stroke-dasharray="4 6"/>')
    b.append(text(40, 48, "STATE" if lang == "en" else "设备状态", 11, INK3, mono=True, ls=1))
    for i, (who, msg, note, state) in enumerate(L["steps"]):
        y = top + i * gap
        x1, x2 = (xd + 6, xp - 6) if who == "d" else (xp - 6, xd + 6)
        col = GREEN if who == "d" else INK2
        mk = "ah" if who == "d" else "ag"
        b.append(f'<line x1="{x1}" y1="{y}" x2="{x2}" y2="{y}" stroke="{col}" stroke-width="2" marker-end="url(#{mk})"/>')
        b.append(text((xd + xp) / 2, y - 8, msg, 13, INK, 600, "middle", mono=True))
        b.append(text((xd + xp) / 2, y + 17, note, 11, INK3, anchor="middle"))
        if state:
            b.append(box(28, y - 14, 150, 28, fill=BG, stroke=GREEN, rx=14))
            b.append(text(103, y + 5, state, 12, GREEN, 700, "middle", mono=lang == "en"))
            b.append(f'<line x1="178" y1="{y}" x2="{xd - 4}" y2="{y}" stroke="{LINE}" stroke-width="1"/>')
    return svg(W, h, "".join(b), L["title"])


# ---------------------------------------------------------------- gate
GATE = {
    "en": dict(title="Firmware validation checks",
               steps=[("Baseline", "upstream checks"), ("Host tests", "33 test_* files"), ("Glyphs", "every string has a glyph"),
                      ("host_render", "overlap · leaks · UTF-8"), ("flow_sim", "--check · 128/128 self-test"), ("Build", "compile · link · image layout")]),
    "zh": dict(title="固件验证项目",
               steps=[("基线检查", "上游自带"), ("主机测试", "33 个 test_*"), ("字形检查", "文字都有字形"),
                      ("host_render", "重叠 · 泄漏 · UTF-8"), ("flow_sim", "--check · 自测 128/128"), ("构建", "编译 · 链接 · 镜像布局")]),
}


def gate(lang):
    L = GATE[lang]
    n, w, gap, y = len(L["steps"]), 150, 16, 34
    b = []
    for i, (a, s) in enumerate(L["steps"]):
        x = 24 + i * (w + gap)
        last = i == n - 1
        b.append(box(x, y, w, 76, fill=BG if not last else "#1a230d", stroke=GREEN if last else LINE, rx=10))
        b.append(text(x + 14, y + 32, a, 14, GREEN if last else INK, 700))
        b.append(text(x + 14, y + 56, s, 10.5, INK2))
    return svg(24 * 2 + n * w + (n - 1) * gap, 140, "".join(b), L["title"])


# ---------------------------------------------------------------- commit chart (inline, with tooltips)
PHASES = [
    ("1", {"en": "Requirements", "zh": "需求"}, {"en": "10-16 eve", "zh": "10-16 晚"}, 6),
    ("2", {"en": "Voice path", "zh": "语音链路"}, {"en": "10-17 AM", "zh": "10-17 上午"}, 10),
    ("3", {"en": "Run data", "zh": "滑行数据"}, {"en": "10-17 noon", "zh": "10-17 中午"}, 9),
    ("4", {"en": "Loop, turns", "zh": "四页、弯型"}, {"en": "10-17 eve", "zh": "10-17 傍晚"}, 4),
    ("5", {"en": "Popups, start", "zh": "弹窗、开始滑行"}, {"en": "10-17 night", "zh": "10-17 晚上"}, 8),
    ("6", {"en": "Rail docs", "zh": "导轨文档"}, {"en": "10-18 10:45", "zh": "10-18 10:45"}, 18),
    ("7", {"en": "Rail code, D-34", "zh": "导轨固件、D-34"}, {"en": "10:55–12:05", "zh": "10:55–12:05"}, 149),
]


def commits(lang):
    W, H, l, r, t, bt = 960, 330, 46, 16, 30, 64
    pw, ph = W - l - r, H - t - bt
    vmax = 150
    bw = pw / len(PHASES)
    unit = "commits" if lang == "en" else "个提交"
    b = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" role="img" '
         f'aria-label="{"Commits on main by phase" if lang == "en" else "各阶段 main 上的提交数"}" font-family="{SANS}">']
    for v in (0, 50, 100, 150):
        y = t + ph - ph * v / vmax
        b.append(f'<line x1="{l}" y1="{y:.1f}" x2="{W - r}" y2="{y:.1f}" stroke="{LINE}" stroke-width="1"/>')
        b.append(text(l - 10, y + 4, str(v), 11, INK3, anchor="end", mono=True))
    for i, (n, name, when, v) in enumerate(PHASES):
        x = l + i * bw + bw * 0.22
        w = bw * 0.56
        hgt = ph * v / vmax
        y = t + ph - hgt
        tip = f"{n} · {name[lang]} · {when[lang]}: {v} {unit}"
        b.append(f'<g><title>{esc(tip)}</title>'
                 f'<rect x="{l + i * bw:.1f}" y="{t}" width="{bw:.1f}" height="{ph}" fill="transparent"/>'
                 f'<path class="bar" d="M{x:.1f},{t + ph} V{y + 4:.1f} Q{x:.1f},{y:.1f} {x + 4:.1f},{y:.1f} '
                 f'H{x + w - 4:.1f} Q{x + w:.1f},{y:.1f} {x + w:.1f},{y + 4:.1f} V{t + ph} Z" fill="{GREEN}"/></g>')
        b.append(text(x + w / 2, y - 8, str(v), 13, INK, 700, "middle", mono=True))
        b.append(text(x + w / 2, t + ph + 21, name[lang], 13, INK2, anchor="middle"))
        b.append(text(x + w / 2, t + ph + 38, when[lang], 11, INK3, anchor="middle"))
    b.append(f'<line x1="{l}" y1="{t + ph}" x2="{W - r}" y2="{t + ph}" stroke="{INK3}" stroke-width="1"/>')
    b.append("</svg>")
    return "".join(b)




# ---------------------------------------------------------------- sprint board (HTML)
SPRINTS = [
    (0, "1", "Device logic for one coach question", "一次教练提问的设备逻辑"),
    (1, "1", "Voice and BLE link", "接上语音和蓝牙链路"),
    (2, "2", "Data layer: LIVE_STATS and RUN_REPORT", "数据层：LIVE_STATS 和 RUN_REPORT"),
    (3, "2", "Live data page", "实时数据页"),
    (4, "2", "Report page", "报告页"),
    (5, "1·2", "Board smoke test with phone_sim", "开发板冒烟：phone_sim 跑通"),
    (6, "1", "Phone app: TC-01, 06, 07, 20", "手机应用联调 TC-01/06/07/20"),
    (7, "2", "Phone app: TC-02, 03, 21 timing", "手机应用联调 TC-02/03/21 计时"),
    (8, "1", "Real service: TC-08", "真实服务 TC-08"),
    (9, "1·2", "U1 usability: TC-15, TC-21", "U1 可用性测试 TC-15/21"),
    (10, "1·2", "Fix defects, review done", "修复缺陷，评审完成定义"),
    (11, "3", "Turns page on the host", "弯型页（主机）"),
    (12, "3", "Turns on the board: TC-05", "弯型设备测试 TC-05"),
    (13, "4", "Popups on the host", "弹窗（主机）"),
    (14, "4", "Popups on the board", "弹窗设备测试"),
    (15, "2", "Run start opens live data (D-32)", "开始滑行跳页（D-32）"),
    (16, "2", "Page rail (D-33)", "页面导轨（D-33）"),
    (17, "2", "Link state on every page (D-34)", "每页显示链路状态（D-34）"),
]
PENDING = set()  # The owner confirmed Sprint 17 on 2026-10-18, after Sprints 0 to 16.


def sprint_board(lang):
    done = "Done · 100%" if lang == "en" else "完成 · 100%"
    inc = "Inc" if lang == "en" else "增量"
    cells = []
    pend = "Code done · to confirm" if lang == "en" else "代码已完成 · 待确认"
    for n, i, en, zh in SPRINTS:
        t = en if lang == "en" else zh
        p = n in PENDING
        cells.append(f'<div class="sprint{" pending" if p else ""}"><span class="inc">{inc} {i}</span><div class="n">SPRINT {n:02d}</div>'
                     f'<div class="t">{esc(t)}</div><div class="bar"></div><div class="s">{pend if p else done}</div></div>')
    return '<div class="sprints">' + "".join(cells) + "</div>"




# ---------------------------------------------------------------- firmware layers
LAYERS = {
    "en": dict(title="Firmware layers", host="Plain C · host tests", rows=[
        ("APP", "Coach app", ["coach_app.c · state machine task", "coach_input.c · keys → events", "generated/ · flow, screens, UI"]),
        ("UI", "Screens", ["ui/ski_view · stats_fmt", "ui/turn_view · turn_page", "ui/popup_view · page_rail"]),
        ("LINK", "BLE link and protocol", ["protocol/ble_proto · packets", "link/link_core · session rules", "link/ble_link · NimBLE, TX queues"]),
        ("AUDIO", "Voice", ["audio/voice · capture and playback", "audio/ima_adpcm · 164 B blocks", "audio/play_gate · envelope"]),
        ("BSP", "Board support", ["display · LVGL port · SPI 80 MHz", "button · ADC on GPIO0", "audio ES8311 · battery CW2017 · I2C"]),
        ("LIBS", "Libraries", ["LVGL 9.5.0 · esp_lvgl_port 2.9.0", "NimBLE (peripheral only)", "esp_codec_dev 1.6.2 · button 4.2.0"]),
        ("IDF", "ESP-IDF 5.5.3", ["ESP32-C3 · RISC-V 160 MHz", "FreeRTOS · 1000 Hz tick", "8 MiB flash · DIO 80 MHz"]),
    ], pure=[(0, 1), (2, 0), (2, 1), (3, 1), (3, 2)]),
    "zh": dict(title="固件分层", host="纯 C · 有主机测试", rows=[
        ("APP", "Coach 应用", ["coach_app.c · 状态机任务", "coach_input.c · 按键 → 事件", "generated/ · 流程、界面、绑定"]),
        ("UI", "界面", ["ui/ski_view · stats_fmt", "ui/turn_view · turn_page", "ui/popup_view · page_rail"]),
        ("LINK", "蓝牙链路与协议", ["protocol/ble_proto · 包编解码", "link/link_core · 会话规则", "link/ble_link · NimBLE、发送队列"]),
        ("AUDIO", "语音", ["audio/voice · 录音和播放", "audio/ima_adpcm · 164 字节一块", "audio/play_gate · envelope"]),
        ("BSP", "板级支持", ["显示 · LVGL 适配 · SPI 80 MHz", "按键 · GPIO0 ADC", "音频 ES8311 · 电量 CW2017 · I2C"]),
        ("LIBS", "第三方库", ["LVGL 9.5.0 · esp_lvgl_port 2.9.0", "NimBLE（只做外设）", "esp_codec_dev 1.6.2 · button 4.2.0"]),
        ("IDF", "ESP-IDF 5.5.3", ["ESP32-C3 · RISC-V 160 MHz", "FreeRTOS · 1000 Hz 节拍", "8 MiB Flash · DIO 80 MHz"]),
    ], pure=[(0, 1), (2, 0), (2, 1), (3, 1), (3, 2)]),
}


def layers(lang):
    L = LAYERS[lang]
    W, x0, lw, gap, rh = 964, 24, 168, 10, 52
    cw = (W - x0 * 2 - lw - 12 - gap * 2) / 3
    b = []
    for i, (tag, name, cells) in enumerate(L["rows"]):
        y = 24 + i * (rh + 8)
        own = i < 5
        b.append(box(x0, y, lw, rh, fill=PANEL2 if own else BG, stroke="#4b6a12" if own else LINE, rx=10))
        b.append(text(x0 + 14, y + 20, tag, 10.5, GREEN if own else INK3, 700, mono=True, ls=1.5))
        b.append(text(x0 + 14, y + 39, name, 12.5, INK if own else INK2, 600))
        for j, c in enumerate(cells):
            x = x0 + lw + 12 + j * (cw + gap)
            pure = (i, j) in L["pure"]
            b.append(box(x, y, cw, rh, fill=BG, stroke=GREEN if pure else LINE, rx=10))
            b.append(text(x + 12, y + 31, c, 11, INK if own else INK2, mono=(i < 4)))
    y = 24 + 7 * (rh + 8) + 6
    b.append(box(x0 + lw + 12, y, 16, 12, fill=BG, stroke=GREEN, rx=3))
    b.append(text(x0 + lw + 36, y + 11, L["host"], 12, INK2))
    return svg(W, y + 30, "".join(b), L["title"])


# ---------------------------------------------------------------- tasks and queues
TASKS = {
    "en": dict(title="Tasks, priorities, stacks, and queues",
               prio="priority", stack="stack", note="Higher number = higher priority. Stacks in bytes.",
               tasks=[("voice", 7, 4096, "Encodes the mic, feeds the speaker"), ("ble_tx", 6, 3072, "Sends the control queue first"),
                      ("coach", 5, 4096, "State machine, one event at a time"), ("LVGL", 4, 7168, "esp_lvgl_port, 5 ms tick"),
                      ("NimBLE host", None, 4096, "GATT, connection, RX callbacks")],
               queues=[("coach queue", "16 messages", "keys · timers · link events"), ("control TX", "8 packets", "drop new packet if full"),
                       ("audio TX", "8 packets", "drop oldest · AGENT_END follows audio"), ("audio down", "8 × 164 B", "drop the oldest")]),
    "zh": dict(title="任务、优先级、栈和队列",
               prio="优先级", stack="栈", note="数字越大优先级越高。栈的单位是字节。",
               tasks=[("voice", 7, 4096, "录音编码、喂给喇叭"), ("ble_tx", 6, 3072, "优先发送控制队列"),
                      ("coach", 5, 4096, "状态机，一次处理一个事件"), ("LVGL", 4, 7168, "esp_lvgl_port，5 ms 节拍"),
                      ("NimBLE host", None, 4096, "GATT、连接、接收回调")],
               queues=[("coach 队列", "16 条消息", "按键 · 计时器 · 链路事件"), ("控制发送", "8 个包", "队列满时丢弃新包"),
                       ("音频发送", "8 个包", "丢最旧包，AGENT_END 排在音频后"), ("音频下行", "8 × 164 字节", "满了丢最旧的")]),
}


def tasks(lang):
    L = TASKS[lang]
    W = 964
    b = []
    b.append(text(24, 40, "TASKS" if lang == "en" else "任务", 11, INK3, mono=True, ls=1.5))
    for i, (n, pr, st, d) in enumerate(L["tasks"]):
        y = 54 + i * 58
        b.append(box(24, y, 520, 48, fill=PANEL2 if pr else BG, rx=10))
        b.append(text(42, y + 30, n, 15, GREEN if pr else INK2, 700, mono=True))
        b.append(text(170, y + 30, d, 12.5, INK2))
        b.append(text(452, y + 20, L["prio"], 10, INK3, anchor="end"))
        b.append(text(452, y + 37, str(pr) if pr else "IDF", 15, INK, 700, "end", mono=True))
        b.append(text(526, y + 20, L["stack"], 10, INK3, anchor="end"))
        b.append(text(526, y + 37, f"{st:,}", 15, INK, 700, "end", mono=True))
    b.append(text(576, 40, "QUEUES" if lang == "en" else "队列", 11, INK3, mono=True, ls=1.5))
    for i, (n, sz, rule) in enumerate(L["queues"]):
        y = 54 + i * 72
        b.append(box(576, y, 364, 60, fill=BG, stroke=LINE, rx=10))
        b.append(text(594, y + 25, n, 14, INK, 700))
        b.append(text(922, y + 25, sz, 13, GREEN, 700, "end", mono=True))
        b.append(text(594, y + 46, rule, 12, INK3))
    b.append(text(24, 362, L["note"], 11.5, INK3))
    return svg(W, 380, "".join(b), L["title"])


# ---------------------------------------------------------------- build pipeline
BUILD = {
    "en": dict(title="Firmware build", cols=[
        ("SOURCES", ["flows/coach.flow.json", "assets/ui/strings.json", "TTF fonts + char lists", "Sprite scripts", "C sources · sdkconfig.defaults"]),
        ("GENERATORS", ["flowc.py → main/generated/", "flow_sim.py → simulator", "gen_lv_font.py → fonts", "gen_sprite.py → rider", "gen_popup_icons.py → badges"]),
        ("BUILD", ["idf.py build", "FoloToy-AI-Passport.bin", "idf.py merge-bin", "…-full.bin · flash at 0x0"]),
        ("CHECKS", ["host tests · glyphs", "host_render screens", "flow_sim --check · self-test", "verify_firmware.py layout"]),
    ]),
    "zh": dict(title="固件生成与构建", cols=[
        ("源文件", ["flows/coach.flow.json", "assets/ui/strings.json", "TTF 字体 + 字表", "精灵图脚本", "C 源码 · sdkconfig.defaults"]),
        ("生成器", ["flowc.py → main/generated/", "flow_sim.py → simulator", "gen_lv_font.py → fonts", "gen_sprite.py → rider", "gen_popup_icons.py → badges"]),
        ("构建", ["idf.py build", "FoloToy-AI-Passport.bin", "idf.py merge-bin", "…-full.bin · 从 0x0 烧录"]),
        ("检查", ["主机测试 · 字形", "host_render 界面", "flow_sim --check · 自测", "verify_firmware.py 布局"]),
    ]),
}


def build(lang):
    L = BUILD[lang]
    W, cw, gap = 964, 214, 18
    b = []
    for i, (h, items) in enumerate(L["cols"]):
        x = 24 + i * (cw + gap)
        last = i == 3
        b.append(text(x, 40, h, 11, GREEN if i in (1, 2) else INK3, 700, mono=True, ls=1.5))
        for j, it in enumerate(items):
            y = 54 + j * 46
            b.append(box(x, y, cw, 36, fill=BG if not (i == 2 and j == 3) else "#1a230d",
                         stroke=GREEN if (i == 2 and j == 3) else LINE, rx=8))
            b.append(text(x + 12, y + 23, it, 11, INK, mono=True))
        if not last:
            b.append(f'<line x1="{x + cw + 2}" y1="150" x2="{x + cw + gap - 2}" y2="150" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    return svg(W, 296, "".join(b), L["title"])


# ---------------------------------------------------------------- flash map
FMAP = [  # (label_en, label_zh, start, end, used)
    ("Bootloader", "Bootloader", 0x0, 0x8000, 0x5220),
    ("Table", "分区表", 0x8000, 0x9000, None),
    ("nvs", "nvs", 0x9000, 0xF000, None),
    ("phy", "phy", 0xF000, 0x10000, None),
    ("factory app", "factory 应用", 0x10000, 0x800000, 1948640),
]


def flashmap(lang):
    W = 964
    x0, x1 = 24, W - 24
    full = 0x800000
    b = []
    t = "Flash map, 8 MiB" if lang == "en" else "Flash 分布，8 MiB"
    b.append(text(x0, 36, "FULL 8 MiB" if lang == "en" else "整片 8 MiB", 11, INK3, mono=True, ls=1.5))
    y = 48
    sc = (x1 - x0) / full
    # factory partition
    fx = x0 + 0x10000 * sc
    b.append(box(x0, y, x1 - x0, 46, fill=BG, stroke=LINE, rx=8))
    b.append(f'<rect x="{x0}" y="{y}" width="{0x10000 * sc:.1f}" height="46" rx="4" fill="{INK3}"/>')
    uw = 1948640 * sc
    b.append(f'<g><title>app 1,948,640 B / factory 8,323,072 B</title><rect x="{fx:.1f}" y="{y}" width="{uw:.1f}" height="46" rx="4" fill="{GREEN}"/></g>')
    b.append(text(fx + 12, y + 28, "app 1.86 MiB" if lang == "en" else "应用 1.86 MiB", 13, "#0e1013", 700))
    b.append(text(fx + uw + 14, y + 28, ("free in factory: 6.08 MiB (76.6%)" if lang == "en" else "factory 剩余 6.08 MiB（76.6%）"), 12.5, INK2))
    for a, lab in ((0, "0x0"), (0x800000, "0x800000")):
        xx = x0 + a * sc
        anchor = "end" if a == full else "start"
        b.append(text(xx if a else x0, y + 66, lab, 10.5, INK3, anchor=anchor, mono=True))
    # zoom of first 64 KiB
    y2 = 150
    b.append(text(x0, y2 - 12, "FIRST 64 KiB" if lang == "en" else "前 64 KiB 放大", 11, INK3, mono=True, ls=1.5))
    sc2 = (x1 - x0) / 0x10000
    for en, zh, s0, e0, used in FMAP[:4]:
        xs, w = x0 + s0 * sc2, (e0 - s0) * sc2
        b.append(f'<g><title>{esc(en)} 0x{s0:X}–0x{e0:X} ({e0 - s0:,} B)</title>'
                 f'<rect x="{xs + 1:.1f}" y="{y2}" width="{w - 2:.1f}" height="54" rx="6" fill="{PANEL2}" stroke="{LINE}"/></g>')
        if used:
            b.append(f'<rect x="{xs + 1:.1f}" y="{y2}" width="{used * sc2 - 2:.1f}" height="54" rx="6" fill="{INK2}" opacity="0.55"/>')
        b.append(text(xs + 8, y2 + 24, en if lang == "en" else zh, 12, INK, 700))
        b.append(text(xs + 8, y2 + 43, f"{(e0 - s0) // 1024} KiB", 11, INK2, mono=True))
        b.append(text(xs + 2, y2 + 72, f"0x{s0:X}", 10.5, INK3, mono=True))
    note = ("Bootloader uses 0x5220 B (21,024 B), 36% of its region is free." if lang == "en"
            else "Bootloader 实占 0x5220 字节（21,024 字节），所在区域剩余 36%。")
    b.append(text(x0, y2 + 100, note, 12, INK3))
    return svg(W, y2 + 118, "".join(b), t)


# ---------------------------------------------------------------- horizontal bar charts (inline, tooltips)
FLASH_PARTS = [  # bytes, from idf.py size-files and size-components on 284ab00
    ({"en": "Chinese font 16 px", "zh": "中文字体 16 px"}, 516932),
    ({"en": "LVGL", "zh": "LVGL"}, 332036),
    ({"en": "BLE controller + NimBLE", "zh": "蓝牙控制器 + NimBLE"}, 168668),
    ({"en": "Popup badges (4 × 4 frames)", "zh": "弹窗徽章（4 种 × 4 帧）"}, 201152),
    ({"en": "Other fonts and rider sprite", "zh": "其他字体和滑雪小人"}, 127789),
    ({"en": "Heygo app code", "zh": "Heygo 应用代码"}, 55393),
    ({"en": "Other ESP-IDF parts", "zh": "ESP-IDF 其他部分"}, 439142),
]

DRAM_PARTS = [
    ({"en": "IRAM code (.text)", "zh": "IRAM 代码（.text）"}, 96852),
    ({"en": ".bss (incl. 28 KiB LVGL pool)", "zh": ".bss（含 28 KiB LVGL 内存池）"}, 44200),
    ({"en": ".data", "zh": ".data"}, 10524),
    ({"en": "Unallocated at link time", "zh": "链接后未分配空间"}, 169720),
]


def hbars(parts, lang, total, label, free_last=False):
    W, l, r, rh = 960, 250, 190, 34
    H = 20 + rh * len(parts) + 10
    pw = W - l - r
    vmax = max(v for _, v in parts)
    b = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" role="img" aria-label="{esc(label)}" font-family="{SANS}">']
    for i, (name, v) in enumerate(parts):
        y = 14 + i * rh
        w = pw * v / vmax
        is_free = free_last and i == len(parts) - 1
        col = INK3 if is_free else GREEN
        pct = 100 * v / total
        tip = f"{name[lang]}: {v:,} B ({pct:.1f}%)"
        b.append(f'<g><title>{esc(tip)}</title><rect x="0" y="{y - 4}" width="{W}" height="{rh}" fill="transparent"/>'
                 f'<rect x="{l}" y="{y}" width="{max(w, 3):.1f}" height="{rh - 12}" rx="4" fill="{col}"/></g>')
        b.append(text(l - 12, y + 16, name[lang], 12.5, INK2, anchor="end"))
        b.append(text(l + w + 10, y + 16, f"{v / 1024:,.1f} KiB · {pct:.1f}%", 12, INK, 600, mono=True))
    b.append("</svg>")
    return '<div class="chart">' + "".join(b) + "</div>"


# ---------------------------------------------------------------- scrum framework
SCRUM = {
    "en": dict(title="The Scrum flow of this project",
               goal=("PRODUCT GOAL", "Live data on the run.", "Voice coach in a rest."),
               backlog=("PRODUCT BACKLOG", "25 stories · US-xx", "Points for risk, not hours"),
               sprint="SPRINT · 4 h · ONE GOAL",
               events=[("Plan", "15 min"), ("Develop", "200 min"), ("Review", "15 min"), ("Retro", "10 min")],
               inc=("INCREMENT", "validate.sh exit 0", "Build · host · device"),
               change=["Feedback", "Decision D-xx", "Docs 01–05", "Backlog order"],
               changelab="CHANGE LOOP · 34 decisions", cut="Cut line: drop a task, never extend the sprint"),
    "zh": dict(title="本项目的 Scrum 流程",
               goal=("产品目标", "滑行时看实时数据。", "休息时问语音教练。"),
               backlog=("产品待办列表", "25 个故事 · US-xx", "故事点比风险，不是小时"),
               sprint="Sprint · 4 小时 · 一个目标",
               events=[("计划", "15 分钟"), ("开发", "200 分钟"), ("评审", "15 分钟"), ("回顾", "10 分钟")],
               inc=("增量", "validate.sh 退出码 0", "证据：构建 · 主机 · 设备"),
               change=["反馈", "决策 D-xx", "文档 01–05", "重排待办"],
               changelab="变更回路 · 34 条决策", cut="砍线：去掉任务，绝不延长 Sprint"),
}


def scrum(lang):
    L = SCRUM[lang]
    W = 964
    b = []
    def card(x, y, w, h, t3, accent=False):
        b.append(box(x, y, w, h, fill=PANEL2 if accent else BG, stroke="#4b6a12" if accent else LINE, rx=12))
        b.append(text(x + 16, y + 26, t3[0], 11, GREEN if accent else INK3, 700, mono=True, ls=1.5))
        b.append(text(x + 16, y + 50, t3[1], 13, INK, 600))
        b.append(text(x + 16, y + 70, t3[2], 12, INK2))
    card(24, 40, 170, 92, L["goal"])
    b.append(f'<line x1="198" y1="86" x2="222" y2="86" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    card(226, 40, 186, 92, L["backlog"])
    b.append(f'<line x1="416" y1="86" x2="440" y2="86" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    # sprint box
    b.append(box(444, 24, 300, 124, fill=PANEL2, stroke=GREEN, rx=14))
    b.append(text(462, 48, L["sprint"], 11, GREEN, 700, mono=True, ls=1.5))
    ws = [40, 120, 40, 40]
    x = 462
    for (n, t), w in zip(L["events"], ws):
        w2 = w * 264 / 240
        b.append(box(x, 62, w2 - 4, 62, fill=BG if n not in ("Develop", "开发") else "#1a230d",
                     stroke=LINE if n not in ("Develop", "开发") else "#4b6a12", rx=8))
        b.append(text(x + (w2 - 4) / 2, 90, n, 12, INK, 700, "middle"))
        b.append(text(x + (w2 - 4) / 2, 108, t, 10, INK3, anchor="middle"))
        x += w2
    b.append(f'<line x1="748" y1="86" x2="772" y2="86" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    card(776, 40, 164, 92, L["inc"], accent=True)
    # cut line note
    b.append(text(594, 168, L["cut"], 11.5, INK3, anchor="middle"))
    # change loop
    y = 214
    b.append(text(940, y - 14, L["changelab"], 11, INK3, 700, "end", mono=True, ls=1.5))
    xs = [24, 254, 484, 714]
    for i, (xx, n) in enumerate(zip(xs, L["change"])):
        b.append(box(xx, y, 206, 44, fill=BG, stroke=LINE, rx=10))
        b.append(text(xx + 103, y + 28, n, 13, INK, 600, "middle"))
        if i < 3:
            b.append(f'<line x1="{xx + 210}" y1="{y + 22}" x2="{xx + 248}" y2="{y + 22}" stroke="{INK2}" stroke-width="1.5" marker-end="url(#ag)"/>')
    # loop arrows: increment -> feedback (down-left), backlog order -> backlog (up)
    b.append(f'<path d="M858,134 V176 H127 V{y - 4}" fill="none" stroke="{INK2}" stroke-width="1.5" stroke-dasharray="5 5" marker-end="url(#ag)"/>')
    b.append(f'<path d="M817,{y + 46} V{y + 66} H242 V136" fill="none" stroke="{INK2}" stroke-width="1.5" stroke-dasharray="5 5" marker-end="url(#ag)"/>')
    return svg(W, y + 80, "".join(b), L["title"])


# ---------------------------------------------------------------- change timeline (D-33, D-34)
CHG = {
    "en": dict(title="How a decision becomes firmware: D-33 and D-34 on 2026-10-18",
               lanes=[("D-33 page rail", [(645, "Docs 01–05", "eef9c65"), (647, "Simulator", "30ba3c7"),
                                         (699, "Review: no arrows", "cec92d8"), (709, "Firmware + 28 KiB", "be8671d"),
                                         (710, "25 screens", "4cb5a26"), (725, "Review: one color", "ea4815a")]),
                      ("D-34 link state", [(668, "Docs 01–05", "21280e6"), (682, "Flow + firmware + sim", "ba53b8d"),
                                           (684, "25 screens", "04d27a7")])]),
    "zh": dict(title="一条决策怎样变成固件：2026-10-18 的 D-33 和 D-34",
               lanes=[("D-33 页面导轨", [(645, "文档 01–05", "eef9c65"), (647, "模拟器", "30ba3c7"),
                                        (699, "评审：去掉箭头", "cec92d8"), (709, "固件 + 28 KiB", "be8671d"),
                                        (710, "25 张界面图", "4cb5a26"), (725, "评审：同一种颜色", "ea4815a")]),
                      ("D-34 链路状态", [(668, "文档 01–05", "21280e6"), (682, "流程图 + 固件 + 模拟器", "ba53b8d"),
                                        (684, "25 张界面图", "04d27a7")])]),
}


def change_timeline(lang):
    L = CHG[lang]
    W, l, r = 964, 150, 30
    t0, t1 = 640, 730  # minutes after midnight: 10:40 to 12:10
    sc = (W - l - r) / (t1 - t0)
    b = []
    for m in range(t0, t1 + 1, 10):
        x = l + (m - t0) * sc
        b.append(f'<line x1="{x:.1f}" y1="34" x2="{x:.1f}" y2="300" stroke="{LINE}" stroke-width="1"/>')
        b.append(text(x, 26, f"{m // 60}:{m % 60:02d}", 10.5, INK3, anchor="middle", mono=True))
    for li, (lab, evs) in enumerate(L["lanes"]):
        y0 = 70 + li * 130
        b.append(text(24, y0 + 4, lab, 12.5, INK, 700))
        b.append(f'<line x1="{l + (evs[0][0] - t0) * sc:.1f}" y1="{y0}" x2="{l + (evs[-1][0] - t0) * sc:.1f}" y2="{y0}" stroke="{GREEN}" stroke-width="2"/>')
        for k, (m, name, h) in enumerate(evs):
            x = l + (m - t0) * sc
            review = name.startswith(("Review", "评审"))
            b.append(f'<g><title>{m // 60}:{m % 60:02d} · {esc(name)} · {h}</title>'
                     f'<circle cx="{x:.1f}" cy="{y0}" r="6" fill="{BG if review else GREEN}" stroke="{GREEN}" stroke-width="2"/></g>')
            dy = 24 + (k % 3) * 30
            b.append(f'<line x1="{x:.1f}" y1="{y0 + 7}" x2="{x:.1f}" y2="{y0 + dy - 12}" stroke="{INK3}" stroke-width="1"/>')
            right = x > W - 170
            tx, anc = (x - 4, "end") if right else (x + 4, "start")
            b.append(text(tx, y0 + dy, name, 11.5, INK if not review else GREEN, 600, anc))
    return svg(W, 310, "".join(b), L["title"])


# ---------------------------------------------------------------- simulator self-test growth (inline)
SELFTEST = [  # (label_en, label_zh, checks, source)
    ("Sprint 4", "Sprint 4", 53), ("Sprint 11", "Sprint 11", 88), ("Sprint 13", "Sprint 13", 104),
    ("Sprint 15", "Sprint 15", 117), ("Sprint 16 sim", "Sprint 16 模拟器", 122), ("Sprints 16–17", "Sprint 16–17", 128),
]


def selftest_bars(lang):
    W, H, l, r, t, bt = 960, 260, 46, 16, 30, 48
    pw, ph = W - l - r, H - t - bt
    vmax = 140
    bw = pw / len(SELFTEST)
    unit = "checks" if lang == "en" else "项自测"
    b = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" role="img" '
         f'aria-label="{"Simulator self-test checks by sprint" if lang == "en" else "各 Sprint 的模拟器自测项数"}" font-family="{SANS}">']
    for v in (0, 35, 70, 105, 140):
        y = t + ph - ph * v / vmax
        b.append(f'<line x1="{l}" y1="{y:.1f}" x2="{W - r}" y2="{y:.1f}" stroke="{LINE}" stroke-width="1"/>')
        b.append(text(l - 10, y + 4, str(v), 11, INK3, anchor="end", mono=True))
    for i, (en, zh, v) in enumerate(SELFTEST):
        x = l + i * bw + bw * 0.25
        w = bw * 0.5
        y = t + ph - ph * v / vmax
        name = en if lang == "en" else zh
        b.append(f'<g><title>{esc(name)}: {v} {unit}</title><rect x="{l + i * bw:.1f}" y="{t}" width="{bw:.1f}" height="{ph}" fill="transparent"/>'
                 f'<path d="M{x:.1f},{t + ph} V{y + 4:.1f} Q{x:.1f},{y:.1f} {x + 4:.1f},{y:.1f} H{x + w - 4:.1f} '
                 f'Q{x + w:.1f},{y:.1f} {x + w:.1f},{y + 4:.1f} V{t + ph} Z" fill="{GREEN}"/></g>')
        b.append(text(x + w / 2, y - 8, str(v), 13, INK, 700, "middle", mono=True))
        b.append(text(x + w / 2, t + ph + 21, name, 12.5, INK2, anchor="middle"))
    b.append(f'<line x1="{l}" y1="{t + ph}" x2="{W - r}" y2="{t + ph}" stroke="{INK3}" stroke-width="1"/>')
    b.append("</svg>")
    return '<div class="chart">' + "".join(b) + "</div>"


if __name__ == "__main__":
    PART = ROOT / "_partials"
    PART.mkdir(exist_ok=True)
    for lang in ("en", "zh"):
        (IMG / f"architecture-{lang}.svg").write_text(arch(lang))
        (IMG / f"coach-sequence-{lang}.svg").write_text(seq(lang))
        (IMG / f"gate-{lang}.svg").write_text(gate(lang))
        (PART / f"commits-{lang}.html").write_text(f'<div class="chart">{commits(lang)}</div>\n')
        (PART / f"sprints-{lang}.html").write_text(sprint_board(lang) + "\n")
        (IMG / f"fw-layers-{lang}.svg").write_text(layers(lang))
        (IMG / f"fw-tasks-{lang}.svg").write_text(tasks(lang))
        (IMG / f"fw-build-{lang}.svg").write_text(build(lang))
        (IMG / f"flash-map-{lang}.svg").write_text(flashmap(lang))
        (IMG / f"scrum-{lang}.svg").write_text(scrum(lang))
        (PART / f"selftest-{lang}.html").write_text(selftest_bars(lang) + "\n")
        (IMG / f"change-{lang}.svg").write_text(change_timeline(lang))
        img_total = sum(v for _, v in FLASH_PARTS)
        (PART / f"flash-parts-{lang}.html").write_text(
            hbars(sorted(FLASH_PARTS, key=lambda p: -p[1]), lang, img_total, "Flash" ) + "\n")
        (PART / f"dram-parts-{lang}.html").write_text(hbars(DRAM_PARTS, lang, 321296, "Internal RAM" if lang == "en" else "内部 RAM", free_last=True) + "\n")
    print("figures: done")
