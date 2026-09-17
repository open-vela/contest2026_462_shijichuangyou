/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_date_editor.h"
#include "ebadge_view.h"
#include "../core/ebadge_calendar.h"
#include <stdio.h>
#include <string.h>

static struct
{
  lv_obj_t *overlay, *frame, *values[3];
  int date[3], scale;
  ebadge_date_commit_t commit;
} editor;

struct adjustment { int field, delta; };
static const struct adjustment adjustments[] = {
  {0, 1}, {1, 1}, {2, 1}, {0, -1}, {1, -1}, {2, -1}
};

static int px(int value) { return value * editor.scale / 1000; }

static void refresh(void)
{
  char value[12];
  for (int i = 0; i < 3; i++)
    {
      snprintf(value, sizeof(value), i ? "%02d" : "%04d", editor.date[i]);
      lv_label_set_text(editor.values[i], value);
    }
}

static void adjust(lv_event_t *event)
{
  const struct adjustment *change = lv_event_get_user_data(event);
  int field = change->field;
  int next = editor.date[field] + change->delta;
  int low = field == 0 ? EBADGE_YEAR_MIN : 1;
  int high = field == 0 ? EBADGE_YEAR_MAX : field == 1 ? 12 :
             ebadge_month_days(editor.date[0], editor.date[1]);
  if (next < low || next > high) return;
  editor.date[field] = next;
  /* Changing year or month can invalidate the day, e.g. 03-31 -> 02-31. */
  ebadge_date_clamp(&editor.date[0], &editor.date[1], &editor.date[2]);
  refresh();
}

void ebadge_date_editor_close(void)
{
  if (editor.overlay) lv_obj_delete(editor.overlay);
  memset(&editor, 0, sizeof(editor));
}

static void cancel(lv_event_t *event)
{
  (void)event;
  ebadge_date_editor_close();
}

static void confirm(lv_event_t *event)
{
  (void)event;
  int year = editor.date[0], month = editor.date[1], day = editor.date[2];
  ebadge_date_commit_t commit = editor.commit;
  ebadge_date_editor_close();
  if (commit) commit(year, month, day);
}

static lv_obj_t *label(const char *text, int x, int y, int width)
{
  lv_obj_t *obj = lv_label_create(editor.frame);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_width(obj, px(width));
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_text(obj, text);
  return obj;
}

static void button(const char *text, int x, int y, int width,
                   lv_event_cb_t callback, void *data)
{
  struct ebadge_palette p = ebadge_view_palette();
  lv_obj_t *obj = lv_button_create(editor.frame);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_size(obj, px(width), px(44));
  lv_obj_set_style_radius(obj, px(12), 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(p.soft), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(p.ink), 0);
  lv_obj_t *caption = lv_label_create(obj);
  lv_obj_remove_flag(caption, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(caption, text);
  lv_obj_center(caption);
  lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
}

bool ebadge_date_editor_open(lv_obj_t *parent, int year, int month, int day,
                            ebadge_date_commit_t commit)
{
  if (editor.overlay || !parent || !commit) return false;
  if (!ebadge_date_valid(year, month, day)) return false;
  int width = lv_obj_get_width(parent), height = lv_obj_get_height(parent);
  if (width < 100 || height < 100) return false;
  editor.scale = width * 1000 / 390;
  if (height * 1000 / 450 < editor.scale) editor.scale = height * 1000 / 450;
  editor.date[0] = year;
  editor.date[1] = month;
  editor.date[2] = day;
  editor.commit = commit;
  struct ebadge_palette p = ebadge_view_palette();
  editor.overlay = lv_obj_create(parent);
  lv_obj_remove_style_all(editor.overlay);
  lv_obj_remove_flag(editor.overlay, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_add_flag(editor.overlay, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(editor.overlay, width, height);
  lv_obj_set_style_bg_color(editor.overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(editor.overlay, LV_OPA_70, 0);
  editor.frame = lv_obj_create(editor.overlay);
  lv_obj_remove_style_all(editor.frame);
  lv_obj_remove_flag(editor.frame, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_size(editor.frame, px(326), px(344));
  lv_obj_center(editor.frame);
  lv_obj_set_style_radius(editor.frame, px(28), 0);
  lv_obj_set_style_bg_color(editor.frame, lv_color_hex(p.paper), 0);
  lv_obj_set_style_bg_opa(editor.frame, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(editor.frame, lv_color_hex(p.ink), 0);
  label("编辑纪念日", 16, 18, 294);
  label("草稿，确定后生效", 16, 50, 294);
  static const char *const units[] = {"年", "月", "日"};
  for (int i = 0; i < 3; i++)
    {
      int x = 22 + i * 100;
      label(units[i], x, 82, 82);
      button("+", x, 110, 82, adjust, (void *)&adjustments[i]);
      editor.values[i] = label("", x, 164, 82);
      button("-", x, 196, 82, adjust, (void *)&adjustments[i + 3]);
    }
  button("取消", 22, 260, 132, cancel, NULL);
  button("确定", 172, 260, 132, confirm, NULL);
  /* Storage state is reported by the store, not assumed here. */
  label(ebadge_view_storage_status(), 16, 313, 294);
  refresh();
  return true;
}
