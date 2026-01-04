/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>

#define ALIGN_CPIO(x) x

#define	MAGIC	"070707"
struct head
{
  char magic[6];
  char dev[6];
  char ino[6];
  char mode[6];
  char uid[6];
  char gid[6];
  char nlink[6];
  char rdev[6];
  char mtime[11];
  char namesize[6];
  char filesize[11];
} holy_PACKED;

static inline unsigned long long
read_number (const char *str, holy_size_t size)
{
  unsigned long long ret = 0;
  while (size-- && *str >= '0' && *str <= '7')
    ret = (ret << 3) | (*str++ & 0xf);
  return ret;
}

#define FSNAME "odc"

#include "cpio_common.c"

holy_MOD_INIT (odc)
{
  holy_fs_register (&holy_cpio_fs);
}

holy_MOD_FINI (odc)
{
  holy_fs_unregister (&holy_cpio_fs);
}
