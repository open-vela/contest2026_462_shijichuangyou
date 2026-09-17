/* SPDX-License-Identifier: Apache-2.0 */
/* Idle display policy for the badge.
 *
 * Split out from the UI on purpose: the timing rules are the part that can be
 * wrong in a way nobody notices on a bench (the screen still looks fine, it
 * just never dims, or it dims while the user is reading). Here they are pure
 * integer arithmetic that the host tests can drive directly.
 *
 * The states mirror the three levels the project manual separates. This module
 * only implements L1 (stop the expensive animation, dim the picture) and L2
 * (actually power the panel down through the display driver). It does not
 * claim L3 system suspend, and a dark overlay on its own is not treated as
 * evidence of L2.
 */
#ifndef EBADGE_POWER_H
#define EBADGE_POWER_H
#include <stdbool.h>
#include <stdint.h>

enum ebadge_power_state
{
  EBADGE_POWER_ACTIVE = 0,
  EBADGE_POWER_DIMMED, /* L1: dimmed, animations stopped */
  EBADGE_POWER_OFF,    /* L2: panel power actually removed */
  EBADGE_POWER_COUNT
};

struct ebadge_power
{
  uint32_t last_activity_ms;
  uint32_t dim_after_ms;
  uint32_t off_after_ms;
  enum ebadge_power_state state;
};

/* A zero threshold disables that stage, so a display mode can keep the badge
 * lit indefinitely without a second code path.
 */
void ebadge_power_init(struct ebadge_power *power, uint32_t now,
                       uint32_t dim_after_ms, uint32_t off_after_ms);

/* Advance the state machine. Call it on every frame; it is a couple of
 * unsigned comparisons.
 */
enum ebadge_power_state ebadge_power_update(struct ebadge_power *power,
                                            uint32_t now);

/* Record user activity.
 *
 * Returns true when the touch should be *consumed*: the badge was off, the
 * user was really asking to see it again, and acting on the same touch would
 * make one gesture do two things. The project manual calls that out
 * explicitly for the wake key, so a waking touch only wakes.
 */
bool ebadge_power_activity(struct ebadge_power *power, uint32_t now);

/* True once the panel power has been removed and rendering can stop. */
bool ebadge_power_rendering_paused(enum ebadge_power_state state);

#endif
