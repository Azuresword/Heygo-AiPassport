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
        dev="DEVICE", devsub="ESP32-C3 · 8 MB flash · no PSRAM",
        devitems=["3 keys · UP / OK / DOWN", "Microphone + speaker · ES8311", "Screen 240×320 · ST7789P3", "Fuel gauge · CW2017"],
        devrole="Keys, microphone, speaker, screen",
        link="BLE · NUS", linkitems=["Protocol 0.2", "IMA ADPCM 16 kHz", "20 ms audio blocks", "No pairing (D-12)"],
        phone="PHONE APP", phoneitems=["Speech to text", "Text to speech", "Run data · reports · turns", "Popups · run start"],
        phonerole="All analysis and voice",
        cloud="HEYGOAI", clouditems=["AI coach answers"],
        up="audio up · events", down="audio down · data · popups",
    ),
    "zh": dict(
        title="系统架构：设备、蓝牙链路、手机应用和 HeygoAI",
        dev="设备 DEVICE", devsub="ESP32-C3 · 8 MB Flash · 无 PSRAM",
        devitems=["三键 · UP / OK / DOWN", "麦克风 + 喇叭 · ES8311", "屏幕 240×320 · ST7789P3", "电量计 · CW2017"],
        devrole="只管按键、麦克风、喇叭、屏幕",
        link="BLE · NUS", linkitems=["协议 0.2", "IMA ADPCM 16 kHz", "20 ms 一块音频", "不配对（D-12）"],
        phone="手机应用 PHONE", phoneitems=["语音识别", "语音合成", "滑行数据 · 报告 · 弯型", "弹窗 · 开始滑行"],
        phonerole="分析和语音都在这里",
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
            ("p", "PLAY_END 0x33", "Last block", "READY"),
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
            ("p", "PLAY_END 0x33", "最后一块", "就绪"),
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
    "en": dict(title="Firmware gate: tools/validate.sh",
               steps=[("Baseline", "upstream checks"), ("Host tests", "33 test_* files"), ("Glyphs", "every string has a glyph"),
                      ("host_render", "overlap · leaks · UTF-8"), ("flow_sim", "--check · 122/122 self-test"), ("Build", "Firmware build: PASS")]),
    "zh": dict(title="固件门禁：tools/validate.sh",
               steps=[("基线检查", "上游自带"), ("主机测试", "33 个 test_*"), ("字形检查", "文字都有字形"),
                      ("host_render", "重叠 · 泄漏 · UTF-8"), ("flow_sim", "--check · 自测 122/122"), ("构建", "Firmware build: PASS")]),
}


def gate(lang):
    L = GATE[lang]
    n, w, gap, y = len(L["steps"]), 150, 16, 34
    b = []
    for i, (a, s) in enumerate(L["steps"]):
        x = 24 + i * (w + gap)
        last = i == n - 1
        b.append(box(x, y, w, 76, fill=BG if not last else "#1a230d", stroke=GREEN if last else LINE, rx=10))
        b.append(text(x + 14, y + 22, f"{i + 1:02d}", 11, INK3, mono=True))
        b.append(text(x + 14, y + 44, a, 14, GREEN if last else INK, 700))
        b.append(text(x + 14, y + 63, s, 10.5, INK2))
        if not last:
            b.append(f'<line x1="{x + w + 2}" y1="{y + 38}" x2="{x + w + gap - 2}" y2="{y + 38}" stroke="{GREEN}" stroke-width="2" marker-end="url(#ah)"/>')
    return svg(24 * 2 + n * w + (n - 1) * gap, 140, "".join(b), L["title"])


# ---------------------------------------------------------------- commit chart (inline, with tooltips)
PHASES = [
    ("1", {"en": "Requirements", "zh": "需求"}, {"en": "10-16 evening", "zh": "10-16 晚"}, 6),
    ("2", {"en": "Voice path", "zh": "语音链路"}, {"en": "10-17 morning", "zh": "10-17 上午"}, 10),
    ("3", {"en": "Run data", "zh": "滑行数据"}, {"en": "10-17 midday", "zh": "10-17 中午"}, 9),
    ("4", {"en": "Page loop, turns", "zh": "四页、弯型"}, {"en": "10-17 evening", "zh": "10-17 傍晚"}, 4),
    ("5", {"en": "Popups, run start", "zh": "弹窗、开始滑行"}, {"en": "10-17 night", "zh": "10-17 晚上"}, 8),
    ("6", {"en": "Page rail", "zh": "页面导轨"}, {"en": "10-18 morning", "zh": "10-18 上午"}, 18),
]


def commits(lang):
    W, H, l, r, t, bt = 960, 330, 46, 16, 30, 64
    pw, ph = W - l - r, H - t - bt
    vmax = 20
    bw = pw / len(PHASES)
    unit = "commits" if lang == "en" else "个提交"
    b = [f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {W} {H}" role="img" '
         f'aria-label="{"Commits on main by phase" if lang == "en" else "各阶段 main 上的提交数"}" font-family="{SANS}">']
    for v in (0, 5, 10, 15, 20):
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
]


def sprint_board(lang):
    done = "Done · 100%" if lang == "en" else "完成 · 100%"
    inc = "Inc" if lang == "en" else "增量"
    cells = []
    for n, i, en, zh in SPRINTS:
        t = en if lang == "en" else zh
        cells.append(f'<div class="sprint"><span class="inc">{inc} {i}</span><div class="n">SPRINT {n:02d}</div>'
                     f'<div class="t">{esc(t)}</div><div class="bar"></div><div class="s">{done}</div></div>')
    return '<div class="sprints">' + "".join(cells) + "</div>"




if __name__ == "__main__":
    PART = ROOT / "_partials"
    PART.mkdir(exist_ok=True)
    for lang in ("en", "zh"):
        (IMG / f"architecture-{lang}.svg").write_text(arch(lang))
        (IMG / f"coach-sequence-{lang}.svg").write_text(seq(lang))
        (IMG / f"gate-{lang}.svg").write_text(gate(lang))
        (PART / f"commits-{lang}.html").write_text(f'<div class="chart">{commits(lang)}</div>\n')
        (PART / f"sprints-{lang}.html").write_text(sprint_board(lang) + "\n")
    print("figures: done")
