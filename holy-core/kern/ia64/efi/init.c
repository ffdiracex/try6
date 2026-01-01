/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/kernel.h>
#include <holy/efi/efi.h>
#include <holy/loader.h>

static holy_uint64_t divisor = 1;

static holy_uint64_t
get_itc (void)
{
  holy_uint64_t ret;
  asm volatile ("mov %0=ar.itc" : "=r" (ret));
  return ret;
}

holy_uint64_t
holy_rtc_get_time_ms (void)
{
  return get_itc () / divisor;
}

void
holy_machine_init (void)
{
  holy_efi_event_t event;
  holy_uint64_t before, after;
  holy_efi_uintn_t idx;
  holy_efi_init ();

  efi_call_5 (holy_efi_system_table->boot_services->create_event,
	      holy_EFI_EVT_TIMER, holy_EFI_TPL_CALLBACK, 0, 0, &event);

  before = get_itc ();
  efi_call_3 (holy_efi_system_table->boot_services->set_timer, event,
	      holy_EFI_TIMER_RELATIVE, 200000);
  efi_call_3 (holy_efi_system_table->boot_services->wait_for_event, 1,
	      &event, &idx);
  after = get_itc ();
  efi_call_1 (holy_efi_system_table->boot_services->close_event, event);
  divisor = (after - before + 5) / 20;
  if (divisor == 0)
    divisor = 800000;
  holy_install_get_time_ms (holy_rtc_get_time_ms);
}

void
holy_machine_fini (int flags)
{
  if (flags & holy_LOADER_FLAG_NORETURN)
    holy_efi_fini ();
}
