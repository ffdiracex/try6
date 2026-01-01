/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/parser.h>
#include <holy/reader.h>
#include <holy/menu_viewer.h>
#include <holy/auth.h>
#include <holy/i18n.h>
#include <holy/charset.h>
#include <holy/script_sh.h>

/* Read a line from the file FILE.  */
char *
holy_file_getline (holy_file_t file)
{
  char c;
  holy_size_t pos = 0;
  char *cmdline;
  int have_newline = 0;
  holy_size_t max_len = 64;

  /* Initially locate some space.  */
  cmdline = holy_malloc (max_len);
  if (! cmdline)
    return 0;

  while (1)
    {
      if (holy_file_read (file, &c, 1) != 1)
	break;

      /* Skip all carriage returns.  */
      if (c == '\r')
	continue;


      if (pos + 1 >= max_len)
	{
	  char *old_cmdline = cmdline;
	  max_len = max_len * 2;
	  cmdline = holy_realloc (cmdline, max_len);
	  if (! cmdline)
	    {
	      holy_free (old_cmdline);
	      return 0;
	    }
	}

      if (c == '\n')
	{
	  have_newline = 1;
	  break;
	}

      cmdline[pos++] = c;
    }

  cmdline[pos] = '\0';

  /* If the buffer is empty, don't return anything at all.  */
  if (pos == 0 && !have_newline)
    {
      holy_free (cmdline);
      cmdline = 0;
    }

  return cmdline;
}
