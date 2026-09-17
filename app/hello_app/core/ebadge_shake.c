/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_shake.h"

/* Integer square root of a 64-bit value, by binary search.
 *
 * The acceleration vectors overflow 32 bits once squared (+-16 g is
 * +-157000 milli-m/s^2, whose square is 2.5e10), so the sum is accumulated in
 * 64 bits and rooted here rather than pulling in floating point.
 */
static int64_t isqrt64(uint64_t value)
{
  uint64_t low = 0;
  uint64_t high = 1;

  /* Find an upper bound by doubling; value is bounded by 3 * (157000)^2, so
   * this settles in a handful of steps.
   */
  while (high < value / high) high <<= 1;

  while (low + 1 < high)
    {
      uint64_t mid = low + (high - low) / 2;
      if (mid <= value / mid)
        low = mid;
      else
        high = mid;
    }
  return (int64_t)low;
}

int32_t ebadge_accel_magnitude(const struct ebadge_accel *sample)
{
  if (!sample) return 0;
  int64_t x = sample->x;
  int64_t y = sample->y;
  int64_t z = sample->z;
  uint64_t sum = (uint64_t)(x * x) + (uint64_t)(y * y) + (uint64_t)(z * z);
  return (int32_t)isqrt64(sum);
}

int32_t ebadge_accel_counts_to_milli(int16_t count)
{
  return ((int32_t)count * EBADGE_ACCEL_MILLI_NUM) / EBADGE_ACCEL_MILLI_DEN;
}

void ebadge_shake_init(struct ebadge_shake *shake)
{
  if (!shake) return;
  shake->baseline_milli = 0;
  shake->last_trigger_ms = 0;
  shake->baseline_valid = false;
  shake->armed = true;
  shake->triggered_before = false;
}

bool ebadge_shake_update(struct ebadge_shake *shake,
                         const struct ebadge_accel *sample, uint32_t now)
{
  if (!shake || !sample) return false;

  int32_t magnitude = ebadge_accel_magnitude(sample);

  /* The first sample only seeds the baseline: without a resting reference
   * every start-up reading would look like a deviation.
   */
  if (!shake->baseline_valid)
    {
      shake->baseline_milli = magnitude;
      shake->baseline_valid = true;
      return false;
    }

  int32_t deviation = magnitude - shake->baseline_milli;
  if (deviation < 0) deviation = -deviation;

  if (!shake->armed)
    {
      if (deviation <= EBADGE_SHAKE_RELEASE_MILLI) shake->armed = true;
    }

  bool fired = false;
  if (shake->armed && deviation >= EBADGE_SHAKE_TRIGGER_MILLI)
    {
      /* The first shake is never rate limited: the cooldown only exists to
       * collapse one long shake into a single report, so applying it before
       * anything has fired would just drop a legitimate gesture.
       * The difference is unsigned, so the cooldown survives the tick wrap.
       */
      if (!shake->triggered_before ||
          (uint32_t)(now - shake->last_trigger_ms) >= EBADGE_SHAKE_COOLDOWN_MS)
        {
          shake->last_trigger_ms = now;
          shake->triggered_before = true;
          shake->armed = false;
          fired = true;
        }
    }

  /* Track the resting magnitude slowly, and only while the badge is not being
   * shaken: letting a shake pull the baseline would hide the next one.
   */
  if (!fired && deviation <= EBADGE_SHAKE_RELEASE_MILLI)
    {
      shake->baseline_milli +=
          (magnitude - shake->baseline_milli) >> EBADGE_SHAKE_BASELINE_SHIFT;
    }

  return fired;
}
