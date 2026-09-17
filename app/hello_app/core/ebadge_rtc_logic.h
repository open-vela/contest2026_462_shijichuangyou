/* SPDX-License-Identifier: Apache-2.0 */
/* RTC reconciliation policy.
 *
 * The dangerous mistake here is directional: writing an unset system clock
 * (1970) into a perfectly good RTC destroys the only copy of the correct
 * time, and it looks like nothing happened until the next boot. So the
 * decision is separated from the device and tested on its own.
 */
#ifndef EBADGE_RTC_LOGIC_H
#define EBADGE_RTC_LOGIC_H
#include <stdbool.h>
#include <stdint.h>

/* A time is only worth keeping if its year is plausible. Matches the range the
 * clock page already uses to decide whether it can show a real date, so the
 * two cannot disagree about what "calibrated" means.
 */
#define EBADGE_RTC_YEAR_MIN 2024
#define EBADGE_RTC_YEAR_MAX 2099

/* How far the system clock may drift from the RTC before the difference is
 * treated as a deliberate correction by the host rather than as drift. */
#define EBADGE_RTC_SKEW_TOLERANCE_S 60

enum ebadge_rtc_action
{
  EBADGE_RTC_DO_NOTHING = 0,
  EBADGE_RTC_ADOPT_SYSTEM, /* system clock -> RTC */
  EBADGE_RTC_ADOPT_RTC,    /* RTC -> system clock */
  EBADGE_RTC_ACTION_COUNT
};

bool ebadge_rtc_year_valid(int year);

/* Decide what to reconcile.
 *
 * system_valid / rtc_valid - whether each side holds a plausible time.
 * skew_seconds             - system minus RTC, ignored unless both are valid.
 *
 * Never invents a time, and never overwrites a valid one with an invalid one.
 */
enum ebadge_rtc_action ebadge_rtc_decide(bool system_valid, bool rtc_valid,
                                         int64_t skew_seconds);

#endif
