/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/disk.h>
#include <holy/i18n.h>

#ifdef holy_UTIL
#include <holy/util/misc.h>
#endif

holy_partition_map_t holy_partition_map_list;

/*
 * Checks that disk->partition contains part.  This function assumes that the
 * start of part is relative to the start of disk->partition.  Returns 1 if
 * disk->partition is null.
 */
static int
holy_partition_check_containment (const holy_disk_t disk,
				  const holy_partition_t part)
{
  if (disk->partition == NULL)
    return 1;

  if (part->start + part->len > disk->partition->len)
    {
      char *partname;

      partname = holy_partition_get_name (disk->partition);
      holy_dprintf ("partition", "sub-partition %s%d of (%s,%s) ends after parent.\n",
		    part->partmap->name, part->number + 1, disk->name, partname);
#ifdef holy_UTIL
      holy_util_warn (_("Discarding improperly nested partition (%s,%s,%s%d)"),
		      disk->name, partname, part->partmap->name, part->number + 1);
#endif
      holy_free (partname);

      return 0;
    }

  return 1;
}

/* Context for holy_partition_map_probe.  */
struct holy_partition_map_probe_ctx
{
  int partnum;
  holy_partition_t p;
};

/* Helper for holy_partition_map_probe.  */
static int
probe_iter (holy_disk_t dsk, const holy_partition_t partition, void *data)
{
  struct holy_partition_map_probe_ctx *ctx = data;

  if (ctx->partnum != partition->number)
    return 0;

  if (!(holy_partition_check_containment (dsk, partition)))
    return 0;

  ctx->p = (holy_partition_t) holy_malloc (sizeof (*ctx->p));
  if (! ctx->p)
    return 1;

  holy_memcpy (ctx->p, partition, sizeof (*ctx->p));
  return 1;
}

static holy_partition_t
holy_partition_map_probe (const holy_partition_map_t partmap,
			  holy_disk_t disk, int partnum)
{
  struct holy_partition_map_probe_ctx ctx = {
    .partnum = partnum,
    .p = 0
  };

  partmap->iterate (disk, probe_iter, &ctx);
  if (holy_errno)
    goto fail;

  return ctx.p;

 fail:
  holy_free (ctx.p);
  return 0;
}

holy_partition_t
holy_partition_probe (struct holy_disk *disk, const char *str)
{
  holy_partition_t part = 0;
  holy_partition_t curpart = 0;
  holy_partition_t tail;
  const char *ptr;

  part = tail = disk->partition;

  for (ptr = str; *ptr;)
    {
      holy_partition_map_t partmap;
      int num;
      const char *partname, *partname_end;

      partname = ptr;
      while (*ptr && holy_isalpha (*ptr))
	ptr++;
      partname_end = ptr; 
      num = holy_strtoul (ptr, (char **) &ptr, 0) - 1;

      curpart = 0;
      /* Use the first partition map type found.  */
      FOR_PARTITION_MAPS(partmap)
      {
	if (partname_end != partname &&
	    (holy_strncmp (partmap->name, partname, partname_end - partname)
	     != 0 || partmap->name[partname_end - partname] != 0))
	  continue;

	disk->partition = part;
	curpart = holy_partition_map_probe (partmap, disk, num);
	disk->partition = tail;
	if (curpart)
	  break;

	if (holy_errno == holy_ERR_BAD_PART_TABLE)
	  {
	    /* Continue to next partition map type.  */
	    holy_errno = holy_ERR_NONE;
	    continue;
	  }

	break;
      }

      if (! curpart)
	{
	  while (part)
	    {
	      curpart = part->parent;
	      holy_free (part);
	      part = curpart;
	    }
	  return 0;
	}
      curpart->parent = part;
      part = curpart;
      if (! ptr || *ptr != ',')
	break;
      ptr++;
    }

  return part;
}

/* Context for holy_partition_iterate.  */
struct holy_partition_iterate_ctx
{
  int ret;
  holy_partition_iterate_hook_t hook;
  void *hook_data;
};

/* Helper for holy_partition_iterate.  */
static int
part_iterate (holy_disk_t dsk, const holy_partition_t partition, void *data)
{
  struct holy_partition_iterate_ctx *ctx = data;
  struct holy_partition p = *partition;

  if (!(holy_partition_check_containment (dsk, partition)))
    return 0;

  p.parent = dsk->partition;
  dsk->partition = 0;
  if (ctx->hook (dsk, &p, ctx->hook_data))
    {
      ctx->ret = 1;
      return 1;
    }
  if (p.start != 0)
    {
      const struct holy_partition_map *partmap;
      dsk->partition = &p;
      FOR_PARTITION_MAPS(partmap)
      {
	holy_err_t err;
	err = partmap->iterate (dsk, part_iterate, ctx);
	if (err)
	  holy_errno = holy_ERR_NONE;
	if (ctx->ret)
	  break;
      }
    }
  dsk->partition = p.parent;
  return ctx->ret;
}

int
holy_partition_iterate (struct holy_disk *disk,
			holy_partition_iterate_hook_t hook, void *hook_data)
{
  struct holy_partition_iterate_ctx ctx = {
    .ret = 0,
    .hook = hook,
    .hook_data = hook_data
  };
  const struct holy_partition_map *partmap;

  FOR_PARTITION_MAPS(partmap)
  {
    holy_err_t err;
    err = partmap->iterate (disk, part_iterate, &ctx);
    if (err)
      holy_errno = holy_ERR_NONE;
    if (ctx.ret)
      break;
  }

  return ctx.ret;
}

char *
holy_partition_get_name (const holy_partition_t partition)
{
  char *out = 0, *ptr;
  holy_size_t needlen = 0;
  holy_partition_t part;
  if (!partition)
    return holy_strdup ("");
  for (part = partition; part; part = part->parent)
    /* Even on 64-bit machines this buffer is enough to hold
       longest number.  */
    needlen += holy_strlen (part->partmap->name) + 1 + 27;
  out = holy_malloc (needlen + 1);
  if (!out)
    return NULL;

  ptr = out + needlen;
  *ptr = 0;
  for (part = partition; part; part = part->parent)
    {
      char buf[27];
      holy_size_t len;
      holy_snprintf (buf, sizeof (buf), "%d", part->number + 1);
      len = holy_strlen (buf);
      ptr -= len;
      holy_memcpy (ptr, buf, len);
      len = holy_strlen (part->partmap->name);
      ptr -= len;
      holy_memcpy (ptr, part->partmap->name, len);
      *--ptr = ',';
    }
  holy_memmove (out, ptr + 1, out + needlen - ptr);
  return out;
}
