/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EBADGE_PAGES_H
#define EBADGE_PAGES_H
#include <stdbool.h>
#include <stdint.h>
#include <lvgl/lvgl.h>
#include "../core/ebadge_store.h"

enum ebadge_page
{
  EBADGE_PAGE_HOME = 0, /* the companion screen; the panel is hidden */
  EBADGE_PAGE_CLOCK,
  EBADGE_PAGE_MESSAGE,
  EBADGE_PAGE_ANNIVERSARY,
  EBADGE_PAGE_APPEARANCE,
  EBADGE_PAGE_COUNT
};

/* Configure from the command line before LVGL starts. Values are RAM-only
 * unless a card is present; see core/ebadge_store.h.
 */
bool ebadge_pages_configure(int argc, char **argv);
void ebadge_pages_open(lv_obj_t *root, int scale, int x, int y);
void ebadge_pages_close(void);

void ebadge_pages_show(enum ebadge_page page);
enum ebadge_page ebadge_pages_current(void);
bool ebadge_pages_active(void);

void ebadge_pages_tick(uint32_t now);
/* A short tap on the page body, equivalent to its primary action. */
void ebadge_pages_tap(void);
/* Re-read the palette from the view and repaint. */
void ebadge_pages_apply_theme(void);

/* Settings carried to/from the store. Command-line values win over stored
 * ones, because an explicit argument is a deliberate override.
 */
void ebadge_pages_export(struct ebadge_settings *out);
void ebadge_pages_import(const struct ebadge_settings *in);

#endif
