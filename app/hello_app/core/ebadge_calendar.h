/* SPDX-License-Identifier: Apache-2.0 */
/* Calendar arithmetic for the anniversary page.
 *
 * Deliberately independent of libc time and of LVGL: the day count must not
 * change with the host timezone, and it must be testable on the build machine
 * (see tools/test_logic.c). The supported range matches the date editor.
 */
#ifndef EBADGE_CALENDAR_H
#define EBADGE_CALENDAR_H
#include <stdbool.h>

#define EBADGE_YEAR_MIN 2024
#define EBADGE_YEAR_MAX 2099

/* Days in the month, or 0 when year or month is out of range. */
int ebadge_month_days(int year, int month);

/* True only for a real calendar date inside the supported year range. */
bool ebadge_date_valid(int year, int month, int day);

/* Gregorian day number, so that differences do not depend on libc. Returns 0
 * for an invalid date; callers validate first.
 */
long ebadge_ordinal(int year, int month, int day);

/* Signed day count from the first date to the second: positive when the second
 * date is later.
 *
 * IMPORTANT: returns 0 both for "the same day" and for "either date is
 * invalid or out of range", so a caller that cannot rule out bad input must
 * validate first - otherwise an unset clock would render as "today".
 * paint_anniversary checks the range before calling this.
 */
long ebadge_days_between(int y0, int m0, int d0, int y1, int m1, int d1);

/* Reduce *day to the last valid day of *month, in place. */
void ebadge_date_clamp(int *year, int *month, int *day);

#endif
