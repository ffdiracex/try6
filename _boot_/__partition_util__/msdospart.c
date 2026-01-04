/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/msdos_partition.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/parttool.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static int activate_table_handle = -1;
static int type_table_handle = -1;

static struct holy_parttool_argdesc holy_pcpart_bootargs[] =
{
  {"boot", N_("Make partition active"), holy_PARTTOOL_ARG_BOOL},
  {0, 0, 0}
};

static holy_err_t holy_pcpart_boot (const holy_device_t dev,
				    const struct holy_parttool_args *args)
{
  int i, index;
  holy_partition_t part;
  struct holy_msdos_partition_mbr mbr;

  if (dev->disk->partition->offset)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("not a primary partition"));

  index = dev->disk->partition->index;
  part = dev->disk->partition;
  dev->disk->partition = part->parent;

  /* Read the MBR.  */
  if (holy_disk_read (dev->disk, 0, 0, sizeof (mbr), &mbr))
    {
      dev->disk->partition = part;
      return holy_errno;
    }

  if (args[0].set && args[0].bool)
    {
      for (i = 0; i < 4; i++)
	mbr.entries[i].flag = 0x0;
      mbr.entries[index].flag = 0x80;
      holy_printf_ (N_("Partition %d is active now. \n"), index);
    }
  else
    {
      mbr.entries[index].flag = 0x0;
      holy_printf (N_("Cleared active flag on %d. \n"), index);
    }

   /* Write the MBR.  */
  holy_disk_write (dev->disk, 0, 0, sizeof (mbr), &mbr);

  dev->disk->partition = part;

  return holy_ERR_NONE;
}

static struct holy_parttool_argdesc holy_pcpart_typeargs[] =
{
  {"type", N_("Change partition type"), holy_PARTTOOL_ARG_VAL},
  {"hidden", N_("Set `hidden' flag in partition type"), holy_PARTTOOL_ARG_BOOL},
  {0, 0, 0}
};

static holy_err_t holy_pcpart_type (const holy_device_t dev,
				    const struct holy_parttool_args *args)
{
  int index;
  holy_uint8_t type;
  holy_partition_t part;
  struct holy_msdos_partition_mbr mbr;

  index = dev->disk->partition->index;
  part = dev->disk->partition;
  dev->disk->partition = part->parent;

  /* Read the parttable.  */
  if (holy_disk_read (dev->disk, part->offset, 0,
		      sizeof (mbr), &mbr))
    {
      dev->disk->partition = part;
      return holy_errno;
    }

  if (args[0].set)
    type = holy_strtoul (args[0].str, 0, 0);
  else
    type = mbr.entries[index].type;

  if (args[1].set)
    {
      if (args[1].bool)
	type |= holy_PC_PARTITION_TYPE_HIDDEN_FLAG;
      else
	type &= ~holy_PC_PARTITION_TYPE_HIDDEN_FLAG;
    }

  if (holy_msdos_partition_is_empty (type)
      || holy_msdos_partition_is_extended (type))
    {
      dev->disk->partition = part;
      return holy_error (holy_ERR_BAD_ARGUMENT,
			 N_("the partition type 0x%x isn't "
			    "valid"));
    }

  mbr.entries[index].type = type;
  /* TRANSLATORS: In this case we're actually writing to the disk and actively
     modifying partition type rather than just defining it.  */
  holy_printf_ (N_("Setting partition type to 0x%x\n"), type);

   /* Write the parttable.  */
  holy_disk_write (dev->disk, part->offset, 0,
		   sizeof (mbr), &mbr);

  dev->disk->partition = part;

  return holy_ERR_NONE;
}

holy_MOD_INIT (msdospart)
{
  activate_table_handle = holy_parttool_register ("msdos",
						  holy_pcpart_boot,
						  holy_pcpart_bootargs);
  type_table_handle = holy_parttool_register ("msdos",
					      holy_pcpart_type,
					      holy_pcpart_typeargs);

}
holy_MOD_FINI(msdospart)
{
  holy_parttool_unregister (activate_table_handle);
  holy_parttool_unregister (type_table_handle);
}
