/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_power.h"

void ebadge_power_init(struct ebadge_power *power, uint32_t now,
                       uint32_t dim_after_ms, uint32_t off_after_ms)
{
  if (!power) return;
  power->last_activity_ms = now;
  power->dim_after_ms = dim_after_ms;
  /* Turning the panel off before it has dimmed would skip L1 entirely, which
   * would make the transition look like a fault. Clamp instead of trusting the
   * caller.
   */
  if (off_after_ms && dim_after_ms && off_after_ms < dim_after_ms)
    off_after_ms = dim_after_ms;
  power->off_after_ms = off_after_ms;
  power->state = EBADGE_POWER_ACTIVE;
}

enum ebadge_power_state ebadge_power_update(struct ebadge_power *power,
                                            uint32_t now)
{
  if (!power) return EBADGE_POWER_ACTIVE;
  /* Unsigned difference, so this stays correct across the 32-bit wrap. */
  uint32_t idle = now - power->last_activity_ms;

  if (power->state == EBADGE_POWER_ACTIVE)
    {
      if (power->dim_after_ms && idle >= power->dim_after_ms)
        power->state = EBADGE_POWER_DIMMED;
    }
  if (power->state == EBADGE_POWER_DIMMED)
    {
      if (power->off_after_ms && idle >= power->off_after_ms)
        power->state = EBADGE_POWER_OFF;
    }
  return power->state;
}

bool ebadge_power_activity(struct ebadge_power *power, uint32_t now)
{
  if (!power) return false;
  bool was_off = power->state == EBADGE_POWER_OFF;
  power->state = EBADGE_POWER_ACTIVE;
  power->last_activity_ms = now;
  return was_off;
}

bool ebadge_power_rendering_paused(enum ebadge_power_state state)
{
  return state == EBADGE_POWER_OFF;
}
