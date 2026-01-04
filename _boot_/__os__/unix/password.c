/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/crypto.h>
#include <holy/mm.h>
#include <holy/term.h>

#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int
holy_password_get (char buf[], unsigned buf_size)
{
  FILE *in;
  struct termios s, t;
  int tty_changed = 0;
  char *ptr;

  holy_refresh ();

  /* Disable echoing. Based on glibc.  */
  in = fopen ("/dev/tty", "w+c");
  if (in == NULL)
    in = stdin;

  if (tcgetattr (fileno (in), &t) == 0)
    {
      /* Save the old one. */
      s = t;
      /* Tricky, tricky. */
      t.c_lflag &= ~(ECHO|ISIG);
      tty_changed = (tcsetattr (fileno (in), TCSAFLUSH, &t) == 0);
    }
  else
    tty_changed = 0;
  holy_memset (buf, 0, buf_size);
  if (!fgets (buf, buf_size, stdin))
    {
      if (in != stdin)
	fclose (in);
      return 0;
    }
  ptr = buf + strlen (buf) - 1;
  while (buf <= ptr && (*ptr == '\n' || *ptr == '\r'))
    *ptr-- = 0;
  /* Restore the original setting.  */
  if (tty_changed)
    (void) tcsetattr (fileno (in), TCSAFLUSH, &s);

  holy_xputs ("\n");
  holy_refresh ();

  if (in != stdin)
    fclose (in);

  return 1;
}
