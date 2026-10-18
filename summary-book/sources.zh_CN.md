<p class="lang"><strong>简体中文</strong> · <a href="sources.html">English</a></p>

<p class="eyebrow">Sources</p>

# 数据来源

## 来自哪里

| 内容 | 来源 |
|---|---|
| Sprint 完成状态 | 项目负责人 2026-10-18 确认 |
| 需求、决策、协议、Sprint 目标 | `需求文件夹/01`–`08`，`main` 提交 `284ab00` |
| 硬件 | `Firmware/docs/README.zh_CN.md:138-155` |
| 门禁和已知限制 | `Firmware/docs/heygo-changes.zh_CN.md` |
| 提交数、时间、作者 | `git rev-list`、`git log` |
| 模拟器自测 128/128 | 2026-10-18 在 `284ab00` 的完整门禁里运行 |
| 完整门禁、镜像大小、内存各段 | 2026-10-18 在 `284ab00` 上运行 `./tools/validate.sh`、`idf.py size`、`size-components`、`size-files` |
| 任务、队列、栈 | `main/coach_app.c`、`main/audio/voice.c`、`main/link/ble_link.c`、esp_lvgl_port、生成的 `sdkconfig` |
| 界面图和动画 | `assets/mockups/`、`assets/sim/` |
| 架构图、时序图、门禁图、提交图 | `summary-book/figures.py` 根据上面的来源绘制 |

## 推断，不是事实

| 说法 | 依据 |
|---|---|
| CI 可能不会自动运行 | GitHub 只读根目录的 `.github/workflows/`；没看 Actions 页面 |
| “41 小时” | 首尾提交时间相减 |

## 范围

本书只写 AI Passport。
