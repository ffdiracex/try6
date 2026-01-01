/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_CPU_TIME_HEADER
#define KERNEL_CPU_TIME_HEADER	1

static __inline void
holy_cpu_idle (void)
{
  /* FIXME: this can't work until we handle interrupts.  */
/*  __asm__ __volatile__ ("wfi"); */
}

#endif /* ! KERNEL_CPU_TIME_HEADER */
