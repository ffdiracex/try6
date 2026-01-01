/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/datetime.h>
#include <holy/cmos.h>
#include <holy/dl.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/misc.h>

volatile holy_uint8_t *holy_cmos_port = 0;

/* Helper for holy_cmos_find_port.  */
static int
holy_cmos_find_port_iter (struct holy_ieee1275_devalias *alias)
{
  holy_ieee1275_phandle_t dev;
  holy_uint32_t addr[2];
  holy_ssize_t actual;
  /* Enough to check if it's "m5819" */
  char compat[100];
  if (holy_ieee1275_finddevice (alias->path, &dev))
    return 0;
  if (holy_ieee1275_get_property (dev, "compatible", compat, sizeof (compat),
				  0))
    return 0;
  if (holy_strcmp (compat, "m5819") != 0)
    return 0;
  if (holy_ieee1275_get_integer_property (dev, "address",
					  addr, sizeof (addr), &actual))
    return 0;
  if (actual == 4)
    {
      holy_cmos_port = (volatile holy_uint8_t *) (holy_addr_t) addr[0];
      return 1;
    }

#if holy_CPU_SIZEOF_VOID_P == 8
  if (actual == 8)
    {
      holy_cmos_port = (volatile holy_uint8_t *)
	((((holy_addr_t) addr[0]) << 32) | addr[1]);
      return 1;
    }
#else
  if (actual == 8 && addr[0] == 0)
    {
      holy_cmos_port = (volatile holy_uint8_t *) addr[1];
      return 1;
    }
#endif
  return 0;
}

holy_err_t
holy_cmos_find_port (void)
{
  holy_ieee1275_devices_iterate (holy_cmos_find_port_iter);
  if (!holy_cmos_port)
    return holy_error (holy_ERR_IO, "no cmos found");
  
  return holy_ERR_NONE;
}
