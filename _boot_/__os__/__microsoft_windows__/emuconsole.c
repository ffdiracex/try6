/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/term.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/err.h>

#include <holy/emu/console.h>

#include <windows.h>

static HANDLE hStdin, hStdout;
static DWORD orig_mode;
static int saved_orig;


static void
holy_console_putchar (struct holy_term_output *term __attribute__ ((unused)),
		      const struct holy_unicode_glyph *c)
{
  TCHAR str[2 + 30];
  unsigned i, j;
  DWORD written;

  /* For now, do not try to use a surrogate pair.  */
  if (c->base > 0xffff)
    str[0] = '?';
  else
    str[0] = (c->base & 0xffff);
  j = 1;
  for (i = 0; i < c->ncomb && j+1 < ARRAY_SIZE (str); i++)
    if (c->base < 0xffff)
      str[j++] = holy_unicode_get_comb (c)[i].code;
  str[j] = 0;

  WriteConsole (hStdout, str, j, &written, NULL);
}

const unsigned windows_codes[] =
  {
    /* 0x21 */ [VK_PRIOR] = holy_TERM_KEY_PPAGE,
    /* 0x22 */ [VK_NEXT] = holy_TERM_KEY_NPAGE,
    /* 0x23 */ [VK_END] = holy_TERM_KEY_END,
    /* 0x24 */ [VK_HOME] = holy_TERM_KEY_HOME,
    /* 0x25 */ [VK_LEFT] = holy_TERM_KEY_LEFT,
    /* 0x26 */ [VK_UP] = holy_TERM_KEY_UP,
    /* 0x27 */ [VK_RIGHT] = holy_TERM_KEY_RIGHT,
    /* 0x28 */ [VK_DOWN] = holy_TERM_KEY_DOWN,
    /* 0x2e */ [VK_DELETE] = holy_TERM_KEY_DC,
    /* 0x70 */ [VK_F1] = holy_TERM_KEY_F1,
    /* 0x71 */ [VK_F2] = holy_TERM_KEY_F2,
    /* 0x72 */ [VK_F3] = holy_TERM_KEY_F3,
    /* 0x73 */ [VK_F4] = holy_TERM_KEY_F4,
    /* 0x74 */ [VK_F5] = holy_TERM_KEY_F5,
    /* 0x75 */ [VK_F6] = holy_TERM_KEY_F6,
    /* 0x76 */ [VK_F7] = holy_TERM_KEY_F7,
    /* 0x77 */ [VK_F8] = holy_TERM_KEY_F8,
    /* 0x78 */ [VK_F9] = holy_TERM_KEY_F9,
    /* 0x79 */ [VK_F10] = holy_TERM_KEY_F10,
    /* 0x7a */ [VK_F11] = holy_TERM_KEY_F11,
    /* 0x7b */ [VK_F12] = holy_TERM_KEY_F12,
  };


static int
holy_console_getkey (struct holy_term_input *term __attribute__ ((unused)))
{
  while (1)
    {
      DWORD nev;
      INPUT_RECORD ir;
      int ret;

      if (!GetNumberOfConsoleInputEvents (hStdin, &nev))
	return holy_TERM_NO_KEY;

      if (nev == 0)
	return holy_TERM_NO_KEY;

      if (!ReadConsoleInput (hStdin, &ir, 1,
			     &nev))
	return holy_TERM_NO_KEY;

      if (ir.EventType != KEY_EVENT)
	continue;

      if (!ir.Event.KeyEvent.bKeyDown)
	continue;
      ret = ir.Event.KeyEvent.uChar.UnicodeChar;
      if (ret == 0)
	{
	  unsigned kc = ir.Event.KeyEvent.wVirtualKeyCode;
	  if (kc < ARRAY_SIZE (windows_codes) && windows_codes[kc])
	    ret = windows_codes[kc];
	  else
	    continue;
	  if (ir.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
	    ret |= holy_TERM_SHIFT;
	}
      /* Workaround for AltGr bug.  */
      if (ir.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED)
	return ret;
      if (ir.Event.KeyEvent.dwControlKeyState & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))
	ret |= holy_TERM_ALT;
      if (ir.Event.KeyEvent.dwControlKeyState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
	ret |= holy_TERM_CTRL;
      return ret;
    }
}

static struct holy_term_coordinate
holy_console_getwh (struct holy_term_output *term __attribute__ ((unused)))
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  csbi.dwSize.X = 80;
  csbi.dwSize.Y = 25;

  GetConsoleScreenBufferInfo (hStdout, &csbi);

  return (struct holy_term_coordinate) { csbi.dwSize.X, csbi.dwSize.Y };
}

static struct holy_term_coordinate
holy_console_getxy (struct holy_term_output *term __attribute__ ((unused)))
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo (hStdout, &csbi);

  return (struct holy_term_coordinate) { csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y };
}

static void
holy_console_gotoxy (struct holy_term_output *term __attribute__ ((unused)),
		     struct holy_term_coordinate pos)
{
  COORD coord = { pos.x, pos.y };

  SetConsoleCursorPosition (hStdout, coord);
}

static void
holy_console_cls (struct holy_term_output *term)
{
  int tsz;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  struct holy_unicode_glyph c =
    {
      .base = ' ',
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .estimated_width = 1
    };

  GetConsoleScreenBufferInfo (hStdout, &csbi);

  SetConsoleTextAttribute (hStdout, 0);
  holy_console_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });
  tsz = csbi.dwSize.X * csbi.dwSize.Y;

  while (tsz--)
    holy_console_putchar (term, &c);

  holy_console_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });
  SetConsoleTextAttribute (hStdout, csbi.wAttributes);
}

static void
holy_console_setcolorstate (struct holy_term_output *term
			    __attribute__ ((unused)),
			    holy_term_color_state state)
{


  switch (state) {
    case holy_TERM_COLOR_STANDARD:
      SetConsoleTextAttribute (hStdout, holy_TERM_DEFAULT_STANDARD_COLOR
			       & 0x7f);
      break;
    case holy_TERM_COLOR_NORMAL:
      SetConsoleTextAttribute (hStdout, holy_term_normal_color & 0x7f);
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      SetConsoleTextAttribute (hStdout, holy_term_highlight_color & 0x7f);
      break;
    default:
      break;
  }
}

static void
holy_console_setcursor (struct holy_term_output *term __attribute__ ((unused)),
			int on)
{
  CONSOLE_CURSOR_INFO ci;
  ci.dwSize = 5;
  ci.bVisible = on;
  SetConsoleCursorInfo (hStdout, &ci);
}

static holy_err_t
holy_efi_console_init (struct holy_term_output *term)
{
  holy_console_setcursor (term, 1);
  return 0;
}

static holy_err_t
holy_efi_console_fini (struct holy_term_output *term)
{
  holy_console_setcursor (term, 1);
  return 0;
}


static holy_err_t
holy_console_init_input (struct holy_term_input *term)
{
  if (!saved_orig)
    {
      GetConsoleMode (hStdin, &orig_mode);
    }

  saved_orig = 1;

  SetConsoleMode (hStdin, orig_mode & ~ENABLE_ECHO_INPUT
		  & ~ENABLE_LINE_INPUT & ~ENABLE_PROCESSED_INPUT);

  return holy_ERR_NONE;
}

static holy_err_t
holy_console_fini_input (struct holy_term_input *term
		       __attribute__ ((unused)))
{
  SetConsoleMode (hStdin, orig_mode);
  saved_orig = 0;
  return holy_ERR_NONE;
}


static struct holy_term_input holy_console_term_input =
  {
    .name = "console",
    .getkey = holy_console_getkey,
    .init = holy_console_init_input,
    .fini = holy_console_fini_input,
  };

static struct holy_term_output holy_console_term_output =
  {
    .name = "console",
    .init = holy_efi_console_init,
    .fini = holy_efi_console_fini,
    .putchar = holy_console_putchar,
    .getwh = holy_console_getwh,
    .getxy = holy_console_getxy,
    .gotoxy = holy_console_gotoxy,
    .cls = holy_console_cls,
    .setcolorstate = holy_console_setcolorstate,
    .setcursor = holy_console_setcursor,
    .flags = holy_TERM_CODE_TYPE_VISUAL_GLYPHS,
    .progress_update_divisor = holy_PROGRESS_FAST
  };

void
holy_console_init (void)
{
  hStdin = GetStdHandle (STD_INPUT_HANDLE);
  hStdout = GetStdHandle (STD_OUTPUT_HANDLE);

  holy_term_register_input ("console", &holy_console_term_input);
  holy_term_register_output ("console", &holy_console_term_output);
}

void
holy_console_fini (void)
{
  if (saved_orig)
    {
      SetConsoleMode (hStdin, orig_mode);
      saved_orig = 0;
    }
  holy_term_unregister_input (&holy_console_term_input);
  holy_term_unregister_output (&holy_console_term_output);
}
