/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_CPU_PIT_HEADER
#define KERNEL_CPU_PIT_HEADER   1

#include <holy/types.h>
#include <holy/err.h>

enum
  {
    /* The PIT channel value ports.  You can write to and read from them.
       Do not mess with timer 0 or 1.  */
    holy_PIT_COUNTER_0 = 0x40,
    holy_PIT_COUNTER_1 = 0x41,
    holy_PIT_COUNTER_2 = 0x42,
    /* The PIT control port.  You can only write to it.  Do not mess with
       timer 0 or 1.  */
    holy_PIT_CTRL = 0x43,
    /* The speaker port.  */
    holy_PIT_SPEAKER_PORT = 0x61,
  };


/* The speaker port.  */
enum
  {
    /* If 0, follow state of SPEAKER_DATA bit, otherwise enable output
       from timer 2.  */
    holy_PIT_SPK_TMR2 = 0x01,
    /* If SPEAKER_TMR2 is not set, this provides direct input into the
       speaker.  Otherwise, this enables or disables the output from the
       timer.  */
    holy_PIT_SPK_DATA = 0x02,

    holy_PIT_SPK_TMR2_LATCH = 0x20
  };

/* The PIT control port.  You can only write to it.  Do not mess with
   timer 0 or 1.  */
enum
  {
    holy_PIT_CTRL_SELECT_MASK = 0xc0,
    holy_PIT_CTRL_SELECT_0 = 0x00,
    holy_PIT_CTRL_SELECT_1 = 0x40,
    holy_PIT_CTRL_SELECT_2 = 0x80,

    /* Read and load control.  */
    holy_PIT_CTRL_READLOAD_MASK= 0x30,
    holy_PIT_CTRL_COUNTER_LATCH = 0x00,	/* Hold timer value until read.  */
    holy_PIT_CTRL_READLOAD_LSB = 0x10,	/* Read/load the LSB.  */
    holy_PIT_CTRL_READLOAD_MSB = 0x20,	/* Read/load the MSB.  */
    holy_PIT_CTRL_READLOAD_WORD = 0x30,	/* Read/load the LSB then the MSB.  */

    /* Mode control.  */
    holy_PIT_CTRL_MODE_MASK = 0x0e,
    /* Interrupt on terminal count.  Setting the mode sets output to low.
       When counter is set and terminated, output is set to high.  */
    holy_PIT_CTRL_INTR_ON_TERM = 0x00,
    /* Programmable one-shot.  When loading counter, output is set to
       high.  When counter terminated, output is set to low.  Can be
       triggered again from that point on by setting the gate pin to
       high.  */
    holy_PIT_CTRL_PROGR_ONE_SHOT = 0x02,

    /* Rate generator.  Output is low for one period of the counter, and
       high for the other.  */
    holy_PIT_CTRL_RATE_GEN = 0x04,

    /* Square wave generator.  Output is low for one half of the period,
       and high for the other half.  */
    holy_PIT_CTRL_SQUAREWAVE_GEN = 0x06,
    /* Software triggered strobe.  Setting the mode sets output to high.
       When counter is set and terminated, output is set to low.  */
    holy_PIT_CTRL_SOFTSTROBE = 0x08,

    /* Hardware triggered strobe.  Like software triggered strobe, but
       only starts the counter when the gate pin is set to high.  */
    holy_PIT_CTRL_HARDSTROBE = 0x0a,


    /* Count mode.  */
    holy_PIT_CTRL_COUNT_MASK = 0x01,
    holy_PIT_CTRL_COUNT_BINARY = 0x00,	/* 16-bit binary counter.  */
    holy_PIT_CTRL_COUNT_BCD = 0x01	/* 4-decade BCD counter.  */
  };

#endif /* ! KERNEL_CPU_PIT_HEADER */
