/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/memory.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/offsets.h>
#include <holy/machine/kernel.h>
#include <holy/uboot/disk.h>
#include <holy/uboot/uboot.h>
#include <holy/uboot/api_public.h>

holy_addr_t start_of_ram;

/*
 * holy_uboot_probe_memory():
 *   Queries U-Boot for available memory regions.
 *
 *   Sets up heap near the image in memory and sets up "start_of_ram".
 */
void
holy_uboot_mm_init (void)
{
  struct sys_info *si = holy_uboot_get_sys_info ();

  holy_mm_init_region ((void *) holy_modules_get_end (),
		       holy_KERNEL_MACHINE_HEAP_SIZE);

  if (si && (si->mr_no != 0))
    {
      int i;
      start_of_ram = holy_UINT_MAX;

      for (i = 0; i < si->mr_no; i++)
	if ((si->mr[i].flags & MR_ATTR_MASK) == MR_ATTR_DRAM)
	  if (si->mr[i].start < start_of_ram)
	    start_of_ram = si->mr[i].start;
    }
}

/*
 * holy_uboot_probe_hardware():
 */
holy_err_t
holy_uboot_probe_hardware (void)
{
  int devcount, i;

  devcount = holy_uboot_dev_enum ();
  holy_dprintf ("init", "%d devices found\n", devcount);

  for (i = 0; i < devcount; i++)
    {
      struct device_info *devinfo = holy_uboot_dev_get (i);

      holy_dprintf ("init", "device handle: %d\n", i);
      holy_dprintf ("init", "  cookie\t= 0x%08x\n",
		    (holy_uint32_t) devinfo->cookie);

      if (devinfo->type & DEV_TYP_STOR)
	{
	  holy_dprintf ("init", "  type\t\t= DISK\n");
	  holy_ubootdisk_register (devinfo);
	}
      else if (devinfo->type & DEV_TYP_NET)
	{
	  /* Dealt with in ubootnet module. */
	  holy_dprintf ("init", "  type\t\t= NET (not supported yet)\n");
	}
      else
	{
	  holy_dprintf ("init", "%s: unknown device type", __FUNCTION__);
	}
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  int i;
  struct sys_info *si = holy_uboot_get_sys_info ();

  if (!si || (si->mr_no < 1))
    return holy_ERR_BUG;

  /* Iterate and call `hook'.  */
  for (i = 0; i < si->mr_no; i++)
    if ((si->mr[i].flags & MR_ATTR_MASK) == MR_ATTR_DRAM)
      hook (si->mr[i].start, si->mr[i].size, holy_MEMORY_AVAILABLE,
	    hook_data);

  return holy_ERR_NONE;
}
