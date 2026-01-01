/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/fdt.h>
#include <holy/mm.h>
#include <holy/cpu/fdtload.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/file.h>
#include <holy/efi/efi.h>

static void *loaded_fdt;
static void *fdt;

void *
holy_fdt_load (holy_size_t additional_size)
{
  void *raw_fdt;
  holy_size_t size;

  if (fdt)
    {
      size = holy_EFI_BYTES_TO_PAGES (holy_fdt_get_totalsize (fdt));
      holy_efi_free_pages ((holy_efi_physical_address_t) fdt, size);
    }

  if (loaded_fdt)
    raw_fdt = loaded_fdt;
  else
    raw_fdt = holy_efi_get_firmware_fdt();

  size =
    raw_fdt ? holy_fdt_get_totalsize (raw_fdt) : holy_FDT_EMPTY_TREE_SZ;
  size += additional_size;

  holy_dprintf ("linux", "allocating %ld bytes for fdt\n", size);
  fdt = holy_efi_allocate_pages (0, holy_EFI_BYTES_TO_PAGES (size));
  if (!fdt)
    return NULL;

  if (raw_fdt)
    {
      holy_memmove (fdt, raw_fdt, size);
      holy_fdt_set_totalsize (fdt, size);
    }
  else
    {
      holy_fdt_create_empty_tree (fdt, size);
    }
  return fdt;
}

holy_err_t
holy_fdt_install (void)
{
  holy_efi_boot_services_t *b;
  holy_efi_guid_t fdt_guid = holy_EFI_DEVICE_TREE_GUID;
  holy_efi_status_t status;

  b = holy_efi_system_table->boot_services;
  status = b->install_configuration_table (&fdt_guid, fdt);
  if (status != holy_EFI_SUCCESS)
    return holy_error (holy_ERR_IO, "failed to install FDT");

  holy_dprintf ("fdt", "Installed/updated FDT configuration table @ %p\n",
		fdt);
  return holy_ERR_NONE;
}

void
holy_fdt_unload (void) {
  if (!fdt) {
    return;
  }
  holy_efi_free_pages ((holy_efi_physical_address_t) fdt,
		       holy_EFI_BYTES_TO_PAGES (holy_fdt_get_totalsize (fdt)));
  fdt = NULL;
}

static holy_err_t
holy_cmd_devicetree (holy_command_t cmd __attribute__ ((unused)),
		     int argc, char *argv[])
{
  holy_file_t dtb;
  void *blob = NULL;
  int size;

  if (loaded_fdt)
    holy_free (loaded_fdt);
  loaded_fdt = NULL;

  /* No arguments means "use firmware FDT".  */
  if (argc == 0)
    {
      return holy_ERR_NONE;
    }

  dtb = holy_file_open (argv[0]);
  if (!dtb)
    goto out;

  size = holy_file_size (dtb);
  blob = holy_malloc (size);
  if (!blob)
    goto out;

  if (holy_file_read (dtb, blob, size) < size)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"), argv[0]);
      goto out;
    }

  if (holy_fdt_check_header (blob, size) != 0)
    {
      holy_error (holy_ERR_BAD_OS, N_("invalid device tree"));
      goto out;
    }

out:
  if (dtb)
    holy_file_close (dtb);

  if (blob)
    {
      if (holy_errno == holy_ERR_NONE)
	loaded_fdt = blob;
      else
	holy_free (blob);
    }

  return holy_errno;
}

static holy_command_t cmd_devicetree;

holy_MOD_INIT (fdt)
{
  cmd_devicetree =
    holy_register_command ("devicetree", holy_cmd_devicetree, 0,
			   N_("Load DTB file."));
}

holy_MOD_FINI (fdt)
{
  holy_unregister_command (cmd_devicetree);
}
