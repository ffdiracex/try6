/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/efiemu/efiemu.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/acpi.h>
#include <holy/smbios.h>

holy_err_t
holy_machine_efiemu_init_tables (void)
{
  void *table;
  holy_err_t err;
  holy_efi_guid_t smbios = holy_EFI_SMBIOS_TABLE_GUID;
  holy_efi_guid_t smbios3 = holy_EFI_SMBIOS3_TABLE_GUID;
  holy_efi_guid_t acpi20 = holy_EFI_ACPI_20_TABLE_GUID;
  holy_efi_guid_t acpi = holy_EFI_ACPI_TABLE_GUID;

  err = holy_efiemu_unregister_configuration_table (smbios);
  if (err)
    return err;
  err = holy_efiemu_unregister_configuration_table (smbios3);
  if (err)
    return err;
  err = holy_efiemu_unregister_configuration_table (acpi);
  if (err)
    return err;
  err = holy_efiemu_unregister_configuration_table (acpi20);
  if (err)
    return err;

  table = holy_smbios_get_eps ();
  if (table)
    {
      err = holy_efiemu_register_configuration_table (smbios, 0, 0, table);
      if (err)
	return err;
    }
  table = holy_smbios_get_eps3 ();
  if (table)
    {
      err = holy_efiemu_register_configuration_table (smbios3, 0, 0, table);
      if (err)
	return err;
    }
  table = holy_acpi_get_rsdpv1 ();
  if (table)
    {
      err = holy_efiemu_register_configuration_table (acpi, 0, 0, table);
      if (err)
	return err;
    }
  table = holy_acpi_get_rsdpv2 ();
  if (table)
    {
      err = holy_efiemu_register_configuration_table (acpi20, 0, 0, table);
      if (err)
	return err;
    }

  return holy_ERR_NONE;
}
