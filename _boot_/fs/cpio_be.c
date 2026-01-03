/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>

#define ALIGN_CPIO(x) (ALIGN_UP ((x), 2))
#define	MAGIC       "\x71\xc7"

struct head
{
  holy_uint16_t magic[1];
  holy_uint16_t dev;
  holy_uint16_t ino;
  holy_uint16_t mode[1];
  holy_uint16_t uid;
  holy_uint16_t gid;
  holy_uint16_t nlink;
  holy_uint16_t rdev;
  holy_uint16_t mtime[2];
  holy_uint16_t namesize[1];
  holy_uint16_t filesize[2];
} holy_PACKED;

static inline unsigned long long
read_number (const holy_uint16_t *arr, holy_size_t size)
{
  long long ret = 0;
  while (size--)
    ret = (ret << 16) | holy_be_to_cpu16 (*arr++);
  return ret;
}

#define FSNAME "cpiofs_be"

#include "cpio_common.c"

holy_MOD_INIT (cpio_be)
{
  holy_fs_register (&holy_cpio_fs);
}

holy_MOD_FINI (cpio_be)
{
  holy_fs_unregister (&holy_cpio_fs);
}
