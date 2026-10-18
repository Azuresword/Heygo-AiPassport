<p class="lang"><strong>简体中文</strong> · <a href="sources.html">English</a></p>

<p class="eyebrow">Sources</p>

# 数据来源

## 来自哪里

| 内容 | 来源 |
|---|---|
| Sprint 完成状态 | 项目负责人 2026-10-18 确认 |
| 需求、决策、协议、Sprint 目标 | `需求文件夹/01`–`08`，`main` 提交 `c865ba8` |
| 硬件 | `Firmware/docs/README.zh_CN.md:138-155` |
| 门禁和已知限制 | `Firmware/docs/heygo-changes.zh_CN.md` |
| 提交数、时间、作者 | `git rev-list`、`git log` |
| 模拟器自测 122/122 | 2026-10-18 在 `d99a7d9` 上运行 `flow_sim.py --run-selftest` |
| 界面图和动画 | `assets/mockups/`、`assets/sim/` |
| 架构图、时序图、门禁图、提交图 | `summary-book/figures.py` 根据上面的来源绘制 |

## 推断，不是事实

| 说法 | 依据 |
|---|---|
| CI 可能不会自动运行 | GitHub 只读根目录的 `.github/workflows/`；没看 Actions 页面 |
| “41 小时” | 首尾提交时间相减 |

## 范围

本书只写 AI Passport。
