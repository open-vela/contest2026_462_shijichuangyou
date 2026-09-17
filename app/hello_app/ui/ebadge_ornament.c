/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_ornament.h"
#include <string.h>

#define MOTIF_COUNT 3
#define GROUP 48 /* logical size of one motif cluster */

struct motif
{
  lv_obj_t *group;
  lv_obj_t *shapes[5];
  int count;
};

static struct
{
  lv_obj_t *root;
  struct motif motifs[MOTIF_COUNT];
  int scale, origin_x, origin_y;
  uint32_t color;
} orn;

static int px(int value) { return value * orn.scale / 1000; }

static lv_obj_t *dot(lv_obj_t *parent, int x, int y, int w, int h, int radius)
{
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_size(obj, px(w), px(h));
  lv_obj_set_style_radius(obj, px(radius), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  return obj;
}

static void add_shape(struct motif *motif, int x, int y, int w, int h, int radius)
{
  if (motif->count >= (int)(sizeof(motif->shapes) / sizeof(motif->shapes[0])))
    return;
  motif->shapes[motif->count++] = dot(motif->group, x, y, w, h, radius);
}

/* 苔苔: a four-petal bloom with a small heart. */
static void build_bloom(struct motif *motif)
{
  add_shape(motif, 18, 2, 13, 13, 7);
  add_shape(motif, 18, 33, 13, 13, 7);
  add_shape(motif, 2, 18, 13, 13, 7);
  add_shape(motif, 33, 18, 13, 13, 7);
  add_shape(motif, 21, 21, 7, 7, 4);
}

/* 暖暖: a cat paw, one pad and three toes. */
static void build_paw(struct motif *motif)
{
  add_shape(motif, 11, 20, 26, 22, 11);
  add_shape(motif, 7, 6, 11, 11, 6);
  add_shape(motif, 19, 2, 11, 11, 6);
  add_shape(motif, 31, 6, 11, 11, 6);
}

/* 月月: a four-point sparkle. */
static void build_star(struct motif *motif)
{
  add_shape(motif, 21, 0, 6, 48, 3);
  add_shape(motif, 0, 21, 48, 6, 3);
  add_shape(motif, 18, 18, 12, 12, 6);
}

static void opa_cb(void *var, int32_t value)
{
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

static void apply_color(void)
{
  for (int m = 0; m < MOTIF_COUNT; m++)
    for (int s = 0; s < orn.motifs[m].count; s++)
      lv_obj_set_style_bg_color(orn.motifs[m].shapes[s],
                                lv_color_hex(orn.color), 0);
}

bool ebadge_ornament_open(lv_obj_t *parent, int scale, int origin_x, int origin_y)
{
  if (orn.root) return false;
  memset(&orn, 0, sizeof(orn));
  orn.scale = scale;
  orn.origin_x = origin_x;
  orn.origin_y = origin_y;
  orn.color = 0xaf9962;
  orn.root = lv_obj_create(parent);
  lv_obj_remove_style_all(orn.root);
  lv_obj_remove_flag(orn.root, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(orn.root,
                  lv_display_get_horizontal_resolution(lv_display_get_default()),
                  lv_display_get_vertical_resolution(lv_display_get_default()));
  lv_obj_set_pos(orn.root, 0, 0);
  lv_obj_set_style_bg_opa(orn.root, LV_OPA_TRANSP, 0);
  lv_obj_set_style_opa(orn.root, LV_OPA_TRANSP, 0);

  void (*const builders[MOTIF_COUNT])(struct motif *) = {
    build_bloom, build_paw, build_star
  };
  for (int m = 0; m < MOTIF_COUNT; m++)
    {
      orn.motifs[m].group = lv_obj_create(orn.root);
      lv_obj_remove_style_all(orn.motifs[m].group);
      lv_obj_remove_flag(orn.motifs[m].group,
                         LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_set_size(orn.motifs[m].group, px(GROUP), px(GROUP));
      lv_obj_set_style_bg_opa(orn.motifs[m].group, LV_OPA_TRANSP, 0);
      builders[m](&orn.motifs[m]);
      lv_obj_add_flag(orn.motifs[m].group, LV_OBJ_FLAG_HIDDEN);
    }
  apply_color();
  return true;
}

void ebadge_ornament_close(void)
{
  if (orn.root)
    {
      lv_anim_delete(orn.root, opa_cb);
      lv_obj_delete(orn.root);
    }
  memset(&orn, 0, sizeof(orn));
}

void ebadge_ornament_recolor(uint32_t color)
{
  orn.color = color;
  if (orn.root) apply_color();
}

void ebadge_ornament_play(enum ebadge_motif motif, int canvas_x, int canvas_y,
                          uint32_t color)
{
  if (!orn.root) return;
  unsigned int index = (unsigned int)motif;
  if (index >= MOTIF_COUNT) index = 0;
  orn.color = color;
  apply_color();

  for (unsigned int m = 0; m < MOTIF_COUNT; m++)
    {
      if (m == index)
        lv_obj_remove_flag(orn.motifs[m].group, LV_OBJ_FLAG_HIDDEN);
      else
        lv_obj_add_flag(orn.motifs[m].group, LV_OBJ_FLAG_HIDDEN);
    }

  /* Centre the cluster on the touch point, then keep it inside the canvas so
   * a tap near an edge does not draw a half motif.
   */
  int32_t half = px(GROUP / 2);
  int32_t x = orn.origin_x + px(canvas_x) - half;
  int32_t y = orn.origin_y + px(canvas_y) - half;
  int32_t max_x = orn.origin_x + px(390) - px(GROUP);
  int32_t max_y = orn.origin_y + px(450) - px(GROUP);
  if (x < orn.origin_x) x = orn.origin_x;
  if (y < orn.origin_y) y = orn.origin_y;
  if (x > max_x) x = max_x;
  if (y > max_y) y = max_y;
  lv_obj_set_pos(orn.motifs[index].group, x, y);

  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, orn.root);
  lv_anim_set_exec_cb(&anim, opa_cb);
  lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_duration(&anim, 140);
  lv_anim_set_playback_delay(&anim, 150);
  lv_anim_set_playback_time(&anim, 420);
  lv_anim_delete(orn.root, opa_cb); /* restart cleanly on rapid taps */
  lv_anim_start(&anim);
}
