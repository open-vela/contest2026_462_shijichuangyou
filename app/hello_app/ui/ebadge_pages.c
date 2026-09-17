/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_pages.h"
#include "ebadge_view.h"
#include "ebadge_theme.h"
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
  lv_obj_t *eyebrow;
  lv_obj_t *value;
  lv_obj_t *detail;
  lv_obj_t *hint;
  lv_obj_t *action, *action_label;
  lv_obj_t *choices, *choice_heading[2], *note;
  lv_obj_t *character[EBADGE_CHARACTER_COUNT];
  lv_obj_t *paper[EBADGE_PAPER_COUNT];
  lv_obj_t *storage;

  int scale, origin_x, origin_y;
  enum ebadge_page page;
  uint32_t last_tick;
  bool dirty;
} pages;

static int px(int n) { return n * pages.scale / 1000; }

/* The style setter takes a selector argument, so it cannot be cast into an
 * lv_anim_exec_xcb_t callback. */
static void anim_opa_cb(void *var, int32_t value)
{
  lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
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
  lv_obj_set_style_radius(obj, px(16), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
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

static void layout_text_page(void)
{
  set_hidden(pages.eyebrow, false);
  set_hidden(pages.value, false);
  set_hidden(pages.detail, false);
  set_hidden(pages.choices, true);
  set_hidden(pages.action, true);
  set_hidden(pages.note, true);
  set_hidden(pages.storage, true);
}

static void paint_clock(void)
{
  time_t utc = time(NULL);
  time_t shifted = utc + config.zone * 60;
  struct tm local;
  if (utc == (time_t)-1 || !gmtime_r(&shifted, &local) ||
      local.tm_year < 124 || local.tm_year > 199)
    {
      lv_label_set_text(pages.eyebrow, "时间尚未校准");
      lv_label_set_text(pages.value, "--:--");
      lv_label_set_text(pages.detail, "用电脑串口同步后即可显示");
      return;
    }
  static const char *const week[] = {"周日", "周一", "周二", "周三", "周四",
                                     "周五", "周六"};
  char buffer[100];
  lv_label_set_text(pages.eyebrow, "此刻 · 北京时间");
  snprintf(buffer, sizeof(buffer), "%02d:%02d", local.tm_hour, local.tm_min);
  lv_label_set_text(pages.value, buffer);
  snprintf(buffer, sizeof(buffer), "%04d年%02d月%02d日 %s",
           local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
           week[local.tm_wday % 7]);
  lv_label_set_text(pages.detail, buffer);
}

static void paint_message(void)
{
  lv_label_set_text(pages.eyebrow, "应援文字");
  lv_label_set_text(pages.value, config.message);
  lv_label_set_text(pages.detail, "轻点下方按钮修改");
  lv_label_set_text(button_label(pages.action), "编辑应援文字");
  set_hidden(pages.action, false);
}

static void paint_anniversary(void)
{
  time_t utc = time(NULL);
  time_t shifted = utc + config.zone * 60;
  struct tm local;
  bool valid = utc != (time_t)-1 && gmtime_r(&shifted, &local) &&
               local.tm_year >= 124 && local.tm_year <= 199;
  if (!config.year)
    {
      lv_label_set_text(pages.eyebrow, "纪念日");
      lv_label_set_text(pages.value, "");
      lv_label_set_text(pages.detail,
                        valid ? "留一个位置，给特别的那一天"
                              : "请先校准时间，再设置纪念日");
    }
  else if (!valid)
    {
      lv_label_set_text(pages.eyebrow, "纪念日");
      lv_label_set_text(pages.value, "");
      lv_label_set_text(pages.detail, "请先校准时间");
    }
  else
    {
      long days = ebadge_days_between(
        local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
        config.year, config.month, config.day);
      if (!days) lv_label_set_text(pages.eyebrow, "就是今天");
      else lv_label_set_text(pages.eyebrow,
                             days > 0 ? "距离那一天，还有" : "从那一天，已经走过");
      char buffer[40];
      snprintf(buffer, sizeof(buffer), "%ld", days > 0 ? days : -days);
      lv_label_set_text(pages.value, days ? buffer : "");
      snprintf(buffer, sizeof(buffer), "%04d年%02d月%02d日", config.year,
               config.month, config.day);
      lv_label_set_text(pages.detail, buffer);
    }
  lv_label_set_text(button_label(pages.action), "设置纪念日");
  set_hidden(pages.action, false);
}

static void paint_appearance(void)
{
  for (int i = 0; i < EBADGE_CHARACTER_COUNT; i++)
    {
      const struct ebadge_character *character = ebadge_theme_character(i);
      lv_label_set_text(button_label(pages.character[i]), character->name);
      lv_obj_set_style_bg_color(pages.character[i],
        lv_color_hex(i == (int)ebadge_view_character() ?
                     ebadge_view_palette().ink : ebadge_view_palette().soft), 0);
      lv_obj_set_style_text_color(button_label(pages.character[i]),
        lv_color_hex(i == (int)ebadge_view_character() ?
                     ebadge_view_palette().paper : ebadge_view_palette().ink), 0);
    }
  for (int i = 0; i < EBADGE_PAPER_COUNT; i++)
    {
      lv_label_set_text(button_label(pages.paper[i]),
                        ebadge_theme_paper_name((enum ebadge_paper)i));
      lv_obj_set_style_bg_color(pages.paper[i],
        lv_color_hex(i == (int)ebadge_view_paper() ?
                     ebadge_view_palette().ink : ebadge_view_palette().soft), 0);
      lv_obj_set_style_text_color(button_label(pages.paper[i]),
        lv_color_hex(i == (int)ebadge_view_paper() ?
                     ebadge_view_palette().paper : ebadge_view_palette().ink), 0);
    }
  lv_label_set_text(pages.note,
                    ebadge_theme_paper_note(ebadge_view_paper()));
  lv_label_set_text(pages.storage, ebadge_view_storage_status());
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

  /* Shared text block: eyebrow, big value, detail line. */
  pages.eyebrow = text(pages.panel, "", 24, 118, 342, 0x000000);
  pages.value = text(pages.panel, "", 24, 152, 342, 0x000000);
  pages.detail = text(pages.panel, "", 24, 268, 342, 0x000000);
  lv_obj_set_style_text_font(pages.value, &lv_font_montserrat_48, 0);

  pages.hint = text(pages.panel, "点左上角返回", 24, 404, 342, 0x000000);

  pages.action = button(pages.panel, "", 60, 330, 270, 56, action_cb, NULL);
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
  pages.storage = text(pages.choices, "", 24, 372, 342, 0x000000);

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
  lv_obj_set_style_text_color(pages.eyebrow, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.value, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_text_color(pages.detail, lv_color_hex(palette.ink), 0);
  lv_obj_set_style_text_color(pages.hint, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.note, lv_color_hex(palette.muted), 0);
  lv_obj_set_style_text_color(pages.storage, lv_color_hex(palette.muted), 0);
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
  lv_obj_set_style_bg_color(pages.action, lv_color_hex(palette.soft), 0);
  lv_obj_set_style_text_color(pages.action_label,
                              lv_color_hex(palette.ink), 0);
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
  /* Text pages share one block; the appearance page uses its own controls. */
  bool appearance = page == EBADGE_PAGE_APPEARANCE;
  set_hidden(pages.choices, !appearance);
  set_hidden(pages.eyebrow, appearance);
  set_hidden(pages.value, appearance);
  set_hidden(pages.detail, appearance);
  set_hidden(pages.hint, appearance);
  set_hidden(pages.action, appearance);

  lv_label_set_text(pages.title, page == EBADGE_PAGE_CLOCK ? "与你共度" :
                    page == EBADGE_PAGE_MESSAGE ? "把喜欢说出来" :
                    page == EBADGE_PAGE_ANNIVERSARY ? "值得记住的日子" :
                    "你的收藏");

  /* The clock and the day count read best as large digits; the message is a
   * native 20 px Chinese label.
   */
  if (page == EBADGE_PAGE_MESSAGE)
    {
      lv_obj_set_style_text_font(pages.value, &ebadge_font_zh_20, 0);
      lv_label_set_long_mode(pages.value, LV_LABEL_LONG_SCROLL_CIRCULAR);
      lv_obj_set_height(pages.value, px(30));
    }
  else
    {
      lv_obj_set_style_text_font(pages.value, &lv_font_montserrat_48, 0);
      lv_label_set_long_mode(pages.value, LV_LABEL_LONG_WRAP);
      lv_obj_set_height(pages.value, LV_SIZE_CONTENT);
    }

  pages.dirty = true;
  ebadge_pages_tick(lv_tick_get());

  /* A short fade makes the page change feel deliberate rather than abrupt. */
  lv_obj_set_style_opa(pages.panel, LV_OPA_TRANSP, 0);
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, pages.panel);
  lv_anim_set_exec_cb(&anim, anim_opa_cb);
  lv_anim_set_values(&anim, LV_OPA_TRANSP, LV_OPA_COVER);
  lv_anim_set_duration(&anim, 180);
  lv_anim_delete(pages.panel, anim_opa_cb);
  lv_anim_start(&anim);
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
  /* The clock advances every second; other pages only on change. */
  bool clock = pages.page == EBADGE_PAGE_CLOCK;
  if (!pages.dirty && !clock && (uint32_t)(now - pages.last_tick) < 1000) return;
  if (!pages.dirty && clock && (uint32_t)(now - pages.last_tick) < 1000) return;
  pages.dirty = false;
  pages.last_tick = now;

  if (pages.page == EBADGE_PAGE_CLOCK)
    {
      layout_text_page();
      paint_clock();
      return;
    }
  if (pages.page == EBADGE_PAGE_MESSAGE)
    {
      layout_text_page();
      paint_message();
      return;
    }
  if (pages.page == EBADGE_PAGE_ANNIVERSARY)
    {
      layout_text_page();
      paint_anniversary();
      return;
    }
  paint_appearance();
}
