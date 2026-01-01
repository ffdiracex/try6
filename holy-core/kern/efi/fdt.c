/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efi/efi.h>
#include <holy/mm.h>

void *
holy_efi_get_firmware_fdt (void)
{
  holy_efi_configuration_table_t *tables;
  holy_efi_guid_t fdt_guid = holy_EFI_DEVICE_TREE_GUID;
  void *firmware_fdt = NULL;
  unsigned int i;

  /* Look for FDT in UEFI config tables. */
  tables = holy_efi_system_table->configuration_table;

  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    if (holy_memcmp (&tables[i].vendor_guid, &fdt_guid, sizeof (fdt_guid)) == 0)
      {
	firmware_fdt = tables[i].vendor_table;
	holy_dprintf ("linux", "found registered FDT @ %p\n", firmware_fdt);
	break;
      }

  return firmware_fdt;
}
