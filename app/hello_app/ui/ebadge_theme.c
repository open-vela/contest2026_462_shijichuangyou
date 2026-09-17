/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_theme.h"

/* Palettes are copied from the reviewed browser prototype so the panel and the
 * prototype agree. They are plain sRGB hex values, not hardware gamma tables.
 */
static const struct ebadge_character characters[EBADGE_CHARACTER_COUNT] = {
  {
    "苔苔", "把喜欢，藏进春天。",
    {0xf7f3e6, 0xe5e8d6, 0x405334, 0x657154, 0xaf9962},
    EBADGE_MOTIF_BLOOM, 9, 24, 372
  },
  {
    "暖暖", "与你分享，今天的暖阳。",
    {0xfff1df, 0xf2d8b7, 0x68442d, 0x856246, 0xbb854c},
    EBADGE_MOTIF_PAW, 12, 19, 366
  },
  {
    "月月", "把心愿，寄给今晚的月亮。",
    {0xedf0f7, 0xdce2ef, 0x344967, 0x63748c, 0xad9b70},
    EBADGE_MOTIF_STAR, 9, 23, 372
  }
};

/* "素纸": a neutral paper that keeps the character artwork untouched. */
static const struct ebadge_palette plain = {
  0xf8f6ef, 0xe9e7dc, 0x43483b, 0x707463, 0x000000
};

/* "夜读": dark surface, warm light text. */
static const struct ebadge_palette night = {
  0x222a31, 0x35414b, 0xf4ebd8, 0xc0c9cb, 0x000000
};

static const char *const paper_names[EBADGE_PAPER_COUNT] = {
  "随角色", "素纸", "夜读"
};

static const char *const paper_notes[EBADGE_PAPER_COUNT] = {
  "底色跟随身边的角色",
  "素净纸色，立绘不变",
  "只改界面底色，不改立绘"
};

const struct ebadge_character *ebadge_theme_character(unsigned int index)
{
  if (index >= EBADGE_CHARACTER_COUNT) index = 0;
  return &characters[index];
}

struct ebadge_palette ebadge_theme_resolve(unsigned int index,
                                           enum ebadge_paper paper)
{
  const struct ebadge_character *character = ebadge_theme_character(index);
  struct ebadge_palette palette = character->palette;
  if (paper == EBADGE_PAPER_PLAIN)
    {
      palette.paper = plain.paper;
      palette.soft = plain.soft;
      palette.ink = plain.ink;
      palette.muted = plain.muted;
    }
  else if (paper == EBADGE_PAPER_NIGHT)
    {
      palette.paper = night.paper;
      palette.soft = night.soft;
      palette.ink = night.ink;
      palette.muted = night.muted;
    }

  /* The accent stays the character's own gold so the companion remains
   * recognisable in every paper mode.
   */
  return palette;
}

const char *ebadge_theme_paper_name(enum ebadge_paper paper)
{
  if (paper >= EBADGE_PAPER_COUNT) paper = EBADGE_PAPER_CHARACTER;
  return paper_names[paper];
}

const char *ebadge_theme_paper_note(enum ebadge_paper paper)
{
  if (paper >= EBADGE_PAPER_COUNT) paper = EBADGE_PAPER_CHARACTER;
  return paper_notes[paper];
}
