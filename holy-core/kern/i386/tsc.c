/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/i386/tsc.h>
#include <holy/i386/cpuid.h>

/* This defines the value TSC had at the epoch (that is, when we calibrated it). */
static holy_uint64_t tsc_boot_time;

/* Calibrated TSC rate.  (In ms per 2^32 ticks) */
/* We assume that the tick is less than 1 ms and hence this value fits
   in 32-bit.  */
holy_uint32_t holy_tsc_rate;

static holy_uint64_t
holy_tsc_get_time_ms (void)
{
  holy_uint64_t a = holy_get_tsc () - tsc_boot_time;
  holy_uint64_t ah = a >> 32;
  holy_uint64_t al = a & 0xffffffff;

  return ((al * holy_tsc_rate) >> 32) + ah * holy_tsc_rate;
}

static int
calibrate_tsc_hardcode (void)
{
  holy_tsc_rate = 5368;/* 800 MHz */
  return 1;
}

void
holy_tsc_init (void)
{
  if (!holy_cpu_is_tsc_supported ())
    {
#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_IEEE1275)
      holy_install_get_time_ms (holy_rtc_get_time_ms);
#else
      holy_fatal ("no TSC found");
#endif
      return;
    }

  tsc_boot_time = holy_get_tsc ();

#ifdef holy_MACHINE_XEN
  (void) (holy_tsc_calibrate_from_xen () || calibrate_tsc_hardcode());
#elif defined (holy_MACHINE_EFI)
  (void) (holy_tsc_calibrate_from_pit () || holy_tsc_calibrate_from_pmtimer () || holy_tsc_calibrate_from_efi() || calibrate_tsc_hardcode());
#elif defined (holy_MACHINE_COREBOOT)
  (void) (holy_tsc_calibrate_from_pmtimer () || holy_tsc_calibrate_from_pit () || calibrate_tsc_hardcode());
#else
  (void) (holy_tsc_calibrate_from_pit () || calibrate_tsc_hardcode());
#endif
  holy_install_get_time_ms (holy_tsc_get_time_ms);
}
