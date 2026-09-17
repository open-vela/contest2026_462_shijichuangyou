/* SPDX-License-Identifier: Apache-2.0 */
/* Settings storage on the removable card.
 *
 * The badge must work with no card at all, so every entry point degrades to
 * "unavailable" instead of failing: the caller keeps its RAM defaults and the
 * UI says so. Nothing here formats, erases or repairs the card.
 *
 * The card is on SPI1 and the build already enables CONFIG_MMCSD (SPI mode)
 * and CONFIG_FS_FAT. /data is a writable tmpfs, so the mount point is created
 * there rather than on the read-only root.
 */
#ifndef EBADGE_STORE_H
#define EBADGE_STORE_H
#include <stdbool.h>
#include <stdint.h>

#define EBADGE_STORE_MESSAGE 160

struct ebadge_settings
{
  uint8_t character;
  uint8_t paper;
  char message[EBADGE_STORE_MESSAGE];
  int16_t year;
  int8_t month, day;
  int16_t zone; /* display offset in minutes */
};

/* Mount the card once. Safe to call when no card is present. */
void ebadge_store_begin(void);
void ebadge_store_end(void);

bool ebadge_store_available(void);
/* Human-readable state for the UI; never NULL. */
const char *ebadge_store_status(void);

/* Return false when unavailable, absent or corrupt; *out is then untouched. */
bool ebadge_store_load(struct ebadge_settings *out);
/* Writes through a temporary file and renames, so a power cut mid-write can
 * not leave a half-written settings file behind.
 */
bool ebadge_store_save(const struct ebadge_settings *in);

#endif
