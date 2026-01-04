/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/fileid.h>
#include <holy/elfload.h>
#include <holy/misc.h>

#pragma GCC diagnostic ignored "-Wcast-align"

int
holy_file_check_netbsdXX (holy_elf_t elf)
{
  Elf_Shdr *s, *s0;

  holy_size_t shnum = elf->ehdr.ehdrXX.e_shnum;
  holy_size_t shentsize = elf->ehdr.ehdrXX.e_shentsize;
  holy_size_t shsize = shnum * shentsize;
  holy_off_t stroff;

  if (!shnum || !shentsize)
    return 0;

  s0 = holy_malloc (shsize);
  if (!s0)
    return 0;

  if (holy_file_seek (elf->file, elf->ehdr.ehdrXX.e_shoff) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (elf->file, s0, shsize) != (holy_ssize_t) shsize)
    goto fail;

  s = (Elf_Shdr *) ((char *) s0 + elf->ehdr.ehdrXX.e_shstrndx * shentsize);
  stroff = s->sh_offset;

  for (s = s0; s < (Elf_Shdr *) ((char *) s0 + shnum * shentsize);
       s = (Elf_Shdr *) ((char *) s + shentsize))
    {
      char name[sizeof(".note.netbsd.ident")];
      holy_memset (name, 0, sizeof (name));
      if (holy_file_seek (elf->file, stroff + s->sh_name) == (holy_off_t) -1)
	goto fail;

      if (holy_file_read (elf->file, name, sizeof (name)) != (holy_ssize_t) sizeof (name))
	{
	  if (holy_errno)
	    goto fail;
	  continue;
	}
      if (holy_memcmp (name, ".note.netbsd.ident",
		       sizeof(".note.netbsd.ident")) != 0)
	continue;
      holy_free (s0);
      return 1;
    }
 fail:
  holy_free (s0);
  return 0;
}
