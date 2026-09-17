# IMU 摇动交互

对应需求 FR-06（P1）。此前为零代码。

## 驱动与接口（均取自源码，非猜测）

| 项 | 值 | 依据 |
| --- | --- | --- |
| 器件 | LSM6DSL | `CONFIG_SENSORS_LSM6DSL=y` |
| 设备节点 | `/dev/lsm6dsl0` | `vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/src/sifli_ap.c` 调用 `lsm6dsl_sensor_register()` |
| 启动采样 | `ioctl(fd, SNIOC_START, 0)` | `drivers/sensors/lsm6dsl.c` |
| 读一次 | `ioctl(fd, SNIOC_LSM6DSLSENSORREAD, &data)` | 同上，arg 为 `struct lsm6dsl_sensor_data_s *` |

板级 bring-up 还会先给传感器 LDO 上电（`SF32LB52_LSM6DS3_LDO_PIN`）并把 INT 脚配成输入。

**注意**：本板驱动是传统接口，**没有 uORB 版本**（`drivers/sensors/` 下只有 `lsm6dsl.c`，
无 `lsm6dsl_uorb.c`），且 defconfig **未启用 `CONFIG_UORB`**。因此不要照抄 uORB 的
`/dev/uorb/sensor_accel0` 写法，那条路在本板不存在。

## 单位换算

驱动在 `lsm6dsl.c` 里写 `CTRL1_XL = 0x74`（±16 g）并自述灵敏度 `0.488 mg/LSB`：

```
0.488 mg = 0.488e-3 g = 0.488e-3 × 9.80665 m/s² = 4.7856e-3 m/s²
```

即 1 count = 4.7856 mm/s²。按约定**内部统一用 mm/s² 整数运算**，换算写成有理数
（`core/ebadge_shake.h` 的 `EBADGE_ACCEL_MILLI_NUM/DEN`），避免浮点。

## 检测算法

纯整数逻辑，位于 `core/ebadge_shake.c`，不依赖 LVGL，由主机测试覆盖。

1. 取模长 |a|（64 位累加平方 + 整数开方；±16 g 时 x² 会超出 32 位）。
2. 维护静止基准（指数滑动平均），**仅在未触发且偏离小于释放阈值时更新**，
   否则一次摇动会把基准拉走，掩盖下一次。
3. 偏离 ≥ `EBADGE_SHAKE_TRIGGER_MILLI`(7000，约 0.71 g) 且已武装 → 触发。
4. 触发后需回落到 `EBADGE_SHAKE_RELEASE_MILLI`(3000) 以下才重新武装（迟滞），
   使一次持续摇动只上报一次而不是每个采样都触发。
5. 触发之间至少间隔 `EBADGE_SHAKE_COOLDOWN_MS`(1200)。
   **首次触发不受冷即限制**——否则开机第一秒内的摇动会被静默吞掉。

## 架构位置

```
/dev/lsm6dsl0 --(poll 20Hz)--> core/ebadge_imu.c --(bool)--> view frame()
                                                               |
                                          EBADGE_SHAKE -> controller 有界队列(32)
                                                               |
                                                          正常绘制路径
```

IMU **不直接绘图**：它只上报"发生了摇动"，由控制器决定含义，与点击走同一条路径。
`EBADGE_SHAKE` 在控制器中与 `EBADGE_TAP` 同样处理（重置空闲计时 + 触发一次反应）。

## 采样方式与已知局限

- 当前是**轮询**（帧循环里 20 Hz 节流），**不是**中断驱动。板子把传感器 INT 脚配成了
  输入，但本应用没有消费它。对几百毫秒量级的手势足够。
- 因此**熄屏时摇动无效**：熄屏会暂停帧定时器。这也意味着本版
  **不声明 IMU 唤醒**——手册明确要求"触摸唤醒、IMU 唤醒只有在对应电源域、唤醒源和
  BSP 接口实测通过后才开启"，且"普通线程里的轮询不是低功耗硬件唤醒"。
- 20 Hz 对快速抖动可能欠采样；若真机实测不灵敏，应改为提高采样率或启用 INT 引脚。

## 未验证点（必须真机确认）

- `/dev/lsm6dsl0` 在本固件中是否真的注册成功（注册代码在板级存在，但**未实测**）；
- 传感器轴向与外壳实际朝向的对应关系（未标定）；
- 阈值 7000 / 3000 是否适合真实佩戴动作。**这是最可能需要真机调参的地方**，
  目前数值来自物理推算（0.71 g）而非实测。
