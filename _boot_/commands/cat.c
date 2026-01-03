/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/charset.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    {"dos", -1, 0, N_("Accept DOS-style CR/NL line endings."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_cat (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  int dos = 0;
  holy_file_t file;
  unsigned char buf[holy_DISK_SECTOR_SIZE];
  holy_ssize_t size;
  int key = holy_TERM_NO_KEY;
  holy_uint32_t code = 0;
  int count = 0;
  unsigned char utbuf[holy_MAX_UTF8_PER_CODEPOINT + 1];
  int utcount = 0;
  int is_0d = 0;
  int j;

  if (state[0].set)
    dos = 1;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  file = holy_file_open (args[0]);
  if (! file)
    return holy_errno;

  while ((size = holy_file_read (file, buf, sizeof (buf))) > 0
	 && key != holy_TERM_ESC)
    {
      int i;

      for (i = 0; i < size; i++)
	{
	  utbuf[utcount++] = buf[i];

	  if (is_0d && buf[i] != '\n')
	    {
	      holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
	      holy_printf ("<%x>", (int) '\r');
	      holy_setcolorstate (holy_TERM_COLOR_STANDARD);
	    }

	  is_0d = 0;

	  if (!holy_utf8_process (buf[i], &code, &count))
	    {
	      holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
	      for (j = 0; j < utcount - 1; j++)
		holy_printf ("<%x>", (unsigned int) utbuf[j]);
	      code = 0;
	      count = 0;
	      if (utcount == 1 || !holy_utf8_process (buf[i], &code, &count))
		{
		  holy_printf ("<%x>", (unsigned int) buf[i]);
		  code = 0;
		  count = 0;
		  utcount = 0;
		  holy_setcolorstate (holy_TERM_COLOR_STANDARD);
		  continue;
		}
	      holy_setcolorstate (holy_TERM_COLOR_STANDARD);
	      utcount = 1;
	    }
	  if (count)
	    continue;

	  if ((code >= 0xa1 || holy_isprint (code)
	       || holy_isspace (code)) && code != '\r')
	    {
	      holy_printf ("%C", code);
	      count = 0; 
	      code = 0;
	      utcount = 0;
	      continue;
	    }

	  if (dos && code == '\r')
	    {
	      is_0d = 1;
	      count = 0; 
	      code = 0;
	      utcount = 0;
	      continue;
	    }

	  holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
	  for (j = 0; j < utcount; j++)
	    holy_printf ("<%x>", (unsigned int) utbuf[j]);
	  holy_setcolorstate (holy_TERM_COLOR_STANDARD);
	  count = 0; 
	  code = 0;
	  utcount = 0;
	}

      do
	key = holy_getkey_noblock ();
      while (key != holy_TERM_ESC && key != holy_TERM_NO_KEY);
    }

  if (is_0d)
    {
      holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
      holy_printf ("<%x>", (unsigned int) '\r');
      holy_setcolorstate (holy_TERM_COLOR_STANDARD);
    }

  if (utcount)
    {
      holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);
      for (j = 0; j < utcount; j++)
	holy_printf ("<%x>", (unsigned int) utbuf[j]);
      holy_setcolorstate (holy_TERM_COLOR_STANDARD);
    }

  holy_xputs ("\n");
  holy_refresh ();
  holy_file_close (file);

  return 0;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(cat)
{
  cmd = holy_register_extcmd ("cat", holy_cmd_cat, 0,
			      N_("FILE"), N_("Show the contents of a file."),
			      options);
}

holy_MOD_FINI(cat)
{
  holy_unregister_extcmd (cmd);
}
