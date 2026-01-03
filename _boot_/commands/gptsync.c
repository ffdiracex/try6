/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/command.h>
#include <holy/dl.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/msdos_partition.h>
#include <holy/partition.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/fs.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* Convert a LBA address to a CHS address in the INT 13 format.  */
/* Taken from holy1. */
/* XXX: use hardcoded geometry of C = 1024, H = 255, S = 63.
   Is it a problem?
*/
static void
lba_to_chs (holy_uint32_t lba, holy_uint8_t *cl, holy_uint8_t *ch,
	    holy_uint8_t *dh)
{
  holy_uint32_t cylinder, head, sector;
  holy_uint32_t sectors = 63, heads = 255, cylinders = 1024;

  sector = lba % sectors + 1;
  head = (lba / sectors) % heads;
  cylinder = lba / (sectors * heads);

  if (cylinder >= cylinders)
    {
      *cl = *ch = *dh = 0xff;
      return;
    }

  *cl = sector | ((cylinder & 0x300) >> 2);
  *ch = cylinder & 0xFF;
  *dh = head;
}

static holy_err_t
holy_cmd_gptsync (holy_command_t cmd __attribute__ ((unused)),
		  int argc, char **args)
{
  holy_device_t dev;
  struct holy_msdos_partition_mbr mbr;
  struct holy_partition *partition;
  holy_disk_addr_t first_sector;
  int numactive = 0;
  int i;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, "device name required");
  if (argc > 4)
    return holy_error (holy_ERR_BAD_ARGUMENT, "only 3 partitions can be "
		       "in hybrid MBR");

  if (args[0][0] == '(' && args[0][holy_strlen (args[0]) - 1] == ')')
    {
      args[0][holy_strlen (args[0]) - 1] = 0;
      dev = holy_device_open (args[0] + 1);
      args[0][holy_strlen (args[0])] = ')';
    }
  else
    dev = holy_device_open (args[0]);

  if (! dev)
    return holy_errno;

  if (! dev->disk)
    {
      holy_device_close (dev);
      return holy_error (holy_ERR_BAD_ARGUMENT, "not a disk");
    }

  /* Read the protective MBR.  */
  if (holy_disk_read (dev->disk, 0, 0, sizeof (mbr), &mbr))
    {
      holy_device_close (dev);
      return holy_errno;
    }

  /* Check if it is valid.  */
  if (mbr.signature != holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE))
    {
      holy_device_close (dev);
      return holy_error (holy_ERR_BAD_PART_TABLE, "no signature");
    }

  /* Make sure the MBR is a protective MBR and not a normal MBR.  */
  for (i = 0; i < 4; i++)
    if (mbr.entries[i].type == holy_PC_PARTITION_TYPE_GPT_DISK)
      break;
  if (i == 4)
    {
      holy_device_close (dev);
      return holy_error (holy_ERR_BAD_PART_TABLE, "no GPT partition map found");
    }

  first_sector = dev->disk->total_sectors;
  for (i = 1; i < argc; i++)
    {
      char *separator, csep = 0;
      holy_uint8_t type;
      separator = holy_strchr (args[i], '+');
      if (! separator)
	separator = holy_strchr (args[i], '-');
      if (separator)
	{
	  csep = *separator;
	  *separator = 0;
	}
      partition = holy_partition_probe (dev->disk, args[i]);
      if (separator)
	*separator = csep;
      if (! partition)
	{
	  holy_device_close (dev);
	  return holy_error (holy_ERR_UNKNOWN_DEVICE,
			     N_("no such partition"));
	}

      if (partition->start + partition->len > 0xffffffff)
	{
	  holy_device_close (dev);
	  return holy_error (holy_ERR_OUT_OF_RANGE,
			     "only partitions residing in the first 2TB "
			     "can be present in hybrid MBR");
	}


      if (first_sector > partition->start)
	first_sector = partition->start;

      if (separator && *(separator + 1))
	type = holy_strtoul (separator + 1, 0, 0);
      else
	{
	  holy_fs_t fs = 0;
	  dev->disk->partition = partition;
	  fs = holy_fs_probe (dev);

	  /* Unknown filesystem isn't fatal. */
	  if (holy_errno == holy_ERR_UNKNOWN_FS)
	    {
	      fs = 0;
	      holy_errno = holy_ERR_NONE;
	    }

	  if (fs && holy_strcmp (fs->name, "ntfs") == 0)
	    type = holy_PC_PARTITION_TYPE_NTFS;
	  else if (fs && holy_strcmp (fs->name, "fat") == 0)
	    /* FIXME: detect FAT16. */
	    type = holy_PC_PARTITION_TYPE_FAT32_LBA;
	  else if (fs && (holy_strcmp (fs->name, "hfsplus") == 0
			  || holy_strcmp (fs->name, "hfs") == 0))
	    type = holy_PC_PARTITION_TYPE_HFS;
	  else
	    /* FIXME: detect more types. */
	    type = holy_PC_PARTITION_TYPE_EXT2FS;

	  dev->disk->partition = 0;
	}

      mbr.entries[i].flag = (csep == '+') ? 0x80 : 0;
      if (csep == '+')
	{
	  numactive++;
	  if (numactive == 2)
	    {
	      holy_device_close (dev);
	      return holy_error (holy_ERR_BAD_ARGUMENT,
				 "only one partition can be active");
	    }
	}
      mbr.entries[i].type = type;
      mbr.entries[i].start = holy_cpu_to_le32 (partition->start);
      lba_to_chs (partition->start,
		  &(mbr.entries[i].start_sector),
		  &(mbr.entries[i].start_cylinder),
		  &(mbr.entries[i].start_head));
      lba_to_chs (partition->start + partition->len - 1,
		  &(mbr.entries[i].end_sector),
		  &(mbr.entries[i].end_cylinder),
		  &(mbr.entries[i].end_head));
      mbr.entries[i].length = holy_cpu_to_le32 (partition->len);
      holy_free (partition);
    }
  for (; i < 4; i++)
    holy_memset (&(mbr.entries[i]), 0, sizeof (mbr.entries[i]));

  /* The protective partition. */
  if (first_sector > 0xffffffff)
    first_sector = 0xffffffff;
  else
    first_sector--;
  mbr.entries[0].flag = 0;
  mbr.entries[0].type = holy_PC_PARTITION_TYPE_GPT_DISK;
  mbr.entries[0].start = holy_cpu_to_le32_compile_time (1);
  lba_to_chs (1,
	      &(mbr.entries[0].start_sector),
	      &(mbr.entries[0].start_cylinder),
	      &(mbr.entries[0].start_head));
  lba_to_chs (first_sector,
	      &(mbr.entries[0].end_sector),
	      &(mbr.entries[0].end_cylinder),
	      &(mbr.entries[0].end_head));
  mbr.entries[0].length = holy_cpu_to_le32 (first_sector);

  mbr.signature = holy_cpu_to_le16_compile_time (holy_PC_PARTITION_SIGNATURE);

  if (holy_disk_write (dev->disk, 0, 0, sizeof (mbr), &mbr))
    {
      holy_device_close (dev);
      return holy_errno;
    }

  holy_device_close (dev);

  holy_printf_ (N_("New MBR is written to `%s'\n"), args[0]);

  return holy_ERR_NONE;
}


static holy_command_t cmd;

holy_MOD_INIT(gptsync)
{
  (void) mod;			/* To stop warning. */
  cmd = holy_register_command ("gptsync", holy_cmd_gptsync,
			       N_("DEVICE [PARTITION[+/-[TYPE]]] ..."),
			       /* TRANSLATORS: MBR type is one-byte partition
				  type id.  */
			       N_("Fill hybrid MBR of GPT drive DEVICE. "
			       "Specified partitions will be a part "
			       "of hybrid MBR. Up to 3 partitions are "
			       "allowed. TYPE is an MBR type. "
			       "+ means that partition is active. "
			       "Only one partition can be active."));
}

holy_MOD_FINI(gptsync)
{
  holy_unregister_command (cmd);
}
