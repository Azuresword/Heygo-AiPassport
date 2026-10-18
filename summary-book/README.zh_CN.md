<p align="right"><strong>简体中文</strong> · <a href="index.html">English</a></p>

# Heygo AI Passport 项目总结

> 快照：2026-10-18，`main` 提交 `c865ba8`
> 每条结论都标了来源：仓库内的文件路径:行号，或 git 提交号。

## 一句话

AI-Passport 是滑雪用的手持小设备。用户休息时不掏手机、不摘手套，就能看本趟数据、问 AI 教练。四个增量的代码都已完成，主机测试都通过，**设备测试一次都没跑**。

## 现状

| 增量 | 功能 | 状态 |
|---|---|---|
| 1 教练语音 | 长按 OK 进教练页，按住 OK 提问，设备播放回答 | 代码完成，设备测试未运行 |
| 2 滑行数据与报告 | 速度、落差、距离、转弯、时间；每趟 Heygo IQ（0–200）；开始滑行跳页（D-32） | 代码完成，设备测试未运行 |
| 3 弯型 | 每趟线路，一页 8 个弯 | 代码完成，设备测试未运行 |
| 4 手机弹窗（D-31） | 摔倒提醒、语音播报、问题报告、成就报告，10 秒倒计时 | 代码完成，主机测试通过，设备测试未运行 |
| D-33 页面导轨 | 屏幕右侧显示页面循环位置 | 模拟器已显示，固件未开始 |

来源：`README.zh_CN.md` "设备做什么"一节；`需求文件夹/04-validation-and-decisions.zh_CN.md:154`

## 最该处理的三件事

1. **上开发板。** Sprint 5 是第一个需要开发板的 Sprint，之后的联调都依赖它（`需求文件夹/08-sprint-roadmap.zh_CN.md:51`）。
2. **App 团队确认 D-20、D-31、D-32。** 三条都写着"等待手机应用团队确认"（`04-validation-and-decisions.zh_CN.md:152-153`）。
3. **HeygoAI 教练能力（Q-01）。** Sprint 8 依赖它，目前没有答案（`08-sprint-roadmap.zh_CN.md:41`）。

详见 [待办与风险](risks.zh_CN.md)。

## 链接

- 交互模拟器：<https://azuresword.github.io/Heygo-AiPassport/>
- 仓库：<https://github.com/Azuresword/Heygo-AiPassport>
- 本书源文件：`summary-book/`。运行 `summary-book/build.sh` 重新生成 `summary/`。
