/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_imu.h"
#include "ebadge_shake.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <nuttx/sensors/ioctl.h>
#include <nuttx/sensors/lsm6dsl.h>

#define EBADGE_IMU_DEV "/dev/lsm6dsl0"

/* 20 Hz. The driver runs the part far faster, but a hand shake lasts a few
 * hundred milliseconds and the I2C read is the cost that matters here, so
 * sampling every frame would spend bus time for no extra sensitivity.
 */
#define EBADGE_IMU_SAMPLE_MS 50u

static int imu_fd = -1;
static bool imu_open_attempted;
static bool imu_started;
static bool imu_got_sample;
static uint32_t imu_last_sample_ms;
static struct ebadge_shake imu_shake;
static char imu_status[96] = "加速度计：尚未尝试";

bool ebadge_imu_open(void)
{
  if (imu_started) return true;
  if (imu_fd < 0)
    {
      if (imu_open_attempted) return false;
      imu_open_attempted = true;
      imu_fd = open(EBADGE_IMU_DEV, O_RDONLY | O_CLOEXEC);
      if (imu_fd < 0)
        {
          snprintf(imu_status, sizeof(imu_status), "加速度计：无法打开 %s（%d）",
                   EBADGE_IMU_DEV, errno);
          return false;
        }
    }

  if (ioctl(imu_fd, SNIOC_START, 0) < 0)
    {
      snprintf(imu_status, sizeof(imu_status), "加速度计：启动失败（%d）", errno);
      return false;
    }

  ebadge_shake_init(&imu_shake);
  imu_started = true;
  snprintf(imu_status, sizeof(imu_status), "加速度计：正常（摇一摇试试）");
  return true;
}

void ebadge_imu_close(void)
{
  if (imu_fd >= 0 && imu_started) ioctl(imu_fd, SNIOC_STOP, 0);
  if (imu_fd >= 0) close(imu_fd);
  imu_fd = -1;
  imu_started = false;
  imu_got_sample = false;
  imu_open_attempted = false;
}

bool ebadge_imu_poll(uint32_t now)
{
  if (!imu_started && !ebadge_imu_open()) return false;

  /* Throttled here rather than by the caller, so the frame rate can change
   * without changing how often the bus is touched.
   */
  if (imu_got_sample &&
      (uint32_t)(now - imu_last_sample_ms) < EBADGE_IMU_SAMPLE_MS)
    return false;

  struct lsm6dsl_sensor_data_s raw;
  if (ioctl(imu_fd, SNIOC_LSM6DSLSENSORREAD, (unsigned long)&raw) < 0)
    {
      snprintf(imu_status, sizeof(imu_status), "加速度计：读取失败（%d）", errno);
      return false;
    }

  imu_last_sample_ms = now;
  imu_got_sample = true;

  struct ebadge_accel sample;
  sample.x = ebadge_accel_counts_to_milli(raw.x_data);
  sample.y = ebadge_accel_counts_to_milli(raw.y_data);
  sample.z = ebadge_accel_counts_to_milli(raw.z_data);

  bool shaken = ebadge_shake_update(&imu_shake, &sample, now);
  /* The magnitude is deliberately not surfaced: it is calibration data rather
   * than something the wearer needs, and its unit glyph is not in the shipped
   * font, so printing it would render as a blank box.
   */
  return shaken;
}

bool ebadge_imu_available(void) { return imu_started && imu_got_sample; }

const char *ebadge_imu_status(void) { return imu_status; }
