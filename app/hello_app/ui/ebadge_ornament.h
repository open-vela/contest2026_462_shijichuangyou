/* SPDX-License-Identifier: Apache-2.0 */
/* Short tap response drawn on the home screen.
 *
 * The reviewed prototype says the portrait itself must not move when tapped,
 * so the acknowledgement is a small character motif that appears near the
 * touch point, brightens and fades. All objects are created once; playing a
 * response only repositions, recolours and re-animates them.
 */
#ifndef EBADGE_ORNAMENT_H
#define EBADGE_ORNAMENT_H
#include <stdbool.h>
#include <stdint.h>
#include <lvgl/lvgl.h>
#include "ebadge_theme.h"

bool ebadge_ornament_open(lv_obj_t *parent, int scale, int origin_x, int origin_y);
void ebadge_ornament_close(void);
/* Show the motif for this character at a canvas point, in the accent colour. */
void ebadge_ornament_play(enum ebadge_motif motif, int canvas_x, int canvas_y,
                          uint32_t color);
/* Re-colour the motifs without restarting an in-flight response. */
void ebadge_ornament_recolor(uint32_t color);

#endif
