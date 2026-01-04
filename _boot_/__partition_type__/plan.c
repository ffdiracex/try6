/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/partition.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_partition_map holy_plan_partition_map;

static holy_err_t
plan_partition_map_iterate (holy_disk_t disk,
			    holy_partition_iterate_hook_t hook,
			    void *hook_data)
{
  struct holy_partition p;
  int ptr = 0;
  holy_err_t err;

  p.partmap = &holy_plan_partition_map;
  p.msdostype = 0;

  for (p.number = 0; ; p.number++)
    {
      char sig[sizeof ("part ") - 1];
      char c;

      p.offset = (ptr >> holy_DISK_SECTOR_BITS) + 1;
      p.index = ptr & (holy_DISK_SECTOR_SIZE - 1);

      err = holy_disk_read (disk, 1, ptr, sizeof (sig), sig);
      if (err)
	return err;
      if (holy_memcmp (sig, "part ", sizeof ("part ") - 1) != 0)
	break;
      ptr += sizeof (sig);
      do
	{
	  err = holy_disk_read (disk, 1, ptr, 1, &c);
	  if (err)
	    return err;
	  ptr++;
	}
      while (holy_isdigit (c) || holy_isalpha (c));
      if (c != ' ')
	break;
      p.start = 0;
      while (1)
	{
	  err = holy_disk_read (disk, 1, ptr, 1, &c);
	  if (err)
	    return err;
	  ptr++;
	  if (!holy_isdigit (c))
	    break;
	  p.start = p.start * 10 + (c - '0');
	}
      if (c != ' ')
	break;
      p.len = 0;
      while (1)
	{
	  err = holy_disk_read (disk, 1, ptr, 1, &c);
	  if (err)
	    return err;
	  ptr++;
	  if (!holy_isdigit (c))
	    break;
	  p.len = p.len * 10 + (c - '0');
	}
      if (c != '\n')
	break;
      p.len -= p.start;
      if (hook (disk, &p, hook_data))
	return holy_errno;
    }
  if (p.number == 0)
    return holy_error (holy_ERR_BAD_PART_TABLE, "not a plan partition table");

  return holy_ERR_NONE;
}

/* Partition map type.  */
static struct holy_partition_map holy_plan_partition_map =
  {
    .name = "plan",
    .iterate = plan_partition_map_iterate,
  };

holy_MOD_INIT(part_plan)
{
  holy_partition_map_register (&holy_plan_partition_map);
}

holy_MOD_FINI(part_plan)
{
  holy_partition_map_unregister (&holy_plan_partition_map);
}

