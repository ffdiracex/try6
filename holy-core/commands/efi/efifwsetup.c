/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_cmd_fwsetup (holy_command_t cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  holy_efi_uint64_t *old_os_indications;
  holy_efi_uint64_t os_indications = holy_EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
  holy_err_t status;
  holy_size_t oi_size;
  holy_efi_guid_t global = holy_EFI_GLOBAL_VARIABLE_GUID;

  old_os_indications = holy_efi_get_variable ("OsIndications", &global,
					      &oi_size);

  if (old_os_indications != NULL && oi_size == sizeof (os_indications))
    os_indications |= *old_os_indications;

  status = holy_efi_set_variable ("OsIndications", &global, &os_indications,
				  sizeof (os_indications));
  if (status != holy_ERR_NONE)
    return status;

  holy_reboot ();

  return holy_ERR_BUG;
}

static holy_command_t cmd = NULL;

static holy_efi_boolean_t
efifwsetup_is_supported (void)
{
  holy_efi_uint64_t *os_indications_supported = NULL;
  holy_size_t oi_size = 0;
  holy_efi_guid_t global = holy_EFI_GLOBAL_VARIABLE_GUID;

  os_indications_supported = holy_efi_get_variable ("OsIndicationsSupported",
						    &global, &oi_size);

  if (!os_indications_supported)
    return 0;

  if (*os_indications_supported & holy_EFI_OS_INDICATIONS_BOOT_TO_FW_UI)
    return 1;

  return 0;
}

holy_MOD_INIT (efifwsetup)
{
  if (efifwsetup_is_supported ())
    cmd = holy_register_command ("fwsetup", holy_cmd_fwsetup, NULL,
				 N_("Reboot into firmware setup menu."));

}

holy_MOD_FINI (efifwsetup)
{
  if (cmd)
    holy_unregister_command (cmd);
}
