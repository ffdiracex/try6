/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>

#define ALIGN_CPIO(x) (ALIGN_UP ((x), 4))
#define	MAGIC	"070701"
#define	MAGIC2	"070702"
struct head
{
  char magic[6];
  char ino[8];
  char mode[8];
  char uid[8];
  char gid[8];
  char nlink[8];
  char mtime[8];
  char filesize[8];
  char devmajor[8];
  char devminor[8];
  char rdevmajor[8];
  char rdevminor[8];
  char namesize[8];
  char check[8];
} holy_PACKED;

static inline unsigned long long
read_number (const char *str, holy_size_t size)
{
  unsigned long long ret = 0;
  while (size-- && holy_isxdigit (*str))
    {
      char dig = *str++;
      if (dig >= '0' && dig <= '9')
	dig &= 0xf;
      else if (dig >= 'a' && dig <= 'f')
	dig -= 'a' - 10;
      else
	dig -= 'A' - 10;
      ret = (ret << 4) | (dig);
    }
  return ret;
}

#define FSNAME "newc"

#include "cpio_common.c"

holy_MOD_INIT (newc)
{
  holy_fs_register (&holy_cpio_fs);
}

holy_MOD_FINI (newc)
{
  holy_fs_unregister (&holy_cpio_fs);
}
