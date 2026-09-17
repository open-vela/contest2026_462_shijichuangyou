/* SPDX-License-Identifier: Apache-2.0 */
#include "ebadge_store.h"
#include "ebadge_record.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#define MOUNT_POINT "/data/sdcard"
#define CARD_DEVICE "/dev/mmcsd0"
#define CARD_FS "vfat"
#define STORE_PATH MOUNT_POINT "/ebadge.cfg"
#define STORE_TEMP MOUNT_POINT "/ebadge.tmp"

enum store_state
{
  STORE_UNKNOWN = 0,
  STORE_READY,
  STORE_ABSENT,
  STORE_ERROR
};

static enum store_state state = STORE_UNKNOWN;

static const char *const status_text[] = {
  "正在检查内存卡",
  "设置会保存在内存卡",
  "未插内存卡，设置仅本次运行有效",
  "内存卡不可用，设置仅本次运行有效"
};

void ebadge_store_begin(void)
{
  if (state != STORE_UNKNOWN) return;
  if (mkdir(MOUNT_POINT, 0777) < 0 && errno != EEXIST)
    {
      state = STORE_ERROR;
      return;
    }
  if (mount(CARD_DEVICE, MOUNT_POINT, CARD_FS, 0, NULL) == 0)
    {
      state = STORE_READY;
      return;
    }
  /* Already mounted by an init script is still usable; anything else means
   * there is no card, an unsupported card, or a filesystem we do not read.
   */
  if (errno == EBUSY)
    {
      state = STORE_READY;
      return;
    }
  state = errno == ENODEV || errno == ENOENT ? STORE_ABSENT : STORE_ERROR;
}

void ebadge_store_end(void)
{
  if (state != STORE_READY) return;
  umount(MOUNT_POINT);
  state = STORE_UNKNOWN;
}

bool ebadge_store_available(void)
{
  if (state == STORE_UNKNOWN) ebadge_store_begin();
  return state == STORE_READY;
}

const char *ebadge_store_status(void)
{
  if (state == STORE_UNKNOWN) ebadge_store_begin();
  return status_text[state];
}

bool ebadge_store_load(struct ebadge_settings *out)
{
  if (!out || !ebadge_store_available()) return false;
  int fd = open(STORE_PATH, O_RDONLY);
  if (fd < 0) return false;
  uint8_t record[EBADGE_RECORD_SIZE];
  ssize_t got = read(fd, record, sizeof(record));
  close(fd);
  if (got != (ssize_t)sizeof(record)) return false;
  return ebadge_record_decode(record, out);
}

bool ebadge_store_save(const struct ebadge_settings *in)
{
  if (!in || !ebadge_store_available()) return false;
  uint8_t record[EBADGE_RECORD_SIZE];
  if (!ebadge_record_encode(in, record)) return false;

  int fd = open(STORE_TEMP, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) return false;
  ssize_t written = write(fd, record, sizeof(record));
  if (written != (ssize_t)sizeof(record))
    {
      close(fd);
      unlink(STORE_TEMP);
      return false;
    }
  if (fsync(fd) < 0)
    {
      /* Not every filesystem implements fsync; the rename below is still the
       * commit point, so treat a failure here as non-fatal.
       */
    }
  close(fd);
  if (rename(STORE_TEMP, STORE_PATH) < 0)
    {
      unlink(STORE_TEMP);
      return false;
    }
  return true;
}
