/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efi/efi.h>
#include <holy/efi/console.h>
#include <holy/efi/disk.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/mm.h>
#include <holy/kernel.h>

holy_addr_t holy_modbase;

void
holy_efi_init (void)
{
  holy_modbase = holy_efi_modules_addr ();
  /* First of all, initialize the console so that holy can display
     messages.  */
  holy_console_init ();

  /* Initialize the memory management system.  */
  holy_efi_mm_init ();

  efi_call_4 (holy_efi_system_table->boot_services->set_watchdog_timer,
	      0, 0, 0, NULL);

  holy_efidisk_init ();
}

void (*holy_efi_net_config) (holy_efi_handle_t hnd,
			     char **device,
			     char **path);

void
holy_machine_get_bootlocation (char **device, char **path)
{
  holy_efi_loaded_image_t *image = NULL;
  char *p;

  image = holy_efi_get_loaded_image (holy_efi_image_handle);
  if (!image)
    return;
  *device = holy_efidisk_get_device_name (image->device_handle);
  if (!*device && holy_efi_net_config)
    {
      holy_efi_net_config (image->device_handle, device, path);
      return;
    }

  *path = holy_efi_get_filename (image->file_path);
  if (*path)
    {
      /* Get the directory.  */
      p = holy_strrchr (*path, '/');
      if (p)
        *p = '\0';
    }
}

void
holy_efi_fini (void)
{
  holy_efidisk_fini ();
  holy_console_fini ();
}
