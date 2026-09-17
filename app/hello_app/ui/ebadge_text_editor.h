/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EBADGE_TEXT_EDITOR_H
#define EBADGE_TEXT_EDITOR_H
#include <stdbool.h>
#include <lvgl/lvgl.h>
typedef void (*ebadge_text_commit_t)(const char *text);
bool ebadge_text_editor_open(lv_obj_t *parent, const char *text,
                            ebadge_text_commit_t commit);
void ebadge_text_editor_close(void);
#endif
