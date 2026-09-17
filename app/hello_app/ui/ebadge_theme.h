/* SPDX-License-Identifier: Apache-2.0 */
/* Design tokens for the eBadge UI.
 *
 * One place owns the per-character palette, copy and portrait geometry so the
 * home screen, the menu, the function pages and the editors cannot drift
 * apart. Values follow the reviewed browser prototype in
 * design/interactive/INTERACTION_SPEC.md; the board code is the only consumer.
 */
#ifndef EBADGE_THEME_H
#define EBADGE_THEME_H
#include <stdint.h>

/* Shared with core/ebadge_controller.h, which defines the same value. The
 * guard keeps the two headers safe to include in either order.
 */
#ifndef EBADGE_CHARACTER_COUNT
#define EBADGE_CHARACTER_COUNT 3
#endif

enum ebadge_motif
{
  EBADGE_MOTIF_BLOOM = 0, /* 苔苔: leaf and flower */
  EBADGE_MOTIF_PAW,       /* 暖暖: cat paw */
  EBADGE_MOTIF_STAR       /* 月月: star */
};

/* Background treatment. "character" follows the selected companion; the other
 * two are user choices from the appearance page.
 */
enum ebadge_paper
{
  EBADGE_PAPER_CHARACTER = 0,
  EBADGE_PAPER_PLAIN,
  EBADGE_PAPER_NIGHT,
  EBADGE_PAPER_COUNT
};

struct ebadge_palette
{
  uint32_t paper, soft, ink, muted, gold;
};

struct ebadge_character
{
  const char *name;
  const char *line;
  struct ebadge_palette palette;
  enum ebadge_motif motif;
  int portrait_x, portrait_y, portrait_size;
};

const struct ebadge_character *ebadge_theme_character(unsigned int index);
/* Character palette with the paper mode applied. Out-of-range indices clamp. */
struct ebadge_palette ebadge_theme_resolve(unsigned int index,
                                           enum ebadge_paper paper);
const char *ebadge_theme_paper_name(enum ebadge_paper paper);
/* Short sentence naming what the current paper mode does, for the UI. */
const char *ebadge_theme_paper_note(enum ebadge_paper paper);

#endif
