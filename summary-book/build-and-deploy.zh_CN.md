<p align="right"><strong>简体中文</strong> · <a href="build-and-deploy.html">English</a></p>

# 门禁与部署

## 门禁

1. 激活 ESP-IDF 5.5.3。
2. 在 `Firmware` 里运行 `./tools/validate.sh`。
3. 最后一行应是 `Firmware build: PASS`。

门禁除基线检查外还跑这些（`Firmware/docs/heygo-changes.zh_CN.md:38-74`）：

| 检查 | 查什么 |
|---|---|
| 主机测试 | `Firmware/tests/` 下 33 个 `test_*` |
| 字形检查 | 屏幕文字在字体里都有 |
| `host_render` | 界面重叠、内存池泄漏、UTF-8 截断 |
| `flow_sim --check` / `--run-selftest` | 模拟器和状态机一致；在无界面 Chrome 里跑自测 |

最近记录的结果：主机测试通过，模拟器自测 117/117（`heygo-changes.zh_CN.md:317`）。

**这些都是主机上的结果，不等于设备能跑。**

`validate.sh:60-63` 已处理 macOS：苹果链接器没有 `--gc-sections`，改用 `-dead_strip`。

## CI

workflow 在 `Firmware/.github/workflows/`：build-firmware、firmware-checks、static-checks、sync-main。

GitHub 只读仓库根目录的 `.github/workflows/`。这个仓库根目录没有 `.github/`，所以这些 workflow **可能不会运行**。这是推断，没在 Actions 页核实。

## 部署

- GitHub Pages 从 `main` 根目录发布。根目录有 `.nojekyll`。
- `index.html` 跳到 `assets/sim/coach-flow.html`（`index.html:8`）。
- 本书发布在 `summary/`。改了 `summary-book/` 后运行 `summary-book/build.sh` 重新生成。
