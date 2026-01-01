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

static holy_uint64_t tmr;
static holy_efi_event_t tmr_evt;

static holy_uint64_t
holy_efi_get_time_ms (void)
{
  return tmr;
}

static void 
increment_timer (holy_efi_event_t event __attribute__ ((unused)),
		 void *context __attribute__ ((unused)))
{
  tmr += 10;
}

void
holy_machine_init (void)
{
  holy_efi_boot_services_t *b;

  holy_efi_init ();

  b = holy_efi_system_table->boot_services;

  efi_call_5 (b->create_event, holy_EFI_EVT_TIMER | holy_EFI_EVT_NOTIFY_SIGNAL,
	      holy_EFI_TPL_CALLBACK, increment_timer, NULL, &tmr_evt);
  efi_call_3 (b->set_timer, tmr_evt, holy_EFI_TIMER_PERIODIC, 100000);

  holy_install_get_time_ms (holy_efi_get_time_ms);
}

void
holy_machine_fini (int flags)
{
  holy_efi_boot_services_t *b;

  if (!(flags & holy_LOADER_FLAG_NORETURN))
    return;

  b = holy_efi_system_table->boot_services;

  efi_call_3 (b->set_timer, tmr_evt, holy_EFI_TIMER_CANCEL, 0);
  efi_call_1 (b->close_event, tmr_evt);

  holy_efi_fini ();
}
