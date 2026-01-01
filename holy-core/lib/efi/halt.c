/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/kernel.h>
#include <holy/acpi.h>
#include <holy/loader.h>

void
holy_halt (void)
{
  holy_machine_fini (holy_LOADER_FLAG_NORETURN);
#if !defined(__ia64__) && !defined(__arm__) && !defined(__aarch64__)
  holy_acpi_halt ();
#endif
  efi_call_4 (holy_efi_system_table->runtime_services->reset_system,
              holy_EFI_RESET_SHUTDOWN, holy_EFI_SUCCESS, 0, NULL);

  while (1);
}
