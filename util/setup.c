/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/partition.h>
#include <holy/env.h>
#include <holy/emu/hostdisk.h>
#include <holy/term.h>
#include <holy/i18n.h>

#ifdef holy_SETUP_SPARC64
#include <holy/util/ofpath.h>
#include <holy/sparc64/ieee1275/boot.h>
#include <holy/sparc64/ieee1275/kernel.h>
#else
#include <holy/i386/pc/boot.h>
#include <holy/i386/pc/kernel.h>
#endif

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <holy/emu/getroot.h>
#include "progname.h"
#include <holy/reed_solomon.h>
#include <holy/msdos_partition.h>
#include <holy/crypto.h>
#include <holy/util/install.h>
#include <holy/emu/hostfile.h>

#include <errno.h>

/* On SPARC this program fills in various fields inside of the 'boot' and 'core'
 * image files.
 *
 * The 'boot' image needs to know the OBP path name of the root
 * device.  It also needs to know the initial block number of
 * 'core' (which is 'diskboot' concatenated with 'kernel' and
 * all the modules, this is created by holy-mkimage).  This resulting
 * 'boot' image is 512 bytes in size and is placed in the second block
 * of a partition.
 *
 * The initial 'diskboot' block acts as a loader for the actual holy
 * kernel.  It contains the loading code and then a block list.
 *
 * The block list of 'core' starts at the end of the 'diskboot' image
 * and works it's way backwards towards the end of the code of 'diskboot'.
 *
 * We patch up the images with the necessary values and write out the
 * result.
 */

#ifdef holy_SETUP_SPARC64
#define holy_target_to_host16(x)	holy_be_to_cpu16(x)
#define holy_target_to_host32(x)	holy_be_to_cpu32(x)
#define holy_target_to_host64(x)	holy_be_to_cpu64(x)
#define holy_host_to_target16(x)	holy_cpu_to_be16(x)
#define holy_host_to_target32(x)	holy_cpu_to_be32(x)
#define holy_host_to_target64(x)	holy_cpu_to_be64(x)
#elif defined (holy_SETUP_BIOS)
#define holy_target_to_host16(x)	holy_le_to_cpu16(x)
#define holy_target_to_host32(x)	holy_le_to_cpu32(x)
#define holy_target_to_host64(x)	holy_le_to_cpu64(x)
#define holy_host_to_target16(x)	holy_cpu_to_le16(x)
#define holy_host_to_target32(x)	holy_cpu_to_le32(x)
#define holy_host_to_target64(x)	holy_cpu_to_le64(x)
#else
#error Complete this
#endif

static void
write_rootdev (holy_device_t root_dev,
	       char *boot_img, holy_uint64_t first_sector)
{
#ifdef holy_SETUP_BIOS
  {
    holy_uint8_t *boot_drive;
    void *kernel_sector;
    boot_drive = (holy_uint8_t *) (boot_img + holy_BOOT_MACHINE_BOOT_DRIVE);
    kernel_sector = (boot_img + holy_BOOT_MACHINE_KERNEL_SECTOR);

    /* FIXME: can this be skipped?  */
    *boot_drive = 0xFF;

    holy_set_unaligned64 (kernel_sector, holy_cpu_to_le64 (first_sector));
  }
#endif
#ifdef holy_SETUP_SPARC64
  {
    void *kernel_byte;
    kernel_byte = (boot_img + holy_BOOT_AOUT_HEADER_SIZE
		   + holy_BOOT_MACHINE_KERNEL_BYTE);
    holy_set_unaligned64 (kernel_byte,
			  holy_cpu_to_be64 (first_sector << holy_DISK_SECTOR_BITS));
  }
#endif
}

#ifdef holy_SETUP_SPARC64
#define BOOT_SECTOR 1
#else
#define BOOT_SECTOR 0
#endif

/* Helper for setup.  */

struct blocklists
{
  struct holy_boot_blocklist *first_block, *block;
#ifdef holy_SETUP_BIOS
  holy_uint16_t current_segment;
#endif
  holy_uint16_t last_length;
  holy_disk_addr_t first_sector;
};

/* Helper for setup.  */
static void
save_blocklists (holy_disk_addr_t sector, unsigned offset, unsigned length,
		 void *data)
{
  struct blocklists *bl = data;
  struct holy_boot_blocklist *prev = bl->block + 1;
  holy_uint64_t seclen;

  holy_util_info ("saving <%"  holy_HOST_PRIuLONG_LONG ",%u,%u>",
		  (unsigned long long) sector, offset, length);

  if (bl->first_sector == (holy_disk_addr_t) -1)
    {
      if (offset != 0 || length < holy_DISK_SECTOR_SIZE)
	holy_util_error ("%s", _("the first sector of the core file is not sector-aligned"));

      bl->first_sector = sector;
      sector++;
      length -= holy_DISK_SECTOR_SIZE;
      if (!length)
	return;
    }

  if (offset != 0 || bl->last_length != 0)
    holy_util_error ("%s", _("non-sector-aligned data is found in the core file"));

  seclen = (length + holy_DISK_SECTOR_SIZE - 1) >> holy_DISK_SECTOR_BITS;

  if (bl->block != bl->first_block
      && (holy_target_to_host64 (prev->start)
	  + holy_target_to_host16 (prev->len)) == sector)
    {
      holy_uint16_t t = holy_target_to_host16 (prev->len);
      t += seclen;
      prev->len = holy_host_to_target16 (t);
    }
  else
    {
      bl->block->start = holy_host_to_target64 (sector);
      bl->block->len = holy_host_to_target16 (seclen);
#ifdef holy_SETUP_BIOS
      bl->block->segment = holy_host_to_target16 (bl->current_segment);
#endif

      bl->block--;
      if (bl->block->len)
	holy_util_error ("%s", _("the sectors of the core file are too fragmented"));
    }

  bl->last_length = length & (holy_DISK_SECTOR_SIZE - 1);
#ifdef holy_SETUP_BIOS
  bl->current_segment += seclen << (holy_DISK_SECTOR_BITS - 4);
#endif
}

#ifdef holy_SETUP_BIOS
/* Context for setup/identify_partmap.  */
struct identify_partmap_ctx
{
  holy_partition_map_t dest_partmap;
  holy_partition_t container;
  int multiple_partmaps;
};

/* Helper for setup.
   Unlike root_dev, with dest_dev we're interested in the partition map even
   if dest_dev itself is a whole disk.  */
static int
identify_partmap (holy_disk_t disk __attribute__ ((unused)),
		  const holy_partition_t p, void *data)
{
  struct identify_partmap_ctx *ctx = data;

  if (p->parent != ctx->container)
    return 0;
  /* NetBSD and OpenBSD subpartitions have metadata inside a partition,
     so they are safe to ignore.
   */
  if (holy_strcmp (p->partmap->name, "netbsd") == 0
      || holy_strcmp (p->partmap->name, "openbsd") == 0)
    return 0;
  if (ctx->dest_partmap == NULL)
    {
      ctx->dest_partmap = p->partmap;
      return 0;
    }
  if (ctx->dest_partmap == p->partmap)
    return 0;
  ctx->multiple_partmaps = 1;
  return 1;
}
#endif

#ifdef holy_SETUP_BIOS
#define SETUP holy_util_bios_setup
#elif holy_SETUP_SPARC64
#define SETUP holy_util_sparc_setup
#else
#error "Shouldn't happen"
#endif

void
SETUP (const char *dir,
       const char *boot_file, const char *core_file,
       const char *dest, int force,
       int fs_probe, int allow_floppy,
       int add_rs_codes __attribute__ ((unused))) /* unused on sparc64 */
{
  char *core_path;
  char *boot_img, *core_img, *boot_path;
  char *root = 0;
  size_t boot_size, core_size;
#ifdef holy_SETUP_BIOS
  holy_uint16_t core_sectors;
#endif
  holy_device_t root_dev = 0, dest_dev, core_dev;
  holy_util_fd_t fp;
  struct blocklists bl;

  bl.first_sector = (holy_disk_addr_t) -1;

#ifdef holy_SETUP_BIOS
  bl.current_segment =
    holy_BOOT_I386_PC_KERNEL_SEG + (holy_DISK_SECTOR_SIZE >> 4);
#endif
  bl.last_length = 0;

  /* Read the boot image by the OS service.  */
  boot_path = holy_util_get_path (dir, boot_file);
  boot_size = holy_util_get_image_size (boot_path);
  if (boot_size != holy_DISK_SECTOR_SIZE)
    holy_util_error (_("the size of `%s' is not %u"),
		     boot_path, holy_DISK_SECTOR_SIZE);
  boot_img = holy_util_read_image (boot_path);
  free (boot_path);

  core_path = holy_util_get_path (dir, core_file);
  core_size = holy_util_get_image_size (core_path);
#ifdef holy_SETUP_BIOS
  core_sectors = ((core_size + holy_DISK_SECTOR_SIZE - 1)
		  >> holy_DISK_SECTOR_BITS);
#endif
  if (core_size < holy_DISK_SECTOR_SIZE)
    holy_util_error (_("the size of `%s' is too small"), core_path);
#ifdef holy_SETUP_BIOS
  if (core_size > 0xFFFF * holy_DISK_SECTOR_SIZE)
    holy_util_error (_("the size of `%s' is too large"), core_path);
#endif

  core_img = holy_util_read_image (core_path);

  /* Have FIRST_BLOCK to point to the first blocklist.  */
  bl.first_block = (struct holy_boot_blocklist *) (core_img
						   + holy_DISK_SECTOR_SIZE
						   - sizeof (*bl.block));
  holy_util_info ("root is `%s', dest is `%s'", root, dest);

  holy_util_info ("Opening dest");
  dest_dev = holy_device_open (dest);
  if (! dest_dev)
    holy_util_error ("%s", holy_errmsg);

  core_dev = dest_dev;

  {
    char **root_devices = holy_guess_root_devices (dir);
    char **cur;
    int found = 0;

    if (!root_devices)
      holy_util_error (_("cannot find a device for %s (is /dev mounted?)"), dir);

    for (cur = root_devices; *cur; cur++)
      {
	char *drive;
	holy_device_t try_dev;

	drive = holy_util_get_holy_dev (*cur);
	if (!drive)
	  continue;
	try_dev = holy_device_open (drive);
	if (! try_dev)
	  {
	    free (drive);
	    continue;
	  }
	if (!found && try_dev->disk->id == dest_dev->disk->id
	    && try_dev->disk->dev->id == dest_dev->disk->dev->id)
	  {
	    if (root_dev)
	      holy_device_close (root_dev);
	    free (root);
	    root_dev = try_dev;
	    root = drive;
	    found = 1;
	    continue;
	  }
	if (!root_dev)
	  {
	    root_dev = try_dev;
	    root = drive;
	    continue;
	  }
	holy_device_close (try_dev);
	free (drive);
      }
    if (!root_dev)
      {
	holy_util_error ("guessing the root device failed, because of `%s'",
			 holy_errmsg);
      }
    holy_util_info ("guessed root_dev `%s' from "
		    "dir `%s'", root_dev->disk->name, dir);

    for (cur = root_devices; *cur; cur++)
      free (*cur);
    free (root_devices);
  }

  holy_util_info ("setting the root device to `%s'", root);
  if (holy_env_set ("root", root) != holy_ERR_NONE)
    holy_util_error ("%s", holy_errmsg);

#ifdef holy_SETUP_BIOS
  {
    char *tmp_img;
    holy_uint8_t *boot_drive_check;

    /* Read the original sector from the disk.  */
    tmp_img = xmalloc (holy_DISK_SECTOR_SIZE);
    if (holy_disk_read (dest_dev->disk, 0, 0, holy_DISK_SECTOR_SIZE, tmp_img))
      holy_util_error ("%s", holy_errmsg);
    
    boot_drive_check = (holy_uint8_t *) (boot_img
					  + holy_BOOT_MACHINE_DRIVE_CHECK);
    /* Copy the possible DOS BPB.  */
    memcpy (boot_img + holy_BOOT_MACHINE_BPB_START,
	    tmp_img + holy_BOOT_MACHINE_BPB_START,
	    holy_BOOT_MACHINE_BPB_END - holy_BOOT_MACHINE_BPB_START);

    /* If DEST_DRIVE is a hard disk, enable the workaround, which is
       for buggy BIOSes which don't pass boot drive correctly. Instead,
       they pass 0x00 or 0x01 even when booted from 0x80.  */
    if (!allow_floppy && !holy_util_biosdisk_is_floppy (dest_dev->disk))
      {
	/* Replace the jmp (2 bytes) with double nop's.  */
	boot_drive_check[0] = 0x90;
	boot_drive_check[1] = 0x90;
      }

    struct identify_partmap_ctx ctx = {
      .dest_partmap = NULL,
      .container = dest_dev->disk->partition,
      .multiple_partmaps = 0
    };
    int is_ldm;
    holy_err_t err;
    holy_disk_addr_t *sectors;
    int i;
    holy_fs_t fs;
    unsigned int nsec, maxsec;

    holy_partition_iterate (dest_dev->disk, identify_partmap, &ctx);

    /* Copy the partition table.  */
    if (ctx.dest_partmap ||
        (!allow_floppy && !holy_util_biosdisk_is_floppy (dest_dev->disk)))
      memcpy (boot_img + holy_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	      tmp_img + holy_BOOT_MACHINE_WINDOWS_NT_MAGIC,
	      holy_BOOT_MACHINE_PART_END - holy_BOOT_MACHINE_WINDOWS_NT_MAGIC);

    free (tmp_img);

    if (ctx.container
	&& holy_strcmp (ctx.container->partmap->name, "msdos") == 0
	&& ctx.dest_partmap
	&& (ctx.container->msdostype == holy_PC_PARTITION_TYPE_NETBSD
	    || ctx.container->msdostype == holy_PC_PARTITION_TYPE_OPENBSD))
      {
	holy_util_warn ("%s", _("Attempting to install holy to a disk with multiple partition labels or both partition label and filesystem.  This is not supported yet."));
	goto unable_to_embed;
      }

    fs = holy_fs_probe (dest_dev);
    if (!fs)
      holy_errno = holy_ERR_NONE;

    is_ldm = holy_util_is_ldm (dest_dev->disk);

    if (fs_probe)
      {
	if (!fs && !ctx.dest_partmap)
	  holy_util_error (_("unable to identify a filesystem in %s; safety check can't be performed"),
			   dest_dev->disk->name);
	if (fs && !fs->reserved_first_sector)
	  /* TRANSLATORS: Filesystem may reserve the space just holy isn't sure about it.  */
	  holy_util_error (_("%s appears to contain a %s filesystem which isn't known to "
			     "reserve space for DOS-style boot.  Installing holy there could "
			     "result in FILESYSTEM DESTRUCTION if valuable data is overwritten "
			     "by holy-setup (--skip-fs-probe disables this "
			     "check, use at your own risk)"), dest_dev->disk->name, fs->name);

	if (ctx.dest_partmap && strcmp (ctx.dest_partmap->name, "msdos") != 0
	    && strcmp (ctx.dest_partmap->name, "gpt") != 0
	    && strcmp (ctx.dest_partmap->name, "bsd") != 0
	    && strcmp (ctx.dest_partmap->name, "netbsd") != 0
	    && strcmp (ctx.dest_partmap->name, "openbsd") != 0
	    && strcmp (ctx.dest_partmap->name, "sunpc") != 0)
	  /* TRANSLATORS: Partition map may reserve the space just holy isn't sure about it.  */
	  holy_util_error (_("%s appears to contain a %s partition map which isn't known to "
			     "reserve space for DOS-style boot.  Installing holy there could "
			     "result in FILESYSTEM DESTRUCTION if valuable data is overwritten "
			     "by holy-setup (--skip-fs-probe disables this "
			     "check, use at your own risk)"), dest_dev->disk->name, ctx.dest_partmap->name);
	if (is_ldm && ctx.dest_partmap && strcmp (ctx.dest_partmap->name, "msdos") != 0
	    && strcmp (ctx.dest_partmap->name, "gpt") != 0)
	  holy_util_error (_("%s appears to contain a %s partition map and "
			     "LDM which isn't known to be a safe combination."
			     "  Installing holy there could "
			     "result in FILESYSTEM DESTRUCTION if valuable data"
			     " is overwritten "
			     "by holy-setup (--skip-fs-probe disables this "
			     "check, use at your own risk)"),
			   dest_dev->disk->name, ctx.dest_partmap->name);

      }

    if (! ctx.dest_partmap && ! fs && !is_ldm)
      {
	holy_util_warn ("%s", _("Attempting to install holy to a partitionless disk or to a partition.  This is a BAD idea."));
	goto unable_to_embed;
      }
    if (ctx.multiple_partmaps || (ctx.dest_partmap && fs) || (is_ldm && fs))
      {
	holy_util_warn ("%s", _("Attempting to install holy to a disk with multiple partition labels.  This is not supported yet."));
	goto unable_to_embed;
      }

    if (ctx.dest_partmap && !ctx.dest_partmap->embed)
      {
	holy_util_warn (_("Partition style `%s' doesn't support embedding"),
			ctx.dest_partmap->name);
	goto unable_to_embed;
      }

    if (fs && !fs->embed)
      {
	holy_util_warn (_("File system `%s' doesn't support embedding"),
			fs->name);
	goto unable_to_embed;
      }

    nsec = core_sectors;

    if (add_rs_codes)
      maxsec = 2 * core_sectors;
    else
      maxsec = core_sectors;

    if (maxsec > ((0x78000 - holy_KERNEL_I386_PC_LINK_ADDR)
		>> holy_DISK_SECTOR_BITS))
      maxsec = ((0x78000 - holy_KERNEL_I386_PC_LINK_ADDR)
		>> holy_DISK_SECTOR_BITS);

    if (is_ldm)
      err = holy_util_ldm_embed (dest_dev->disk, &nsec, maxsec,
				 holy_EMBED_PCBIOS, &sectors);
    else if (ctx.dest_partmap)
      err = ctx.dest_partmap->embed (dest_dev->disk, &nsec, maxsec,
				     holy_EMBED_PCBIOS, &sectors);
    else
      err = fs->embed (dest_dev, &nsec, maxsec,
		       holy_EMBED_PCBIOS, &sectors);
    if (!err && nsec < core_sectors)
      {
	err = holy_error (holy_ERR_OUT_OF_RANGE,
			  N_("Your embedding area is unusually small.  "
			     "core.img won't fit in it."));
      }
    
    if (err)
      {
	holy_util_warn ("%s", holy_errmsg);
	holy_errno = holy_ERR_NONE;
	goto unable_to_embed;
      }

    assert (nsec <= maxsec);

    /* Clean out the blocklists.  */
    bl.block = bl.first_block;
    while (bl.block->len)
      {
	holy_memset (bl.block, 0, sizeof (*bl.block));
      
	bl.block--;

	if ((char *) bl.block <= core_img)
	  holy_util_error ("%s", _("no terminator in the core image"));
      }

    bl.block = bl.first_block;
    for (i = 0; i < nsec; i++)
      save_blocklists (sectors[i] + holy_partition_get_start (ctx.container),
		       0, holy_DISK_SECTOR_SIZE, &bl);

    /* Make sure that the last blocklist is a terminator.  */
    if (bl.block == bl.first_block)
      bl.block--;
    bl.block->start = 0;
    bl.block->len = 0;
    bl.block->segment = 0;

    write_rootdev (root_dev, boot_img, bl.first_sector);

    /* Round up to the nearest sector boundary, and zero the extra memory */
    core_img = xrealloc (core_img, nsec * holy_DISK_SECTOR_SIZE);
    assert (core_img && (nsec * holy_DISK_SECTOR_SIZE >= core_size));
    memset (core_img + core_size, 0, nsec * holy_DISK_SECTOR_SIZE - core_size);

    bl.first_block = (struct holy_boot_blocklist *) (core_img
						     + holy_DISK_SECTOR_SIZE
						     - sizeof (*bl.block));

    holy_size_t no_rs_length;
    no_rs_length = holy_target_to_host16
      (holy_get_unaligned16 (core_img
			     + holy_DISK_SECTOR_SIZE
			     + holy_KERNEL_I386_PC_NO_REED_SOLOMON_LENGTH));

    if (no_rs_length == 0xffff)
      holy_util_error ("%s", _("core.img version mismatch"));

    if (add_rs_codes)
      {
	holy_set_unaligned32 ((core_img + holy_DISK_SECTOR_SIZE
			       + holy_KERNEL_I386_PC_REED_SOLOMON_REDUNDANCY),
			      holy_host_to_target32 (nsec * holy_DISK_SECTOR_SIZE - core_size));

	void *tmp = xmalloc (core_size);
	holy_memcpy (tmp, core_img, core_size);
	holy_reed_solomon_add_redundancy (core_img + no_rs_length + holy_DISK_SECTOR_SIZE,
					  core_size - no_rs_length - holy_DISK_SECTOR_SIZE,
					  nsec * holy_DISK_SECTOR_SIZE
					  - core_size);
	assert (holy_memcmp (tmp, core_img, core_size) == 0);
	free (tmp);
      }

    /* Write the core image onto the disk.  */
    for (i = 0; i < nsec; i++)
      holy_disk_write (dest_dev->disk, sectors[i], 0,
		       holy_DISK_SECTOR_SIZE,
		       core_img + i * holy_DISK_SECTOR_SIZE);

    holy_free (sectors);

    goto finish;
  }

unable_to_embed:
#endif

  if (dest_dev->disk->dev->id != root_dev->disk->dev->id)
    holy_util_error ("%s", _("embedding is not possible, but this is required for "
			     "RAID and LVM install"));

  {
    holy_fs_t fs;
    fs = holy_fs_probe (root_dev);
    if (!fs)
      holy_util_error (_("can't determine filesystem on %s"), root);

    if (!fs->blocklist_install)
      holy_util_error (_("filesystem `%s' doesn't support blocklists"),
		       fs->name);
  }

#ifdef holy_SETUP_BIOS
  if (dest_dev->disk->id != root_dev->disk->id
      || dest_dev->disk->dev->id != root_dev->disk->dev->id)
    /* TRANSLATORS: cross-disk refers to /boot being on one disk
       but MBR on another.  */
    holy_util_error ("%s", _("embedding is not possible, but this is required for "
			     "cross-disk install"));
#else
  core_dev = root_dev;
#endif

  holy_util_warn ("%s", _("Embedding is not possible.  holy can only be installed in this "
			  "setup by using blocklists.  However, blocklists are UNRELIABLE and "
			  "their use is discouraged."));
  if (! force)
    /* TRANSLATORS: Here holy refuses to continue with blocklist install.  */
    holy_util_error ("%s", _("will not proceed with blocklists"));

  /* The core image must be put on a filesystem unfortunately.  */
  holy_util_info ("will leave the core image on the filesystem");

  holy_util_biosdisk_flush (root_dev->disk);

  /* Clean out the blocklists.  */
  bl.block = bl.first_block;
  while (bl.block->len)
    {
      bl.block->start = 0;
      bl.block->len = 0;
#ifdef holy_SETUP_BIOS
      bl.block->segment = 0;
#endif

      bl.block--;

      if ((char *) bl.block <= core_img)
	holy_util_error ("%s", _("no terminator in the core image"));
    }

  bl.block = bl.first_block;

  holy_install_get_blocklist (root_dev, core_path, core_img, core_size,
			      save_blocklists, &bl);

  if (bl.first_sector == (holy_disk_addr_t)-1)
    holy_util_error ("%s", _("can't retrieve blocklists"));

#ifdef holy_SETUP_SPARC64
  {
    char *boot_devpath;
    boot_devpath = (char *) (boot_img
			     + holy_BOOT_AOUT_HEADER_SIZE
			     + holy_BOOT_MACHINE_BOOT_DEVPATH);
    if (dest_dev->disk->id != root_dev->disk->id
	|| dest_dev->disk->dev->id != root_dev->disk->dev->id)
      {
	char *dest_ofpath;
	dest_ofpath
	  = holy_util_devname_to_ofpath (holy_util_biosdisk_get_osdev (root_dev->disk));
	/* FIXME handle NULL result */
	holy_util_info ("dest_ofpath is `%s'", dest_ofpath);
	strncpy (boot_devpath, dest_ofpath,
		 holy_BOOT_MACHINE_BOOT_DEVPATH_END
		 - holy_BOOT_MACHINE_BOOT_DEVPATH - 1);
	boot_devpath[holy_BOOT_MACHINE_BOOT_DEVPATH_END
		   - holy_BOOT_MACHINE_BOOT_DEVPATH - 1] = 0;
	free (dest_ofpath);
      }
    else
      {
	holy_util_info ("non cross-disk install");
	memset (boot_devpath, 0, holy_BOOT_MACHINE_BOOT_DEVPATH_END
		- holy_BOOT_MACHINE_BOOT_DEVPATH);
      }
    holy_util_info ("boot device path %s", boot_devpath);
  }
#endif

  write_rootdev (root_dev, boot_img, bl.first_sector);

  /* Write the first two sectors of the core image onto the disk.  */
  holy_util_info ("opening the core image `%s'", core_path);
  fp = holy_util_fd_open (core_path, holy_UTIL_FD_O_WRONLY);
  if (! holy_UTIL_FD_IS_VALID (fp))
    holy_util_error (_("cannot open `%s': %s"), core_path,
		     holy_util_fd_strerror ());

  if (holy_util_fd_write (fp, core_img, holy_DISK_SECTOR_SIZE * 2)
      != holy_DISK_SECTOR_SIZE * 2)
    holy_util_error (_("cannot write to `%s': %s"),
		     core_path, strerror (errno));
  holy_util_fd_sync (fp);
  holy_util_fd_close (fp);
  holy_util_biosdisk_flush (root_dev->disk);

  holy_disk_cache_invalidate_all ();

  {
    char *buf, *ptr = core_img;
    size_t len = core_size;
    holy_uint64_t blk;
    holy_partition_t container = core_dev->disk->partition;
    holy_err_t err;

    core_dev->disk->partition = 0;

    buf = xmalloc (core_size);
    blk = bl.first_sector;
    err = holy_disk_read (core_dev->disk, blk, 0, holy_DISK_SECTOR_SIZE, buf);
    if (err)
      holy_util_error (_("cannot read `%s': %s"), core_dev->disk->name,
		       holy_errmsg);
    if (holy_memcmp (buf, ptr, holy_DISK_SECTOR_SIZE) != 0)
      holy_util_error ("%s", _("blocklists are invalid"));

    ptr += holy_DISK_SECTOR_SIZE;
    len -= holy_DISK_SECTOR_SIZE;

    bl.block = bl.first_block;
    while (bl.block->len)
      {
	size_t cur = holy_target_to_host16 (bl.block->len) << holy_DISK_SECTOR_BITS;
	blk = holy_target_to_host64 (bl.block->start);

	if (cur > len)
	  cur = len;

	err = holy_disk_read (core_dev->disk, blk, 0, cur, buf);
	if (err)
	  holy_util_error (_("cannot read `%s': %s"), core_dev->disk->name,
			   holy_errmsg);

	if (holy_memcmp (buf, ptr, cur) != 0)
	  holy_util_error ("%s", _("blocklists are invalid"));

	ptr += cur;
	len -= cur;
	bl.block--;
	
	if ((char *) bl.block <= core_img)
	  holy_util_error ("%s", _("no terminator in the core image"));
      }
    if (len)
      holy_util_error ("%s", _("blocklists are incomplete"));
    core_dev->disk->partition = container;
    free (buf);
  }

#ifdef holy_SETUP_BIOS
 finish:
#endif

  /* Write the boot image onto the disk.  */
  if (holy_disk_write (dest_dev->disk, BOOT_SECTOR,
		       0, holy_DISK_SECTOR_SIZE, boot_img))
    holy_util_error ("%s", holy_errmsg);

  holy_util_biosdisk_flush (root_dev->disk);
  holy_util_biosdisk_flush (dest_dev->disk);

  free (core_path);
  free (core_img);
  free (boot_img);
  holy_device_close (dest_dev);
  holy_device_close (root_dev);
}

