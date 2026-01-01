/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/partition.h>
#include <holy/bsdlabel.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/msdos_partition.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef holy_UTIL
#include <holy/emu/misc.h>
#endif

static struct holy_partition_map holy_bsdlabel_partition_map;
static struct holy_partition_map holy_netbsdlabel_partition_map;
static struct holy_partition_map holy_openbsdlabel_partition_map;



static holy_err_t
iterate_real (holy_disk_t disk, holy_disk_addr_t sector, int freebsd,
	      struct holy_partition_map *pmap,
	      holy_partition_iterate_hook_t hook, void *hook_data)
{
  struct holy_partition_bsd_disk_label label;
  struct holy_partition p;
  holy_disk_addr_t delta = 0;
  holy_disk_addr_t pos;

  /* Read the BSD label.  */
  if (holy_disk_read (disk, sector, 0, sizeof (label), &label))
    return holy_errno;

  /* Check if it is valid.  */
  if (label.magic != holy_cpu_to_le32_compile_time (holy_PC_PARTITION_BSD_LABEL_MAGIC))
    return holy_error (holy_ERR_BAD_PART_TABLE, "no signature");

  /* A kludge to determine a base of be.offset.  */
  if (holy_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION
      < holy_cpu_to_le16 (label.num_partitions) && freebsd)
    {
      struct holy_partition_bsd_entry whole_disk_be;

      pos = sizeof (label) + sector * holy_DISK_SECTOR_SIZE
	+ sizeof (struct holy_partition_bsd_entry)
	* holy_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION;

      if (holy_disk_read (disk, pos / holy_DISK_SECTOR_SIZE,
			  pos % holy_DISK_SECTOR_SIZE, sizeof (whole_disk_be),
			  &whole_disk_be))
	return holy_errno;

      delta = holy_le_to_cpu32 (whole_disk_be.offset);
    }

  pos = sizeof (label) + sector * holy_DISK_SECTOR_SIZE;

  for (p.number = 0;
       p.number < holy_cpu_to_le16 (label.num_partitions);
       p.number++, pos += sizeof (struct holy_partition_bsd_entry))
    {
      struct holy_partition_bsd_entry be;

      if (p.number == holy_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION)
	continue;

      p.offset = pos / holy_DISK_SECTOR_SIZE;
      p.index = pos % holy_DISK_SECTOR_SIZE;

      if (holy_disk_read (disk, p.offset, p.index, sizeof (be),  &be))
	return holy_errno;

      p.start = holy_le_to_cpu32 (be.offset);
      p.len = holy_le_to_cpu32 (be.size);
      p.partmap = pmap;

      if (p.len == 0)
	continue;

      if (p.start < delta)
	{
#ifdef holy_UTIL
	  char *partname;
	  /* disk->partition != NULL as 0 < delta */
	  partname = disk->partition ? holy_partition_get_name (disk->partition)
	    : 0;
	  holy_util_warn (_("Discarding improperly nested partition (%s,%s,%s%d)"),
			  disk->name, partname ? : "", p.partmap->name,
			  p.number + 1);
	  holy_free (partname);
#endif
	  continue;
	}

      p.start -= delta;

      if (hook (disk, &p, hook_data))
	return holy_errno;
    }
  return holy_ERR_NONE;
}

static holy_err_t
bsdlabel_partition_map_iterate (holy_disk_t disk,
				holy_partition_iterate_hook_t hook,
				void *hook_data)
{

  if (disk->partition && holy_strcmp (disk->partition->partmap->name, "msdos")
      == 0 && disk->partition->msdostype == holy_PC_PARTITION_TYPE_FREEBSD)
    return iterate_real (disk, holy_PC_PARTITION_BSD_LABEL_SECTOR, 1,
			 &holy_bsdlabel_partition_map, hook, hook_data);

  if (disk->partition 
      && (holy_strcmp (disk->partition->partmap->name, "msdos") == 0
	  || disk->partition->partmap == &holy_bsdlabel_partition_map
	  || disk->partition->partmap == &holy_netbsdlabel_partition_map
	  || disk->partition->partmap == &holy_openbsdlabel_partition_map))
      return holy_error (holy_ERR_BAD_PART_TABLE, "no embedding supported");

  return iterate_real (disk, holy_PC_PARTITION_BSD_LABEL_SECTOR, 0,
		       &holy_bsdlabel_partition_map, hook, hook_data);
}

/* Context for netopenbsdlabel_partition_map_iterate.  */
struct netopenbsdlabel_ctx
{
  holy_uint8_t type;
  struct holy_partition_map *pmap;
  holy_partition_iterate_hook_t hook;
  void *hook_data;
  int count;
};

/* Helper for netopenbsdlabel_partition_map_iterate.  */
static int
check_msdos (holy_disk_t dsk, const holy_partition_t partition, void *data)
{
  struct netopenbsdlabel_ctx *ctx = data;
  holy_err_t err;

  if (partition->msdostype != ctx->type)
    return 0;

  err = iterate_real (dsk, partition->start
		      + holy_PC_PARTITION_BSD_LABEL_SECTOR, 0, ctx->pmap,
		      ctx->hook, ctx->hook_data);
  if (err == holy_ERR_NONE)
    {
      ctx->count++;
      return 1;
    }
  if (err == holy_ERR_BAD_PART_TABLE)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }
  holy_print_error ();
  return 0;
}

/* This is a total breakage. Even when net-/openbsd label is inside partition
   it actually describes the whole disk.
 */
static holy_err_t
netopenbsdlabel_partition_map_iterate (holy_disk_t disk, holy_uint8_t type,
				       struct holy_partition_map *pmap,
				       holy_partition_iterate_hook_t hook,
				       void *hook_data)
{
  if (disk->partition && holy_strcmp (disk->partition->partmap->name, "msdos")
      == 0)
    return holy_error (holy_ERR_BAD_PART_TABLE, "no embedding supported");

  {
    struct netopenbsdlabel_ctx ctx = {
      .type = type,
      .pmap = pmap,
      .hook = hook,
      .hook_data = hook_data,
      .count = 0
    };
    holy_err_t err;

    err = holy_partition_msdos_iterate (disk, check_msdos, &ctx);

    if (err)
      return err;
    if (!ctx.count)
      return holy_error (holy_ERR_BAD_PART_TABLE, "no bsdlabel found");
  }
  return holy_ERR_NONE;
}

static holy_err_t
netbsdlabel_partition_map_iterate (holy_disk_t disk,
				   holy_partition_iterate_hook_t hook,
				   void *hook_data)
{
  return netopenbsdlabel_partition_map_iterate (disk,
						holy_PC_PARTITION_TYPE_NETBSD,
						&holy_netbsdlabel_partition_map,
						hook, hook_data);
}

static holy_err_t
openbsdlabel_partition_map_iterate (holy_disk_t disk,
				    holy_partition_iterate_hook_t hook,
				    void *hook_data)
{
  return netopenbsdlabel_partition_map_iterate (disk,
						holy_PC_PARTITION_TYPE_OPENBSD,
						&holy_openbsdlabel_partition_map,
						hook, hook_data);
}


static struct holy_partition_map holy_bsdlabel_partition_map =
  {
    .name = "bsd",
    .iterate = bsdlabel_partition_map_iterate,
  };

static struct holy_partition_map holy_openbsdlabel_partition_map =
  {
    .name = "openbsd",
    .iterate = openbsdlabel_partition_map_iterate,
  };

static struct holy_partition_map holy_netbsdlabel_partition_map =
  {
    .name = "netbsd",
    .iterate = netbsdlabel_partition_map_iterate,
  };



holy_MOD_INIT(part_bsd)
{
  holy_partition_map_register (&holy_bsdlabel_partition_map);
  holy_partition_map_register (&holy_netbsdlabel_partition_map);
  holy_partition_map_register (&holy_openbsdlabel_partition_map);
}

holy_MOD_FINI(part_bsd)
{
  holy_partition_map_unregister (&holy_bsdlabel_partition_map);
  holy_partition_map_unregister (&holy_netbsdlabel_partition_map);
  holy_partition_map_unregister (&holy_openbsdlabel_partition_map);
}
