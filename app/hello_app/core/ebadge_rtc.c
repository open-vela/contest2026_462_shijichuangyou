/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_rtc.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <nuttx/timers/rtc.h>

#define EBADGE_RTC_DEV "/dev/rtc0"

static int rtc_fd = -1;
static bool rtc_open_attempted;
static bool rtc_saw_ioctl;
static char rtc_status[96] = "时钟：尚未尝试";

static int rtc_handle(void)
{
  if (!rtc_open_attempted)
    {
      rtc_open_attempted = true;
      rtc_fd = open(EBADGE_RTC_DEV, O_RDWR | O_CLOEXEC);
      if (rtc_fd < 0)
        {
          snprintf(rtc_status, sizeof(rtc_status), "时钟：无法打开 %s（%d）",
                   EBADGE_RTC_DEV, errno);
        }
    }
  return rtc_fd;
}

/* Read one side. Returns false when it holds no plausible time. */
static bool read_rtc(time_t *out)
{
  int fd = rtc_handle();
  if (fd < 0) return false;

  /* struct rtc_time is documented as cast compatible with struct tm, and
   * timegm() wants a struct tm to turn it into an epoch.
   */
  struct tm when;
  memset(&when, 0, sizeof(when));
  if (ioctl(fd, RTC_RD_TIME, (unsigned long)(FAR struct rtc_time *)&when) < 0)
    {
      snprintf(rtc_status, sizeof(rtc_status), "时钟：读取 RTC 失败（%d）", errno);
      return false;
    }
  rtc_saw_ioctl = true;

  /* RTC_HAVE_SET_TIME distinguishes "the RTC was never set" from "the RTC
   * holds 1970", which matters because the second must never be copied out.
   * If the ioctl is unsupported, the year check below still protects us.
   */
  bool ever_set = true;
  if (ioctl(fd, RTC_HAVE_SET_TIME, (unsigned long)&ever_set) < 0) ever_set = true;
  if (!ever_set)
    {
      snprintf(rtc_status, sizeof(rtc_status), "时钟：RTC 从未设置");
      return false;
    }

  if (!ebadge_rtc_year_valid(when.tm_year + 1900))
    {
      snprintf(rtc_status, sizeof(rtc_status), "时钟：RTC 时间超出合理范围");
      return false;
    }

  time_t epoch = timegm(&when);
  if (epoch == (time_t)-1) return false;
  *out = epoch;
  return true;
}

static bool read_system(time_t *out)
{
  time_t now = time(NULL);
  if (now == (time_t)-1) return false;

  struct tm when;
  if (!gmtime_r(&now, &when)) return false;
  if (!ebadge_rtc_year_valid(when.tm_year + 1900)) return false;

  *out = now;
  return true;
}

static bool write_rtc(time_t epoch)
{
  int fd = rtc_handle();
  if (fd < 0) return false;

  struct tm when;
  if (!gmtime_r(&epoch, &when)) return false;

  if (ioctl(fd, RTC_SET_TIME, (unsigned long)(FAR const struct rtc_time *)&when) < 0)
    {
      snprintf(rtc_status, sizeof(rtc_status), "时钟：写入 RTC 失败（%d）", errno);
      return false;
    }
  rtc_saw_ioctl = true;
  return true;
}

static bool write_system(time_t epoch)
{
  struct timespec ts;
  ts.tv_sec = epoch;
  ts.tv_nsec = 0;
  if (clock_settime(CLOCK_REALTIME, &ts) < 0) return false;
  return true;
}

bool ebadge_rtc_store(void)
{
  time_t system_now;
  if (!read_system(&system_now))
    {
      snprintf(rtc_status, sizeof(rtc_status), "时钟：系统时间无意义，未写入 RTC");
      return false;
    }
  if (!write_rtc(system_now)) return false;
  snprintf(rtc_status, sizeof(rtc_status), "时钟：已把当前时间写入 RTC");
  return true;
}

enum ebadge_rtc_action ebadge_rtc_sync(void)
{
  time_t system_now = 0;
  time_t rtc_now = 0;
  bool system_valid = read_system(&system_now);
  bool rtc_valid = read_rtc(&rtc_now);

  int64_t skew = 0;
  if (system_valid && rtc_valid)
    skew = (int64_t)system_now - (int64_t)rtc_now;

  enum ebadge_rtc_action action =
      ebadge_rtc_decide(system_valid, rtc_valid, skew);

  switch (action)
    {
      case EBADGE_RTC_ADOPT_RTC:
        if (write_system(rtc_now))
          snprintf(rtc_status, sizeof(rtc_status), "时钟：已由 RTC 恢复");
        else
          snprintf(rtc_status, sizeof(rtc_status), "时钟：设置系统时间失败（%d）",
                   errno);
        break;

      case EBADGE_RTC_ADOPT_SYSTEM:
        if (write_rtc(system_now))
          snprintf(rtc_status, sizeof(rtc_status), "时钟：已把当前时间写入 RTC");
        break;

      case EBADGE_RTC_DO_NOTHING:
      default:
        if (system_valid && rtc_valid)
          snprintf(rtc_status, sizeof(rtc_status), "时钟：RTC 与系统一致");
        else
          snprintf(rtc_status, sizeof(rtc_status), "时钟：RTC 未设置，等待校准");
        break;
    }
  return action;
}

bool ebadge_rtc_available(void) { return rtc_saw_ioctl; }

const char *ebadge_rtc_status(void) { return rtc_status; }
