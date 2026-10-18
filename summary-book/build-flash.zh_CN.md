<p class="lang"><strong>简体中文</strong> · <a href="build-flash.html">English</a></p>

<p class="eyebrow">Build</p>

<a id="制作方法"></a>

# 固件构建

## 从源文件到固件

<figure>
<img src="images/fw-build-zh.svg" alt="固件构建流程">
<figcaption>图 11　生成、验证和固件打包流程。</figcaption>
</figure>

`284ab00` 快照使用四类生成物：

| 生成物 | 输入 | 生成脚本 | 输出 |
|---|---|---|---|
| 状态机和界面 | `flows/coach.flow.json` | `tools/flow/flowc.py` | `main/generated/` 四组 `.c/.h` |
| 交互模拟器 | 同一个流程图 | `tools/flow/flow_sim.py` | `assets/sim/coach-flow.html` |
| 字体 | Noto Sans SC、Staatliches，OFL 1.1，附字符表 | `assets/tools/gen_lv_font.py` | `assets/fonts/heygo_*.c` |
| 精灵图 | 绘制定义 | `gen_sprite.py`、`gen_popup_icons.py` | `assets/sprites/*.c` |

状态机和模拟器共用流程图，使声明的转移规则保持一致。模拟器测试不能证明 BLE 时序、音频行为、任务调度或硬件渲染完全等同于设备。`flowc.py --check` 和 `flow_sim.py --check` 在生成物过期时会报错。

### 字体

| 字体 | 字形数 | 用途 |
|---|---|---|
| `heygo_sc_16` | 3,872 | 正文、转写和回答。包含 3,755 个 GB2312 一级字、ASCII、标点和应用词汇 |
| `heygo_sc_28` | 203 | 大号状态标签、ASCII 和应用词汇 |
| `heygo_display_20/34/48` | 各 96 | Staatliches，用于数字和英文标签。包含 95 个可打印 ASCII 字符和度数符号 |

字形数包含空格。中文字体使用固定字表，不支持任意 Unicode 文本。

## 构建

1. 安装 ESP-IDF 5.5.3。
2. 执行 `. "$IDF_PATH/export.sh"`，激活环境。
3. 进入 `Firmware/` 目录。
4. 执行 `./tools/validate.sh`。

门禁先运行仓库检查、主机测试、生成物检查和字形检查，再用独立的 `sdkconfig` 在临时目录构建固件。随后合并镜像、检查镜像偏移和分区、渲染界面，并归档调试文件。

`./tools/validate.sh --static` 运行不含固件构建的检查。`./tools/validate.sh --firmware` 运行固件构建和镜像检查。

| 产物 | 内容 | 位置 |
|---|---|---|
| 合并镜像 | Bootloader、分区表和应用 | `build/FoloToy-AI-Passport-full.bin` |
| 调试归档 | 镜像、ELF、map、清单和烧录参数 | `build/firmware/<full-image-sha256>/` |
| 应用镜像 | 不含 Bootloader 和分区表的应用 | `build/firmware/<full-image-sha256>/FoloToy-AI-Passport.bin` |

门禁完成后删除临时构建目录。单独运行 `idf.py build` 会保留构建目录，供 `idf.py -p <port> flash` 使用。

<a id="本书写作时的构建结果"></a>

## 构建快照

完整门禁于 2026-10-18 在提交 `284ab00` 上通过，退出码为 0。以下结果仅描述该快照，不代表硬件验收通过。

<div class="tiles">
<div class="tile accent"><div class="k">门禁</div><div class="v" style="font-size:24px">PASS</div></div>
<div class="tile"><div class="k">应用镜像</div><div class="v">1.86<small>MiB</small></div></div>
<div class="tile"><div class="k">合并镜像</div><div class="v">1.92<small>MiB</small></div></div>
<div class="tile"><div class="k">构建步骤</div><div class="v">1,995</div></div>
</div>

| 检查 | 结果 |
|---|---|
| 主机测试 | PASS，含协议、ADPCM、播放规则、按键、链路核心和界面组件 |
| 模拟器 | `--check` PASS，自测 128/128 |
| 字形 | PASS，72 条界面词汇、47 条流程文字 |
| 应用布局 | 镜像为 1,948,640 字节，位于容量为 8,323,072 字节的 `factory` 分区，偏移 `0x10000` |
| 合并镜像 | 2,014,176 字节，烧录地址 `0x0` |

编译选项使用 `-Og` 调试优化并启用断言。Flash 使用 DIO 模式，频率为 80 MHz。

## 烧录

| 场景 | 方法 | 数据影响 |
|---|---|---|
| 新设备或完整刷新 | 将 `FoloToy-AI-Passport-full.bin` 写入 `0x0` | 合并文件填充镜像间空隙，可能覆盖 NVS 和 PHY 初始化数据 |
| 日常开发 | 在保留的构建目录中执行 `idf.py -p <port> flash` | 分段烧录在分区布局兼容时保留 NVS |
| 整片擦除 | 执行 `idf.py erase-flash` | 擦除全部用户数据，仅用于明确需要完整擦除的场景 |

单独的应用镜像依赖兼容的 Bootloader 和分区表，不能独立完成新设备初始化。

日志和烧录使用 ESP32-C3 的 USB Serial/JTAG 接口，占用 GPIO18 和 GPIO19。烧录前不要求备份旧固件。单 factory 分区不提供固件回滚槽位。
