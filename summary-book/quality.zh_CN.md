<p class="lang"><strong>简体中文</strong> · <a href="quality.html">English</a></p>

<p class="eyebrow">Quality</p>

# 质量与部署

## 门禁

每个 Sprint 结束时，`./tools/validate.sh` 都必须退出码为 0（`需求文件夹/08-sprint-roadmap.zh_CN.md` 第 1 节）。

<figure>
<img src="images/gate-zh.svg" alt="固件门禁流水线">
<figcaption>图 8　门禁的六步。来源：<code>Firmware/docs/heygo-changes.zh_CN.md</code> 第 3 节。</figcaption>
</figure>

| 步骤 | 查什么 |
|---|---|
| 主机测试 | `Firmware/tests/` 下 33 个 `test_*`，例如链路核心、弯型页、弹窗页、ADPCM 编解码 |
| 字形检查 | 屏幕上的每个字在字体里都有 |
| `host_render` | 界面重叠、内存池泄漏、UTF-8 截断。同时产出 `assets/mockups/` 的界面图 |
| `flow_sim` | 模拟器和状态机一致。在无界面 Chrome 里跑自测 |

## 本书写作时复核的结果

2026-10-18 在 `main` 提交 `284ab00` 上运行完整门禁 `./tools/validate.sh`，退出码 0。

<div class="tiles">
<div class="tile accent"><div class="k">完整门禁</div><div class="v" style="font-size:24px">PASS</div></div>
<div class="tile accent"><div class="k">模拟器自测</div><div class="v">128<small>/128</small></div></div>
<div class="tile"><div class="k">字形</div><div class="v">72<small>+47</small></div></div>
<div class="tile"><div class="k">合并镜像</div><div class="v">1.92<small>MiB</small></div></div>
</div>

逐项结果见 [制作方法](build-flash.zh_CN.md)。

## 设备侧验证

开发板烧录、手机应用联调（TC-01 到 TC-24）、U1 可用性测试（5 名测试者）都在线下设备上完成，记录不在本仓库里。

## 部署

| 内容 | 地址 |
|---|---|
| 交互模拟器 | <https://azuresword.github.io/Heygo-AiPassport/> |
| 本书 | <https://azuresword.github.io/Heygo-AiPassport/summary/> |

- GitHub Pages 直接发布 `main` 的根目录，根目录有 `.nojekyll`。
- `index.html` 跳转到 `assets/sim/coach-flow.html`。这个页面由 `Firmware/flows/coach.flow.json` 生成，不要手改。
- 本书的源文件在 `summary-book/`，生成物在 `summary/`。

<div class="callout"><b>CI 提示。</b>workflow 放在 <code>Firmware/.github/workflows/</code>。GitHub 只读取仓库根目录下的 <code>.github/workflows/</code>，所以这些 workflow 可能不会自动运行。这是推断，没有在 Actions 页面核实。</div>
