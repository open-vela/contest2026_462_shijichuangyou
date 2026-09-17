/* SPDX-License-Identifier: Apache-2.0 */
#ifndef EBADGE_DATE_EDITOR_H
#define EBADGE_DATE_EDITOR_H
#include <stdbool.h>
#include <lvgl/lvgl.h>
typedef void (*ebadge_date_commit_t)(int year, int month, int day);
bool ebadge_date_editor_open(lv_obj_t *parent, int year, int month, int day,
                            ebadge_date_commit_t commit);
void ebadge_date_editor_close(void);
#endif
