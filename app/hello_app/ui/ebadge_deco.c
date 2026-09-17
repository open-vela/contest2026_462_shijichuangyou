/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_deco.h"
#include "ebadge_portraits.h"

/* LVGL angles are tenths of a degree. */
#define ROT_45 450

static lv_obj_t *box(lv_obj_t *parent, int x, int y, int w, int h,
                     uint32_t fill, int radius)
{
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(obj, x, y);
  lv_obj_set_size(obj, w, h);
  lv_obj_set_style_bg_color(obj, lv_color_hex(fill), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, radius, 0);
  return obj;
}

/* The tail is a small square rotated 45 degrees, tucked under the card edge so
 * its outer half reads as a triangle. It is created before the caller's labels
 * so it never covers the text.
 */
static void tail(lv_obj_t *bubble, enum ebadge_deco_tail where, uint32_t fill)
{
  if (where == EBADGE_DECO_TAIL_NONE) return;
  int span = 18;
  int width = lv_obj_get_width(bubble), height = lv_obj_get_height(bubble);
  int x = 0, y = 0;
  if (where == EBADGE_DECO_TAIL_LEFT)
    {
      x = -span / 2;
      y = height / 2 - span / 2;
    }
  else if (where == EBADGE_DECO_TAIL_RIGHT)
    {
      x = width - span / 2;
      y = height / 2 - span / 2;
    }
  else
    {
      x = width / 2 - span / 2;
      y = height - span / 2;
    }
  lv_obj_t *obj = box(bubble, x, y, span, span, fill, 3);
  lv_obj_set_style_transform_rotation(obj, ROT_45, 0);
}

lv_obj_t *ebadge_deco_bubble(lv_obj_t *parent, int x, int y, int w, int h,
                             uint32_t fill, enum ebadge_deco_tail where)
{
  lv_obj_t *bubble = box(parent, x, y, w, h, fill, 22);
  /* The tail sits half outside the card. LVGL clips children to the parent by
   * default, so without this flag the tail would simply not be drawn.
   */
  if (where != EBADGE_DECO_TAIL_NONE)
    lv_obj_add_flag(bubble, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  tail(bubble, where, fill);
  return bubble;
}

lv_obj_t *ebadge_deco_sparkle(lv_obj_t *parent, int x, int y, int size,
                              uint32_t color)
{
  lv_obj_t *sparkle = lv_obj_create(parent);
  lv_obj_remove_style_all(sparkle);
  lv_obj_remove_flag(sparkle, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(sparkle, x, y);
  lv_obj_set_size(sparkle, size, size);
  lv_obj_set_style_bg_opa(sparkle, LV_OPA_TRANSP, 0);

  int arm = size / 5;
  if (arm < 2) arm = 2;
  /* A tall bar and a wide bar crossing in the middle: the classic twinkle. */
  lv_obj_t *vertical = box(sparkle, size / 2 - arm / 2, 0, arm, size, color,
                           arm / 2);
  lv_obj_t *horizontal = box(sparkle, 0, size / 2 - arm / 2, size, arm, color,
                             arm / 2);
  lv_obj_set_style_opa(vertical, LV_OPA_80, 0);
  lv_obj_set_style_opa(horizontal, LV_OPA_80, 0);
  return sparkle;
}

lv_obj_t *ebadge_deco_avatar(lv_obj_t *parent, int x, int y, int size,
                             unsigned int character, uint32_t ring,
                             uint32_t fill)
{
  if (character > 2) character = 0;
  lv_obj_t *avatar = box(parent, x, y, size, size, ring, size / 4);
  int inset = size / 16;
  if (inset < 2) inset = 2;
  int inner = size - inset * 2;

  lv_obj_t *picture = box(avatar, inset, inset, inner, inner, fill,
                          inner / 4);
  lv_obj_set_style_clip_corner(picture, true, 0);
  lv_obj_remove_flag(picture, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *image = lv_image_create(picture);
  lv_obj_remove_flag(image, LV_OBJ_FLAG_CLICKABLE);
  lv_image_set_src(image, &ebadge_portraits[character]);
  lv_obj_set_size(image, inner, inner);
  lv_image_set_inner_align(image, LV_IMAGE_ALIGN_STRETCH);
  lv_obj_center(image);
  return avatar;
}

void ebadge_deco_avatar_set(lv_obj_t *avatar, unsigned int character)
{
  if (!avatar) return;
  if (character > 2) character = 0;
  /* avatar -> inner picture plate -> portrait image. */
  lv_obj_t *picture = lv_obj_get_child(avatar, 0);
  if (!picture) return;
  lv_obj_t *image = lv_obj_get_child(picture, 0);
  if (!image) return;
  lv_image_set_src(image, &ebadge_portraits[character]);
}

lv_obj_t *ebadge_deco_chip(lv_obj_t *parent, int x, int y, int w, int h,
                           const char *text, uint32_t fill, uint32_t ink)
{
  lv_obj_t *chip = box(parent, x, y, w, h, fill, h / 2);
  lv_obj_t *label = lv_label_create(chip);
  lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(ink), 0);
  lv_obj_center(label);
  return chip;
}

/* The style setters take a selector argument, so they cannot be cast into an
 * lv_anim_exec_xcb_t callback. */
static void anim_opa_cb(void *var, int32_t value)
{
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

void ebadge_deco_breathe(lv_obj_t *obj, uint32_t period_ms, int cycles,
                         lv_opa_t low, lv_opa_t high)
{
  if (!obj || cycles <= 0) return;
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, obj);
  lv_anim_set_exec_cb(&anim, anim_opa_cb);
  lv_anim_set_values(&anim, high, low);
  lv_anim_set_duration(&anim, period_ms);
  lv_anim_set_playback_time(&anim, period_ms);
  lv_anim_set_repeat_count(&anim, cycles);
  lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
  lv_anim_delete(obj, anim_opa_cb);
  lv_anim_start(&anim);
}

void ebadge_deco_twinkle(lv_obj_t *obj, uint32_t period_ms, int cycles,
                         uint32_t delay_ms)
{
  if (!obj || cycles <= 0) return;
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, obj);
  lv_anim_set_exec_cb(&anim, anim_opa_cb);
  lv_anim_set_values(&anim, LV_OPA_20, LV_OPA_COVER);
  lv_anim_set_duration(&anim, period_ms / 2);
  lv_anim_set_playback_time(&anim, period_ms / 2);
  lv_anim_set_repeat_count(&anim, cycles);
  lv_anim_set_delay(&anim, delay_ms);
  lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
  lv_anim_delete(obj, anim_opa_cb);
  lv_anim_start(&anim);
}
