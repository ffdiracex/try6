/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/i386/tsc.h>
#include <holy/efi/efi.h>
#include <holy/efi/api.h>

int
holy_tsc_calibrate_from_efi (void)
{
  holy_uint64_t start_tsc, end_tsc;
  /* Use EFI Time Service to calibrate TSC */
  start_tsc = holy_get_tsc ();
  efi_call_1 (holy_efi_system_table->boot_services->stall, 1000);
  end_tsc = holy_get_tsc ();
  holy_tsc_rate = holy_divmod64 ((1ULL << 32), end_tsc - start_tsc, 0);
  return 1;
}
