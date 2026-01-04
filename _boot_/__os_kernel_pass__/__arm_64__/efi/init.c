/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/efi/efi.h>
#include <holy/loader.h>

static holy_uint64_t timer_frequency_in_khz;

static holy_uint64_t
holy_efi_get_time_ms (void)
{
  holy_uint64_t tmr;
  asm volatile("mrs %0,   cntvct_el0" : "=r" (tmr));

  return tmr / timer_frequency_in_khz;
}


void
holy_machine_init (void)
{
  holy_uint64_t timer_frequency;

  holy_efi_init ();

  asm volatile("mrs %0,   cntfrq_el0" : "=r" (timer_frequency));
  timer_frequency_in_khz = timer_frequency / 1000;

  holy_install_get_time_ms (holy_efi_get_time_ms);
}

void
holy_machine_fini (int flags)
{
  if (!(flags & holy_LOADER_FLAG_NORETURN))
    return;

  holy_efi_fini ();
}
