/* SPDX-License-Identifier: Apache-2.0 */
/* RTC adapter: makes the clock survive a reboot.
 *
 * Sources, not guesses:
 *
 *   vendor/sifli/boards/sf32lb52/lckfb_huangshan_pi/src/sifli_ap.c
 *       rtc_initialize(0, sf32lb_rtc_lowerhalf()), guarded by
 *       CONFIG_RTC && CONFIG_RTC_DRIVER (both set on this board)
 *   drivers/timers/rtc.c   registers "/dev/rtc%d"
 *   include/nuttx/timers/rtc.h   RTC_RD_TIME / RTC_SET_TIME / RTC_HAVE_SET_TIME,
 *       and struct rtc_time is documented cast-compatible with struct tm
 *
 * Without this the clock is only correct if a host ran `date` over the serial
 * console that session, and the anniversary page falls back to "time not
 * calibrated" on every cold start.
 *
 * The direction of any copy is decided by ebadge_rtc_decide(), which is pure
 * and host-tested; this file only performs the I/O.
 */
#ifndef EBADGE_RTC_H
#define EBADGE_RTC_H
#include <stdbool.h>

#include "ebadge_rtc_logic.h"

/* Reconcile the system clock and the RTC once, early at startup. Returns what
 * was done, so the caller can log it. Never fails destructively: on any error
 * the system clock is simply left as it was.
 */
enum ebadge_rtc_action ebadge_rtc_sync(void);

/* Copy the current system clock into the RTC, unless the system clock is not
 * plausible. False when nothing was written.
 */
bool ebadge_rtc_store(void);

/* True once an RTC ioctl has actually succeeded. */
bool ebadge_rtc_available(void);

/* One-line status for logs and diagnostics. Never NULL. */
const char *ebadge_rtc_status(void);

#endif
