/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/macho.h>
#include <holy/machoload.h>
#include <holy/file.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/i18n.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_macho_close (holy_macho_t macho)
{
  holy_file_t file = macho->file;

  if (!macho->uncompressed32)
    holy_free (macho->cmds32);
  holy_free (macho->uncompressed32);
  if (!macho->uncompressed64)
    holy_free (macho->cmds64);
  holy_free (macho->uncompressed64);

  holy_free (macho);

  if (file)
    holy_file_close (file);

  return holy_errno;
}

holy_macho_t
holy_macho_file (holy_file_t file, const char *filename, int is_64bit)
{
  holy_macho_t macho;
  union holy_macho_filestart filestart;

  macho = holy_malloc (sizeof (*macho));
  if (! macho)
    return 0;

  macho->file = file;
  macho->offset32 = -1;
  macho->offset64 = -1;
  macho->end32 = -1;
  macho->end64 = -1;
  macho->cmds32 = 0;
  macho->cmds64 = 0;
  macho->uncompressed32 = 0;
  macho->uncompressed64 = 0;
  macho->compressed32 = 0;
  macho->compressed64 = 0;

  if (holy_file_seek (macho->file, 0) == (holy_off_t) -1)
    goto fail;

  if (holy_file_read (macho->file, &filestart, sizeof (filestart))
      != sizeof (filestart))
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);
      goto fail;
    }

  /* Is it a fat file? */
  if (filestart.fat.magic == holy_cpu_to_be32_compile_time (holy_MACHO_FAT_MAGIC))
    {
      struct holy_macho_fat_arch *archs;
      int i, narchs;

      /* Load architecture description. */
      narchs = holy_be_to_cpu32 (filestart.fat.nfat_arch);
      if (holy_file_seek (macho->file, sizeof (struct holy_macho_fat_header))
	  == (holy_off_t) -1)
	goto fail;
      archs = holy_malloc (sizeof (struct holy_macho_fat_arch) * narchs);
      if (!archs)
	goto fail;
      if (holy_file_read (macho->file, archs,
			  sizeof (struct holy_macho_fat_arch) * narchs)
	  != (holy_ssize_t) sizeof(struct holy_macho_fat_arch) * narchs)
	{
	  holy_free (archs);
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			filename);
	  goto fail;
	}

      for (i = 0; i < narchs; i++)
	{
	  if ((archs[i].cputype
	       == holy_cpu_to_be32_compile_time (holy_MACHO_CPUTYPE_IA32))
	      && !is_64bit)
	    {
	      macho->offset32 = holy_be_to_cpu32 (archs[i].offset);
	      macho->end32 = holy_be_to_cpu32 (archs[i].offset)
		+ holy_be_to_cpu32 (archs[i].size);
	    }
	  if ((archs[i].cputype
	       == holy_cpu_to_be32_compile_time (holy_MACHO_CPUTYPE_AMD64))
	      && is_64bit)
	    {
	      macho->offset64 = holy_be_to_cpu32 (archs[i].offset);
	      macho->end64 = holy_be_to_cpu32 (archs[i].offset)
		+ holy_be_to_cpu32 (archs[i].size);
	    }
	}
      holy_free (archs);
    }

  /* Is it a thin 32-bit file? */
  if (filestart.thin32.magic == holy_MACHO_MAGIC32 && !is_64bit)
    {
      macho->offset32 = 0;
      macho->end32 = holy_file_size (file);
    }

  /* Is it a thin 64-bit file? */
  if (filestart.thin64.magic == holy_MACHO_MAGIC64 && is_64bit)
    {
      macho->offset64 = 0;
      macho->end64 = holy_file_size (file);
    }

  if (holy_memcmp (filestart.lzss.magic, holy_MACHO_LZSS_MAGIC,
		   sizeof (filestart.lzss.magic)) == 0 && !is_64bit)
    {
      macho->offset32 = 0;
      macho->end32 = holy_file_size (file);
    }

  /* Is it a thin 64-bit file? */
  if (holy_memcmp (filestart.lzss.magic, holy_MACHO_LZSS_MAGIC,
		   sizeof (filestart.lzss.magic)) == 0 && is_64bit)
    {
      macho->offset64 = 0;
      macho->end64 = holy_file_size (file);
    }

  holy_macho_parse32 (macho, filename);
  holy_macho_parse64 (macho, filename);

  if (macho->offset32 == -1 && !is_64bit)
    {
      holy_error (holy_ERR_BAD_OS,
		  "Mach-O doesn't contain suitable 32-bit architecture");
      goto fail;
    }

  if (macho->offset64 == -1 && is_64bit)
    {
      holy_error (holy_ERR_BAD_OS,
		  "Mach-O doesn't contain suitable 64-bit architecture");
      goto fail;
    }

  return macho;

fail:
  macho->file = 0;
  holy_macho_close (macho);
  return 0;
}

holy_macho_t
holy_macho_open (const char *name, int is_64bit)
{
  holy_file_t file;
  holy_macho_t macho;

  file = holy_file_open (name);
  if (! file)
    return 0;

  macho = holy_macho_file (file, name, is_64bit);
  if (! macho)
    holy_file_close (file);

  return macho;
}
