/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_view.h"
#include "ebadge_pages.h"
#include "ebadge_portraits.h"
#include "ebadge_motion.h"
#include "ebadge_ornament.h"
#include "../core/ebadge_controller.h"
#include "../core/ebadge_store.h"
#include <lvgl/lvgl.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(ebadge_font_zh_20);
LV_FONT_DECLARE(ebadge_font_title_26);

/* The design canvas is 390x450 and everything below is authored in those
 * units; view.scale converts to real panel pixels at open time.
 */
#define CANVAS_W 390
#define CANVAS_H 450

/* Gesture thresholds, in canvas units. */
#define SLOP 10
#define SWIPE_MIN 55
#define MENU_SWIPE_MIN 60
#define DOMINANCE_NUM 135
#define DOMINANCE_DEN 100
#define HOLD_MS 600u

#define HINT_MS 7000u
#define HINT_FADE_MS 700u
#define SAVE_DEBOUNCE_MS 1500u

static struct
{
  struct ebadge_controller controller;
  unsigned int character;
  enum ebadge_paper paper;
  struct ebadge_palette palette;

  lv_obj_t *root;
  lv_obj_t *portrait;
  lv_obj_t *fade;
  lv_obj_t *name;
  lv_obj_t *caption;
  lv_obj_t *dots[EBADGE_CHARACTER_COUNT];
  lv_obj_t *hint_pill;
  lv_obj_t *hint_label;
  lv_obj_t *menu; /* NULL while the menu is closed */

  lv_timer_t *timer;
  int32_t scale, origin_x, origin_y;

  /* Gesture state for the current press. */
  lv_point_t press;
  uint32_t press_ms;
  int32_t max_dist_sq;
  bool pressed, hold_fired;

  /* Portrait animation. */
  const lv_image_dsc_t *displayed;
  uint32_t random, motion_started, motion_due;
  uint8_t motion_clip;
  enum ebadge_character_state painted_state;
  unsigned int painted_character;
  bool motion_paused;

  /* Deferred storage write. */
  bool dirty;
  uint32_t dirty_at;
  bool opened;
} view;

static int32_t px(int value) { return value * view.scale / 1000; }

/* lv_anim_exec_xcb_t is (void *, int32_t) but the style setters take a third
 * selector argument, so they must never be cast into an animation callback:
 * the missing argument would be read from an undefined register.
 */
static void anim_opa_cb(void *var, int32_t value)
{
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

static void anim_translate_y_cb(void *var, int32_t value)
{
  lv_obj_set_style_translate_y((lv_obj_t *)var, value, 0);
}

static void anim_translate_x_cb(void *var, int32_t value)
{
  lv_obj_set_style_translate_x((lv_obj_t *)var, value, 0);
}

static int32_t to_canvas_x(int32_t display_x)
{
  return (display_x - view.origin_x) * 1000 / view.scale;
}

static int32_t to_canvas_y(int32_t display_y)
{
  return (display_y - view.origin_y) * 1000 / view.scale;
}

/* ------------------------------------------------------------------ theme */

static void paint_dots(void)
{
  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    lv_obj_set_style_bg_color(view.dots[i],
                              lv_color_hex(i == (int)view.character ?
                                           view.palette.ink :
                                           view.palette.soft), 0);
}

static void apply_theme(void)
{
  view.palette = ebadge_theme_resolve(view.character, view.paper);
  lv_obj_set_style_bg_color(view.root, lv_color_hex(view.palette.paper), 0);
  lv_obj_set_style_bg_color(view.fade, lv_color_hex(view.palette.paper), 0);
  lv_obj_set_style_bg_grad_color(view.fade, lv_color_hex(view.palette.paper), 0);
  lv_obj_set_style_text_color(view.name, lv_color_hex(view.palette.ink), 0);
  lv_obj_set_style_text_color(view.caption, lv_color_hex(view.palette.muted), 0);
  if (view.hint_pill)
    lv_obj_set_style_bg_color(view.hint_pill, lv_color_hex(view.palette.soft), 0);
  if (view.hint_label)
    lv_obj_set_style_text_color(view.hint_label, lv_color_hex(view.palette.ink), 0);
  paint_dots();
  ebadge_ornament_recolor(view.palette.gold);
  ebadge_pages_apply_theme();
}

/* ------------------------------------------------------- portrait motion */

static uint32_t random_delay(void)
{
  view.random ^= view.random << 13;
  view.random ^= view.random >> 17;
  view.random ^= view.random << 5;
  return 6000u + view.random % 6001u;
}

static void show_portrait(const lv_image_dsc_t *image)
{
  if (view.displayed == image) return;
  lv_image_set_src(view.portrait, image);
  view.displayed = image;
}

static void reset_motion(uint32_t now)
{
  view.motion_clip = 0;
  view.motion_due = now + random_delay();
  show_portrait(view.character == 0 ?
                &ebadge_motion_frames[0] :
                &ebadge_portraits[view.character]);
}

static void animate_portrait(uint32_t now, bool tapped)
{
  static const uint8_t blink[] = {1, 2, 3, 2, 1};
  static const uint16_t blink_ms[] = {60, 60, 100, 60, 60};
  static const uint8_t turn[] = {4, 5, 6, 5, 4, 0, 7, 8, 9, 8, 7};
  static const uint16_t turn_ms[] = {120, 120, 400, 120, 120, 500,
                                    120, 120, 400, 120, 120};
  /* Only 苔苔 has generated frames; the other two stay still and rely on the
   * tap ornament for feedback.
   */
  bool paused = ebadge_pages_active() || view.menu || view.character != 0;
  if (paused != view.motion_paused)
    {
      reset_motion(now);
      view.motion_paused = paused;
    }
  if (paused) return;
  if (tapped || (!view.motion_clip && (int32_t)(now - view.motion_due) >= 0))
    {
      view.motion_clip = (tapped || (view.random & 3u) != 0) ? 1 : 2;
      view.motion_started = now;
    }
  if (!view.motion_clip) return;
  const uint8_t *sequence = view.motion_clip == 1 ? blink : turn;
  const uint16_t *durations = view.motion_clip == 1 ? blink_ms : turn_ms;
  unsigned int count = view.motion_clip == 1 ? sizeof(blink) : sizeof(turn);
  uint32_t elapsed = now - view.motion_started;
  for (unsigned int i = 0; i < count; i++)
    {
      if (elapsed < durations[i])
        {
          show_portrait(&ebadge_motion_frames[sequence[i]]);
          return;
        }
      elapsed -= durations[i];
    }
  reset_motion(now);
}

/* ------------------------------------------------------------ persistence */

static void mark_dirty(void)
{
  view.dirty = true;
  view.dirty_at = lv_tick_get();
}

static void persist(void)
{
  struct ebadge_settings out;
  memset(&out, 0, sizeof(out));
  out.character = (uint8_t)view.character;
  out.paper = (uint8_t)view.paper;
  ebadge_pages_export(&out);
  if (ebadge_store_save(&out)) view.dirty = false;
}

/* ------------------------------------------------------------- home paint */

static void paint_character(void)
{
  const struct ebadge_character *character = ebadge_theme_character(view.character);
  lv_label_set_text(view.name, character->name);
  lv_label_set_text(view.caption, character->line);
  lv_obj_set_pos(view.portrait, view.origin_x + px(character->portrait_x),
                 view.origin_y + px(character->portrait_y));
  lv_obj_set_size(view.portrait, px(character->portrait_size),
                  px(character->portrait_size));
  view.displayed = NULL;
  reset_motion(lv_tick_get());
  paint_dots();
}

/* ------------------------------------------------------------------ menu */

/* Menu order is fixed here rather than derived from enum arithmetic. */
static const enum ebadge_page menu_pages[] = {
  EBADGE_PAGE_CLOCK, EBADGE_PAGE_MESSAGE, EBADGE_PAGE_ANNIVERSARY,
  EBADGE_PAGE_APPEARANCE
};
#define MENU_ITEM_COUNT ((int)(sizeof(menu_pages) / sizeof(menu_pages[0])))

static void menu_close(void);

static void menu_item_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  if (index < 0 || index >= MENU_ITEM_COUNT) return;
  menu_close();
  ebadge_pages_show(menu_pages[index]);
}

static void menu_backdrop_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  /* Events bubble, so ignore clicks that came from a menu button. */
  if (lv_event_get_target(event) != lv_event_get_current_target(event)) return;
  menu_close();
}

static lv_obj_t *flat(lv_obj_t *parent, int x, int y, int w, int h,
                      uint32_t color, int radius)
{
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_size(obj, px(w), px(h));
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, px(radius), 0);
  return obj;
}

static lv_obj_t *menu_button(lv_obj_t *sheet, const char *text, int x, int y,
                             int w, int h, int index)
{
  lv_obj_t *button = flat(sheet, x, y, w, h, view.palette.soft, 18);
  if (index >= 0)
    lv_obj_add_event_cb(button, menu_item_cb, LV_EVENT_CLICKED,
                        (void *)(intptr_t)index);
  else
    lv_obj_add_event_cb(button, menu_backdrop_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_t *label = lv_label_create(button);
  lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(view.palette.ink), 0);
  lv_obj_center(label);
  return button;
}

static void menu_open(void)
{
  if (!view.root || view.menu) return;
  view.pressed = false;
  view.hold_fired = false;

  view.menu = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.menu);
  lv_obj_remove_flag(view.menu, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(view.menu, lv_display_get_horizontal_resolution(
                                 lv_display_get_default()),
                  lv_display_get_vertical_resolution(lv_display_get_default()));
  lv_obj_set_pos(view.menu, 0, 0);
  lv_obj_set_style_bg_color(view.menu, lv_color_hex(view.palette.ink), 0);
  lv_obj_set_style_bg_opa(view.menu, LV_OPA_50, 0);
  lv_obj_add_event_cb(view.menu, menu_backdrop_cb, LV_EVENT_CLICKED, NULL);

  const int sheet_h = 250;
  const int sheet_top = CANVAS_H - sheet_h;
  lv_obj_t *sheet = flat(view.menu, 0, 0, CANVAS_W, sheet_h,
                         view.palette.paper, 26);
  /* The base position is the resting place; the slide animation only drives
   * translate_y, so the two must not both carry the offset.
   */
  lv_obj_set_pos(sheet, view.origin_x, view.origin_y + px(sheet_top));

  lv_obj_t *title = lv_label_create(sheet);
  lv_obj_remove_flag(title, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(title, "随身小册");
  lv_obj_set_pos(title, px(24), px(18));
  lv_obj_set_style_text_font(title, &ebadge_font_title_26, 0);
  lv_obj_set_style_text_color(title, lv_color_hex(view.palette.ink), 0);

  menu_button(sheet, "收起", 292, 16, 78, 42, -1);
  menu_button(sheet, "时钟", 20, 68, 165, 64, 0);
  menu_button(sheet, "文字应援", 205, 68, 165, 64, 1);
  menu_button(sheet, "纪念日", 20, 144, 165, 64, 2);
  menu_button(sheet, "配色", 205, 144, 165, 64, 3);

  lv_obj_t *hint = lv_label_create(sheet);
  lv_obj_remove_flag(hint, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(hint, "点上方空白处也可以收起");
  lv_obj_set_width(hint, px(342));
  lv_obj_set_pos(hint, px(24), px(220));
  lv_obj_set_style_text_align(hint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(hint, lv_color_hex(view.palette.muted), 0);

  /* Fade the backdrop in while the sheet slides up from the bottom edge. */
  lv_obj_set_style_translate_y(sheet, px(250), 0);
  lv_anim_t slide;
  lv_anim_init(&slide);
  lv_anim_set_var(&slide, sheet);
  lv_anim_set_exec_cb(&slide, anim_translate_y_cb);
  lv_anim_set_values(&slide, px(250), 0);
  lv_anim_set_duration(&slide, 220);
  lv_anim_set_path_cb(&slide, lv_anim_path_ease_out);
  lv_anim_start(&slide);

  lv_obj_set_style_opa(view.menu, LV_OPA_TRANSP, 0);
  lv_anim_t fade_in;
  lv_anim_init(&fade_in);
  lv_anim_set_var(&fade_in, view.menu);
  lv_anim_set_exec_cb(&fade_in, anim_opa_cb);
  lv_anim_set_values(&fade_in, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_duration(&fade_in, 200);
  lv_anim_start(&fade_in);
}

static void menu_close(void)
{
  if (!view.menu) return;
  lv_obj_t *menu = view.menu;
  view.menu = NULL;
  /* Deleting the container also drops the animations attached to it and its
   * children (lv_obj_delete calls lv_anim_delete).
   */
  lv_obj_delete(menu);
}

/* --------------------------------------------------------------- gestures */

static void switch_by(int delta)
{
  struct ebadge_event action = {0};
  action.kind = delta > 0 ? EBADGE_NEXT : EBADGE_PREV;
  ebadge_controller_post(&view.controller, action);
}

static void respond(int32_t display_x, int32_t display_y)
{
  struct ebadge_event action = {0};
  action.kind = EBADGE_TAP;
  ebadge_controller_post(&view.controller, action);
  ebadge_ornament_play(ebadge_theme_character(view.character)->motif,
                       to_canvas_x(display_x), to_canvas_y(display_y),
                       view.palette.gold);
}

static void gesture(lv_event_t *event)
{
  /* Only raw touches on the home background. Buttons, the sheet and the page
   * widgets handle their own events.
   */
  if (lv_event_get_target(event) != lv_event_get_current_target(event)) return;
  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_PRESS_LOST)
    {
      view.pressed = false;
      return;
    }
  if (code != LV_EVENT_PRESSED && code != LV_EVENT_PRESSING &&
      code != LV_EVENT_RELEASED) return;
  lv_indev_t *input = lv_event_get_indev(event);
  if (!input) return;
  lv_point_t point;
  lv_indev_get_point(input, &point);

  if (code == LV_EVENT_PRESSED)
    {
      view.press = point;
      view.press_ms = lv_tick_get();
      view.max_dist_sq = 0;
      view.pressed = true;
      view.hold_fired = false;
      view.random ^= view.press_ms;
      if (!view.random) view.random = 0x9e3779b9u;
      return;
    }
  if (!view.pressed) return;

  int32_t dx = point.x - view.press.x;
  int32_t dy = point.y - view.press.y;
  int32_t dist_sq = (int32_t)((int64_t)dx * dx + (int64_t)dy * dy);
  if (dist_sq > view.max_dist_sq) view.max_dist_sq = dist_sq;
  if (code != LV_EVENT_RELEASED) return;

  view.pressed = false;
  if (view.hold_fired)
    {
      /* The hold already opened the menu; this release is consumed. */
      view.hold_fired = false;
      return;
    }
  if (view.menu) return;

  int32_t adx = abs(dx), ady = abs(dy);
  bool horizontal = adx >= px(SWIPE_MIN) &&
                    (int64_t)adx * DOMINANCE_DEN >= (int64_t)ady * DOMINANCE_NUM;
  bool upward = dy <= -px(MENU_SWIPE_MIN) &&
                (int64_t)ady * DOMINANCE_DEN >= (int64_t)adx * DOMINANCE_NUM;
  bool tap = view.max_dist_sq <= px(SLOP) * px(SLOP);

  if (ebadge_pages_active())
    {
      if (tap && !horizontal && !upward) ebadge_pages_tap();
      return;
    }
  if (upward)
    {
      menu_open();
      return;
    }
  if (horizontal)
    {
      switch_by(dx < 0 ? 1 : -1);
      return;
    }
  if (tap) respond(view.press.x, view.press.y);
}

/* Slide the incoming companion in from the side the swipe came from, so the
 * change reads as movement rather than a hard cut.
 */
static void slide_portrait(int direction)
{
  int32_t offset = px(70) * (direction >= 0 ? 1 : -1);
  lv_obj_set_style_translate_x(view.portrait, offset, 0);
  lv_obj_set_style_opa(view.portrait, LV_OPA_TRANSP, 0);

  lv_anim_t slide;
  lv_anim_init(&slide);
  lv_anim_set_var(&slide, view.portrait);
  lv_anim_set_exec_cb(&slide, anim_translate_x_cb);
  lv_anim_set_values(&slide, offset, 0);
  lv_anim_set_duration(&slide, 240);
  lv_anim_set_path_cb(&slide, lv_anim_path_ease_out);
  lv_anim_delete(view.portrait, anim_translate_x_cb);
  lv_anim_start(&slide);

  lv_anim_t fade;
  lv_anim_init(&fade);
  lv_anim_set_var(&fade, view.portrait);
  lv_anim_set_exec_cb(&fade, anim_opa_cb);
  lv_anim_set_values(&fade, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_duration(&fade, 240);
  lv_anim_delete(view.portrait, anim_opa_cb);
  lv_anim_start(&fade);
}

/* ------------------------------------------------------------------ frame */

static void frame(lv_timer_t *timer)
{
  (void)timer;
  uint32_t now = lv_tick_get();
  ebadge_pages_tick(now);
  ebadge_controller_step(&view.controller, now);
  struct ebadge_state *state = &view.controller.state;

  if (view.painted_character != state->character)
    {
      int delta = ((int)state->character - (int)view.painted_character +
                   EBADGE_CHARACTER_COUNT) % EBADGE_CHARACTER_COUNT;
      view.character = state->character;
      view.painted_character = state->character;
      apply_theme();
      paint_character();
      /* Wrap-around from 月月 back to 苔苔 is a backward step. */
      slide_portrait(delta == 1 ? 1 : -1);
    }

  bool became_reacting = state->character_state == EBADGE_REACTING &&
                         view.painted_state != EBADGE_REACTING;
  view.painted_state = state->character_state;
  animate_portrait(now, became_reacting);

  if (view.pressed && !view.hold_fired && !view.menu &&
      !ebadge_pages_active() && view.max_dist_sq <= px(SLOP) * px(SLOP) &&
      (uint32_t)(now - view.press_ms) >= HOLD_MS)
    {
      view.hold_fired = true;
      menu_open();
    }

  if (view.dirty && (uint32_t)(now - view.dirty_at) >= SAVE_DEBOUNCE_MS)
    persist();
}

/* ------------------------------------------------------------------- hint */

static void hint_done_cb(lv_anim_t *anim)
{
  lv_obj_t *pill = (lv_obj_t *)anim->var;
  if (pill) lv_obj_add_flag(pill, LV_OBJ_FLAG_HIDDEN);
}

static void build_hint(void)
{
  view.hint_pill = flat(view.root, 30, 14, 330, 62, view.palette.soft, 18);
  lv_obj_set_pos(view.hint_pill, view.origin_x + px(30),
                 view.origin_y + px(14));
  lv_obj_set_style_bg_opa(view.hint_pill, LV_OPA_80, 0);
  /* The hint must never swallow a home gesture. */
  lv_obj_remove_flag(view.hint_pill, LV_OBJ_FLAG_CLICKABLE);
  view.hint_label = lv_label_create(view.hint_pill);
  lv_obj_remove_flag(view.hint_label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(view.hint_label, "左右滑动换伙伴\n上滑或长按打开小册");
  lv_obj_set_width(view.hint_label, px(300));
  lv_obj_set_style_text_align(view.hint_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(view.hint_label);

  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, view.hint_pill);
  lv_anim_set_exec_cb(&anim, anim_opa_cb);
  lv_anim_set_values(&anim, LV_OPA_COVER, LV_OPA_TRANSP);
  lv_anim_set_delay(&anim, HINT_MS);
  lv_anim_set_duration(&anim, HINT_FADE_MS);
  lv_anim_set_ready_cb(&anim, hint_done_cb);
  lv_anim_start(&anim);
}

/* ------------------------------------------------------------------ public */

struct ebadge_palette ebadge_view_palette(void) { return view.palette; }
unsigned int ebadge_view_character(void) { return view.character; }
enum ebadge_paper ebadge_view_paper(void) { return view.paper; }
const char *ebadge_view_storage_status(void) { return ebadge_store_status(); }

void ebadge_view_touch_settings(void) { mark_dirty(); }

void ebadge_view_set_character(unsigned int index)
{
  if (index >= EBADGE_CHARACTER_COUNT || index == view.character) return;
  view.controller.state.character = (uint8_t)index;
  view.controller.state.generation++;
  if (!view.controller.state.generation) view.controller.state.generation = 1;
  view.controller.state.animation_token++;
  if (!view.controller.state.animation_token)
    view.controller.state.animation_token = 1;
  view.controller.state.character_state = EBADGE_IDLE;
  mark_dirty();
}

void ebadge_view_set_paper(enum ebadge_paper paper)
{
  if (paper >= EBADGE_PAPER_COUNT || paper == view.paper) return;
  view.paper = paper;
  apply_theme();
  mark_dirty();
}

void ebadge_view_home(void)
{
  ebadge_pages_show(EBADGE_PAGE_HOME);
  apply_theme();
  paint_character();
}

bool ebadge_view_open(void)
{
  if (view.opened) return false;
  memset(&view, 0, sizeof(view));

  int32_t w = lv_display_get_horizontal_resolution(lv_display_get_default());
  int32_t h = lv_display_get_vertical_resolution(lv_display_get_default());
  if (w < 100 || h < 100) return false;
  view.scale = w * 1000 / CANVAS_W;
  if (h * 1000 / CANVAS_H < view.scale) view.scale = h * 1000 / CANVAS_H;
  view.origin_x = (w - px(CANVAS_W)) / 2;
  view.origin_y = (h - px(CANVAS_H)) / 2;
  view.random = lv_tick_get() ^ 0x9e3779b9u;
  if (!view.random) view.random = 1;

  /* Restore first so the first painted frame is already the saved state. */
  ebadge_store_begin();
  struct ebadge_settings saved;
  if (ebadge_store_load(&saved))
    {
      view.character = saved.character;
      view.paper = (enum ebadge_paper)saved.paper;
      ebadge_pages_import(&saved);
    }

  ebadge_controller_init(&view.controller, lv_tick_get());
  view.controller.state.character = (uint8_t)view.character;
  view.painted_character = view.character;
  view.palette = ebadge_theme_resolve(view.character, view.paper);

  view.root = lv_obj_create(lv_screen_active());
  lv_obj_remove_style_all(view.root);
  lv_obj_set_style_text_font(view.root, &ebadge_font_zh_20, 0);
  lv_obj_set_size(view.root, w, h);
  lv_obj_set_style_bg_opa(view.root, LV_OPA_COVER, 0);
  lv_obj_add_flag(view.root, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(view.root, LV_OBJ_FLAG_SCROLLABLE);

  const struct ebadge_character *character = ebadge_theme_character(view.character);
  view.portrait = lv_image_create(view.root);
  lv_obj_remove_flag(view.portrait, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_pos(view.portrait, view.origin_x + px(character->portrait_x),
                 view.origin_y + px(character->portrait_y));
  lv_obj_set_size(view.portrait, px(character->portrait_size),
                  px(character->portrait_size));
  /* Source and destination are square, so stretching preserves aspect. */
  lv_image_set_inner_align(view.portrait, LV_IMAGE_ALIGN_STRETCH);

  /* Bottom fade so the name and caption stay readable over the artwork. */
  view.fade = lv_obj_create(view.root);
  lv_obj_remove_style_all(view.fade);
  lv_obj_remove_flag(view.fade, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(view.fade, view.origin_x + px(0), view.origin_y + px(300));
  lv_obj_set_size(view.fade, px(CANVAS_W), px(150));
  lv_obj_set_style_bg_opa(view.fade, LV_OPA_TRANSP, 0);
  lv_obj_set_style_bg_grad_dir(view.fade, LV_GRAD_DIR_VER, 0);
  lv_obj_set_style_bg_grad_opa(view.fade, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_main_stop(view.fade, 0, 0);
  lv_obj_set_style_bg_grad_stop(view.fade, 255, 0);

  view.name = lv_label_create(view.root);
  lv_obj_remove_flag(view.name, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_width(view.name, px(CANVAS_W));
  lv_obj_set_pos(view.name, view.origin_x, view.origin_y + px(362));
  lv_obj_set_style_text_align(view.name, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(view.name, &ebadge_font_title_26, 0);
  lv_obj_set_style_text_letter_space(view.name, px(4), 0);

  view.caption = lv_label_create(view.root);
  lv_obj_remove_flag(view.caption, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_width(view.caption, px(CANVAS_W));
  lv_obj_set_pos(view.caption, view.origin_x, view.origin_y + px(400));
  lv_obj_set_style_text_align(view.caption, LV_TEXT_ALIGN_CENTER, 0);

  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    {
      view.dots[i] = flat(view.root, 176 + i * 16, 430, 6, 6,
                          view.palette.soft, 3);
      /* Children of the root carry the letterbox origin; children of a
       * container do not, because the container already sits at the origin.
       */
      lv_obj_set_pos(view.dots[i], view.origin_x + px(176 + i * 16),
                     view.origin_y + px(430));
      lv_obj_remove_flag(view.dots[i], LV_OBJ_FLAG_CLICKABLE);
    }

  /* The hint is created before the page panel so the panel covers it: the
   * z-order alone stops the toast from floating over an open page.
   */
  build_hint();
  if (!ebadge_ornament_open(view.root, view.scale, view.origin_x, view.origin_y))
    {
      ebadge_view_close();
      return false;
    }
  ebadge_pages_open(view.root, view.scale, view.origin_x, view.origin_y);
  apply_theme();
  paint_character();
  lv_obj_add_event_cb(view.root, gesture, LV_EVENT_ALL, NULL);

  view.dirty = false;
  view.opened = true;
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
  view.timer = NULL;
  /* Read the settings out while the page widgets still exist, then tear down. */
  if (view.opened && view.dirty) persist();
  menu_close();
  ebadge_ornament_close();
  ebadge_pages_close();
  if (view.root) lv_obj_delete(view.root);
  ebadge_store_end();
  memset(&view, 0, sizeof(view));
}
