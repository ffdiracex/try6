/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/symbol.h>
#include <holy/uboot/uboot.h>
#include <holy/datetime.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

/* No simple platform-independent RTC access exists in U-Boot. */

holy_err_t
holy_get_datetime (struct holy_datetime *datetime __attribute__ ((unused)))
{
  return holy_error (holy_ERR_INVALID_COMMAND,
		     "can\'t get datetime using U-Boot");
}

holy_err_t
holy_set_datetime (struct holy_datetime * datetime __attribute__ ((unused)))
{
  return holy_error (holy_ERR_INVALID_COMMAND,
		     "can\'t set datetime using U-Boot");
}
