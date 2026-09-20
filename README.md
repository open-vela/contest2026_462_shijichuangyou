# 拾迹创游 · 互动电子吧唧

> 一块可以戴在身上的电子吧唧（徽章）：三个二次元角色陪着你看时间、说想说的话、数纪念日，摇一摇会有反应。
> 运行在 openvela / NuttX 上，UI 用 LVGL，全部界面文字中文。

---

## 一、作品简介

**它是什么**：一块 390×450 的 AMOLED 小屏挂件，跑在黄山派（SiFli SF32LB52）开发板上。开机即用，不依赖手机、不依赖网络。

**解决什么问题**：把"随手一瞥就能获得情绪价值"这件事做扎实。手机需要掏、需要解锁、需要点开 App；电子吧唧戴在身上，抬手就是一张喜欢的脸和一句想说的话。

**亮点**：

1. **三个角色、四套时段问候**——苔苔、暖月、月月各有人设，问候语按清晨／午后／黄昏／深夜四个时段分别写，不是一句模板套到底。
2. **完整的中文矢量字体**（正文 4034 字形 + 标题 153 字形），自绘立绘，不依赖外挂字库或图片文件。
3. **真实硬件交互，而不是只有界面**：摇动设备角色会响应（LSM6DSL 加速度计）；闲置 15 秒变暗、30 秒**真正给面板断电**（不是画一张黑图），触摸即恢复。
4. **时钟掉电可恢复**：把时间写进 RTC，冷启动自动读回，纪念日页因此能算出真实天数。
5. **设置可持久化**：角色、配色、留言、纪念日存在板上，重启不丢。
6. **纯逻辑与设备分层**：所有时序／状态机规则都抽成不依赖 LVGL 的纯函数，可在主机上跑 1622 项断言。

---

## 二、选题方向

**AI 硬件产品创新**。

理由：本作品是一个**独立的消费级硬件形态**（可佩戴徽章），核心价值在"情绪陪伴"这一真实场景，而非复刻手机上的应用。它验证的是"openvela + 小屏 + 传感器"能不能撑起一个愿意天天戴在身上的物件。

> 说明：本仓同时包含 `quickapp/` 与 `board/` 子目录（组委会约定的三种作品形态以子目录组织），但**本次参赛的形态是 `app/` 下的板端原生应用**，quickapp 与 board 未纳入本次提交范围。

---

## 三、目录结构

| 路径 | 作用 |
| --- | --- |
| `app/hello_app/` | **作品主体**：板端 LVGL 应用（C 语言） |
| `app/hello_app/core/` | 纯逻辑层：状态机、设置编解码、日历、摇动检测、RTC 同步判定。**不依赖 LVGL，可在主机测试** |
| `app/hello_app/ui/` | 表现层：主题、自绘装饰、页面、立绘动画、字体、设备适配（屏幕电源／IMU／RTC） |
| `skills/dev/huangshan-bringup/` | **开发用 Skill**：黄山派构建／串口 bring-up 的环境自检与排错流程 |
| `docs/` | 设计与验收文档（UI 改版、熄屏策略、IMU、RTC、能力使用说明、验收清单等） |
| `docs/evidence/` | 真实证据：preflight 报告等 |
| `logs/` | AI Coding 对话日志（主动导出后提交） |
| `tools/` | 主机侧工具：纯逻辑测试、字形覆盖校验、字体生成、串口脚本 |
| `assets/portraits/` | 立绘素材源文件 |
| `design/` | 交互原型与设计稿（浏览器原型、动效方案） |
| `board/contest_board/` | 板级适配目录（本次未使用） |
| `quickapp/hello_quickapp/` | 快应用目录（本次未使用） |
| `contest2026_462_shijichuangyou.xml` | 团队仓 manifest：把本仓子目录映射到 openvela 工程位置 |
| `LICENSE` | Apache License 2.0 全文 |
| `submission/` | 另外三项提交物：作品介绍文档（PDF + DOCX）、路演 PPT、演示视频脚本（见该目录 `README.md`） |

---

## 四、运行方式

### 4.1 拉取完整工程

```bash
repo init -u <组委会提供的 manifest 地址> -b dev-ai-contest-2026
repo sync -c -j8
```

`repo sync` **必须确认退出码为 0**。`.repo` 已初始化不等于同步完成。

### 4.2 编译

工具链先入环境（本仓自带本地工具包）：

```bash
source /path/to/.tools/env.sh
```

官方文档给出的 CMake 入口（在 openvela 工作区根目录执行）：

```bash
cmake -B cmake_out/ebadge_app -S "$PWD/nuttx" -GNinja \
  -DBOARD_CONFIG=../vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/configs/nsh \
  -DEXTRA_FLAGS="-Wno-cpp -Wno-deprecated-declarations"
cmake --build cmake_out/ebadge_app
```

> **本仓的工作区的实际差异（务必注意）**：本次开发实际使用的源码目录是 `nuttx-ebadge-clean/`，不是 `nuttx/`。原因是工作区里的 `nuttx/` 存在异常——`git -C nuttx status --short` 显示 **23986 个受跟踪文件处于已暂存删除状态**，即该工作树缺 `net/`、`mm/`、`sched/` 等目录。证据见 `openvela/cmake_out/ebadge_app/CMakeCache.txt` 中的 `NUTTX_SOURCE_DIR`。
> **在干净 `repo sync` 出来的新工作区里应使用 `nuttx/`**；本仓记录的 `nuttx-ebadge-clean` 是应对上述异常的工作区特定做法。
> ⚠️ **不要在 `nuttx/` 目录内执行 `git commit`**，那会提交掉这近 2.4 万个文件的删除。

### 4.3 烧录

```bash
cd <工作区根目录>
.tools/sftool-0.2.5/sftool -c SF32LB52 -p /dev/ttyUSB0 -b 1000000 \
  --before default_reset --after soft_reset \
  write_flash openvela/cmake_out/ebadge_app/nuttx.bin@0x12010000
```

- 固件位于 `openvela/cmake_out/ebadge_app/nuttx.bin`；烧录地址 `0x12010000`。
- `sftool` 在**工作区根目录** `.tools/` 下，不在 `openvela/` 内。
- 不要加 `-e/--erase-all`（会擦掉校准区）。`write_flash` 默认回读校验，无需再加 `--verify`。
- 串口需要读写权限（`/dev/ttyUSB0` 通常属 `dialout` 组）。**本工作区该权限未取得，因此烧录步骤在本仓为未执行状态。**

### 4.4 运行

烧录后板子自动运行。接入串口可看到 NSH 与应用的启动日志：

```
ebadge 0.1.0; local-interaction slice; hardware acceptance pending
ebadge: 时钟：已由 RTC 恢复 (action 2)
```

操作方式：

| 操作 | 效果 |
| --- | --- |
| 左右滑动 | 切换角色 |
| 轻点 | 角色做出反应 |
| **摇一摇设备** | 角色做出反应 |
| 长按 | 打开菜单（时钟／留言／纪念日／配色） |
| 闲置 15 秒 | 画面变暗 |
| 闲置 30 秒 | **面板断电熄屏** |
| 触摸屏幕 | 从熄屏恢复（该次触摸只唤醒，不触发其他操作） |

### 4.5 主机侧验证（不需要开发板）

```bash
sh tools/run_logic_test.sh        # 纯逻辑断言
python3 tools/verify_font_coverage.py   # 界面用字是否都有字形
```

---

## 五、AI Coding 使用说明

完整对话日志见 `logs/` 目录。

### 怎么协作的

1. **需求拆解与方案设计**：先把官方手册、评分标准、板级 defconfig 交给 AI 一起读，让它列出"哪些能力是真能用的"。这一步直接推翻了若干想当然的判断——例如**音频**：厂商原厂固件里有 `audcodec` 设备，但 openvela 的 vendor 树里只有 CMSIS 寄存器头文件、**没有 NuttX 音频驱动**，所以"加个提示音"这条路从一开始就不通。
2. **编码**：AI 负责按既有分层写实现。我把架构约束前置写进了项目级约定（唯一 UI 上下文、事件经有界队列进 controller、IMU 不直接绘图、单位统一），避免它写出"能跑但违反架构"的代码。
3. **调试**：编译器与测试是主要反馈回路。AI 每轮都要跑 `ninja`（要求 0 warning）与主机测试；**发现问题必须改代码，不许改测试来迁就**。
4. **文档**：每个功能配一份说明，且**必须写明"未验证点"**。

### AI 带来的实际帮助

- **把"看起来对"变成"可验证"**：所有时序／状态机规则被抽成纯函数后能在主机上跑断言，最终 **1622 项 0 失败**。
- **主动抓到过真实缺陷**，例如：
  - 摇动检测里 `last_trigger_ms` 初值 0 与合法 tick 0 语义冲突，导致**开机后 1.2 秒内的摇动被静默吞掉**；
  - 界面状态串里用了 `²`（U+00B2），**字体里没有该字形**，真机会渲染成空白框——这是字形覆盖脚本自己抓出来的。
- **减少了"想当然"**：设备节点、ioctl 常量、数据结构和单位换算**全部要求给出源码出处**（文件+行号），不接受推测。`.tools/contest-docs/` 下保留了官方文档原件备查。

### 使用边界（诚实说明）

- AI **不能**替代真机验证。本仓所有硬件行为目前都是 `NOT_RUN`——板子从未烧录成功（串口权限未取得），代码存在不等于实测通过。
- AI 参与的每个功能都保留了"未验证点"清单，不使用"应该可以"这类表述。
- 开发 Skill 位于 `skills/dev/huangshan-bringup/`，其真实使用记录见 `docs/SKILL_USAGE.md`。

---

## 六、当前完成度与验证状态（诚实声明）

**已完成并验证（主机侧）**

| 项 | 证据 |
| --- | --- |
| 固件编译 | `ninja` 退出码 0，**0 warning**；flash 9,468,380 B / 16 MB = 56.44%，SRAM 107,180 B / 512 KB = 20.44% |
| 纯逻辑测试 | `sh tools/run_logic_test.sh` → **1622 项断言，0 失败** |
| 字形覆盖 | `tools/verify_font_coverage.py` → 界面用字**全部有字形**（本次提交实测 445 条字面量、282 个不同宽字符、0 缺失） |
| 环境自检 | `docs/evidence/preflight-2026-09-17.json`（工具链版本齐全） |

**未验证（需要开发板，本仓不声称通过）**

- 全部硬件行为：屏幕电源 ioctl、触摸唤醒、加速度计、RTC——**代码与板级注册代码都在，但从未在真机跑过**。
- 摇动阈值（0.71 g）来自物理推算，**未经真实手腕标定**，最可能需要现场调参。
- RTC 是否**掉电**保时未确认；若不保，只能跨热重启。
- 无任何续航／功耗数字。

验收清单见 `docs/UI_ACCEPTANCE.md`（共 50 项）。

---

## 附：关于 PR 与 CLA

- 本作品以 **PR** 形式合入本专属仓，可自行 review 合入。
- 首次提交 PR 会触发 `cla/signature` 检查；需先在 [openvela 官网](https://openvela.com/#/community/cla) 用报名账号签署 CLA，然后在原 PR 下评论 `/check-cla` 触发复检。
- 官方文档：<https://github.com/open-vela/docs/blob/dev-ai-contest-2026/zh-cn/contest_2026/code_submission_guide.md>

## 附：开源许可

- **本项目代码遵循 Apache License 2.0**（《大赛总览》要求参赛作品遵循 Apache 2.0），全文见仓根 `LICENSE`。
- 本项目手写源文件均带 `SPDX-License-Identifier: Apache-2.0` 标识。以下四类文件除外，原因如实说明：
  - `ui/ebadge_font_zh_20.c`、`ui/ebadge_font_title_26.c`、`ui/ebadge_portraits.c`、`ui/ebadge_motion.c` —— 由 `tools/` 下脚本生成，头部为生成器注释，**手改会在下次生成时丢失**；
  - `hello_app_main.c` —— 组委会模板自带的 team 000 示例，与 `CMakeLists.txt` 中 `CONFIG_LVX_USE_DEMO_CONTEST2026_000_HELLO_APP` 分支绑定。本作品走的是 `CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE` 分支（实测 `.config` 中 team 000 为 `not set`），故该文件**不参与本次构建**。
- 第三方素材（字体、立绘）的许可与来源说明见 `docs/FONT_LICENSE.txt` 与 `docs/PORTRAIT_ASSETS.md`，**其许可不因本项目采用 Apache 2.0 而改变**。
