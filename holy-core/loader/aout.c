/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/aout.h>

holy_MOD_LICENSE ("GPLv2+");

int
holy_aout_get_type (union holy_aout_header *header)
{
  int magic;

  magic = AOUT_GETMAGIC (header->aout32);
  if ((magic == AOUT32_OMAGIC) || (magic == AOUT32_NMAGIC) ||
      (magic == AOUT32_ZMAGIC) || (magic == AOUT32_QMAGIC))
    return AOUT_TYPE_AOUT32;
  else if ((magic == AOUT64_OMAGIC) || (magic == AOUT64_NMAGIC) ||
	   (magic == AOUT64_ZMAGIC))
    return AOUT_TYPE_AOUT64;
  else
    return AOUT_TYPE_NONE;
}

holy_err_t
holy_aout_load (holy_file_t file, int offset,
                void *load_addr,
		int load_size, holy_size_t bss_size)
{
  if ((holy_file_seek (file, offset)) == (holy_off_t) - 1)
    return holy_errno;

  if (!load_size)
    load_size = file->size - offset;

  holy_file_read (file, load_addr, load_size);

  if (holy_errno)
    return holy_errno;

  if (bss_size)
    holy_memset ((char *) load_addr + load_size, 0, bss_size);

  return holy_ERR_NONE;
}
