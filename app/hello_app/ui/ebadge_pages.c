/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_pages.h"
#include "ebadge_view.h"
#include "ebadge_theme.h"
#include "ebadge_deco.h"
#include "ebadge_display.h"
#include "../core/ebadge_imu.h"
#include "ebadge_date_editor.h"
#include "ebadge_text_editor.h"
#include "../core/ebadge_calendar.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

LV_FONT_DECLARE(ebadge_font_zh_20);
LV_FONT_DECLARE(ebadge_font_title_26);

#define CANVAS_W 390
#define CANVAS_H 450

/* Illustrated header: the companion's portrait chip, and the bubble it speaks
 * from. The bubble's tail points left at the portrait.
 */
#define AVATAR_X 24
#define AVATAR_Y 88
#define AVATAR_SIZE 104
#define BUBBLE_X 140
#define BUBBLE_Y 88
#define BUBBLE_W 226
#define BUBBLE_H 104

/* Content card and the furniture below it. */
#define CARD_X 24
#define CARD_Y 208
#define CARD_W 342
#define CARD_H 148
#define TAGLINE_Y 366
#define ACTION_X 60
#define ACTION_Y 398
#define ACTION_W 270
#define ACTION_H 48

static struct
{
  char message[EBADGE_STORE_MESSAGE];
  int year, month, day, zone;
  /* Set when the command line supplied the value explicitly. An explicit
   * argument is a deliberate override and wins over stored settings.
   */
  bool explicit_message, explicit_date, explicit_zone;
} config;

static struct
{
  lv_obj_t *panel;
  lv_obj_t *back, *back_label;
  lv_obj_t *title;
  lv_obj_t *avatar;
  lv_obj_t *bubble, *bubble_label;
  lv_obj_t *card, *card_main, *card_sub;
  lv_obj_t *sparkle[2];
  lv_obj_t *tagline;
  lv_obj_t *action, *action_label;
  lv_obj_t *choices, *choice_heading[2], *note;
  lv_obj_t *character[EBADGE_CHARACTER_COUNT];
  lv_obj_t *paper[EBADGE_PAPER_COUNT];
  lv_obj_t *storage;
  lv_obj_t *display;
  lv_obj_t *imu;

  int scale, origin_x, origin_y;
  enum ebadge_page page;
  uint32_t last_tick;
  bool dirty;
} pages;

static int px(int n) { return n * pages.scale / 1000; }

/* The style setters take a selector argument, so they cannot be cast into an
 * lv_anim_exec_xcb_t callback. */
static void anim_opa_cb(void *var, int32_t value)
{
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

static void anim_translate_y_cb(void *var, int32_t value)
{
  lv_obj_set_style_translate_y((lv_obj_t *)var, (int32_t)value, 0);
}

/* ------------------------------------------------------------ configure */

bool ebadge_pages_configure(int argc, char **argv)
{
  memset(&config, 0, sizeof(config));
  config.zone = 480;
  strcpy(config.message, "让喜欢发光，每一天都与你相伴。");
  for (int i = 1; i < argc; i++)
    {
      if (i + 1 >= argc) return false;
      const char *key = argv[i++];
      const char *value = argv[i];
      if (!strcmp(key, "--message"))
        {
          size_t length = strlen(value);
          if (!length || length >= sizeof(config.message)) return false;
          memcpy(config.message, value, length + 1);
          config.explicit_message = true;
        }
      else if (!strcmp(key, "--date"))
        {
          int end = 0;
          if (sscanf(value, "%d-%d-%d%n", &config.year, &config.month,
                     &config.day, &end) != 3 || value[end] != '\0') return false;
          if (config.year < EBADGE_YEAR_MIN || config.year > EBADGE_YEAR_MAX ||
              !ebadge_date_valid(config.year, config.month, config.day))
            return false;
          config.explicit_date = true;
        }
      else if (!strcmp(key, "--tz-minutes"))
        {
          char *end;
          long zone = strtol(value, &end, 10);
          if (end == value || *end || zone < -720 || zone > 840) return false;
          config.zone = (int)zone;
          config.explicit_zone = true;
        }
      else return false;
    }
  return true;
}

void ebadge_pages_export(struct ebadge_settings *out)
{
  size_t length = strnlen(config.message, sizeof(config.message));
  if (length >= sizeof(config.message)) length = sizeof(config.message) - 1;
  memcpy(out->message, config.message, length);
  out->message[length] = '\0';
  out->year = (int16_t)config.year;
  out->month = (int8_t)config.month;
  out->day = (int8_t)config.day;
  out->zone = (int16_t)config.zone;
}

void ebadge_pages_import(const struct ebadge_settings *in)
{
  if (!in) return;
  size_t length = strnlen(in->message, EBADGE_STORE_MESSAGE);
  if (!config.explicit_message && length && length < sizeof(config.message))
    memcpy(config.message, in->message, length + 1);
  if (!config.explicit_date && in->year)
    {
      config.year = in->year;
      config.month = in->month;
      config.day = in->day;
    }
  if (!config.explicit_zone && in->zone >= -720 && in->zone <= 840)
    config.zone = in->zone;
}

/* -------------------------------------------------------------- widgets */

static void set_hidden(lv_obj_t *obj, bool hidden)
{
  if (!obj) return;
  if (hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *text(lv_obj_t *parent, const char *value, int x, int y, int w,
                      uint32_t color)
{
  lv_obj_t *obj = lv_label_create(parent);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(obj, value);
  lv_obj_set_width(obj, px(w));
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
  return obj;
}

static lv_obj_t *button(lv_obj_t *parent, const char *value, int x, int y,
                        int w, int h, lv_event_cb_t cb, void *user_data)
{
  lv_obj_t *obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(obj, px(x), px(y));
  lv_obj_set_size(obj, px(w), px(h));
  lv_obj_set_style_radius(obj, px(18), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  /* Press feedback: the panel is resistive and has no haptics, so the button
   * itself has to acknowledge the touch. Opacity alone is easy to miss, hence
   * the slight shrink as well.
   */
  lv_obj_set_style_opa(obj, LV_OPA_70, LV_STATE_PRESSED);
  lv_obj_set_style_transform_scale(obj, 240, LV_STATE_PRESSED);
  lv_obj_add_event_cb(obj, cb, LV_EVENT_CLICKED, user_data);
  lv_obj_t *label = lv_label_create(obj);
  lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(label, value);
  lv_obj_center(label);
  return obj;
}

static lv_obj_t *button_label(lv_obj_t *obj)
{
  return lv_obj_get_child(obj, 0);
}

/* Rounded card with centred column layout, used for the page's main value. */
static lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h,
                      uint32_t fill, enum ebadge_deco_tail tail)
{
  lv_obj_t *obj = ebadge_deco_bubble(parent, px(x), px(y), px(w), px(h), fill,
                                     tail);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER,
                        LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_all(obj, px(10), 0);
  lv_obj_set_style_pad_row(obj, px(6), 0);
  return obj;
}

/* ------------------------------------------------------------ callbacks */

static void refresh(void)
{
  pages.dirty = true;
  ebadge_pages_tick(lv_tick_get());
}

static void back_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  ebadge_view_home();
}

static void text_committed(const char *value)
{
  size_t length = strlen(value);
  if (!length || length >= sizeof(config.message)) return;
  memcpy(config.message, value, length + 1);
  ebadge_view_touch_settings();
  refresh();
}

static void date_committed(int year, int month, int day)
{
  config.year = year;
  config.month = month;
  config.day = day;
  ebadge_view_touch_settings();
  refresh();
}

static void action_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  if (pages.page == EBADGE_PAGE_MESSAGE)
    ebadge_text_editor_open(pages.panel, config.message, text_committed);
  else if (pages.page == EBADGE_PAGE_ANNIVERSARY)
    {
      int year = config.year, month = config.month, day = config.day;
      if (!year)
        {
          /* Initial editor draft only: never silently save a guessed date. */
          year = 2026;
          month = day = 1;
          time_t now = time(NULL) + config.zone * 60;
          struct tm local;
          if (gmtime_r(&now, &local) && local.tm_year >= 124 &&
              local.tm_year <= 199)
            {
              year = local.tm_year + 1900;
              month = local.tm_mon + 1;
              day = local.tm_mday;
            }
        }
      ebadge_date_editor_open(pages.panel, year, month, day, date_committed);
    }
}

static void character_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  ebadge_view_set_character((unsigned int)index);
  refresh();
}

static void paper_cb(lv_event_t *event)
{
  if (lv_event_get_code(event) != LV_EVENT_CLICKED) return;
  int index = (int)(intptr_t)lv_event_get_user_data(event);
  ebadge_view_set_paper((enum ebadge_paper)index);
  refresh();
}

/* ----------------------------------------------------------- page paint */

/* Bring back the illustrated header, card, sparkles and tagline; the caller
 * decides whether this page has a primary action button.
 */
static void layout_text_page(bool with_action)
{
  set_hidden(pages.avatar, false);
  set_hidden(pages.bubble, false);
  set_hidden(pages.card, false);
  set_hidden(pages.sparkle[0], false);
  set_hidden(pages.sparkle[1], false);
  set_hidden(pages.tagline, false);
  set_hidden(pages.choices, true);
  set_hidden(pages.action, !with_action);
}

/* The clock and the day count read best as large digits; the message is a
 * native 20 px Chinese label that may need to scroll.
 */
static void main_value_style(bool big)
{
  if (big)
    {
      lv_obj_set_style_text_font(pages.card_main, &lv_font_montserrat_48, 0);
      lv_label_set_long_mode(pages.card_main, LV_LABEL_LONG_WRAP);
      lv_obj_set_size(pages.card_main, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    }
  else
    {
      lv_obj_set_style_text_font(pages.card_main, &ebadge_font_zh_20, 0);
      lv_label_set_long_mode(pages.card_main, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_size(pages.card_main, px(300), px(34));
    }
}

/* Current local time, or false when the clock has never been set. */
static bool local_now(struct tm *out)
{
  time_t utc = time(NULL);
  if (utc == (time_t)-1) return false;
  time_t shifted = utc + (time_t)config.zone * 60;
  if (!gmtime_r(&shifted, out)) return false;
  return out->tm_year >= 124 && out->tm_year <= 199;
}

static void paint_clock(void)
{
  unsigned int character = ebadge_view_character();
  struct tm local;
  bool valid = local_now(&local);

  lv_label_set_text(pages.bubble_label,
                    valid ? ebadge_theme_greeting(character,
                              ebadge_theme_daypart(local.tm_hour))
                          : "我还没对上时间，帮我同步一下好吗。");

  main_value_style(true);
  set_hidden(pages.card_sub, false);
  if (!valid)
    {
      lv_label_set_text(pages.card_main, "--:--");
      lv_label_set_text(pages.card_sub, "用电脑串口同步后就能对上啦");
    }
  else
    {
      static const char *const week[] = {"周日", "周一", "周二", "周三",
                                         "周四", "周五", "周六"};
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "%02d:%02d", local.tm_hour,
               local.tm_min);
      lv_label_set_text(pages.card_main, buffer);
      snprintf(buffer, sizeof(buffer), "%04d年%02d月%02d日 %s",
               local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
               week[local.tm_wday % 7]);
      lv_label_set_text(pages.card_sub, buffer);
    }
  lv_label_set_text(pages.tagline, ebadge_theme_tagline(character));
}

static void paint_message(void)
{
  lv_label_set_text(pages.bubble_label, "把想说的话，写在下面吧。");
  main_value_style(false);
  set_hidden(pages.card_sub, true);
  lv_label_set_text(pages.card_main, config.message);
  lv_label_set_text(pages.tagline,
                    ebadge_theme_tagline(ebadge_view_character()));
  lv_label_set_text(button_label(pages.action), "写一句新的");
}

static void paint_anniversary(void)
{
  struct tm local;
  bool valid = local_now(&local);
  bool set = config.year != 0;

  if (!set)
    lv_label_set_text(pages.bubble_label, "留一个位置，给特别的那一天吧。");
  else if (!valid)
    lv_label_set_text(pages.bubble_label, "先帮我对一下时间，我才数得清日子。");
  else
    lv_label_set_text(pages.bubble_label, "和你一起数着的日子，我都记得。");

  main_value_style(true);
  set_hidden(pages.card_sub, false);
  if (!set)
    {
      lv_label_set_text(pages.card_main, "--");
      lv_label_set_text(pages.card_sub, "还没有设置纪念日");
    }
  else if (!valid)
    {
      lv_label_set_text(pages.card_main, "--");
      lv_label_set_text(pages.card_sub, "时间尚未校准");
    }
  else
    {
      long days = ebadge_days_between(
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        config.year, config.month, config.day);
      char buffer[40];
      /* The big value is montserrat, which has no CJK glyphs and no fallback
       * face, so it must stay numeric. Every Chinese word goes in the
       * supporting line, which uses the 20 px Chinese font.
       */
      snprintf(buffer, sizeof(buffer), "%ld", days > 0 ? days : -days);
      lv_label_set_text(pages.card_main, buffer);
      snprintf(buffer, sizeof(buffer), "%s %04d年%02d月%02d日",
               !days ? "就是今天 ·" : (days > 0 ? "还有" : "已经走过"),
               config.year, config.month, config.day);
      lv_label_set_text(pages.card_sub, buffer);
    }
  lv_label_set_text(pages.tagline,
                    ebadge_theme_tagline(ebadge_view_character()));
  lv_label_set_text(button_label(pages.action), "设置纪念日");
}

static void paint_appearance(void)
{
  struct ebadge_palette palette = ebadge_view_palette();
  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    {
      const struct ebadge_character *character = ebadge_theme_character(i);
      bool chosen = i == (int)ebadge_view_character();
      lv_label_set_text(button_label(pages.character[i]), character->name);
      lv_obj_set_style_bg_color(pages.character[i],
        lv_color_hex(chosen ? palette.gold : palette.soft), 0);
      lv_obj_set_style_text_color(button_label(pages.character[i]),
        lv_color_hex(chosen ? palette.paper : palette.ink), 0);
    }
  for (int i = 0; i < EBADGE_PAPER_COUNT; i++)
    {
      bool chosen = i == (int)ebadge_view_paper();
      lv_label_set_text(button_label(pages.paper[i]),
                        ebadge_theme_paper_name((enum ebadge_paper)i));
      lv_obj_set_style_bg_color(pages.paper[i],
        lv_color_hex(chosen ? palette.gold : palette.soft), 0);
      lv_obj_set_style_text_color(button_label(pages.paper[i]),
        lv_color_hex(chosen ? palette.paper : palette.ink), 0);
    }
  lv_label_set_text(pages.note,
                    ebadge_theme_paper_note(ebadge_view_paper()));
  lv_label_set_text(pages.storage, ebadge_view_storage_status());
  lv_label_set_text(pages.display, ebadge_display_status());
  lv_label_set_text(pages.imu, ebadge_imu_status());
}

/* ---------------------------------------------------------------- public */

void ebadge_pages_open(lv_obj_t *root, int scale, int x, int y)
{
  memset(&pages, 0, sizeof(pages));
  pages.scale = scale;
  pages.origin_x = x;
  pages.origin_y = y;
  pages.page = EBADGE_PAGE_HOME;

  pages.panel = lv_obj_create(root);
  lv_obj_remove_style_all(pages.panel);
  /* Left clickable: raw touches on the page background reach the home gesture
   * handler only when no widget claims them, which is what we want for taps.
   */
  lv_obj_remove_flag(pages.panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(pages.panel, x, y);
  lv_obj_set_size(pages.panel, px(CANVAS_W), px(CANVAS_H));
  lv_obj_set_style_bg_opa(pages.panel, LV_OPA_COVER, 0);

  pages.back = button(pages.panel, "返回", 20, 18, 84, 46, back_cb, NULL);
  pages.back_label = button_label(pages.back);

  pages.title = lv_label_create(pages.panel);
  lv_obj_remove_flag(pages.title, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(pages.title, "");
  lv_obj_set_pos(pages.title, px(116), px(24));
  lv_obj_set_style_text_font(pages.title, &ebadge_font_title_26, 0);

  /* Illustrated header. Created before the bubble so the tail, which is the
   * bubble's first child, stays under the greeting text.
   */
  pages.avatar = ebadge_deco_avatar(pages.panel, px(AVATAR_X), px(AVATAR_Y),
                                    px(AVATAR_SIZE), 0, 0x000000, 0x000000);
  pages.bubble = ebadge_deco_bubble(pages.panel, px(BUBBLE_X), px(BUBBLE_Y),
                                    px(BUBBLE_W), px(BUBBLE_H), 0x000000,
                                    EBADGE_DECO_TAIL_LEFT);
  pages.bubble_label = lv_label_create(pages.bubble);
  lv_obj_remove_flag(pages.bubble_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_width(pages.bubble_label, px(BUBBLE_W - 40));
  lv_label_set_long_mode(pages.bubble_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(pages.bubble_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_center(pages.bubble_label);

  pages.card = card(pages.panel, CARD_X, CARD_Y, CARD_W, CARD_H, 0x000000,
                    EBADGE_DECO_TAIL_NONE);
  pages.card_main = lv_label_create(pages.card);
  lv_obj_remove_flag(pages.card_main, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(pages.card_main, "");
  lv_obj_set_style_text_align(pages.card_main, LV_TEXT_ALIGN_CENTER, 0);
  pages.card_sub = lv_label_create(pages.card);
  lv_obj_remove_flag(pages.card_sub, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_width(pages.card_sub, px(CARD_W - 40));
  lv_label_set_long_mode(pages.card_sub, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_align(pages.card_sub, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(pages.card_sub, &ebadge_font_zh_20, 0);

  pages.sparkle[0] = ebadge_deco_sparkle(pages.panel, px(6), px(196), px(22),
                                         0x000000);
  pages.sparkle[1] = ebadge_deco_sparkle(pages.panel, px(362), px(344), px(18),
                                         0x000000);

  pages.tagline = text(pages.panel, "", 24, TAGLINE_Y, 342, 0x000000);

  pages.action = button(pages.panel, "", ACTION_X, ACTION_Y, ACTION_W, ACTION_H,
                        action_cb, NULL);
  pages.action_label = button_label(pages.action);

  /* Appearance page owns its own controls. */
  pages.choices = lv_obj_create(pages.panel);
  lv_obj_remove_style_all(pages.choices);
  lv_obj_remove_flag(pages.choices,
                     LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_pos(pages.choices, 0, 0);
  lv_obj_set_size(pages.choices, px(CANVAS_W), px(CANVAS_H));
  lv_obj_set_style_bg_opa(pages.choices, LV_OPA_TRANSP, 0);

  pages.choice_heading[0] = text(pages.choices, "身边的角色", 24, 96, 342,
                                 0x000000);
  lv_obj_set_style_text_align(pages.choice_heading[0], LV_TEXT_ALIGN_LEFT, 0);
  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    pages.character[i] = button(pages.choices, "", 24 + i * 116, 128, 110, 54,
                                character_cb, (void *)(intptr_t)i);

  pages.choice_heading[1] = text(pages.choices, "画面的底色", 24, 202, 342,
                                 0x000000);
  lv_obj_set_style_text_align(pages.choice_heading[1], LV_TEXT_ALIGN_LEFT, 0);
  for (int i = 0; i < EBADGE_PAPER_COUNT; i++)
    pages.paper[i] = button(pages.choices, "", 24 + i * 116, 234, 110, 54,
                            paper_cb, (void *)(intptr_t)i);

  pages.note = text(pages.choices, "", 24, 300, 342, 0x000000);
  pages.storage = text(pages.choices, "", 24, 408, 342, 0x000000);
  /* Shown so the acceptance run can confirm on the panel whether panel power
   * is really controllable, instead of taking the code's word for it.
   */
  pages.display = text(pages.choices, "", 24, 336, 342, 0x000000);
  pages.imu = text(pages.choices, "", 24, 372, 342, 0x000000);

  lv_obj_add_flag(pages.panel, LV_OBJ_FLAG_HIDDEN);
  ebadge_pages_apply_theme();
}

void ebadge_pages_close(void)
{
  ebadge_date_editor_close();
  ebadge_text_editor_close();
  if (pages.panel) lv_obj_delete(pages.panel);
  memset(&pages, 0, sizeof(pages));
}

bool ebadge_pages_active(void) { return pages.page != EBADGE_PAGE_HOME; }
enum ebadge_page ebadge_pages_current(void) { return pages.page; }

void ebadge_pages_apply_theme(void)
{
  if (!pages.panel) return;
  struct ebadge_palette palette = ebadge_view_palette();
  lv_obj_set_style_bg_color(pages.panel, lv_color_hex(palette.paper), 0);
  lv_obj_set_style_text_color(pages.title, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_bg_color(pages.bubble, lv_color_hex(palette.soft), 0);
  /* The bubble's first child is its tail; it must follow the bubble fill or it
   * renders as the placeholder colour it was created with.
   */
  lv_obj_set_style_bg_color(lv_obj_get_child(pages.bubble, 0),
                            lv_color_hex(palette.soft), 0);
  lv_obj_set_style_text_color(pages.bubble_label, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_bg_color(pages.card, lv_color_hex(palette.soft), 0);
  lv_obj_set_style_text_color(pages.card_main, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_text_color(pages.card_sub, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.tagline, lv_color_hex(palette.gold), 0);
  for (int i = 0; i < 2; i++)
    {
      lv_obj_t *vertical = lv_obj_get_child(pages.sparkle[i], 0);
      lv_obj_t *horizontal = lv_obj_get_child(pages.sparkle[i], 1);
      lv_obj_set_style_bg_color(vertical, lv_color_hex(palette.gold), 0);
      lv_obj_set_style_bg_color(horizontal, lv_color_hex(palette.gold), 0);
    }
  lv_obj_set_style_text_color(pages.note, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.storage, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.display, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.imu, lv_color_hex(palette.muted), 0);
  for (int i = 0; i < 2; i++)
    lv_obj_set_style_text_color(pages.choice_heading[i],
                                lv_color_hex(palette.muted), 0);
  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    lv_obj_set_style_text_color(button_label(pages.character[i]),
                                lv_color_hex(palette.ink), 0);
  for (int i = 0; i < EBADGE_PAPER_COUNT; i++)
    lv_obj_set_style_text_color(button_label(pages.paper[i]),
                                lv_color_hex(palette.ink), 0);
  pages.back_label = button_label(pages.back);
  lv_obj_set_style_text_color(pages.back_label, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_bg_color(pages.back, lv_color_hex(palette.soft), 0);
  lv_obj_set_style_bg_color(pages.action, lv_color_hex(palette.gold), 0);
  lv_obj_set_style_text_color(pages.action_label,
                              lv_color_hex(palette.paper), 0);
  /* The portrait chip's ring follows the accent, its inner plate the paper,
   * and its artwork the companion that is selected right now.
   */
  lv_obj_set_style_bg_color(pages.avatar, lv_color_hex(palette.gold), 0);
  lv_obj_set_style_bg_color(lv_obj_get_child(pages.avatar, 0),
                            lv_color_hex(palette.soft), 0);
  ebadge_deco_avatar_set(pages.avatar, ebadge_view_character());
  pages.dirty = true;
}

void ebadge_pages_show(enum ebadge_page page)
{
  if (!pages.panel) return;
  if (page >= EBADGE_PAGE_COUNT) page = EBADGE_PAGE_HOME;
  pages.page = page;
  if (page == EBADGE_PAGE_HOME)
    {
      ebadge_date_editor_close();
      ebadge_text_editor_close();
      lv_obj_add_flag(pages.panel, LV_OBJ_FLAG_HIDDEN);
      return;
    }

  lv_obj_remove_flag(pages.panel, LV_OBJ_FLAG_HIDDEN);
  bool appearance = page == EBADGE_PAGE_APPEARANCE;
  set_hidden(pages.choices, !appearance);
  if (!appearance)
    {
      layout_text_page(page != EBADGE_PAGE_CLOCK);
      set_hidden(pages.avatar, false);
      set_hidden(pages.bubble, false);
      set_hidden(pages.card, false);
      set_hidden(pages.tagline, false);
    }
  else
    {
      set_hidden(pages.avatar, true);
      set_hidden(pages.bubble, true);
      set_hidden(pages.card, true);
      set_hidden(pages.tagline, true);
      set_hidden(pages.sparkle[0], true);
      set_hidden(pages.sparkle[1], true);
      set_hidden(pages.action, true);
    }

  lv_label_set_text(pages.title, page == EBADGE_PAGE_CLOCK ? "心动时刻" :
                    page == EBADGE_PAGE_MESSAGE ? "把喜欢说出来" :
                    page == EBADGE_PAGE_ANNIVERSARY ? "值得记住的日子" :
                    "你的收藏");

  pages.dirty = true;
  ebadge_pages_tick(lv_tick_get());

  /* Entrance: a short rise plus fade reads as the page sliding in rather than
   * snapping over the home screen.
   */
  lv_obj_set_style_opa(pages.panel, LV_OPA_TRANSP, 0);
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, pages.panel);
  lv_anim_set_exec_cb(&anim, anim_opa_cb);
  lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_duration(&anim, 200);
  lv_anim_delete(pages.panel, anim_opa_cb);
  lv_anim_start(&anim);

  lv_anim_init(&anim);
  lv_anim_set_var(&anim, pages.panel);
  lv_anim_set_exec_cb(&anim, anim_translate_y_cb);
  lv_anim_set_values(&anim, px(18), 0);
  lv_anim_set_duration(&anim, 200);
  lv_anim_set_path_cb(&anim, lv_anim_path_ease_out);
  lv_anim_delete(pages.panel, anim_translate_y_cb);
  lv_anim_start(&anim);

  /* Two sparkles twinkle a few times, then settle: an endless animation would
   * keep the display refreshing and fight the idle screen-off work.
   */
  ebadge_deco_twinkle(pages.sparkle[0], 520, 3, 0);
  ebadge_deco_twinkle(pages.sparkle[1], 520, 3, 260);
  if (!appearance) ebadge_deco_breathe(pages.avatar, 900, 2, LV_OPA_70,
                                       LV_OPA_COVER);
}

void ebadge_pages_tap(void)
{
  /* A tap anywhere on the body does what the page's button does. */
  if (pages.page == EBADGE_PAGE_MESSAGE || pages.page == EBADGE_PAGE_ANNIVERSARY)
    {
      lv_obj_send_event(pages.action, LV_EVENT_CLICKED, NULL);
    }
}

void ebadge_pages_tick(uint32_t now)
{
  if (!pages.panel || pages.page == EBADGE_PAGE_HOME) return;
  if (!pages.dirty && (uint32_t)(now - pages.last_tick) < 1000) return;
  pages.dirty = false;
  pages.last_tick = now;

  if (pages.page == EBADGE_PAGE_CLOCK)
    {
      layout_text_page(false);
      paint_clock();
      return;
    }
  if (pages.page == EBADGE_PAGE_MESSAGE)
    {
      layout_text_page(true);
      paint_message();
      return;
    }
  if (pages.page == EBADGE_PAGE_ANNIVERSARY)
    {
      layout_text_page(true);
      paint_anniversary();
      return;
    }
  /* The appearance page shows the chooser rather than an action button, so the
   * button has to be hidden explicitly: layout_text_page() would also hide the
   * chooser, and without this the button from the previous page stays visible.
   */
  set_hidden(pages.action, true);
  paint_appearance();
}
