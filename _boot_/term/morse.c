/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/time.h>
#include <holy/speaker.h>

holy_MOD_LICENSE ("GPLv2+");

#define BASE_TIME 250
#define DIH 1
#define DAH 3
#define END 0

static const char codes[0x80][6] =
  {
    ['0'] = { DAH, DAH, DAH, DAH, DAH, END },
    ['1'] = { DIH, DAH, DAH, DAH, DAH, END },
    ['2'] = { DIH, DIH, DAH, DAH, DAH, END },
    ['3'] = { DIH, DIH, DIH, DAH, DAH, END },
    ['4'] = { DIH, DIH, DIH, DIH, DAH, END },
    ['5'] = { DIH, DIH, DIH, DIH, DIH, END },
    ['6'] = { DAH, DIH, DIH, DIH, DIH, END },
    ['7'] = { DAH, DAH, DIH, DIH, DIH, END },
    ['8'] = { DAH, DAH, DAH, DIH, DIH, END },
    ['9'] = { DAH, DAH, DAH, DAH, DIH, END },
    ['a'] = { DIH, DAH, END },
    ['b'] = { DAH, DIH, DIH, DIH, END },
    ['c'] = { DAH, DIH, DAH, DIH, END },
    ['d'] = { DAH, DIH, DIH, END },
    ['e'] = { DIH, END },
    ['f'] = { DIH, DIH, DAH, DIH, END },
    ['g'] = { DAH, DAH, DIH, END },
    ['h'] = { DIH, DIH, DIH, DIH, END },
    ['i'] = { DIH, DIH, END },
    ['j'] = { DIH, DAH, DAH, DAH, END },
    ['k'] = { DAH, DIH, DAH, END },
    ['l'] = { DIH, DAH, DIH, DIH, END },
    ['m'] = { DAH, DAH, END },
    ['n'] = { DAH, DIH, END },
    ['o'] = { DAH, DAH, DAH, END },
    ['p'] = { DIH, DAH, DAH, DIH, END },
    ['q'] = { DAH, DAH, DIH, DAH, END },
    ['r'] = { DIH, DAH, DIH, END },
    ['s'] = { DIH, DIH, DIH, END },
    ['t'] = { DAH, END },
    ['u'] = { DIH, DIH, DAH, END },
    ['v'] = { DIH, DIH, DIH, DAH, END },
    ['w'] = { DIH, DAH, DAH, END },
    ['x'] = { DAH, DIH, DIH, DAH, END },
    ['y'] = { DAH, DIH, DAH, DAH, END },
    ['z'] = { DAH, DAH, DIH, DIH, END }
  };

static void
holy_audio_tone (int length)
{
  holy_speaker_beep_on (1000);
  holy_millisleep (length);
  holy_speaker_beep_off ();
}

static void
holy_audio_putchar (struct holy_term_output *term __attribute__ ((unused)),
		    const struct holy_unicode_glyph *c_in)
{
  holy_uint8_t c;
  int i;

  /* For now, do not try to use a surrogate pair.  */
  if (c_in->base > 0x7f)
    c = '?';
  else
    c = holy_tolower (c_in->base);
  for (i = 0; codes[c][i]; i++)
    {
      holy_audio_tone (codes[c][i] * BASE_TIME);
      holy_millisleep (BASE_TIME);
    }
  holy_millisleep (2 * BASE_TIME);
}


static int
dummy (void)
{
  return 0;
}

static struct holy_term_output holy_audio_term_output =
  {
   .name = "morse",
   .init = (void *) dummy,
   .fini = (void *) dummy,
   .putchar = holy_audio_putchar,
   .getwh = (void *) dummy,
   .getxy = (void *) dummy,
   .gotoxy = (void *) dummy,
   .cls = (void *) dummy,
   .setcolorstate = (void *) dummy,
   .setcursor = (void *) dummy,
   .flags = holy_TERM_CODE_TYPE_ASCII | holy_TERM_DUMB,
   .progress_update_divisor = holy_PROGRESS_NO_UPDATE
  };

holy_MOD_INIT (morse)
{
  holy_term_register_output ("audio", &holy_audio_term_output);
}

holy_MOD_FINI (morse)
{
  holy_term_unregister_output (&holy_audio_term_output);
}
