/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/xen.h>

holy_MOD_LICENSE ("GPLv2+");

holy_err_t
holy_get_datetime (struct holy_datetime *datetime)
{
  long long nix;
  nix = (holy_xen_shared_info->wc_sec
	 + holy_divmod64 (holy_xen_shared_info->vcpu_info[0].time.system_time, 1000000000, 0));
  holy_unixtime2datetime (nix, datetime);
  return holy_ERR_NONE;
}

holy_err_t
holy_set_datetime (struct holy_datetime *datetime __attribute__ ((unused)))
{
  return holy_error (holy_ERR_IO, "setting time isn't supported");
}
