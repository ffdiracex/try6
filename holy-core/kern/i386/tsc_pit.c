/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/i386/tsc.h>
#include <holy/i386/pit.h>
#include <holy/cpu/io.h>

static int
holy_pit_wait (void)
{
  int ret = 0;

  /* Disable timer2 gate and speaker.  */
  holy_outb (holy_inb (holy_PIT_SPEAKER_PORT)
	     & ~ (holy_PIT_SPK_DATA | holy_PIT_SPK_TMR2),
             holy_PIT_SPEAKER_PORT);

  /* Set tics.  */
  holy_outb (holy_PIT_CTRL_SELECT_2 | holy_PIT_CTRL_READLOAD_WORD,
	     holy_PIT_CTRL);
  /* 0xffff ticks: 55ms. */
  holy_outb (0xff, holy_PIT_COUNTER_2);
  holy_outb (0xff, holy_PIT_COUNTER_2);

  /* Enable timer2 gate, keep speaker disabled.  */
  holy_outb ((holy_inb (holy_PIT_SPEAKER_PORT) & ~ holy_PIT_SPK_DATA)
	     | holy_PIT_SPK_TMR2,
             holy_PIT_SPEAKER_PORT);

  if ((holy_inb (holy_PIT_SPEAKER_PORT) & holy_PIT_SPK_TMR2_LATCH) == 0x00) {
    ret = 1;
    /* Wait.  */
    while ((holy_inb (holy_PIT_SPEAKER_PORT) & holy_PIT_SPK_TMR2_LATCH) == 0x00);
  }

  /* Disable timer2 gate and speaker.  */
  holy_outb (holy_inb (holy_PIT_SPEAKER_PORT)
	     & ~ (holy_PIT_SPK_DATA | holy_PIT_SPK_TMR2),
             holy_PIT_SPEAKER_PORT);

  return ret;
}

/* Calibrate the TSC based on the RTC.  */
int
holy_tsc_calibrate_from_pit (void)
{
  /* First calibrate the TSC rate (relative, not absolute time). */
  holy_uint64_t start_tsc, end_tsc;

  start_tsc = holy_get_tsc ();
  if (!holy_pit_wait ())
    return 0;
  end_tsc = holy_get_tsc ();

  holy_tsc_rate = 0;
  if (end_tsc > start_tsc)
    holy_tsc_rate = holy_divmod64 ((55ULL << 32), end_tsc - start_tsc, 0);
  if (holy_tsc_rate == 0)
    return 0;
  return 1;
}
