/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_CPU_TIME_HEADER
#define KERNEL_CPU_TIME_HEADER	1

#ifndef holy_UTIL

#define holy_TICKS_PER_SECOND	(holy_arch_cpuclock / 2)

/* Return the real time in ticks.  */
holy_uint64_t EXPORT_FUNC (holy_get_rtc) (void);

extern holy_uint32_t EXPORT_VAR (holy_arch_cpuclock) __attribute__ ((section(".text")));
#endif

static inline void
holy_cpu_idle(void)
{
}

#endif
