/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/loader.h>
#include <holy/machine/chainloader.h>
#include <holy/machine/memory.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/partition.h>
#include <holy/memory.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/msdos_partition.h>
#include <holy/machine/biosnum.h>
#include <holy/cpu/floppy.h>
#include <holy/i18n.h>
#include <holy/video.h>
#include <holy/mm.h>
#include <holy/fat.h>
#include <holy/ntfs.h>
#include <holy/i386/relocator.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_dl_t my_mod;
static int boot_drive;
static holy_addr_t boot_part_addr;
static struct holy_relocator *rel;

typedef enum
  {
    holy_CHAINLOADER_FORCE = 0x1,
    holy_CHAINLOADER_BPB = 0x2,
  } holy_chainloader_flags_t;

static holy_err_t
holy_chainloader_boot (void)
{
  struct holy_relocator16_state state = {
    .edx = boot_drive,
    .esi = boot_part_addr,
    .ds = 0,
    .es = 0,
    .fs = 0,
    .gs = 0,
    .ss = 0,
    .cs = 0,
    .sp = holy_MEMORY_MACHINE_BOOT_LOADER_ADDR,
    .ip = holy_MEMORY_MACHINE_BOOT_LOADER_ADDR,
    .a20 = 0
  };
  holy_video_set_mode ("text", 0, 0);

  return holy_relocator16_boot (rel, state);
}

static holy_err_t
holy_chainloader_unload (void)
{
  holy_relocator_unload (rel);
  rel = NULL;
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

void
holy_chainloader_patch_bpb (void *bs, holy_device_t dev, holy_uint8_t dl)
{
  holy_uint32_t part_start = 0;
  if (dev && dev->disk)
    part_start = holy_partition_get_start (dev->disk->partition);
  if (holy_memcmp ((char *) &((struct holy_ntfs_bpb *) bs)->oem_name,
		   "NTFS", 4) == 0)
    {
      struct holy_ntfs_bpb *bpb = (struct holy_ntfs_bpb *) bs;
      bpb->num_hidden_sectors = holy_cpu_to_le32 (part_start);
      bpb->bios_drive = dl;
      return;
    }

  do
    {
      struct holy_fat_bpb *bpb = (struct holy_fat_bpb *) bs;
      if (holy_strncmp((const char *) bpb->version_specific.fat12_or_fat16.fstype, "FAT12", 5)
	  && holy_strncmp((const char *) bpb->version_specific.fat12_or_fat16.fstype, "FAT16", 5)
	  && holy_strncmp((const char *) bpb->version_specific.fat32.fstype, "FAT32", 5))
	break;

      if (holy_le_to_cpu16 (bpb->bytes_per_sector) < 512
	  || (holy_le_to_cpu16 (bpb->bytes_per_sector)
	      & (holy_le_to_cpu16 (bpb->bytes_per_sector) - 1)))
	break;
	  
      if (bpb->sectors_per_cluster == 0
	  || (bpb->sectors_per_cluster & (bpb->sectors_per_cluster - 1)))
	break;

      if (bpb->num_reserved_sectors == 0)
	break;
      if (bpb->num_total_sectors_16 == 0 || bpb->num_total_sectors_32 == 0)
	break;

      if (bpb->num_fats == 0)
	break;

      if (bpb->sectors_per_fat_16)
	{
	  bpb->num_hidden_sectors = holy_cpu_to_le32 (part_start);
	  bpb->version_specific.fat12_or_fat16.num_ph_drive = dl;
	  return;
	}
      if (bpb->version_specific.fat32.sectors_per_fat_32)
	{
	  bpb->num_hidden_sectors = holy_cpu_to_le32 (part_start);
	  bpb->version_specific.fat32.num_ph_drive = dl;
	  return;
	}
      break;
    }
  while (0);
}

static void
holy_chainloader_cmd (const char *filename, holy_chainloader_flags_t flags)
{
  holy_file_t file = 0;
  holy_uint16_t signature;
  holy_device_t dev;
  int drive = -1;
  holy_addr_t part_addr = 0;
  holy_uint8_t *bs, *ptable;

  rel = holy_relocator_new ();
  if (!rel)
    goto fail;

  holy_dl_ref (my_mod);

  holy_file_filter_disable_compression ();
  file = holy_file_open (filename);
  if (! file)
    goto fail;

  {
    holy_relocator_chunk_t ch;
    holy_err_t err;

    err = holy_relocator_alloc_chunk_addr (rel, &ch, 0x7C00,
					   holy_DISK_SECTOR_SIZE);
    if (err)
      goto fail;
    bs = get_virtual_current_address (ch);
    err = holy_relocator_alloc_chunk_addr (rel, &ch,
					   holy_MEMORY_MACHINE_PART_TABLE_ADDR,
					   64);
    if (err)
      goto fail;
    ptable = get_virtual_current_address (ch);
  }

  /* Read the first block.  */
  if (holy_file_read (file, bs, holy_DISK_SECTOR_SIZE)
      != holy_DISK_SECTOR_SIZE)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
		    filename);

      goto fail;
    }

  /* Check the signature.  */
  signature = *((holy_uint16_t *) (bs + holy_DISK_SECTOR_SIZE - 2));
  if (signature != holy_le_to_cpu16 (0xaa55)
      && ! (flags & holy_CHAINLOADER_FORCE))
    {
      holy_error (holy_ERR_BAD_OS, "invalid signature");
      goto fail;
    }

  holy_file_close (file);

  /* Obtain the partition table from the root device.  */
  drive = holy_get_root_biosnumber ();
  dev = holy_device_open (0);
  if (dev && dev->disk && dev->disk->partition)
    {
      holy_disk_t disk = dev->disk;

      if (disk)
	{
	  holy_partition_t p = disk->partition;

	  if (p && holy_strcmp (p->partmap->name, "msdos") == 0)
	    {
	      disk->partition = p->parent;
	      holy_disk_read (disk, p->offset, 446, 64, ptable);
	      part_addr = (holy_MEMORY_MACHINE_PART_TABLE_ADDR
			   + (p->index << 4));
	      disk->partition = p;
	    }
	}
    }

  if (flags & holy_CHAINLOADER_BPB)
    holy_chainloader_patch_bpb ((void *) 0x7C00, dev, drive);

  if (dev)
    holy_device_close (dev);
 
  /* Ignore errors. Perhaps it's not fatal.  */
  holy_errno = holy_ERR_NONE;

  boot_drive = drive;
  boot_part_addr = part_addr;

  holy_loader_set (holy_chainloader_boot, holy_chainloader_unload, 1);
  return;

 fail:

  if (file)
    holy_file_close (file);

  holy_dl_unref (my_mod);
}

static holy_err_t
holy_cmd_chainloader (holy_command_t cmd __attribute__ ((unused)),
		      int argc, char *argv[])
{
  holy_chainloader_flags_t flags = 0;

  while (argc > 0)
    {
      if (holy_strcmp (argv[0], "--force") == 0)
	{
	  flags |= holy_CHAINLOADER_FORCE;
	  argc--;
	  argv++;
	  continue;
	}
      if (holy_strcmp (argv[0], "--bpb") == 0)
	{
	  flags |= holy_CHAINLOADER_BPB;
	  argc--;
	  argv++;
	  continue;
	}
      break;
    }

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  holy_chainloader_cmd (argv[0], flags);

  return holy_errno;
}

static holy_command_t cmd;

holy_MOD_INIT(chainloader)
{
  cmd = holy_register_command ("chainloader", holy_cmd_chainloader,
			       N_("[--force|--bpb] FILE"),
			       N_("Load another boot loader."));
  my_mod = mod;
}

holy_MOD_FINI(chainloader)
{
  holy_unregister_command (cmd);
}
