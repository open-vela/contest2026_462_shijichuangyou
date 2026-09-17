/* SPDX-License-Identifier: Apache-2.0 */
/* Illustrated (ACG) building blocks shared by the function pages.
 *
 * These exist so the pages read as a character's own screen rather than as a
 * settings form: a speech bubble the companion "talks" from, a portrait chip,
 * sticker-style chips and drawn sparkles. Everything is built from LVGL
 * primitives, so nothing here depends on glyph coverage except the text the
 * caller passes in.
 */
#ifndef EBADGE_DECO_H
#define EBADGE_DECO_H
#include <lvgl/lvgl.h>

/* Where the speech bubble's tail points. */
enum ebadge_deco_tail
{
  EBADGE_DECO_TAIL_NONE = 0,
  EBADGE_DECO_TAIL_LEFT,
  EBADGE_DECO_TAIL_RIGHT,
  EBADGE_DECO_TAIL_BOTTOM
};

/* Rounded bubble card. Add the caller's labels as children of the returned
 * object: the tail is created first so any later child draws over it.
 */
lv_obj_t *ebadge_deco_bubble(lv_obj_t *parent, int x, int y, int w, int h,
                             uint32_t fill, enum ebadge_deco_tail tail);

/* Four-point sparkle drawn from two crossing bars. No font glyph involved. */
lv_obj_t *ebadge_deco_sparkle(lv_obj_t *parent, int x, int y, int size,
                              uint32_t color);

/* Rounded portrait chip with a ring, used as the companion's avatar. */
lv_obj_t *ebadge_deco_avatar(lv_obj_t *parent, int x, int y, int size,
                             unsigned int character, uint32_t ring,
                             uint32_t fill);

/* Point an existing avatar at another companion's portrait. */
void ebadge_deco_avatar_set(lv_obj_t *avatar, unsigned int character);

/* Pill-shaped text chip. Returns the chip; its label is child 0. */
lv_obj_t *ebadge_deco_chip(lv_obj_t *parent, int x, int y, int w, int h,
                           const char *text, uint32_t fill, uint32_t ink);

/* Gentle loop on the object's opacity. Bounded cycles on purpose: an endless
 * animation would keep the display busy and work against the idle screen-off
 * requirement (FR-08).
 */
void ebadge_deco_breathe(lv_obj_t *obj, uint32_t period_ms, int cycles,
                         lv_opa_t low, lv_opa_t high);

/* Same idea for a sparkle, with an optional stagger delay. */
void ebadge_deco_twinkle(lv_obj_t *obj, uint32_t period_ms, int cycles,
                         uint32_t delay_ms);

#endif
