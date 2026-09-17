/* SPDX-License-Identifier: Apache-2.0 */
#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/boardctl.h>
#include <lvgl/lvgl.h>
#include "ui/ebadge_view.h"
#include "ui/ebadge_pages.h"
#ifdef CONFIG_LV_USE_NUTTX_LIBUV
#include <uv.h>
#endif

int main(int argc, char *argv[])
{
  if (!ebadge_pages_configure(argc, argv))
    {
      fprintf(stderr, "usage: ebadge [--message UTF8_TEXT] [--date YYYY-MM-DD] "
                      "[--tz-minutes -720..840]\n");
      return 1;
    }
  int status = 1;
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result = {0};
  if (lv_is_initialized())
    {
      fprintf(stderr, "ebadge: another LVGL application is already running\n");
      return 1;
    }
#if defined(CONFIG_BOARDCTL) && !defined(CONFIG_NSH_ARCHINIT)
  if (boardctl(BOARDIOC_INIT, 0) < 0) return 1;
#endif
  puts("ebadge 0.1.0; local-interaction slice; hardware acceptance pending");
  lv_init();
  lv_nuttx_dsc_init(&info);
#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif
  info.input_path = CONFIG_CONTEST2026_462_EBADGE_INPUT_PATH;
  lv_nuttx_init(&info, &result);
  if (!result.disp || !result.indev)
    {
      fprintf(stderr, "ebadge: display or touchscreen initialization failed\n");
      goto cleanup;
    }
  if (!ebadge_view_open()) goto cleanup;
#ifdef CONFIG_LV_USE_NUTTX_LIBUV
  uv_loop_t loop;
  lv_nuttx_uv_t uv_info = {0};
  if (uv_loop_init(&loop) < 0) goto cleanup;
  uv_info.loop = &loop;
  uv_info.disp = result.disp;
  uv_info.indev = result.indev;
#ifdef CONFIG_UINPUT_TOUCH
  uv_info.uindev = result.utouch_indev;
#endif
#ifdef CONFIG_LV_USE_NUTTX_MOUSE
  uv_info.mouse_indev = result.mouse_indev;
#endif
  void *uv_data = lv_nuttx_uv_init(&uv_info);
  if (uv_data)
    {
      uv_run(&loop, UV_RUN_DEFAULT);
      ebadge_view_close();
      lv_nuttx_uv_deinit(&uv_data);
      uv_run(&loop, UV_RUN_DEFAULT);
      status = 0;
    }
  if (uv_loop_close(&loop) < 0) status = 1;
#else
  for (;;)
    {
      uint32_t idle = lv_timer_handler();
      if (idle > 20) idle = 20;
      usleep((idle ? idle : 1) * 1000);
    }
#endif
cleanup:
  ebadge_view_close();
  lv_nuttx_deinit(&result);
  lv_deinit();
  return status;
}
