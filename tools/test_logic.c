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
#include "../app/hello_app/core/ebadge_power.h"
#include "../app/hello_app/core/ebadge_record.h"
#include "../app/hello_app/core/ebadge_rtc_logic.h"
#include "../app/hello_app/core/ebadge_shake.h"
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

/* The companion's voice and the palette invariants the pages rely on. Pure
 * logic, so a wrong bucket boundary shows up here instead of as the wrong
 * sentence on the panel.
 */
static void test_theme(void)
{
  static const struct { int hour; enum ebadge_daypart want; } buckets[] = {
    {0, EBADGE_DAYPART_NIGHT},      {4, EBADGE_DAYPART_NIGHT},
    {5, EBADGE_DAYPART_MORNING},    {10, EBADGE_DAYPART_MORNING},
    {11, EBADGE_DAYPART_AFTERNOON}, {16, EBADGE_DAYPART_AFTERNOON},
    {17, EBADGE_DAYPART_EVENING},   {21, EBADGE_DAYPART_EVENING},
    {22, EBADGE_DAYPART_NIGHT},     {23, EBADGE_DAYPART_NIGHT}
  };
  for (unsigned i = 0; i < sizeof(buckets) / sizeof(buckets[0]); i++)
    CHECK(ebadge_theme_daypart(buckets[i].hour) == buckets[i].want,
          "hour %d bucketed wrong", buckets[i].hour);

  /* Out-of-range hours must not index past the greeting table. */
  CHECK(ebadge_theme_daypart(-1) == EBADGE_DAYPART_MORNING,
        "negative hour not clamped");
  CHECK(ebadge_theme_daypart(24) == EBADGE_DAYPART_MORNING,
        "hour 24 not clamped");

  /* Every combination, including out-of-range indices, returns usable text. */
  for (unsigned c = 0; c <= EBADGE_CHARACTER_COUNT; c++)
    {
      CHECK(ebadge_theme_tagline(c) && ebadge_theme_tagline(c)[0],
            "tagline %u is empty", c);
      for (unsigned p = 0; p <= EBADGE_DAYPART_COUNT; p++)
        {
          const char *line = ebadge_theme_greeting(c, (enum ebadge_daypart)p);
          CHECK(line && line[0], "greeting %u/%u is empty", c, p);
        }
    }

  /* The clock has to actually change its sentence through the day, otherwise
   * the greeting is decoration rather than behaviour.
   */
  for (unsigned c = 0; c < EBADGE_CHARACTER_COUNT; c++)
    {
      const char *morning = ebadge_theme_greeting(c, EBADGE_DAYPART_MORNING);
      const char *night = ebadge_theme_greeting(c, EBADGE_DAYPART_NIGHT);
      CHECK(strcmp(morning, night) != 0,
            "character %u says the same thing day and night", c);
    }

  /* Duplicate names would make the appearance page show two identical chips. */
  for (unsigned a = 0; a < EBADGE_CHARACTER_COUNT; a++)
    for (unsigned b = a + 1; b < EBADGE_CHARACTER_COUNT; b++)
      CHECK(strcmp(ebadge_theme_character(a)->name,
                   ebadge_theme_character(b)->name) != 0,
            "characters %u and %u share a name", a, b);
  CHECK(ebadge_theme_character(EBADGE_CHARACTER_COUNT)->name ==
        ebadge_theme_character(0)->name, "character index not clamped");

  /* A paper mode replaces the surface colours but must leave the character's
   * accent alone: that is what keeps the companion recognisable.
   */
  struct ebadge_palette own = ebadge_theme_resolve(1, EBADGE_PAPER_CHARACTER);
  struct ebadge_palette plain = ebadge_theme_resolve(1, EBADGE_PAPER_PLAIN);
  struct ebadge_palette night = ebadge_theme_resolve(1, EBADGE_PAPER_NIGHT);
  CHECK(plain.paper != night.paper, "plain and night share a paper colour");
  CHECK(plain.gold == own.gold && night.gold == own.gold,
        "paper mode overrode the character accent");
  CHECK(plain.paper == ebadge_theme_resolve(2, EBADGE_PAPER_PLAIN).paper,
        "plain paper colour depends on the character");
  CHECK(night.ink != own.ink, "night mode kept the light-surface ink colour");
}

/* The idle display policy. A wrong threshold here is invisible on a bench -
 * the screen still looks right, it just never dims, or it dims mid-gesture -
 * so the boundaries are pinned down explicitly.
 */
static void test_power(void)
{
  const uint32_t dim = 15000u, off = 30000u;

  /* Boundaries: one millisecond either side of each edge. */
  struct ebadge_power p;
  ebadge_power_init(&p, 1000u, dim, off);
  CHECK(ebadge_power_update(&p, 1000u + dim - 1) == EBADGE_POWER_ACTIVE,
        "dimmed one millisecond early");
  CHECK(ebadge_power_update(&p, 1000u + dim) == EBADGE_POWER_DIMMED,
        "did not dim at the threshold");
  CHECK(ebadge_power_update(&p, 1000u + off - 1) == EBADGE_POWER_DIMMED,
        "powered off one millisecond early");
  CHECK(ebadge_power_update(&p, 1000u + off) == EBADGE_POWER_OFF,
        "did not power off at the threshold");

  /* Once off it must stay off: repeatedly re-evaluating must not walk the
   * state backwards or forwards. */
  CHECK(ebadge_power_update(&p, 1000u + off + 60000u) == EBADGE_POWER_OFF,
        "left the off state on its own");

  /* Activity returns to active, and reports whether the touch was a wake. */
  ebadge_power_init(&p, 0u, dim, off);
  CHECK(!ebadge_power_activity(&p, 5000u), "active touch reported as a wake");
  CHECK(p.state == EBADGE_POWER_ACTIVE, "active touch changed the state");
  ebadge_power_update(&p, 5000u + dim);
  CHECK(p.state == EBADGE_POWER_DIMMED, "setup: expected dimmed");
  CHECK(!ebadge_power_activity(&p, 5000u + dim + 1u),
        "dimmed touch reported as a wake");
  CHECK(p.state == EBADGE_POWER_ACTIVE, "dimmed touch did not undim");
  /* That undim touch restarted the idle count, so the off threshold is now
   * measured from it rather than from the original start.
   */
  const uint32_t resumed = 5000u + dim + 1u;
  ebadge_power_update(&p, resumed + off);
  CHECK(p.state == EBADGE_POWER_OFF, "setup: expected off");
  CHECK(ebadge_power_activity(&p, resumed + off + 1u),
        "touch on a dark panel was not reported as a wake");
  CHECK(p.state == EBADGE_POWER_ACTIVE, "wake did not restore active");

  /* Activity must restart the idle count, not merely clear the state. */
  ebadge_power_init(&p, 0u, dim, off);
  ebadge_power_activity(&p, 14000u);
  CHECK(ebadge_power_update(&p, 14000u + dim - 1) == EBADGE_POWER_ACTIVE,
        "idle timer was not reset by activity");
  CHECK(ebadge_power_update(&p, 14000u + dim) == EBADGE_POWER_DIMMED,
        "idle timer reset by the wrong amount");

  /* A zero threshold disables that stage, which is how the always-on display
   * mode works without a second code path. */
  ebadge_power_init(&p, 0u, 0u, 0u);
  CHECK(ebadge_power_update(&p, 3600000u) == EBADGE_POWER_ACTIVE,
        "zero thresholds still dimmed");
  ebadge_power_init(&p, 0u, dim, 0u);
  CHECK(ebadge_power_update(&p, dim) == EBADGE_POWER_DIMMED,
        "dim disabled by a zero off threshold");
  CHECK(ebadge_power_update(&p, 3600000u) == EBADGE_POWER_DIMMED,
        "zero off threshold still powered the panel down");

  /* Unsigned differences must survive the 32-bit tick wrap. */
  ebadge_power_init(&p, 0xfffff000u, dim, off);
  CHECK(ebadge_power_update(&p, 0xfffff000u + dim) == EBADGE_POWER_DIMMED,
        "dim threshold broken across the tick wrap");
  CHECK(ebadge_power_update(&p, 0xfffff000u + off) == EBADGE_POWER_OFF,
        "off threshold broken across the tick wrap");

  /* An off threshold below the dim threshold is clamped rather than trusted:
   * skipping L1 entirely would look like a fault. */
  ebadge_power_init(&p, 0u, off, dim);
  CHECK(p.off_after_ms == off, "off threshold below dim was not clamped");
  CHECK(ebadge_power_update(&p, dim - 1u) == EBADGE_POWER_ACTIVE,
        "clamped policy dimmed early");

  /* Rendering may only stop once panel power is actually gone. */
  CHECK(!ebadge_power_rendering_paused(EBADGE_POWER_ACTIVE),
        "active paused rendering");
  CHECK(!ebadge_power_rendering_paused(EBADGE_POWER_DIMMED),
        "dimmed paused rendering");
  CHECK(ebadge_power_rendering_paused(EBADGE_POWER_OFF),
        "off did not pause rendering");

  /* NULL must not fault; the UI calls these every frame. */
  CHECK(ebadge_power_update(NULL, 0u) == EBADGE_POWER_ACTIVE,
        "NULL update did not return active");
  CHECK(!ebadge_power_activity(NULL, 0u), "NULL activity reported a wake");
  ebadge_power_init(NULL, 0u, dim, off);
}

/* Shake detection. The failure that matters is not a crash: it is a detector
 * that fires while the badge hangs still, or that never fires at all. So the
 * tests drive sustained rest, a single shake, and repeated shaking.
 */
static void test_shake(void)
{
  /* One count is 0.488 mg at +-16 g, i.e. 4.7856 milli-m/s^2. 2049 counts is
   * the resting 1 g that the baseline is built from.
   */
  CHECK(ebadge_accel_counts_to_milli(0) == 0, "zero count not zeroed");
  CHECK(ebadge_accel_counts_to_milli(2049) == 9806,
        "1 g conversion is wrong");
  CHECK(ebadge_accel_counts_to_milli(-2049) == -9806,
        "negative conversion is wrong");
  CHECK(ebadge_accel_counts_to_milli(32767) == 156822,
        "full-scale conversion is wrong");

  /* Magnitude, axis aligned and on a 3-4-5 diagonal. */
  struct ebadge_accel a = {0, 0, 9806};
  CHECK(ebadge_accel_magnitude(&a) == 9806, "axis magnitude is wrong");
  a.x = 3000; a.y = 4000; a.z = 0;
  CHECK(ebadge_accel_magnitude(&a) == 5000, "diagonal magnitude is wrong");
  a.x = 0; a.y = 0; a.z = 0;
  CHECK(ebadge_accel_magnitude(&a) == 0, "zero magnitude is not zero");
  CHECK(ebadge_accel_magnitude(NULL) == 0, "NULL magnitude not zero");

  struct ebadge_shake s;
  ebadge_shake_init(&s);

  /* The first sample only seeds the baseline; it must never fire. */
  struct ebadge_accel rest = {0, 0, 9806};
  CHECK(!ebadge_shake_update(&s, &rest, 0u), "first sample fired");

  /* Sitting still for ten seconds must produce nothing. This is the case that
   * would ruin the product: the badge is worn, not held. */
  int fires = 0;
  for (uint32_t t = 100u; t <= 10000u; t += 100u)
    if (ebadge_shake_update(&s, &rest, t)) fires++;
  CHECK(fires == 0, "fired while the badge was at rest");

  /* A deliberate shake fires exactly once, even though the excursion lasts
   * for several samples. */
  struct ebadge_accel hard = {0, 0, 18806}; /* 9806 + 9000 deviation */
  fires = 0;
  for (uint32_t t = 10100u; t <= 10600u; t += 100u)
    if (ebadge_shake_update(&s, &hard, t)) fires++;
  CHECK(fires == 1, "one shake did not report exactly once");

  /* Continued shaking is rate limited by the cooldown rather than reported on
   * every sample. */
  fires = 0;
  for (uint32_t t = 11000u; t <= 15000u; t += 100u)
    if (ebadge_shake_update(&s, &hard, t)) fires++;
  CHECK(fires <= 3, "cooldown did not rate limit repeated shakes");

  /* A deviation below the release threshold must not re-arm, so a slow drift
   * cannot be mistaken for a shake. */
  ebadge_shake_init(&s);
  ebadge_shake_update(&s, &rest, 0u);
  struct ebadge_accel small = {0, 0, 9806 + 4000}; /* above release, below trigger */
  fires = 0;
  for (uint32_t t = 100u; t <= 3000u; t += 100u)
    if (ebadge_shake_update(&s, &small, t)) fires++;
  CHECK(fires == 0, "a sub-threshold wobble fired");

  /* Back to rest must re-arm, so a second real shake is still reported. */
  for (uint32_t t = 3100u; t <= 4000u; t += 100u)
    ebadge_shake_update(&s, &rest, t);
  CHECK(ebadge_shake_update(&s, &hard, 4100u), "did not re-arm after rest");

  /* Slow thermal or orientation drift must not fire: the baseline follows it. */
  ebadge_shake_init(&s);
  ebadge_shake_update(&s, &rest, 0u);
  fires = 0;
  for (uint32_t i = 1; i <= 200; i++)
    {
      struct ebadge_accel drift = {0, 0, 9806 + (int32_t)i * 5};
      if (ebadge_shake_update(&s, &drift, i * 50u)) fires++;
    }
  CHECK(fires == 0, "slow drift fired");

  /* Free fall is a big deviation in the other direction and must fire. */
  ebadge_shake_init(&s);
  ebadge_shake_update(&s, &rest, 0u);
  struct ebadge_accel falling = {0, 0, 0};
  CHECK(ebadge_shake_update(&s, &falling, 100u), "free fall did not fire");

  /* A shake made right after power-on must not be swallowed by the cooldown:
   * last_trigger_ms starts at 0, and 0 is also a valid tick. */
  ebadge_shake_init(&s);
  ebadge_shake_update(&s, &rest, 0u);
  CHECK(ebadge_shake_update(&s, &hard, 50u),
        "a shake just after boot was swallowed by the cooldown");

  /* NULL safety: this is fed from a device read that can fail. */
  CHECK(!ebadge_shake_update(NULL, &rest, 0u), "NULL detector fired");
  CHECK(!ebadge_shake_update(&s, NULL, 0u), "NULL sample fired");
  ebadge_shake_init(NULL);
}

/* RTC reconciliation. The bug that matters is directional: copying an unset
 * system clock (1970) into a good RTC destroys the only correct copy of the
 * time, and nothing looks wrong until the next boot.
 */
static void test_rtc(void)
{
  /* What counts as a plausible year, matching the clock page's own rule. */
  CHECK(!ebadge_rtc_year_valid(1970), "1970 was treated as valid");
  CHECK(!ebadge_rtc_year_valid(2023), "2023 was treated as valid");
  CHECK(ebadge_rtc_year_valid(2024), "2024 was treated as invalid");
  CHECK(ebadge_rtc_year_valid(2026), "2026 was treated as invalid");
  CHECK(ebadge_rtc_year_valid(2099), "2099 was treated as invalid");
  CHECK(!ebadge_rtc_year_valid(2100), "2100 was treated as valid");

  /* Neither side knows the time: never invent one. */
  CHECK(ebadge_rtc_decide(false, false, 0) == EBADGE_RTC_DO_NOTHING,
        "invented a time from nothing");
  CHECK(ebadge_rtc_decide(false, false, 999999) == EBADGE_RTC_DO_NOTHING,
        "invented a time from nothing, ignoring the skew");

  /* The boot case: the system clock is at 1970 and the RTC holds the time set
   * last time the badge was used. */
  CHECK(ebadge_rtc_decide(false, true, 0) == EBADGE_RTC_ADOPT_RTC,
        "did not restore the clock from the RTC");

  /* The catastrophic direction: an unset system clock must never be written
   * into a good RTC, whatever the skew says. */
  CHECK(ebadge_rtc_decide(false, true, -1700000000) == EBADGE_RTC_ADOPT_RTC,
        "an unset system clock overwrote a valid RTC");
  CHECK(ebadge_rtc_decide(false, true, 1700000000) == EBADGE_RTC_ADOPT_RTC,
        "an unset system clock overwrote a valid RTC");

  /* The host set a time and the RTC is unset: persist it. */
  CHECK(ebadge_rtc_decide(true, false, 0) == EBADGE_RTC_ADOPT_SYSTEM,
        "did not persist a host-set time");

  /* Both sides agree: leave the RTC alone rather than rewriting it. */
  CHECK(ebadge_rtc_decide(true, true, 0) == EBADGE_RTC_DO_NOTHING,
        "rewrote an RTC that already agreed");

  /* Small differences are drift and must not cause a write. */
  CHECK(ebadge_rtc_decide(true, true, 30) == EBADGE_RTC_DO_NOTHING,
        "drift caused a rewrite (positive)");
  CHECK(ebadge_rtc_decide(true, true, -30) == EBADGE_RTC_DO_NOTHING,
        "drift caused a rewrite (negative)");
  CHECK(ebadge_rtc_decide(true, true, EBADGE_RTC_SKEW_TOLERANCE_S) ==
        EBADGE_RTC_DO_NOTHING, "the tolerance boundary caused a rewrite");

  /* A deliberate correction is adopted in either direction. */
  CHECK(ebadge_rtc_decide(true, true, EBADGE_RTC_SKEW_TOLERANCE_S + 1) ==
        EBADGE_RTC_ADOPT_SYSTEM, "a correction was not adopted");
  CHECK(ebadge_rtc_decide(true, true, -(EBADGE_RTC_SKEW_TOLERANCE_S + 1)) ==
        EBADGE_RTC_ADOPT_SYSTEM, "a correction was not adopted (negative)");
}

int main(void)
{
  printf("ebadge logic tests\n");
  test_record();
  test_calendar();
  test_theme();
  test_power();
  test_shake();
  test_rtc();
  printf("\n%d checks, %d failures\n", checks, failures);
  if (failures)
    {
      printf("RESULT: FAIL\n");
      return 1;
    }
  printf("RESULT: PASS\n");
  return 0;
}
