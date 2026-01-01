/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef KERNEL_CPU_TSC_HEADER
#define KERNEL_CPU_TSC_HEADER   1

#include <holy/types.h>
#include <holy/i386/cpuid.h>

void holy_tsc_init (void);
/* In ms per 2^32 ticks.  */
extern holy_uint32_t EXPORT_VAR(holy_tsc_rate);
int
holy_tsc_calibrate_from_xen (void);
int
holy_tsc_calibrate_from_efi (void);
int
holy_tsc_calibrate_from_pmtimer (void);
int
holy_tsc_calibrate_from_pit (void);

/* Read the TSC value, which increments with each CPU clock cycle. */
static __inline holy_uint64_t
holy_get_tsc (void)
{
  holy_uint32_t lo, hi;
  holy_uint32_t a,b,c,d;

  /* The CPUID instruction is a 'serializing' instruction, and
     avoids out-of-order execution of the RDTSC instruction. */
  holy_cpuid (0,a,b,c,d);
  /* Read TSC value.  We cannot use "=A", since this would use
     %rax on x86_64. */
  __asm__ __volatile__ ("rdtsc":"=a" (lo), "=d" (hi));

  return (((holy_uint64_t) hi) << 32) | lo;
}

static __inline int
holy_cpu_is_tsc_supported (void)
{
#ifndef holy_MACHINE_XEN
  holy_uint32_t a,b,c,d;
  if (! holy_cpu_is_cpuid_supported ())
    return 0;

  holy_cpuid(1,a,b,c,d);

  return (d & (1 << 4)) != 0;
#else
  return 1;
#endif
}

#endif /* ! KERNEL_CPU_TSC_HEADER */
