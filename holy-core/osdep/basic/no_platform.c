/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/install.h>
#include <holy/util/misc.h>

#include "platform.c"

void
holy_install_register_ieee1275 (int is_prep, const char *install_device,
				int partno, const char *relpath)
{
  holy_util_error ("%s", _("no IEEE1275 routines are available for your platform"));
}

void
holy_install_register_efi (holy_device_t efidir_holy_dev,
			   const char *efifile_path,
			   const char *efi_distributor)
{
  holy_util_error ("%s", _("no EFI routines are available for your platform"));
}

void
holy_install_sgi_setup (const char *install_device,
			const char *imgfile, const char *destname)
{
  holy_util_error ("%s", _("no SGI routines are available for your platform"));
}
