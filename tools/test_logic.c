/* SPDX-License-Identifier: Apache-2.0
 *
 * Host-side tests for the pure logic that the badge firmware relies on:
 * the on-card settings record codec and the anniversary calendar maths.
 *
 * These are the two places where a wrong answer is silent on the panel - a
 * corrupt record would just look like "no settings", and an off-by-one day
 * count would just look like a different number. Both are pure functions, so
 * they can be pinned down on the build machine before flashing.
 *
 * Build and run: tools/run_logic_test.sh
 */
#include <stdio.h>
#include <string.h>
#include "../app/hello_app/core/ebadge_calendar.h"
#include "../app/hello_app/core/ebadge_record.h"
#include "../app/hello_app/core/ebadge_store.h"
#include "../app/hello_app/ui/ebadge_theme.h"

static int failures;
static int checks;

#define CHECK(condition, ...)                                                  \
  do                                                                           \
    {                                                                          \
      checks++;                                                                \
      if (!(condition))                                                        \
        {                                                                      \
          failures++;                                                          \
          printf("FAIL %s:%d: ", __FILE__, __LINE__);                          \
          printf(__VA_ARGS__);                                                 \
          printf("\n");                                                        \
        }                                                                      \
    }                                                                          \
  while (0)

static struct ebadge_settings sample(void)
{
  struct ebadge_settings in;
  memset(&in, 0, sizeof(in));
  in.character = 1;
  in.paper = 2;
  strcpy(in.message, "与你分享，今天的暖阳。");
  in.year = 2026;
  in.month = 9;
  in.day = 17;
  in.zone = 480;
  return in;
}

static bool same(const struct ebadge_settings *a, const struct ebadge_settings *b)
{
  return a->character == b->character && a->paper == b->paper &&
         a->year == b->year && a->month == b->month && a->day == b->day &&
         a->zone == b->zone && !strcmp(a->message, b->message);
}

/* Recompute the checksum the way the format defines it, so the tests can craft
 * records that are internally consistent but semantically invalid.
 */
static void reseal(uint8_t *record)
{
  uint32_t sum = 0;
  for (int i = 0; i < 174; i++) sum += record[i];
  for (int i = 0; i < 4; i++)
    record[174 + i] = (uint8_t)((sum >> (8 * i)) & 0xff);
}

static void test_record(void)
{
  printf("-- settings record\n");
  printf("   record size %d\n", EBADGE_RECORD_SIZE);
  CHECK(EBADGE_RECORD_SIZE == 178, "record must stay 178 bytes");
  CHECK(EBADGE_STORE_MESSAGE == 160, "message field must stay 160 bytes");
  CHECK(EBADGE_CHARACTER_COUNT == 3, "character count changed; update tests");
  CHECK(EBADGE_PAPER_COUNT == 3, "paper count changed; update tests");

  uint8_t record[EBADGE_RECORD_SIZE];
  struct ebadge_settings in = sample();
  struct ebadge_settings out;

  /* Round trip, including every boundary field. */
  CHECK(ebadge_record_encode(&in, record), "encode of a valid record failed");
  CHECK(ebadge_record_decode(record, &out), "decode of a valid record failed");
  CHECK(same(&in, &out), "round trip changed a field");

  /* Field offsets are part of the on-card format. */
  CHECK(!memcmp(record, "EBC1", 4), "magic moved");
  CHECK(record[4] == 1 && record[5] == 0, "version moved");
  CHECK(record[6] == 1, "character offset moved");
  CHECK(record[7] == 2, "paper offset moved");
  CHECK(!memcmp(record + 8, in.message, strlen(in.message)), "message offset moved");
  CHECK(record[8 + strlen(in.message)] == 0, "message not NUL padded");
  CHECK(record[168] == 0xea && record[169] == 0x07, "year offset moved");
  CHECK(record[170] == 9, "month offset moved");
  CHECK(record[171] == 17, "day offset moved");
  CHECK(record[172] == 0xe0 && record[173] == 0x01, "zone offset moved");

  /* Encoding is deterministic. */
  uint8_t again[EBADGE_RECORD_SIZE];
  CHECK(ebadge_record_encode(&in, again), "second encode failed");
  CHECK(!memcmp(record, again, sizeof(record)), "encode is not deterministic");

  /* Empty and maximum-length messages. */
  struct ebadge_settings edge = sample();
  edge.message[0] = '\0';
  CHECK(ebadge_record_encode(&edge, record), "empty message rejected");
  CHECK(ebadge_record_decode(record, &out) && out.message[0] == '\0',
        "empty message did not round trip");

  edge = sample();
  memset(edge.message, 'x', EBADGE_STORE_MESSAGE - 1);
  edge.message[EBADGE_STORE_MESSAGE - 1] = '\0';
  CHECK(ebadge_record_encode(&edge, record), "159-char message rejected");
  CHECK(ebadge_record_decode(record, &out), "159-char message did not decode");
  CHECK(strlen(out.message) == EBADGE_STORE_MESSAGE - 1, "159-char message truncated");

  /* A full field leaves no room for the terminator, so it must be refused. */
  edge = sample();
  memset(edge.message, 'x', EBADGE_STORE_MESSAGE);
  CHECK(!ebadge_record_encode(&edge, record), "160-char message accepted");

  /* Every single-bit error in the covered range must be caught. */
  int undetected = 0;
  for (int byte = 0; byte < 174; byte++)
    for (int bit = 0; bit < 8; bit++)
      {
        CHECK(ebadge_record_encode(&in, record), "encode failed mid-sweep");
        record[byte] ^= (uint8_t)(1u << bit);
        if (ebadge_record_decode(record, &out)) undetected++;
      }
  CHECK(undetected == 0, "%d single-bit corruptions went undetected", undetected);

  /* The checksum itself is covered. */
  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[174] ^= 0x01;
  CHECK(!ebadge_record_decode(record, &out), "checksum byte not verified");

  /* Structural rejections. */
  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[0] = 'X';
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "bad magic accepted");

  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[4] = 2;
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "unknown version accepted");

  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[6] = EBADGE_CHARACTER_COUNT;
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "out-of-range character accepted");

  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[7] = EBADGE_PAPER_COUNT;
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "out-of-range paper accepted");

  /* A message field with no NUL would read past its own bounds. */
  CHECK(ebadge_record_encode(&in, record), "encode failed");
  memset(record + 8, 'x', EBADGE_STORE_MESSAGE);
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "unterminated message accepted");

  /* Impossible dates must be rejected, not silently wrapped. 2026-02-30 and
   * 2026-04-31 are well-formed bytes but not real days.
   */
  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[170] = 2; record[171] = 30;
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "2026-02-30 accepted");
  record[170] = 4; record[171] = 31;
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "2026-04-31 accepted");
  /* 2024 is a leap year, so 02-29 is real. */
  record[168] = 0xe8; record[169] = 0x07; record[170] = 2; record[171] = 29;
  reseal(record);
  CHECK(ebadge_record_decode(record, &out), "2024-02-29 rejected");
  CHECK(out.year == 2024 && out.month == 2 && out.day == 29, "leap date mangled");

  /* An unset anniversary is a fully zero date. */
  struct ebadge_settings unset = sample();
  unset.year = 0; unset.month = 0; unset.day = 0;
  CHECK(ebadge_record_encode(&unset, record), "unset date rejected");
  CHECK(ebadge_record_decode(record, &out), "unset date did not decode");
  CHECK(out.year == 0 && out.month == 0 && out.day == 0, "unset date mangled");

  unset.month = 1;
  CHECK(!ebadge_record_encode(&unset, record), "year=0 with month set accepted");

  /* Out-of-range years and zones. */
  struct ebadge_settings bad = sample();
  bad.year = 2100;
  CHECK(!ebadge_record_encode(&bad, record), "year 2100 accepted");
  bad = sample();
  bad.year = 2023;
  CHECK(!ebadge_record_encode(&bad, record), "year 2023 accepted");
  bad = sample();
  bad.character = EBADGE_CHARACTER_COUNT;
  CHECK(!ebadge_record_encode(&bad, record), "out-of-range character encoded");
  bad = sample();
  bad.paper = EBADGE_PAPER_COUNT;
  CHECK(!ebadge_record_encode(&bad, record), "out-of-range paper encoded");

  CHECK(ebadge_record_encode(&in, record), "encode failed");
  record[172] = 0x51; record[173] = 0x03; /* 849 minutes */
  reseal(record);
  CHECK(!ebadge_record_decode(record, &out), "zone 849 accepted");
  /* Negative zones are stored as two's complement and must survive. */
  struct ebadge_settings west = sample();
  west.zone = -300;
  CHECK(ebadge_record_encode(&west, record), "negative zone rejected");
  CHECK(ebadge_record_decode(record, &out), "negative zone did not decode");
  CHECK(out.zone == -300, "negative zone mangled: got %d", out.zone);

  /* NULL safety. */
  CHECK(!ebadge_record_encode(NULL, record), "NULL input accepted");
  CHECK(!ebadge_record_decode(record, NULL), "NULL output accepted");
}

static void test_calendar(void)
{
  printf("-- calendar\n");

  /* Month lengths, common and leap year. */
  static const int common[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
  static const int leap_year[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
  for (int m = 1; m <= 12; m++)
    {
      CHECK(ebadge_month_days(2025, m) == common[m - 1],
            "2025-%02d length wrong: %d", m, ebadge_month_days(2025, m));
      CHECK(ebadge_month_days(2024, m) == leap_year[m - 1],
            "2024-%02d length wrong: %d", m, ebadge_month_days(2024, m));
    }
  CHECK(ebadge_month_days(2025, 0) == 0, "month 0 accepted");
  CHECK(ebadge_month_days(2025, 13) == 0, "month 13 accepted");

  /* Validity. */
  CHECK(ebadge_date_valid(2024, 2, 29), "2024-02-29 should be valid");
  CHECK(!ebadge_date_valid(2025, 2, 29), "2025-02-29 should be invalid");
  CHECK(!ebadge_date_valid(2024, 2, 30), "2024-02-30 should be invalid");
  CHECK(!ebadge_date_valid(2025, 4, 31), "2025-04-31 should be invalid");
  CHECK(ebadge_date_valid(2025, 4, 30), "2025-04-30 should be valid");
  CHECK(!ebadge_date_valid(2025, 13, 1), "month 13 should be invalid");
  CHECK(!ebadge_date_valid(2025, 0, 1), "month 0 should be invalid");
  CHECK(!ebadge_date_valid(2025, 1, 0), "day 0 should be invalid");
  CHECK(!ebadge_date_valid(2023, 12, 31), "year below range should be invalid");
  CHECK(!ebadge_date_valid(2100, 1, 1), "year above range should be invalid");
  CHECK(ebadge_date_valid(2099, 12, 31), "2099-12-31 should be valid");

  /* Known differences, including the leap-day boundary. */
  CHECK(ebadge_days_between(2026, 1, 1, 2026, 1, 2) == 1, "one day apart");
  CHECK(ebadge_days_between(2026, 1, 1, 2026, 1, 1) == 0, "same day");
  CHECK(ebadge_days_between(2026, 1, 2, 2026, 1, 1) == -1, "reverse is negative");
  CHECK(ebadge_days_between(2026, 2, 28, 2026, 3, 1) == 1,
        "non-leap February boundary");
  CHECK(ebadge_days_between(2024, 2, 28, 2024, 3, 1) == 2,
        "leap February boundary: got %ld",
        ebadge_days_between(2024, 2, 28, 2024, 3, 1));
  CHECK(ebadge_days_between(2024, 1, 1, 2025, 1, 1) == 366, "2024 is a leap year");
  CHECK(ebadge_days_between(2025, 1, 1, 2026, 1, 1) == 365, "2025 is not");
  CHECK(ebadge_days_between(2024, 1, 1, 2026, 1, 1) == 731, "two years across a leap day");
  CHECK(ebadge_days_between(2026, 9, 17, 2026, 9, 20) == 3, "deadline countdown");
  CHECK(ebadge_days_between(2026, 9, 20, 2026, 9, 17) == -3, "past deadline");

  /* Invalid input must not produce a plausible number. */
  CHECK(ebadge_days_between(2026, 2, 30, 2026, 3, 1) == 0,
        "invalid start date returned a result");

  /* A birthday recurring yearly: the countdown must be 365 or 366 apart. */
  long a = ebadge_days_between(2026, 5, 4, 2027, 5, 4);
  long b = ebadge_days_between(2027, 5, 4, 2028, 5, 4);
  CHECK(a == 365, "2026->2027 birthday gap: %ld", a);
  CHECK(b == 366, "2027->2028 birthday gap: %ld", b);

  /* Clamping. */
  int y = 2024, m = 2, d = 31;
  ebadge_date_clamp(&y, &m, &d);
  CHECK(y == 2024 && m == 2 && d == 29, "clamp to leap February gave %d-%d-%d", y, m, d);
  y = 2026; m = 2; d = 31;
  ebadge_date_clamp(&y, &m, &d);
  CHECK(d == 28, "clamp to common February gave %d", d);
  y = 2026; m = 4; d = 31;
  ebadge_date_clamp(&y, &m, &d);
  CHECK(d == 30, "clamp to April gave %d", d);
  y = 2026; m = 1; d = 0;
  ebadge_date_clamp(&y, &m, &d);
  CHECK(d == 1, "clamp of day 0 gave %d", d);

  /* Whole-range consistency: the ordinal difference between 2024-01-01 and
   * 2099-12-31 must equal a day count accumulated from the month lengths.
   * This is what pins down the leap rule across all 76 supported years.
   */
  long counted = 0;
  for (int year = EBADGE_YEAR_MIN; year <= EBADGE_YEAR_MAX; year++)
    for (int month = 1; month <= 12; month++)
      counted += ebadge_month_days(year, month);
  long spanned = ebadge_days_between(EBADGE_YEAR_MIN, 1, 1,
                                     EBADGE_YEAR_MAX, 12, 31) + 1;
  CHECK(counted == spanned,
        "ordinal span %ld disagrees with accumulated month lengths %ld",
        spanned, counted);

  /* 0 is ambiguous: it means both "same day" and "invalid input", so callers
   * must validate before relying on the result. paint_anniversary checks the
   * current date is inside the supported range first.
   */
  CHECK(ebadge_days_between(2023, 12, 1, 2026, 1, 1) == 0,
        "out-of-range start should report 0, not a day count");
  CHECK(ebadge_days_between(2026, 1, 1, 2100, 1, 1) == 0,
        "out-of-range end should report 0, not a day count");
  CHECK(ebadge_ordinal(2023, 12, 1) == 0, "out-of-range ordinal should be 0");

  /* And the ordinal must advance by exactly one across every day from the
   * first supported date, which is where an off-by-one would hide.
   */
  long previous = ebadge_ordinal(EBADGE_YEAR_MIN, 1, 1);
  CHECK(previous != 0, "ordinal of the first supported day is 0");
  int gaps = 0;
  int sequenced = 0;
  for (int year = 2024; year <= 2025; year++)
    for (int month = 1; month <= 12; month++)
      for (int day = 1; day <= ebadge_month_days(year, month); day++)
        {
          if (year == 2024 && month == 1 && day == 1) continue;
          long current = ebadge_ordinal(year, month, day);
          if (current - previous != 1) gaps++;
          previous = current;
          sequenced++;
        }
  /* 2024 is a leap year: 366 + 365 = 731 days, minus the skipped first day. */
  CHECK(sequenced == 730, "expected 730 walked days, got %d", sequenced);
  CHECK(gaps == 0, "%d discontinuities in the ordinal sequence", gaps);
}

int main(void)
{
  printf("ebadge logic tests\n");
  test_record();
  test_calendar();
  printf("\n%d checks, %d failures\n", checks, failures);
  if (failures)
    {
      printf("RESULT: FAIL\n");
      return 1;
    }
  printf("RESULT: PASS\n");
  return 0;
}
