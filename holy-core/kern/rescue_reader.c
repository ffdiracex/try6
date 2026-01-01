/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/reader.h>
#include <holy/parser.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/mm.h>

#define holy_RESCUE_BUF_SIZE	256

static char linebuf[holy_RESCUE_BUF_SIZE];

/* Prompt to input a command and read the line.  */
static holy_err_t
holy_rescue_read_line (char **line, int cont,
		       void *data __attribute__ ((unused)))
{
  int c;
  int pos = 0;
  char str[4];

  holy_printf ((cont) ? "> " : "holy rescue> ");
  holy_memset (linebuf, 0, holy_RESCUE_BUF_SIZE);

  while ((c = holy_getkey ()) != '\n' && c != '\r')
    {
      if (holy_isprint (c))
	{
	  if (pos < holy_RESCUE_BUF_SIZE - 1)
	    {
	      str[0] = c;
	      str[1] = 0;
	      linebuf[pos++] = c;
	      holy_xputs (str);
	    }
	}
      else if (c == '\b')
	{
	  if (pos > 0)
	    {
	      str[0] = c;
	      str[1] = ' ';
	      str[2] = c;
	      str[3] = 0;
	      linebuf[--pos] = 0;
	      holy_xputs (str);
	    }
	}
      holy_refresh ();
    }

  holy_xputs ("\n");
  holy_refresh ();

  *line = holy_strdup (linebuf);

  return 0;
}

void __attribute__ ((noreturn))
holy_rescue_run (void)
{
  holy_printf ("Entering rescue mode...\n");

  while (1)
    {
      char *line;

      /* Print an error, if any.  */
      holy_print_error ();
      holy_errno = holy_ERR_NONE;

      holy_rescue_read_line (&line, 0, NULL);
      if (! line || line[0] == '\0')
	continue;

      holy_rescue_parse_line (line, holy_rescue_read_line, NULL);
      holy_free (line);
    }
}
