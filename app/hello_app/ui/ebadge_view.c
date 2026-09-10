/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_view.h"
#include "../core/ebadge_controller.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <string.h>

/* Original code-drawn study characters. No external images or decoder.
 * Names/designs are provisional, not an assertion of approved brand IP.
 */
LV_FONT_DECLARE(ebadge_font_zh_20);
static const char *const names[] = {"苔苔", "暖暖", "月月"};
static const char *const greetings[] = {"送你一份小小的快乐。",
  "有你在，今天更温暖。", "慢慢来，每一步都算数。"};
static const uint32_t colors[] = {0x9ee4be, 0xffb56b, 0xa7c9f1};

static struct
{
  struct ebadge_controller controller;
  lv_obj_t *root, *actor, *face, *ears[2], *eyes[2], *mouth;
  lv_obj_t *name, *message, *dots[3];
  lv_timer_t *timer;
  uint32_t painted_generation;
  enum ebadge_character_state painted_state;
  int32_t scale, origin_x, origin_y;
  lv_point_t press;
  uint32_t press_ms;
  bool pressed, moved;
} view;

static int32_t px(int32_t value) { return value * view.scale / 1000; }

static lv_obj_t *shape(lv_obj_t *parent, int x, int y, int w, int h,
                       uint32_t color, int radius)
{
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_size(obj, px(w), px(h));
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, px(radius), 0);
  return obj;
}

static lv_obj_t *caption(const char *text, int y, uint32_t color)
{
  lv_obj_t *obj = lv_label_create(view.root);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(obj, text);
  lv_obj_set_width(obj, px(326));
  lv_obj_set_pos(obj, view.origin_x + px(32), view.origin_y + px(y));
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
  return obj;
}

static void paint_character(void)
{
  const struct ebadge_state *state = &view.controller.state;
  unsigned int id = state->character;
  lv_color_t color = lv_color_hex(colors[id]);
  lv_obj_set_style_bg_color(view.face, color, 0);
  lv_obj_set_style_radius(view.face, px(id == 2 ? 28 : 70), 0);
  for (int i = 0; i < 2; i++)
    {
      lv_obj_set_style_bg_color(view.ears[i], color, 0);
      lv_obj_set_size(view.ears[i], px(id == 0 ? 28 : 42),
                      px(id == 0 ? 72 : 42));
      lv_obj_set_style_radius(view.ears[i], px(id == 2 ? 8 : 24), 0);
      lv_obj_set_style_bg_color(view.dots[i], lv_color_hex(0x31505a), 0);
    }
  for (int i = 0; i < 3; i++)
    lv_obj_set_style_bg_color(view.dots[i],
      lv_color_hex(i == (int)id ? colors[id] : 0x31505a), 0);
  lv_label_set_text(view.name, names[id]);
  view.painted_generation = state->generation;
}

static void frame(lv_timer_t *timer)
{
  (void)timer;
  uint32_t now = lv_tick_get();
  ebadge_controller_step(&view.controller, now);
  const struct ebadge_state *state = &view.controller.state;
  bool switched = view.painted_generation != state->generation;
  bool reacting = state->character_state == EBADGE_REACTING;
  if (switched) paint_character();
  if (switched || view.painted_state != state->character_state)
    {
      lv_label_set_text(view.message, reacting ? greetings[state->character] :
                        "轻点打招呼，左右滑动换伙伴");
      view.painted_state = state->character_state;
    }

  uint32_t phase = reacting ? (now - state->reaction_started_ms) % 360 : now % 2400;
  int period = reacting ? 360 : 2400;
  int height = reacting ? 18 : 8;
  int triangle = (int)phase < period / 2 ? (int)phase : period - (int)phase;
  int bob = height * triangle * 2 / period - height / 2;
  lv_obj_set_pos(view.actor, view.origin_x + px(105),
                view.origin_y + px(105 + bob));
  int eye_height = reacting || now % 3100 > 2950 ? 4 : 18;
  for (int i = 0; i < 2; i++)
    {
      lv_obj_set_height(view.eyes[i], px(eye_height));
      lv_obj_set_y(view.eyes[i], px(88 + (18 - eye_height) / 2));
    }
  lv_obj_set_width(view.mouth, px(reacting ? 32 : 16));
  lv_obj_set_x(view.mouth, px(reacting ? 74 : 82));
}

static void touch(lv_event_t *event)
{
  lv_event_code_t code = lv_event_get_code(event);
  lv_indev_t *input = lv_event_get_indev(event);
  lv_point_t point;
  if (code == LV_EVENT_PRESS_LOST)
    {
      view.pressed = false;
      return;
    }
  if (!input || (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
                 code != LV_EVENT_RELEASED)) return;
  lv_indev_get_point(input, &point);
  if (code == LV_EVENT_PRESSED)
    {
      view.press = point;
      view.press_ms = lv_tick_get();
      view.pressed = true;
      view.moved = false;
      return;
    }
  if (!view.pressed) return;
  int32_t dx = point.x - view.press.x;
  int32_t dy = point.y - view.press.y;
  if ((int64_t)dx * dx + (int64_t)dy * dy > 12 * 12) view.moved = true;
  if (code != LV_EVENT_RELEASED) return;
  view.pressed = false;
  struct ebadge_event action = {0};
  if (abs(dx) >= 45 && (int64_t)abs(dx) * 2 >= (int64_t)abs(dy) * 3)
    action.kind = dx < 0 ? EBADGE_NEXT : EBADGE_PREV;
  else if (!view.moved && (uint32_t)(lv_tick_get() - view.press_ms) <= 300)
    action.kind = EBADGE_TAP;
  else return;
  /* One recognizer only: do not also handle LVGL CLICKED/GESTURE events. */
  ebadge_controller_post(&view.controller, action);
}

bool ebadge_view_open(void)
{
  if (view.root) return false;
  memset(&view, 0, sizeof(view));
  int32_t w = lv_display_get_horizontal_resolution(lv_display_get_default());
  int32_t h = lv_display_get_vertical_resolution(lv_display_get_default());
  if (w < 100 || h < 100) return false;
  view.scale = w * 1000 / 390;
  if (h * 1000 / 450 < view.scale) view.scale = h * 1000 / 450;
  view.origin_x = (w - px(390)) / 2;
  view.origin_y = (h - px(450)) / 2;
  ebadge_controller_init(&view.controller, lv_tick_get());
  view.root = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(view.root);
  lv_obj_set_style_text_font(view.root, &ebadge_font_zh_20, 0);
  lv_obj_set_size(view.root, w, h);
  lv_obj_set_style_bg_color(view.root, lv_color_hex(0x102b35), 0);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_COVER, 0);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(view.root, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *halo = shape(view.root, 0, 0, 270, 270, 0x1c3a43, 135);
  lv_obj_set_pos(halo, view.origin_x + px(60), view.origin_y + px(80));
  caption("拾迹创游 / 随身小伙伴", 36, 0xd8d5bd);
  view.actor = shape(view.root, 0, 0, 180, 190, 0, 0);
  lv_obj_set_style_bg_opa(view.actor, LV_OPA_TRANSP, 0);
  view.ears[0] = shape(view.actor, 24, 4, 28, 72, colors[0], 24);
  view.ears[1] = shape(view.actor, 128, 4, 28, 72, colors[0], 24);
  view.face = shape(view.actor, 10, 40, 160, 140, colors[0], 70);
  view.eyes[0] = shape(view.actor, 50, 88, 12, 18, 0x16333b, 6);
  view.eyes[1] = shape(view.actor, 118, 88, 12, 18, 0x16333b, 6);
  view.mouth = shape(view.actor, 82, 120, 16, 7, 0x16333b, 4);
  view.name = caption(names[0], 307, 0xf5ecd5);
  view.message = caption("轻点打招呼，左右滑动换伙伴", 337, 0xb2c7c6);
  for (int i = 0; i < 3; i++)
    {
      view.dots[i] = shape(view.root, 0, 0, 8, 8, 0x31505a, 4);
      lv_obj_set_pos(view.dots[i], view.origin_x + px(175 + 16 * i),
                    view.origin_y + px(381));
    }
  caption("本地互动 / 中文版", 407, 0x92aaa9);
  lv_obj_add_event_cb(view.root, touch, LV_EVENT_ALL, NULL);
  frame(NULL);
  view.timer = lv_timer_create(frame, 33, NULL);
  if (!view.timer)
    {
      ebadge_view_close();
      return false;
    }
  return true;
}

void ebadge_view_close(void)
{
  if (view.timer) lv_timer_delete(view.timer);
  if (view.root) lv_obj_delete(view.root);
  memset(&view, 0, sizeof(view));
}
