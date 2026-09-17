/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_display.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <nuttx/lcd/lcd.h>
#include <nuttx/lcd/lcd_dev.h>

#define EBADGE_LCD_DEV "/dev/lcd0"

static int display_fd = -1;
static bool open_attempted;
static bool ioctl_succeeded;
static char status[96] = "屏幕电源：尚未尝试";

static int display_handle(void)
{
  if (!open_attempted)
    {
      open_attempted = true;
      display_fd = open(EBADGE_LCD_DEV, O_RDWR | O_CLOEXEC);
      if (display_fd < 0)
        {
          snprintf(status, sizeof(status), "屏幕电源：无法打开 %s（%d）",
                   EBADGE_LCD_DEV, errno);
        }
    }
  return display_fd;
}

static bool set_power(int power)
{
  int fd = display_handle();
  if (fd < 0) return false;
  if (ioctl(fd, LCDDEVIO_SETPOWER, (unsigned long)power) < 0)
    {
      snprintf(status, sizeof(status), "屏幕电源：%s 调用失败（%d）",
               EBADGE_LCD_DEV, errno);
      return false;
    }
  ioctl_succeeded = true;
  snprintf(status, sizeof(status), "屏幕电源：%s 可控（L2 可用）",
           EBADGE_LCD_DEV);
  return true;
}

bool ebadge_display_off(void) { return set_power(0); }
bool ebadge_display_on(void) { return set_power(LCD_FULL_ON); }

bool ebadge_display_verified(void) { return ioctl_succeeded; }

const char *ebadge_display_status(void) { return status; }
