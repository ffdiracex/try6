/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FLOPPY_CPU_HEADER
#define holy_FLOPPY_CPU_HEADER	1

#define holy_FLOPPY_REG_DIGITAL_OUTPUT		0x3f2

#ifndef ASM_FILE
#include <holy/cpu/io.h>

/* Stop the floppy drive from spinning, so that other software is
   jumped to with a known state.  */
static inline void
holy_stop_floppy (void)
{
  holy_outb (0, holy_FLOPPY_REG_DIGITAL_OUTPUT);
}
#endif

#endif
