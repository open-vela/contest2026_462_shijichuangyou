/* SPDX-License-Identifier: Apache-2.0 */
/* Shake detection.
 *
 * Pure integer arithmetic on purpose. A shake detector that is subtly wrong
 * does not fail loudly - it either fires while the badge hangs still on a
 * lanyard, or never fires at all - and neither shows up in a build. Keeping
 * it free of LVGL and of the device means the host tests can drive real
 * sample sequences through it.
 *
 * Units follow the project convention: internally the IMU works in
 * millimetres per second squared.
 */
#ifndef EBADGE_SHAKE_H
#define EBADGE_SHAKE_H
#include <stdbool.h>
#include <stdint.h>

/* One accelerometer sample, milli-m/s^2. */
struct ebadge_accel
{
  int32_t x, y, z;
};

/* A deviation from the resting gravity magnitude, in milli-m/s^2, that counts
 * as a deliberate shake. Roughly 0.71 g: high enough that walking and arm
 * swing do not trigger it, low enough that a wrist flick does.
 */
#define EBADGE_SHAKE_TRIGGER_MILLI 7000
/* Hysteresis: the motion must fall back below this before another shake can
 * fire, so a single long shake is reported once rather than every sample.
 */
#define EBADGE_SHAKE_RELEASE_MILLI 3000
/* Minimum spacing between reported shakes. */
#define EBADGE_SHAKE_COOLDOWN_MS 1200u
/* Weight of each new sample in the resting-gravity estimate. */
#define EBADGE_SHAKE_BASELINE_SHIFT 4

struct ebadge_shake
{
  int32_t baseline_milli; /* slow estimate of the resting magnitude */
  uint32_t last_trigger_ms;
  bool baseline_valid;
  bool armed; /* below the release threshold: a shake may fire */
  /* Kept separately from last_trigger_ms: a zero timestamp means "fired at
   * boot", not "never fired", and treating them the same would silently
   * swallow a shake made in the first second after power-on.
   */
  bool triggered_before;
};

/* The driver hands back raw counts and documents 0.488 mg/LSB
 * (drivers/sensors/lsm6dsl.c writes CTRL1_XL = 0x74 for +-16 g). Converting:
 *
 *   0.488 mg = 0.488e-3 g = 0.488e-3 * 9.80665 m/s^2 = 4.7856e-3 m/s^2
 *
 * so one count is 4.7856 milli-m/s^2. Kept as a rational so the conversion
 * stays exact integer arithmetic.
 */
#define EBADGE_ACCEL_MILLI_NUM 4786
#define EBADGE_ACCEL_MILLI_DEN 1000

void ebadge_shake_init(struct ebadge_shake *shake);

/* Feed one sample. Returns true exactly once per detected shake. */
bool ebadge_shake_update(struct ebadge_shake *shake,
                         const struct ebadge_accel *sample, uint32_t now);

/* Raw driver counts -> milli-m/s^2. */
int32_t ebadge_accel_counts_to_milli(int16_t count);

/* Magnitude of a sample, milli-m/s^2. Exposed for tests and diagnostics. */
int32_t ebadge_accel_magnitude(const struct ebadge_accel *sample);

#endif
