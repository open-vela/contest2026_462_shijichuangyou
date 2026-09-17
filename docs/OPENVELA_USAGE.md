# 本项目对 openvela 系统能力的使用说明

对应大赛硬性资格条款（《大赛总览》）：

> 项目须使用 openvela 提供的系统能力（**NuttX 内核仓库除外**），且**至少落地图形、AI、多媒体三项之一**。

本文逐条给出**证据**（配置符号、源码路径、实际调用点），并把"用了什么"与"没用什么"分开写清楚。

---

## 一、结论速览

| 三项要求 | 本项目 | 依据 |
| --- | --- | --- |
| 至少落地**图形** | ✅ **已落地** | LVGL 9.1 全界面，见 §二 |
| 至少落地 AI | ❌ 未落地 | 见 §五（诚实说明原因） |
| 至少落地多媒体 | ❌ 未落地 | 见 §五（该板 openvela 无音频驱动） |

满足"三项之一"，且所用能力来自 openvela 提供的仓库，**不限于 NuttX 内核**。

---

## 二、图形（已落地）

### 2.1 使用的是 openvela 提供的 LVGL，不是自行移植

| 项 | 值 | 证据 |
| --- | --- | --- |
| LVGL 源码 | 9.1.0 | `apps/graphics/lvgl/lvgl/lvgl.h` → `LVGL_VERSION_MAJOR 9` / `MINOR 1` |
| 构建开关 | 已启用 | `CONFIG_GRAPHICS_LVGL=y` |
| NuttX 适配层 | 已启用 | `CONFIG_LV_USE_NUTTX=y` |
| 显示对接 | 已启用 | `CONFIG_LV_USE_NUTTX_LCD=y` |
| 触摸对接 | 已启用 | `CONFIG_LV_USE_NUTTX_TOUCHSCREEN=y` |
| 应用初始化入口 | `lv_nuttx_dsc_init()` / `lv_nuttx_init()` | `app/hello_app/ebadge_main.c:45,50` |

即：本作品**直接使用 openvela 仓库内的 LVGL 及其官方 NuttX 适配层**，没有自行移植图形栈，也没有绕开它直接操作帧缓冲。

### 2.2 按 openvela 的应用框架注册

本应用不是游离的独立程序，而是通过组委会 manifest 的 `<linkfile>` 映射进 openvela 的 apps 树，并走 openvela 的 Kconfig + CMake 机制注册：

```
contest2026_462_shijichuangyou.xml
    <linkfile src="app/hello_app" dest="packages/demos/contest2026_462_hello_app"/>

构建配置：CONFIG_LVX_USE_DEMO_CONTEST2026_462_EBADGE=y
Kconfig：  app/hello_app/Kconfig   → config LVX_USE_DEMO_CONTEST2026_462_EBADGE
```

### 2.3 图形能力的具体落地范围

| 能力 | 实现位置 |
| --- | --- |
| 中文矢量字体渲染 | `app/hello_app/ui/ebadge_font_zh_20.c`（4034 字形）、`ebadge_font_title_26.c`（153 字形） |
| 自绘装饰（对话气泡、头像、闪光、纸片） | `app/hello_app/ui/ebadge_deco.c` |
| 立绘逐帧动画 | `app/hello_app/ui/ebadge_motion.c`（LVGL 图像对象 + 定时器） |
| 主题与时段配色 | `app/hello_app/ui/ebadge_theme.c`（四时段 × 三角色） |
| 页面与手势（滑动／点击／长按） | `app/hello_app/ui/ebadge_view.c`、`ebadge_pages.c` |
| 动画系统 | LVGL `lv_anim`（透明度、位移、缩放补间） |

---

## 三、其他 openvela 能力的深度使用

除图形外，本作品还使用了 openvela **vendor 仓库**提供的板级支持，以及板级已经接好的设备：

| 能力 | 设备／接口 | 源码依据 |
| --- | --- | --- |
| 面板电源控制（L2 熄屏） | `ioctl("/dev/lcd0", LCDDEVIO_SETPOWER, 0)` | `nuttx/drivers/lcd/lcd_dev.c` 注册 `/dev/lcd%i`；`vendor/sifli/.../lcd/sf32lb_lcd.c` 的 `sf32lb_lcd_setpower()` → DisplayOn/Off |
| 加速度计（摇动交互） | `/dev/lsm6dsl0` + `SNIOC_START` / `SNIOC_LSM6DSLSENSORREAD` | `vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/src/sifli_ap.c` 调用 `lsm6dsl_sensor_register()`（含 LDO 上电、INT 脚配置） |
| 实时时钟（掉电恢复） | `/dev/rtc0` + `RTC_RD_TIME` / `RTC_SET_TIME` / `RTC_HAVE_SET_TIME` | `vendor/sifli/.../sifli_ap.c` 调用 `rtc_initialize(0, sf32lb_rtc_lowerhalf())` |
| 板上设置持久化 | 文件系统存储 | `app/hello_app/core/ebadge_store.c` |

**重要边界**：上述 `drivers/lcd`、`drivers/sensors`、`drivers/timers` 位于 NuttX 仓库内，属于资格条款**排除**的部分。因此本作品**不把它们当作满足资格条款的依据**——满足条款的是 §二 的图形能力（LVGL，来自 openvela 的 apps 仓库）与 vendor BSP（openvela 的 vendor 仓库）。

---

## 四、工程与测试能力（辅助证据）

| 项 | 说明 |
| --- | --- |
| 编译产物 | `ninja` 退出码 0，**0 warning**；flash 9,468,380 B / 16 MB = 56.44%，SRAM 107,180 B / 512 KB = 20.44% |
| 纯逻辑主机测试 | `tools/run_logic_test.sh` → **1622 项断言 0 失败**（逻辑层不依赖 LVGL，可脱离硬件运行） |
| 字形覆盖校验 | `tools/verify_font_coverage.py` → 界面用字全部有字形（本次提交实测 445 条字面量、282 个不同宽字符、0 缺失） |
| 环境自检 | `skills/dev/huangshan-bringup/scripts/preflight.py` → `docs/evidence/preflight-2026-09-17.json` |

---

## 五、未落地的能力，以及原因（诚实说明）

### 5.1 AI：本板不具备可用的真实 LLM 通路

- **`packages/ai_agent` 未同步到工作区**：`.repo/manifests/openvela.xml:208` 登记了 `path="packages/ai_agent"`，但工作区 `packages/` 下不存在该目录。
- **板子没有可用的网络接口**：构建配置只启用了 `CONFIG_NET_LOOPBACK` / `NET_ROUTE` / `NET_TCP` / `NET_UDP`，**没有任何 WiFi 或以太网驱动**。
- 手册允许"电脑作为透明网络代理"，且本 NuttX 树确实存在 `NET_SLIP` / `NET_TUN` / `NET_USRSOCK`。但该路径需要：同步 ai_agent、打通串口 SLIP 代理、提供模型凭证，并做大量集成——**在剩余时间内无法完成且无法真机验证**，因此**不做，也不声称已做**。

按项目约定，P1 遇阻必须记录范围决策而非静默删除，故在此明确记录：**AI 相关需求（AI-01..04）保持未实现状态。**

### 5.2 多媒体：该板 openvela 侧没有音频驱动

- 厂商原厂（RT-Thread）固件的设备表里**确实**有 `audcodec` / `audprc`（Sound Device），容易据此以为可以直接加提示音。
- 但在 openvela 的 vendor 树中，音频**只有 CMSIS 寄存器头文件**（`vendor/sifli/chips/drivers/cmsis/sf32lb52x/audcodec.h` 等），**没有 NuttX 音频驱动**。
- 构建 defconfig 中音频相关配置项数量为 **0**。

即"芯片里有音频外设"**不等于**"openvela 能用音频"。要做多媒体需先写完整音频驱动，超出本次范围。

### 5.3 亮度调节

- `sf32lb_lcd_setpower()` **只做面板 DisplayOn/Off，没有亮度级**。
- CO5300 为 AMOLED（自发光），且厂商固件中的 `lcdlight` 背光设备**未移植**到 openvela。

因此本作品的"变暗"是叠加遮罩（属 L1），"熄屏"是真实断电（L2）。详见 `docs/POWER_POLICY.md`。

---

## 六、可复核方式

以上每条均可按给出的路径与配置符号自行核对：

```bash
# 图形能力
grep -E "GRAPHICS_LVGL|LV_USE_NUTTX" openvela/cmake_out/ebadge_app/.config

# 应用注册
grep -n "hello_app" contest2026_462_shijichuangyou.xml

# 网络能力的有无
grep -E "^CONFIG_NET_" openvela/cmake_out/ebadge_app/.config

# 音频能力的有无
grep -icE "AUDIO|CODEC|I2S" \
  openvela/vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/configs/nsh/defconfig
```
