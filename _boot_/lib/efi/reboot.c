/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/kernel.h>
#include <holy/loader.h>

void
holy_reboot (void)
{
  holy_machine_fini (holy_LOADER_FLAG_NORETURN);
  efi_call_4 (holy_efi_system_table->runtime_services->reset_system,
              holy_EFI_RESET_COLD, holy_EFI_SUCCESS, 0, NULL);
  for (;;) ;
}
