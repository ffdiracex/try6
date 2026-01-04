/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/normal.h>
#include <holy/charset.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>

holy_MOD_LICENSE ("GPLv2+");

struct guid_mapping
{
  holy_efi_guid_t guid;
  const char *name;
};

static const struct guid_mapping guid_mappings[] =
  {
    { holy_EFI_ACPI_20_TABLE_GUID, "ACPI-2.0"},
    { holy_EFI_ACPI_TABLE_GUID, "ACPI-1.0"},
    { holy_EFI_CRC32_GUIDED_SECTION_EXTRACTION_GUID,
      "CRC32 GUIDED SECTION EXTRACTION"},
    { holy_EFI_DEBUG_IMAGE_INFO_TABLE_GUID, "DEBUG IMAGE INFO"},
    { holy_EFI_DXE_SERVICES_TABLE_GUID, "DXE SERVICES"},
    { holy_EFI_HCDP_TABLE_GUID, "HCDP"},
    { holy_EFI_HOB_LIST_GUID, "HOB LIST"},
    { holy_EFI_LZMA_CUSTOM_DECOMPRESS_GUID, "LZMA CUSTOM DECOMPRESS"},
    { holy_EFI_MEMORY_TYPE_INFORMATION_GUID, "MEMORY TYPE INFO"},
    { holy_EFI_MPS_TABLE_GUID, "MPS"},
    { holy_EFI_SAL_TABLE_GUID, "SAL"},
    { holy_EFI_SMBIOS_TABLE_GUID, "SMBIOS"},
    { holy_EFI_SMBIOS3_TABLE_GUID, "SMBIOS3"},
    { holy_EFI_SYSTEM_RESOURCE_TABLE_GUID, "SYSTEM RESOURCE TABLE"},
    { holy_EFI_TIANO_CUSTOM_DECOMPRESS_GUID, "TIANO CUSTOM DECOMPRESS"},
    { holy_EFI_TSC_FREQUENCY_GUID, "TSC FREQUENCY"},
  };

static holy_err_t
holy_cmd_lsefisystab (struct holy_command *cmd __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char **args __attribute__ ((unused)))
{
  const holy_efi_system_table_t *st = holy_efi_system_table;
  holy_efi_configuration_table_t *t;
  unsigned int i;

  holy_printf ("Address: %p\n", st);
  holy_printf ("Signature: %016" PRIxholy_UINT64_T " revision: %08x\n",
	       st->hdr.signature, st->hdr.revision);
  {
    char *vendor;
    holy_uint16_t *vendor_utf16;
    holy_printf ("Vendor: ");
    
    for (vendor_utf16 = st->firmware_vendor; *vendor_utf16; vendor_utf16++);
    vendor = holy_malloc (4 * (vendor_utf16 - st->firmware_vendor) + 1);
    if (!vendor)
      return holy_errno;
    *holy_utf16_to_utf8 ((holy_uint8_t *) vendor, st->firmware_vendor,
			 vendor_utf16 - st->firmware_vendor) = 0;
    holy_printf ("%s", vendor);
    holy_free (vendor);
  }

  holy_printf (", Version=%x\n", st->firmware_revision);

  holy_printf ("%lld tables:\n", (long long) st->num_table_entries);
  t = st->configuration_table;
  for (i = 0; i < st->num_table_entries; i++)
    {
      unsigned int j;

      holy_printf ("%p  ", t->vendor_table);

      holy_printf ("%08x-%04x-%04x-",
		   t->vendor_guid.data1, t->vendor_guid.data2,
		   t->vendor_guid.data3);
      for (j = 0; j < 8; j++)
	holy_printf ("%02x", t->vendor_guid.data4[j]);
      
      for (j = 0; j < ARRAY_SIZE (guid_mappings); j++)
	if (holy_memcmp (&guid_mappings[j].guid, &t->vendor_guid,
			 sizeof (holy_efi_guid_t)) == 0)
	  holy_printf ("   %s", guid_mappings[j].name);

      holy_printf ("\n");
      t++;
    }
  return holy_ERR_NONE;
}

static holy_command_t cmd;

holy_MOD_INIT(lsefisystab)
{
  cmd = holy_register_command ("lsefisystab", holy_cmd_lsefisystab,
			       "", "Display EFI system tables.");
}

holy_MOD_FINI(lsefisystab)
{
  holy_unregister_command (cmd);
}
