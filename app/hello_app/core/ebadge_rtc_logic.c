/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_rtc_logic.h"

bool ebadge_rtc_year_valid(int year)
{
  return year >= EBADGE_RTC_YEAR_MIN && year <= EBADGE_RTC_YEAR_MAX;
}

enum ebadge_rtc_action ebadge_rtc_decide(bool system_valid, bool rtc_valid,
                                         int64_t skew_seconds)
{
  /* Neither side has a real time. There is nothing to copy, and guessing
   * would be worse than staying unset.
   */
  if (!system_valid && !rtc_valid) return EBADGE_RTC_DO_NOTHING;

  /* One side is good: copy it to the side that is not. This is the case that
   * matters at boot, where the system clock starts at 1970 and the RTC holds
   * the time that was set the last time the badge was used.
   */
  if (system_valid && !rtc_valid) return EBADGE_RTC_ADOPT_SYSTEM;
  if (!system_valid && rtc_valid) return EBADGE_RTC_ADOPT_RTC;

  /* Both are plausible. A large difference means the host deliberately set a
   * new time, so the RTC should follow it; a small one is drift and is left
   * alone, because rewriting the RTC constantly would be pointless bus
   * traffic.
   */
  int64_t skew = skew_seconds < 0 ? -skew_seconds : skew_seconds;
  if (skew > EBADGE_RTC_SKEW_TOLERANCE_S) return EBADGE_RTC_ADOPT_SYSTEM;
  return EBADGE_RTC_DO_NOTHING;
}
