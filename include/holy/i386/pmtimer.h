/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_CPU_PMTIMER_HEADER
#define KERNEL_CPU_PMTIMER_HEADER   1

#include <holy/i386/tsc.h>
#include <holy/i386/io.h>

/*
  Preconditions:
  * Caller has ensured that both pmtimer and tsc are supported
  * 1 <= num_pm_ticks <= 3580
  Return:
  * Number of TSC ticks elapsed
  * 0 on failure.
*/
holy_uint64_t
EXPORT_FUNC(holy_pmtimer_wait_count_tsc) (holy_port_t pmtimer,
					  holy_uint16_t num_pm_ticks);

#endif
