/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/types.h>
#include <holy/macho.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Please don't internationalise this file. It's pointless.  */

/* XXX: this file assumes particular Mach-O layout and does no checks. */
/* However as build system ensures correct usage of this tool this
   shouldn't be a problem. */

int
main (int argc, char **argv)
{
  FILE *in, *out;
  int do_bss = 0;
  char *buf;
  int bufsize;
  struct holy_macho_header32 *head;
  struct holy_macho_segment32 *curcmd;
  unsigned i;
  unsigned bssstart = 0;
  unsigned bssend = 0;

  if (argc && strcmp (argv[1], "--bss") == 0)
    do_bss = 1;
  if (argc < 2 + do_bss)
    {
      printf ("Usage: %s [--bss] filename.exec filename.img\n"
	      "Convert Mach-O into raw image\n", argv[0]);
      return 0;
    }
  in = fopen (argv[1 + do_bss], "rb");
  if (! in)
    {
      printf ("Couldn't open %s\n", argv[1 + do_bss]);
      return 1;
    }
  out = fopen (argv[2 + do_bss], "wb");
  if (! out)
    {
      fclose (in);
      printf ("Couldn't open %s\n", argv[2 + do_bss]);
      return 2;
    }
  fseek (in, 0, SEEK_END);
  bufsize = ftell (in);
  fseek (in, 0, SEEK_SET);
  buf = malloc (bufsize);
  if (! buf)
    {
      fclose (in);
      fclose (out);
      printf ("Couldn't allocate buffer\n");
      return 3;
    }
  fread (buf, 1, bufsize, in);
  head = (struct holy_macho_header32 *) buf;
  if (holy_le_to_cpu32 (head->magic) != holy_MACHO_MAGIC32)
    {
      fclose (in);
      fclose (out);
      free (buf);
      printf ("Invalid Mach-O file\n");
      return 4;
    }
  curcmd = (struct holy_macho_segment32 *) (buf + sizeof (*head));
  for (i = 0; i < holy_le_to_cpu32 (head->ncmds); i++,
	 curcmd = (struct holy_macho_segment32 *)
	 (((char *) curcmd) + curcmd->cmdsize))
    {
      if (curcmd->cmd != holy_MACHO_CMD_SEGMENT32)
	continue;
      fwrite (buf + holy_le_to_cpu32 (curcmd->fileoff), 1,
	      holy_le_to_cpu32 (curcmd->filesize), out);
      if (holy_le_to_cpu32 (curcmd->vmsize)
	  > holy_le_to_cpu32 (curcmd->filesize))
	{
	  bssstart = holy_le_to_cpu32 (curcmd->vmaddr)
	    + holy_le_to_cpu32 (curcmd->filesize) ;
	  bssend = holy_le_to_cpu32 (curcmd->vmaddr)
	    + holy_le_to_cpu32 (curcmd->vmsize) ;
	}
    }
  if (do_bss)
    {
      holy_uint32_t tmp;
      fseek (out, 0x5c, SEEK_SET);
      tmp = holy_cpu_to_le32 (bssstart);
      fwrite (&tmp, 4, 1, out);
      tmp = holy_cpu_to_le32 (bssend);
      fwrite (&tmp, 4, 1, out);
    }
  fclose (in);
  fclose (out);
  printf("macho2img complete\n");
  return 0;
}
