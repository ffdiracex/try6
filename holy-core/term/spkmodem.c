/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/terminfo.h>
#include <holy/dl.h>
#include <holy/speaker.h>

holy_MOD_LICENSE ("GPLv2+");

extern struct holy_terminfo_output_state holy_spkmodem_terminfo_output;

static void
make_tone (holy_uint16_t freq_count, unsigned int duration)
{
  /* Program timer 2.  */
  holy_outb (holy_PIT_CTRL_SELECT_2
	     | holy_PIT_CTRL_READLOAD_WORD
	     | holy_PIT_CTRL_SQUAREWAVE_GEN
	     | holy_PIT_CTRL_COUNT_BINARY, holy_PIT_CTRL);
  holy_outb (freq_count & 0xff, holy_PIT_COUNTER_2);		/* LSB */
  holy_outb ((freq_count >> 8) & 0xff, holy_PIT_COUNTER_2);	/* MSB */

  /* Start speaker.  */
  holy_outb (holy_inb (holy_PIT_SPEAKER_PORT)
	     | holy_PIT_SPK_TMR2 | holy_PIT_SPK_DATA,
	     holy_PIT_SPEAKER_PORT);

  for (; duration; duration--)
    {
      unsigned short counter, previous_counter = 0xffff;
      while (1)
	{
	  counter = holy_inb (holy_PIT_COUNTER_2);
	  counter |= ((holy_uint16_t) holy_inb (holy_PIT_COUNTER_2)) << 8;
	  if (counter > previous_counter)
	    {
	      previous_counter = counter;
	      break;
	    }
	  previous_counter = counter;
	}
    }
}

static int inited;

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  int i;

  make_tone (holy_SPEAKER_PIT_FREQUENCY / 200, 4);
  for (i = 7; i >= 0; i--)
    {
      if ((c >> i) & 1)
	make_tone (holy_SPEAKER_PIT_FREQUENCY / 2000, 20);
      else
	make_tone (holy_SPEAKER_PIT_FREQUENCY / 4000, 40);
      make_tone (holy_SPEAKER_PIT_FREQUENCY / 1000, 10);
    }
  make_tone (holy_SPEAKER_PIT_FREQUENCY / 200, 0);
}

static holy_err_t
holy_spkmodem_init_output (struct holy_term_output *term)
{
  /* Some models shutdown sound when not in use and it takes for it
     around 30 ms to come back on which loses 3 bits. So generate a base
     200 Hz continously. */

  make_tone (holy_SPEAKER_PIT_FREQUENCY / 200, 0);
  holy_terminfo_output_init (term);

  return 0;
}

static holy_err_t
holy_spkmodem_fini_output (struct holy_term_output *term __attribute__ ((unused)))
{
  holy_speaker_beep_off ();
  inited = 0;
  return 0;
}




struct holy_terminfo_output_state holy_spkmodem_terminfo_output =
  {
    .put = put,
    .size = { 80, 24 }
  };

static struct holy_term_output holy_spkmodem_term_output =
  {
    .name = "spkmodem",
    .init = holy_spkmodem_init_output,
    .fini = holy_spkmodem_fini_output,
    .putchar = holy_terminfo_putchar,
    .getxy = holy_terminfo_getxy,
    .getwh = holy_terminfo_getwh,
    .gotoxy = holy_terminfo_gotoxy,
    .cls = holy_terminfo_cls,
    .setcolorstate = holy_terminfo_setcolorstate,
    .setcursor = holy_terminfo_setcursor,
    .flags = holy_TERM_CODE_TYPE_ASCII,
    .data = &holy_spkmodem_terminfo_output,
    .progress_update_divisor = holy_PROGRESS_NO_UPDATE
  };

holy_MOD_INIT (spkmodem)
{
  holy_term_register_output ("spkmodem", &holy_spkmodem_term_output);
}


holy_MOD_FINI (spkmodem)
{
  holy_term_unregister_output (&holy_spkmodem_term_output);
}
