# 开发 Skill 的实现与使用记录

对应需求 DEV-02。Skill 本体位于 `skills/dev/huangshan-bringup/`。

> 本 Skill 是**开发期** Skill（构建／串口 bring-up／证据采集流程），
> 与"设备运行时加载的 ai_agent Skill"是两回事。本作品**没有**实现运行时 AI Skill
> （原因见 `docs/OPENVELA_USAGE.md` §5.1），此处不混淆二者。

---

## 一、Skill 是什么

| 文件 | 作用 |
| --- | --- |
| `skills/dev/huangshan-bringup/SKILL.md` | 黄山派基线构建、串口 bring-up、证据采集的流程与红线 |
| `skills/dev/huangshan-bringup/scripts/preflight.py` | 只读环境自检：输出 JSON，**不开串口、不改配置、不读凭证、不下载** |

Skill 明确约束了若干容易出错的地方，例如：

- 用本队真实 manifest 与锁定的 BSP，**不拿 SiFli SDK 或 RT-Thread 例程当最终运行时**；
- `.repo` 已初始化**不等于** sync 完成，必须看退出码；
- 官方文档给的是 `-S "$PWD/nuttx"`，但**必须对照实际检出的 README**；
- 串口 CH340N 的 RTS 驱动复位，**仅仅打开串口并保持默认设置可能让板子一直处于复位**；
- 烧录地址基线为 `0x12010000`，**不得猜地址、不得整片擦除**；
- **黑屏不等于低功耗**；IMU／网络／充电／深睡是彼此独立的能力，各自需要驱动与硬件证据。

---

## 二、实际使用记录

### 2.1 执行环境自检

```bash
cd skills/dev/huangshan-bringup
source /path/to/.tools/env.sh
python3 scripts/preflight.py --workspace /media/ran/EXTERNAL_USB/watch/openvela
```

**退出码 0**，完整输出留存于 `docs/evidence/preflight-2026-09-17.json`。要点：

| 项 | 实测值 |
| --- | --- |
| `repo_initialized` | `true` |
| `free_bytes` | 1,650,838,843,392（约 1.5 TB） |
| git | 2.43.0 |
| git-lfs | 3.4.1 |
| cmake | 3.28.3 |
| ninja | 1.11.1 |
| arm-none-eabi-gcc | 13.2.1 20231009 |
| `nuttx/CMakeLists.txt` 等三个关键源文件 | 均存在 |
| 串口 | `/dev/ttyUSB0`，身份 `usb-1a86_USB_Serial-if00-port0`，**`read_write_access: false`** |
| `firmware_build` / `hardware_validation` | `NOT_RUN` |

> 说明：`firmware_build: NOT_RUN` 是 **preflight 自身没有执行构建**的意思，
> 不是"构建失败"。本仓实际构建结果见 §2.3。

### 2.2 Skill 的指导**实际改变了**本次工作

这不是"跑过一次脚本"，Skill 里的判断在本次开发中**直接命中了两个真实问题**：

**（a）"必须对照实际检出的 README，而不是照抄文档命令" → 命中了烧录工具路径错误**

按 Skill 要求核对后确认：官方 README 写的烧录路径是 `cmake_out/lckfb_huangshan_pi/`，
但本工程实际产物在 `cmake_out/ebadge_app/nuttx.bin`；且 `sftool` 在**工作区根目录**
`.tools/` 下，**不在 `openvela/` 内**——按文档原样执行会直接失败。
结论已回写进 `docs/STATUS.md` 与 `docs/UI_ACCEPTANCE.md`（提交 `dcaa532`），
并写进本仓 README 的 §4.3。

**（b）"`.repo` 已初始化不等于 sync 完成" → 命中了 `nuttx/` 工作树残缺**

按 Skill 要求检查实际检出状态后发现：`git -C nuttx status --short` 显示
**23986 个受跟踪文件处于已暂存删除状态**，`net/`、`mm/`、`sched/` 等目录不在磁盘上，
而 `git ls-tree HEAD` 证明这些目录**在仓库里是被跟踪的**。这正是构建实际改用
`nuttx-ebadge-clean/` 的原因（`CMakeCache.txt` 的 `NUTTX_SOURCE_DIR` 为证）。
若不做这一步检查，会得出"源码已就绪、直接构建即可"的错误结论。

**（c）串口权限被量化确认**

`read_write_access: false` 加上设备身份 `usb-1a86_USB_Serial-if00-port0`，
把"烧录不了"从一句模糊描述变成了**可复现、可转交的具体事实**。

### 2.3 Skill 未覆盖、但按其证据要求执行的部分

Skill 要求"保留构建 stdout/stderr 与退出码，不要用 tee 的成功退出码掩盖失败的构建"。
本次构建按要求单独取退出码，未使用管道尾部的状态：

```bash
ninja -C cmake_out/ebadge_app
# 退出码 0，0 warning
# flash 9,468,380 B / 16 MB = 56.44%
# SRAM  107,180 B / 512 KB = 20.44%
```

---

## 三、Skill 的复用性

- **无硬编码路径**：`preflight.py` 通过 `--workspace` 接收工作区路径，换机器只需换参数。
- **只读安全**：脚本不打开串口、不改配置、不读凭证、不触发下载，可在任何环境放心执行。
- **语言无关**：检查的是工具链与源码存在性，不绑定本作品。
- **可迁移**：流程（环境自检 → 核对实际检出 → 构建留证 → 授权后烧录 → 采集证据）适用于任何
  基于本 manifest 的黄山派项目，不限于电子吧唧。

---

## 四、未完成部分（诚实说明）

- **Skill 未覆盖真机阶段的实际使用**：因串口权限未取得，烧录与 NSH 证据采集**从未执行**，
  因此 Skill 中"授权后烧录 → 采集启动／触摸／按键／堆输出"这一段**尚无真实使用记录**。
- `docs/evidence/preflight-2026-09-17.json` 是**时点快照**，不是固件或硬件测试结论。
