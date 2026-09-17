/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_record.h"
#include "ebadge_calendar.h"
#include "../ui/ebadge_theme.h"
#include <string.h>

#define RECORD_MESSAGE_OFFSET 8
#define RECORD_YEAR_OFFSET 168
#define RECORD_MONTH_OFFSET 170
#define RECORD_DAY_OFFSET 171
#define RECORD_ZONE_OFFSET 172
#define RECORD_CHECKSUM_OFFSET 174

static const char record_magic[4] = { 'E', 'B', 'C', '1' };

static void put16(uint8_t *out, uint16_t value)
{
  out[0] = (uint8_t)(value & 0xff);
  out[1] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *in)
{
  return (uint16_t)(in[0] | (in[1] << 8));
}

static uint32_t checksum(const uint8_t *record)
{
  uint32_t sum = 0;
  for (int i = 0; i < RECORD_CHECKSUM_OFFSET; i++) sum += record[i];
  return sum;
}

/* strnlen is POSIX 2008 and is not declared under a strict C11 host build, so
 * the bounded length is computed here instead of depending on a feature-test
 * macro. Returns `limit` when no terminator is present.
 */
static size_t bounded_length(const char *text, size_t limit)
{
  size_t length = 0;
  while (length < limit && text[length] != '\0') length++;
  return length;
}

bool ebadge_record_encode(const struct ebadge_settings *in, uint8_t *out)
{
  if (!in || !out) return false;
  if (in->character >= EBADGE_CHARACTER_COUNT) return false;
  if (in->paper >= EBADGE_PAPER_COUNT) return false;
  /* An unset anniversary must be stored as a fully zero date, and a set one
   * must be a real calendar date.
   */
  if (in->year == 0)
    {
      if (in->month != 0 || in->day != 0) return false;
    }
  else if (!ebadge_date_valid(in->year, in->month, in->day))
    {
      return false;
    }

  memset(out, 0, EBADGE_RECORD_SIZE);
  memcpy(out, record_magic, sizeof(record_magic));
  put16(out + 4, EBADGE_RECORD_VERSION);
  out[6] = in->character;
  out[7] = in->paper;

  size_t length = bounded_length(in->message, EBADGE_STORE_MESSAGE);
  if (length >= EBADGE_STORE_MESSAGE) return false;
  memcpy(out + RECORD_MESSAGE_OFFSET, in->message, length);

  put16(out + RECORD_YEAR_OFFSET, (uint16_t)in->year);
  out[RECORD_MONTH_OFFSET] = (uint8_t)in->month;
  out[RECORD_DAY_OFFSET] = (uint8_t)in->day;
  put16(out + RECORD_ZONE_OFFSET, (uint16_t)in->zone);

  uint32_t sum = checksum(out);
  for (int i = 0; i < 4; i++)
    out[RECORD_CHECKSUM_OFFSET + i] = (uint8_t)((sum >> (8 * i)) & 0xff);
  return true;
}

bool ebadge_record_decode(const uint8_t *in, struct ebadge_settings *out)
{
  if (!in || !out) return false;
  if (memcmp(in, record_magic, sizeof(record_magic))) return false;
  if (get16(in + 4) != EBADGE_RECORD_VERSION) return false;

  uint32_t stored = 0;
  for (int i = 0; i < 4; i++)
    stored |= (uint32_t)in[RECORD_CHECKSUM_OFFSET + i] << (8 * i);
  if (stored != checksum(in)) return false;

  if (in[6] >= EBADGE_CHARACTER_COUNT || in[7] >= EBADGE_PAPER_COUNT)
    return false;

  /* The message must be NUL-terminated inside its own field, otherwise a
   * truncated file would read past the buffer.
   */
  const uint8_t *message = in + RECORD_MESSAGE_OFFSET;
  bool terminated = false;
  for (int i = 0; i < EBADGE_STORE_MESSAGE; i++)
    if (message[i] == '\0') { terminated = true; break; }
  if (!terminated) return false;

  int year = (int)get16(in + RECORD_YEAR_OFFSET);
  int month = in[RECORD_MONTH_OFFSET];
  int day = in[RECORD_DAY_OFFSET];
  int zone = (int)(int16_t)get16(in + RECORD_ZONE_OFFSET);
  if (zone < -720 || zone > 840) return false;
  if (year == 0)
    {
      if (month != 0 || day != 0) return false;
    }
  else if (!ebadge_date_valid(year, month, day))
    {
      return false;
    }

  struct ebadge_settings value;
  memset(&value, 0, sizeof(value));
  value.character = in[6];
  value.paper = in[7];
  memcpy(value.message, message, EBADGE_STORE_MESSAGE);
  value.year = (int16_t)year;
  value.month = (int8_t)month;
  value.day = (int8_t)day;
  value.zone = (int16_t)zone;
  *out = value;
  return true;
}
