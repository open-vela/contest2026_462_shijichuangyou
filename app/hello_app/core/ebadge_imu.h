/* SPDX-License-Identifier: Apache-2.0 */
/* Accelerometer adapter for the shake gesture.
 *
 * Device and ioctl names come from the sources, not from guesswork:
 *
 *   vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/src/sifli_ap.c
 *       lsm6dsl_sensor_register("/dev/lsm6dsl0", ...)  (guarded by
 *       CONFIG_SENSORS_LSM6DSL, which this board's defconfig sets)
 *   drivers/sensors/lsm6dsl.c
 *       case SNIOC_LSM6DSLSENSORREAD -> ops->sensor_read(priv, arg)
 *       case SNIOC_START / SNIOC_STOP
 *
 * This module deliberately knows nothing about LVGL or the controller: it
 * reports "a shake happened" and the caller decides what that means. The
 * project rules put IMU handling outside the drawing context for exactly that
 * reason.
 *
 * It is a polled reader, not an interrupt-driven one. The board wires the
 * sensor's INT pin and reads it as an input, but nothing in this application
 * consumes it, so sampling happens from the frame loop. That is adequate for
 * a gesture measured in hundreds of milliseconds, and it means shake detection
 * is inactive while the panel is off, which is also when the frame loop is
 * paused.
 */
#ifndef EBADGE_IMU_H
#define EBADGE_IMU_H
#include <stdbool.h>
#include <stdint.h>

/* Open the accelerometer and start sampling. Safe to call repeatedly. */
bool ebadge_imu_open(void);
void ebadge_imu_close(void);

/* Sample and run the detector, at most once per internal interval. Returns
 * true when a shake was detected on this call. Cheap enough for a frame loop.
 */
bool ebadge_imu_poll(uint32_t now);

/* True when the device is open and has produced at least one valid sample, so
 * a caller can distinguish "no shake" from "no sensor".
 */
bool ebadge_imu_available(void);

/* One-line status for the acceptance screen. Never NULL. */
const char *ebadge_imu_status(void);

#endif
