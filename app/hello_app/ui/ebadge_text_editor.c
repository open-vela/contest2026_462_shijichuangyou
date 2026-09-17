/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_text_editor.h"
#include "ebadge_view.h"
#include <string.h>

static struct
{
  lv_obj_t *overlay, *frame, *input, *keyboard, *notice;
  int scale;
  ebadge_text_commit_t commit;
} editor;

static const char *const phrases[] = {
  "每一天都与你相伴。", "让喜欢发光！", "生日快乐！"
};

static int px(int n) { return n * editor.scale / 1000; }

void ebadge_text_editor_close(void)
{
  if (editor.keyboard) lv_keyboard_set_textarea(editor.keyboard, NULL);
  if (editor.overlay) lv_obj_delete(editor.overlay);
  memset(&editor, 0, sizeof(editor));
}

static void cancel(lv_event_t *event)
{
  (void)event;
  ebadge_text_editor_close();
}

static void confirm(lv_event_t *event)
{
  (void)event;
  const char *text = lv_textarea_get_text(editor.input);
  size_t length = strlen(text);
  bool nonblank = false;
  for (size_t i = 0; i < length; i++)
    if (text[i] != ' ' && text[i] != '\t' && text[i] != '\n' && text[i] != '\r')
      nonblank = true;
  if (!nonblank)
    {
      lv_label_set_text(editor.notice, "请先输入文字");
      return;
    }
  if (length >= 160)
    {
      lv_label_set_text(editor.notice, "文字过长，请删减");
      return;
    }
  char saved[160];
  memcpy(saved, text, length + 1);
  ebadge_text_commit_t commit = editor.commit;
  ebadge_text_editor_close();
  if (commit) commit(saved);
}

static void insert_phrase(lv_event_t *event)
{
  lv_textarea_add_text(editor.input, lv_event_get_user_data(event));
}

static lv_obj_t *label(const char *text, int y)
{
  lv_obj_t *obj = lv_label_create(editor.frame);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(obj, px(16), px(y));
  lv_obj_set_width(obj, px(330));
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
  lv_obj_set_size(obj, px(width), px(38));
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(obj, lv_color_hex(p.soft), 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(p.ink), 0);
  lv_obj_set_style_radius(obj, px(10), 0);
  lv_obj_t *caption = lv_label_create(obj);
  lv_obj_remove_flag(caption, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(caption, text);
  lv_obj_center(caption);
  lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
}

bool ebadge_text_editor_open(lv_obj_t *parent, const char *text,
                            ebadge_text_commit_t commit)
{
  if (editor.overlay || !parent || !text || !commit || strlen(text) >= 160)
    return false;
  int width = lv_obj_get_width(parent), height = lv_obj_get_height(parent);
  if (width < 100 || height < 100) return false;
  editor.scale = width * 1000 / 390;
  if (height * 1000 / 450 < editor.scale) editor.scale = height * 1000 / 450;
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
  lv_obj_set_size(editor.frame, px(362), px(430));
  lv_obj_center(editor.frame);
  lv_obj_set_style_radius(editor.frame, px(22), 0);
  lv_obj_set_style_bg_color(editor.frame, lv_color_hex(p.paper), 0);
  lv_obj_set_style_bg_opa(editor.frame, LV_OPA_COVER, 0);
  lv_obj_set_style_text_color(editor.frame, lv_color_hex(p.ink), 0);
  label("编辑应援文字", 14);
  editor.input = lv_textarea_create(editor.frame);
  lv_obj_remove_flag(editor.input, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_pos(editor.input, px(18), px(49));
  lv_obj_set_size(editor.input, px(326), px(78));
  lv_obj_set_style_bg_color(editor.input, lv_color_hex(p.soft), 0);
  lv_obj_set_style_text_color(editor.input, lv_color_hex(p.ink), 0);
  lv_obj_set_style_text_font(editor.input,
                             lv_obj_get_style_text_font(parent, LV_PART_MAIN), 0);
  lv_textarea_set_max_length(editor.input, 159);
  lv_textarea_set_text(editor.input, text);
  static const char *const titles[] = {"陪伴", "应援", "祝福"};
  for (int i = 0; i < 3; i++)
    button(titles[i], 18 + i * 112, 135, 102, insert_phrase, (void *)phrases[i]);
  editor.keyboard = lv_keyboard_create(editor.frame);
  lv_obj_remove_flag(editor.keyboard, LV_OBJ_FLAG_EVENT_BUBBLE);
  lv_obj_set_pos(editor.keyboard, px(18), px(182));
  lv_obj_set_size(editor.keyboard, px(326), px(164));
  /* The standard keyboard font includes its control-key symbols. */
  lv_obj_set_style_text_font(editor.keyboard, LV_FONT_DEFAULT, LV_PART_ITEMS);
  lv_keyboard_set_textarea(editor.keyboard, editor.input);
  lv_obj_add_event_cb(editor.keyboard, confirm, LV_EVENT_READY, NULL);
  lv_obj_add_event_cb(editor.keyboard, cancel, LV_EVENT_CANCEL, NULL);
  button("取消", 18, 358, 154, cancel, NULL);
  button("确定", 190, 358, 154, confirm, NULL);
  /* Storage state is reported by the store, not assumed here. */
  editor.notice = label(ebadge_view_storage_status(), 402);
  return true;
}
