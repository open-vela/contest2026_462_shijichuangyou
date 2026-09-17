/* SPDX-License-Identifier: Apache-2.0 */
/* Byte-exact codec for the on-card settings record.
 *
 * Split out of ebadge_store.c so it carries no filesystem dependency and can
 * be exercised directly by tools/test_logic.c on the build machine. The layout
 * is written field by field instead of dumping a struct, so compiler padding
 * can never change the file format between builds.
 *
 *   0..3    magic "EBC1"
 *   4..5    version, little endian
 *   6       character index
 *   7       paper index
 *   8..167  message, NUL padded, must contain a NUL
 *   168..169 year, little endian, 0 means unset
 *   170     month
 *   171     day
 *   172..173 display zone in minutes, little-endian signed
 *   174..177 additive checksum of bytes 0..173, little endian
 */
#ifndef EBADGE_RECORD_H
#define EBADGE_RECORD_H
#include <stdbool.h>
#include <stdint.h>
#include "ebadge_store.h"

#define EBADGE_RECORD_SIZE 178
#define EBADGE_RECORD_VERSION 1

/* Both return false rather than clamping: a record that does not describe a
 * valid state is treated as "no settings", never repaired by guessing.
 */
bool ebadge_record_encode(const struct ebadge_settings *in, uint8_t *out);
bool ebadge_record_decode(const uint8_t *in, struct ebadge_settings *out);

#endif
