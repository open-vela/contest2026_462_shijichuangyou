/* SPDX-License-Identifier: Apache-2.0 */
/* Home screen, gesture state machine and the hidden menu.
 *
 * Interaction contract (see design/interactive/INTERACTION_SPEC.md):
 *   clean home with no permanent chrome
 *   hold 600 ms or swipe up  -> open the bottom menu
 *   horizontal swipe         -> change companion
 *   short tap                -> character motif response
 *   pages and the menu own their own input; home gestures are gated while
 *   either is open.
 */
#ifndef EBADGE_VIEW_H
#define EBADGE_VIEW_H
#include <stdbool.h>
#include <stdint.h>
#include "ebadge_theme.h"

/* Resolved palette for the current companion and paper mode. */
struct ebadge_palette ebadge_view_palette(void);
unsigned int ebadge_view_character(void);
enum ebadge_paper ebadge_view_paper(void);

/* Applied immediately; persisted once the change settles. */
void ebadge_view_set_character(unsigned int index);
void ebadge_view_set_paper(enum ebadge_paper paper);
/* Leave the function pages and return to the companion screen. */
void ebadge_view_home(void);
/* Record that message/date changed so it is written to storage. */
void ebadge_view_touch_settings(void);
/* Storage state for the appearance page; never NULL. */
const char *ebadge_view_storage_status(void);

bool ebadge_view_open(void);
void ebadge_view_close(void);
#endif
