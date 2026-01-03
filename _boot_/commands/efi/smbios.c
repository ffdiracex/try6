/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/smbios.h>
#include <holy/misc.h>
#include <holy/efi/efi.h>
#include <holy/efi/api.h>

struct holy_smbios_eps *
holy_machine_smbios_get_eps (void)
{
  unsigned i;
  static holy_efi_packed_guid_t smbios_guid = holy_EFI_SMBIOS_TABLE_GUID;

  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    {
      holy_efi_packed_guid_t *guid =
	&holy_efi_system_table->configuration_table[i].vendor_guid;

      if (! holy_memcmp (guid, &smbios_guid, sizeof (holy_efi_packed_guid_t)))
	return (struct holy_smbios_eps *)
	  holy_efi_system_table->configuration_table[i].vendor_table;
    }
  return 0;
}

struct holy_smbios_eps3 *
holy_machine_smbios_get_eps3 (void)
{
  unsigned i;
  static holy_efi_packed_guid_t smbios3_guid = holy_EFI_SMBIOS3_TABLE_GUID;

  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    {
      holy_efi_packed_guid_t *guid =
	&holy_efi_system_table->configuration_table[i].vendor_guid;

      if (! holy_memcmp (guid, &smbios3_guid, sizeof (holy_efi_packed_guid_t)))
	return (struct holy_smbios_eps3 *)
	  holy_efi_system_table->configuration_table[i].vendor_table;
    }
  return 0;
}
