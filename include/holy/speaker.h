/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SPEAKER_HEADER
#define holy_SPEAKER_HEADER 1

#include <holy/cpu/io.h>
#include <holy/i386/pit.h>

/* The frequency of the PIT clock.  */
#define holy_SPEAKER_PIT_FREQUENCY		0x1234dd

static inline void
holy_speaker_beep_off (void)
{
  unsigned char status;

  status = holy_inb (holy_PIT_SPEAKER_PORT);
  holy_outb (status & ~(holy_PIT_SPK_TMR2 | holy_PIT_SPK_DATA),
	     holy_PIT_SPEAKER_PORT);
}

static inline void
holy_speaker_beep_on (holy_uint16_t pitch)
{
  unsigned char status;
  unsigned int counter;

  if (pitch < 20)
    pitch = 20;
  else if (pitch > 20000)
    pitch = 20000;

  counter = holy_SPEAKER_PIT_FREQUENCY / pitch;

  /* Program timer 2.  */
  holy_outb (holy_PIT_CTRL_SELECT_2
	     | holy_PIT_CTRL_READLOAD_WORD
	     | holy_PIT_CTRL_SQUAREWAVE_GEN
	     | holy_PIT_CTRL_COUNT_BINARY, holy_PIT_CTRL);
  holy_outb (counter & 0xff, holy_PIT_COUNTER_2);		/* LSB */
  holy_outb ((counter >> 8) & 0xff, holy_PIT_COUNTER_2);	/* MSB */

  /* Start speaker.  */
  status = holy_inb (holy_PIT_SPEAKER_PORT);
  holy_outb (status | holy_PIT_SPK_TMR2 | holy_PIT_SPK_DATA,
	     holy_PIT_SPEAKER_PORT);
}

#endif
