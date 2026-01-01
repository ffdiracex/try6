/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/term.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/terminfo.h>
#include <holy/dl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <langinfo.h>

#include <holy/emu/console.h>

extern struct holy_terminfo_output_state holy_console_terminfo_output;
static int original_fl;
static int saved_orig;
static struct termios orig_tty;
static struct termios new_tty;

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  char chr = c;
  ssize_t actual;

  actual = write (STDOUT_FILENO, &chr, 1);
  if (actual < 1)
    {
      /* We cannot do anything about this, but some systems require us to at
	 least pretend to check the result.  */
    }
}

static int
readkey (struct holy_term_input *term __attribute__ ((unused)))
{
  holy_uint8_t c;
  ssize_t actual;

  actual = read (STDIN_FILENO, &c, 1);
  if (actual > 0)
    return c;
  return -1;
}

static holy_err_t
holy_console_init_input (struct holy_term_input *term)
{
  if (!saved_orig)
    {
      original_fl = fcntl (STDIN_FILENO, F_GETFL);
      fcntl (STDIN_FILENO, F_SETFL, original_fl | O_NONBLOCK);
    }

  saved_orig = 1;

  tcgetattr(STDIN_FILENO, &orig_tty);
  new_tty = orig_tty;
  new_tty.c_lflag &= ~(ICANON | ECHO);
  new_tty.c_cc[VMIN] = 1;
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tty);

  return holy_terminfo_input_init (term);
}

static holy_err_t
holy_console_fini_input (struct holy_term_input *term
		       __attribute__ ((unused)))
{
  fcntl (STDIN_FILENO, F_SETFL, original_fl);
  tcsetattr(STDIN_FILENO, TCSANOW, &orig_tty);
  saved_orig = 0;
  return holy_ERR_NONE;
}

static holy_err_t
holy_console_init_output (struct holy_term_output *term)
{
  struct winsize size;
  if (ioctl (STDOUT_FILENO, TIOCGWINSZ, &size) >= 0)
    {
      holy_console_terminfo_output.size.x = size.ws_col;
      holy_console_terminfo_output.size.y = size.ws_row;
    }
  else
    {
      holy_console_terminfo_output.size.x = 80;
      holy_console_terminfo_output.size.y = 24;
    }

  holy_terminfo_output_init (term);

  return 0;
}



struct holy_terminfo_input_state holy_console_terminfo_input =
  {
    .readkey = readkey
  };

struct holy_terminfo_output_state holy_console_terminfo_output =
  {
    .put = put,
    .size = { 80, 24 }
  };

static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .init = holy_console_init_input,
    .fini = holy_console_fini_input,
    .getkey = holy_terminfo_getkey,
    .data = &holy_console_terminfo_input
  };

static struct holy_term_output holy_console_term_output =
  {
    .name = "console",
    .init = holy_console_init_output,
    .putchar = holy_terminfo_putchar,
    .getxy = holy_terminfo_getxy,
    .getwh = holy_terminfo_getwh,
    .gotoxy = holy_terminfo_gotoxy,
    .cls = holy_terminfo_cls,
    .setcolorstate = holy_terminfo_setcolorstate,
    .setcursor = holy_terminfo_setcursor,
    .data = &holy_console_terminfo_output,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

void
holy_console_init (void)
{
  const char *cs = nl_langinfo (CODESET);
  if (cs && holy_strcasecmp (cs, "UTF-8"))
    holy_console_term_output.flags = holy_TERM_CODE_TYPE_UTF8_LOGICAL;
  else
    holy_console_term_output.flags = holy_TERM_CODE_TYPE_ASCII;
  holy_term_register_input ("console", &holy_console_term_input);
  holy_term_register_output ("console", &holy_console_term_output);
  holy_terminfo_init ();
  holy_terminfo_output_register (&holy_console_term_output, "vt100-color");
}

void
holy_console_fini (void)
{
  if (saved_orig)
    {
      fcntl (STDIN_FILENO, F_SETFL, original_fl);
      tcsetattr(STDIN_FILENO, TCSANOW, &orig_tty);
    }
  saved_orig = 0;
}
