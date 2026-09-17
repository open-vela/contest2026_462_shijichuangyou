/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_calendar.h"

static const int month_length[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

static bool leap(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

int ebadge_month_days(int year, int month)
{
  if (year < EBADGE_YEAR_MIN || year > EBADGE_YEAR_MAX) return 0;
  if (month < 1 || month > 12) return 0;
  return month_length[month - 1] + (month == 2 && leap(year));
}

bool ebadge_date_valid(int year, int month, int day)
{
  int length = ebadge_month_days(year, month);
  return length > 0 && day >= 1 && day <= length;
}

long ebadge_ordinal(int year, int month, int day)
{
  if (!ebadge_date_valid(year, month, day)) return 0;
  long previous = year - 1;
  long value = previous * 365 + previous / 4 - previous / 100 + previous / 400;
  for (int i = 1; i < month; i++) value += month_length[i - 1];
  if (month > 2 && leap(year)) value += 1;
  return value + day;
}

long ebadge_days_between(int y0, int m0, int d0, int y1, int m1, int d1)
{
  long from = ebadge_ordinal(y0, m0, d0);
  long to = ebadge_ordinal(y1, m1, d1);
  if (!from || !to) return 0;
  return to - from;
}

void ebadge_date_clamp(int *year, int *month, int *day)
{
  if (!year || !month || !day) return;
  if (*month < 1) *month = 1;
  if (*month > 12) *month = 12;
  int length = ebadge_month_days(*year, *month);
  if (length <= 0) return;
  if (*day < 1) *day = 1;
  if (*day > length) *day = length;
}
