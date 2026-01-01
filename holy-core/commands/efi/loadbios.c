/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/file.h>
#include <holy/efi/efi.h>
#include <holy/pci.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_efi_guid_t acpi_guid = holy_EFI_ACPI_TABLE_GUID;
static holy_efi_guid_t acpi2_guid = holy_EFI_ACPI_20_TABLE_GUID;
static holy_efi_guid_t smbios_guid = holy_EFI_SMBIOS_TABLE_GUID;
static holy_efi_guid_t smbios3_guid = holy_EFI_SMBIOS3_TABLE_GUID;

#define EBDA_SEG_ADDR	0x40e
#define LOW_MEM_ADDR	0x413
#define FAKE_EBDA_SEG	0x9fc0

#define BLANK_MEM	0xffffffff
#define VBIOS_ADDR	0xc0000
#define SBIOS_ADDR	0xf0000

static int
enable_rom_area (void)
{
  holy_pci_address_t addr;
  holy_uint32_t *rom_ptr;
  holy_pci_device_t dev = { .bus = 0, .device = 0, .function = 0};

  rom_ptr = (holy_uint32_t *) VBIOS_ADDR;
  if (*rom_ptr != BLANK_MEM)
    {
      holy_puts_ (N_("ROM image is present."));
      return 0;
    }

  /* FIXME: should be macroified.  */
  addr = holy_pci_make_address (dev, 144);
  holy_pci_write_byte (addr++, 0x30);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr++, 0x33);
  holy_pci_write_byte (addr, 0);

  *rom_ptr = 0;
  if (*rom_ptr != 0)
    {
      holy_puts_ (N_("Can\'t enable ROM area."));
      return 0;
    }

  return 1;
}

static void
lock_rom_area (void)
{
  holy_pci_address_t addr;
  holy_pci_device_t dev = { .bus = 0, .device = 0, .function = 0};

  /* FIXME: should be macroified.  */
  addr = holy_pci_make_address (dev, 144);
  holy_pci_write_byte (addr++, 0x10);
  holy_pci_write_byte (addr++, 0x11);
  holy_pci_write_byte (addr++, 0x11);
  holy_pci_write_byte (addr++, 0x11);
  holy_pci_write_byte (addr, 0x11);
}

static void
fake_bios_data (int use_rom)
{
  unsigned i;
  void *acpi, *smbios, *smbios3;
  holy_uint16_t *ebda_seg_ptr, *low_mem_ptr;

  ebda_seg_ptr = (holy_uint16_t *) EBDA_SEG_ADDR;
  low_mem_ptr = (holy_uint16_t *) LOW_MEM_ADDR;
  if ((*ebda_seg_ptr) || (*low_mem_ptr))
    return;

  acpi = 0;
  smbios = 0;
  smbios3 = 0;
  for (i = 0; i < holy_efi_system_table->num_table_entries; i++)
    {
      holy_efi_packed_guid_t *guid =
	&holy_efi_system_table->configuration_table[i].vendor_guid;

      if (! holy_memcmp (guid, &acpi2_guid, sizeof (holy_efi_guid_t)))
	{
	  acpi = holy_efi_system_table->configuration_table[i].vendor_table;
	  holy_dprintf ("efi", "ACPI2: %p\n", acpi);
	}
      else if (! holy_memcmp (guid, &acpi_guid, sizeof (holy_efi_guid_t)))
	{
	  void *t;

	  t = holy_efi_system_table->configuration_table[i].vendor_table;
	  if (! acpi)
	    acpi = t;
	  holy_dprintf ("efi", "ACPI: %p\n", t);
	}
      else if (! holy_memcmp (guid, &smbios_guid, sizeof (holy_efi_guid_t)))
	{
	  smbios = holy_efi_system_table->configuration_table[i].vendor_table;
	  holy_dprintf ("efi", "SMBIOS: %p\n", smbios);
	}
      else if (! holy_memcmp (guid, &smbios3_guid, sizeof (holy_efi_guid_t)))
	{
	  smbios3 = holy_efi_system_table->configuration_table[i].vendor_table;
	  holy_dprintf ("efi", "SMBIOS3: %p\n", smbios3);
	}
    }

  *ebda_seg_ptr = FAKE_EBDA_SEG;
  *low_mem_ptr = (FAKE_EBDA_SEG >> 6);

  *((holy_uint16_t *) (FAKE_EBDA_SEG << 4)) = 640 - *low_mem_ptr;

  if (acpi)
    holy_memcpy ((char *) ((FAKE_EBDA_SEG << 4) + 16), acpi, 1024 - 16);

  if (use_rom)
    {
      if (smbios)
	holy_memcpy ((char *) SBIOS_ADDR, (char *) smbios, 31);
      if (smbios3)
	holy_memcpy ((char *) SBIOS_ADDR + 32, (char *) smbios3, 24);
    }
}

static holy_err_t
holy_cmd_fakebios (struct holy_command *cmd __attribute__ ((unused)),
		   int argc __attribute__ ((unused)),
		   char *argv[] __attribute__ ((unused)))
{
  if (enable_rom_area ())
    {
      fake_bios_data (1);
      lock_rom_area ();
    }
  else
    fake_bios_data (0);

  return 0;
}

static holy_err_t
holy_cmd_loadbios (holy_command_t cmd __attribute__ ((unused)),
		   int argc, char *argv[])
{
  holy_file_t file;
  int size;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  if (argc > 1)
    {
      file = holy_file_open (argv[1]);
      if (! file)
	return holy_errno;

      if (file->size != 4)
	holy_error (holy_ERR_BAD_ARGUMENT, "invalid int10 dump size");
      else
	holy_file_read (file, (void *) 0x40, 4);

      holy_file_close (file);
      if (holy_errno)
	return holy_errno;
    }

  file = holy_file_open (argv[0]);
  if (! file)
    return holy_errno;

  size = file->size;
  if ((size < 0x10000) || (size > 0x40000))
    holy_error (holy_ERR_BAD_ARGUMENT, "invalid bios dump size");
  else if (enable_rom_area ())
    {
      holy_file_read (file, (void *) VBIOS_ADDR, size);
      fake_bios_data (size <= 0x40000);
      lock_rom_area ();
    }

  holy_file_close (file);
  return holy_errno;
}

static holy_command_t cmd_fakebios, cmd_loadbios;

holy_MOD_INIT(loadbios)
{
  cmd_fakebios = holy_register_command ("fakebios", holy_cmd_fakebios,
					0, N_("Create BIOS-like structures for"
					      " backward compatibility with"
					      " existing OS."));

  cmd_loadbios = holy_register_command ("loadbios", holy_cmd_loadbios,
					N_("BIOS_DUMP [INT10_DUMP]"),
					N_("Load BIOS dump."));
}

holy_MOD_FINI(loadbios)
{
  holy_unregister_command (cmd_fakebios);
  holy_unregister_command (cmd_loadbios);
}
