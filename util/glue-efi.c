/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/macho.h>

#define _GNU_SOURCE	1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

static void
write_fat (FILE *in32, FILE *in64, FILE *out, const char *out_filename,
	   const char *name32, const char *name64)
{
  struct holy_macho_fat_header head;
  struct holy_macho_fat_arch arch32, arch64;
  holy_uint32_t size32, size64;
  char *buf;

  fseek (in32, 0, SEEK_END);
  size32 = ftell (in32);
  fseek (in32, 0, SEEK_SET);
  fseek (in64, 0, SEEK_END);
  size64 = ftell (in64);
  fseek (in64, 0, SEEK_SET);

  head.magic = holy_cpu_to_le32_compile_time (holy_MACHO_FAT_EFI_MAGIC);
  head.nfat_arch = holy_cpu_to_le32_compile_time (2);
  arch32.cputype = holy_cpu_to_le32_compile_time (holy_MACHO_CPUTYPE_IA32);
  arch32.cpusubtype = holy_cpu_to_le32_compile_time (3);
  arch32.offset = holy_cpu_to_le32_compile_time (sizeof (head)
						 + sizeof (arch32)
						 + sizeof (arch64));
  arch32.size = holy_cpu_to_le32 (size32);
  arch32.align = 0;

  arch64.cputype = holy_cpu_to_le32_compile_time (holy_MACHO_CPUTYPE_AMD64);
  arch64.cpusubtype = holy_cpu_to_le32_compile_time (3);
  arch64.offset = holy_cpu_to_le32 (sizeof (head) + sizeof (arch32)
				    + sizeof (arch64) + size32);
  arch64.size = holy_cpu_to_le32 (size64);
  arch64.align = 0;
  if (fwrite (&head, 1, sizeof (head), out) != sizeof (head)
      || fwrite (&arch32, 1, sizeof (arch32), out) != sizeof (arch32)
      || fwrite (&arch64, 1, sizeof (arch64), out) != sizeof (arch64))
    {
      if (out_filename)
	holy_util_error ("cannot write to `%s': %s",
			 out_filename, strerror (errno));
      else
	holy_util_error ("cannot write to the stdout: %s", strerror (errno));
    }

  buf = xmalloc (size32);
  if (fread (buf, 1, size32, in32) != size32)
    holy_util_error (_("cannot read `%s': %s"), name32,
                     strerror (errno));
  if (fwrite (buf, 1, size32, out) != size32)
    {
      if (out_filename)
	holy_util_error ("cannot write to `%s': %s",
			 out_filename, strerror (errno));
      else
	holy_util_error ("cannot write to the stdout: %s", strerror (errno));
    }
  free (buf);

  buf = xmalloc (size64);
  if (fread (buf, 1, size64, in64) != size64)
    holy_util_error (_("cannot read `%s': %s"), name64,
                     strerror (errno));
  if (fwrite (buf, 1, size64, out) != size64)
    {
      if (out_filename)
	holy_util_error ("cannot write to `%s': %s",
			 out_filename, strerror (errno));
      else
	holy_util_error ("cannot write to the stdout: %s", strerror (errno));
    }
  free (buf);
}

void
holy_util_glue_efi (const char *file32, const char *file64, const char *outname)
{
  FILE *in32, *in64, *out;

  in32 = holy_util_fopen (file32, "r");

  if (!in32)
    holy_util_error (_("cannot open `%s': %s"), file32,
		     strerror (errno));

  in64 = holy_util_fopen (file64, "r");
  if (!in64)
    holy_util_error (_("cannot open `%s': %s"), file64,
		     strerror (errno));

  if (outname)
    out = holy_util_fopen (outname, "wb");
  else
    out = stdout;

  if (!out)
    {
      holy_util_error (_("cannot open `%s': %s"), outname ? : "stdout",
		       strerror (errno));
    }

  write_fat (in32, in64, out, outname,
	     file32, file64);

  fclose (in32);
  fclose (in64);

  if (out != stdout)
    fclose (out);
}

