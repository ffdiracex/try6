/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/acpi.h>
#include <holy/misc.h>
#include <holy/efi/efi.h>
#include <holy/efi/api.h>

struct holy_acpi_rsdp_v10 *
holy_machine_acpi_get_rsdpv1 (void)
{
  unsigned i;
  static holy_efi_packed_guid_t acpi_guid = holy_EFI_ACPI_TABLE_GUID;

  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    {
      holy_efi_packed_guid_t *guid =
	&holy_efi_system_table->configuration_table[i].vendor_guid;

      if (! holy_memcmp (guid, &acpi_guid, sizeof (holy_efi_packed_guid_t)))
	return (struct holy_acpi_rsdp_v10 *)
	  holy_efi_system_table->configuration_table[i].vendor_table;
    }
  return 0;
}

struct holy_acpi_rsdp_v20 *
holy_machine_acpi_get_rsdpv2 (void)
{
  unsigned i;
  static holy_efi_packed_guid_t acpi20_guid = holy_EFI_ACPI_20_TABLE_GUID;

  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    {
      holy_efi_packed_guid_t *guid =
	&holy_efi_system_table->configuration_table[i].vendor_guid;

      if (! holy_memcmp (guid, &acpi20_guid, sizeof (holy_efi_packed_guid_t)))
	return (struct holy_acpi_rsdp_v20 *)
	  holy_efi_system_table->configuration_table[i].vendor_table;
    }
  return 0;
}
