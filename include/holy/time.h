/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_TIME_HEADER
#define KERNEL_TIME_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>
#if !defined(holy_MACHINE_EMU) && !defined(holy_UTIL)
#include <holy/cpu/time.h>
#else
static inline void
holy_cpu_idle(void)
{
}
#endif

void EXPORT_FUNC(holy_millisleep) (holy_uint32_t ms);
holy_uint64_t EXPORT_FUNC(holy_get_time_ms) (void);

holy_uint64_t holy_rtc_get_time_ms (void);

static __inline void
holy_sleep (holy_uint32_t s)
{
  holy_millisleep (1000 * s);
}

void holy_install_get_time_ms (holy_uint64_t (*get_time_ms_func) (void));

#endif /* ! KERNEL_TIME_HEADER */
