/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/arc/arc.h>
#include <holy/i18n.h>

static holy_arc_fileno_t last_handle = 0;
static char *last_path = NULL;
static int handle_writable = 0;

static int lnum = 0;

struct arcdisk_hash_ent
{
  char *devpath;
  int num;
  struct arcdisk_hash_ent *next;
};

#define ARCDISK_HASH_SZ	8
static struct arcdisk_hash_ent *arcdisk_hash[ARCDISK_HASH_SZ];

static int
arcdisk_hash_fn (const char *devpath)
{
  int hash = 0;
  while (*devpath)
    hash ^= *devpath++;
  return (hash & (ARCDISK_HASH_SZ - 1));
}

static struct arcdisk_hash_ent *
arcdisk_hash_find (const char *devpath)
{
  struct arcdisk_hash_ent *p = arcdisk_hash[arcdisk_hash_fn (devpath)];

  while (p)
    {
      if (!holy_strcmp (p->devpath, devpath))
	break;
      p = p->next;
    }
  return p;
}

static struct arcdisk_hash_ent *
arcdisk_hash_add (char *devpath)
{
  struct arcdisk_hash_ent *p;
  struct arcdisk_hash_ent **head = &arcdisk_hash[arcdisk_hash_fn(devpath)];

  p = holy_malloc (sizeof (*p));
  if (!p)
    return NULL;

  p->devpath = devpath;
  p->next = *head;
  p->num = lnum++;
  *head = p;
  return p;
}


/* Context for holy_arcdisk_iterate.  */
struct holy_arcdisk_iterate_ctx
{
  holy_disk_dev_iterate_hook_t hook;
  void *hook_data;
};

/* Helper for holy_arcdisk_iterate.  */
static int
holy_arcdisk_iterate_iter (const char *name,
			   const struct holy_arc_component *comp, void *data)
{
  struct holy_arcdisk_iterate_ctx *ctx = data;

  if (!(comp->type == holy_ARC_COMPONENT_TYPE_DISK
	|| comp->type == holy_ARC_COMPONENT_TYPE_FLOPPY
	|| comp->type == holy_ARC_COMPONENT_TYPE_TAPE))
    return 0;
  return ctx->hook (name, ctx->hook_data);
}

static int
holy_arcdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		      holy_disk_pull_t pull)
{
  struct holy_arcdisk_iterate_ctx ctx = { hook, hook_data };

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  return holy_arc_iterate_devs (holy_arcdisk_iterate_iter, &ctx, 1);
}

#ifdef holy_CPU_MIPSEL
#define RAW_SUFFIX "partition(0)"
#else
#define RAW_SUFFIX "partition(10)"
#endif

static holy_err_t
reopen (const char *name, int writable)
{
  holy_arc_fileno_t handle;

  if (last_path && holy_strcmp (last_path, name) == 0
      && (!writable || handle_writable))
    {
      holy_dprintf ("arcdisk", "using already opened %s\n", name);
      return holy_ERR_NONE;
    }
  if (last_path)
    {
      holy_ARC_FIRMWARE_VECTOR->close (last_handle);
      holy_free (last_path);
      last_path = NULL;
      last_handle = 0;
      handle_writable = 0;
    }
  if (holy_ARC_FIRMWARE_VECTOR->open (name,
				      writable ? holy_ARC_FILE_ACCESS_OPEN_RW
				      : holy_ARC_FILE_ACCESS_OPEN_RO, &handle))
    {
      holy_dprintf ("arcdisk", "couldn't open %s\n", name);
      return holy_error (holy_ERR_IO, "couldn't open %s", name);
    }
  handle_writable = writable;
  last_path = holy_strdup (name);
  if (!last_path)
    return holy_errno;
  last_handle = handle;
  holy_dprintf ("arcdisk", "opened %s\n", name);
  return holy_ERR_NONE;
}

static holy_err_t
holy_arcdisk_open (const char *name, holy_disk_t disk)
{
  char *fullname;
  holy_err_t err;
  holy_arc_err_t r;
  struct holy_arc_fileinfo info;
  struct arcdisk_hash_ent *hash;

  if (holy_memcmp (name, "arc/", 4) != 0)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not arc device");
  fullname = holy_arc_alt_name_to_norm (name, RAW_SUFFIX);
  disk->data = fullname;
  holy_dprintf ("arcdisk", "opening %s\n", fullname);

  hash = arcdisk_hash_find (fullname);
  if (!hash)
    hash = arcdisk_hash_add (fullname);
  if (!hash)
    return holy_errno;

  err = reopen (fullname, 0);
  if (err)
    return err;

  r = holy_ARC_FIRMWARE_VECTOR->getfileinformation (last_handle, &info);
  if (r)
    {
      holy_uint64_t res = 0;
      int i;

      holy_dprintf ("arcdisk", "couldn't retrieve size: %ld\n", r);
      for (i = 40; i >= 9; i--)
	{
	  holy_uint64_t pos = res | (1ULL << i);
	  char buf[512];
	  long unsigned count = 0;
	  holy_dprintf ("arcdisk",
			"seek to 0x%" PRIxholy_UINT64_T "\n", pos);
	  if (holy_ARC_FIRMWARE_VECTOR->seek (last_handle, &pos, 0))
	    continue;
	  if (holy_ARC_FIRMWARE_VECTOR->read (last_handle, buf,
					      0x200, &count))
	    continue;
	  if (count == 0)
	    continue;
	  res |= (1ULL << i);
	}
      holy_dprintf ("arcdisk",
		    "determined disk size 0x%" PRIxholy_UINT64_T "\n", res);
      disk->total_sectors = (res + 0x200) >> 9;
    }
  else
    disk->total_sectors = (info.end >> 9);

  disk->id = hash->num;
  return holy_ERR_NONE;
}

static void
holy_arcdisk_close (holy_disk_t disk)
{
  holy_free (disk->data);
}

static holy_err_t
holy_arcdisk_read (holy_disk_t disk, holy_disk_addr_t sector,
		   holy_size_t size, char *buf)
{
  holy_err_t err;
  holy_uint64_t pos = sector << 9;
  unsigned long count;
  holy_uint64_t totl = size << 9;
  holy_arc_err_t r;

  err = reopen (disk->data, 0);
  if (err)
    return err;
  r = holy_ARC_FIRMWARE_VECTOR->seek (last_handle, &pos, 0);
  if (r)
    {
      holy_dprintf ("arcdisk", "seek to 0x%" PRIxholy_UINT64_T " failed: %ld\n",
		    pos, r);
      return holy_error (holy_ERR_IO, "couldn't seek");
    }

  while (totl)
    {
      if (holy_ARC_FIRMWARE_VECTOR->read (last_handle, buf,
					  totl, &count))
	return holy_error (holy_ERR_READ_ERROR,
			   N_("failure reading sector 0x%llx "
			      "from `%s'"),
			   (unsigned long long) sector,
			   disk->name);
      totl -= count;
      buf += count;
    }

  return holy_ERR_NONE;
}

static holy_err_t
holy_arcdisk_write (holy_disk_t disk, holy_disk_addr_t sector,
		    holy_size_t size, const char *buf)
{
  holy_err_t err;
  holy_uint64_t pos = sector << 9;
  unsigned long count;
  holy_uint64_t totl = size << 9;
  holy_arc_err_t r;

  err = reopen (disk->data, 1);
  if (err)
    return err;
  r = holy_ARC_FIRMWARE_VECTOR->seek (last_handle, &pos, 0);
  if (r)
    {
      holy_dprintf ("arcdisk", "seek to 0x%" PRIxholy_UINT64_T " failed: %ld\n",
		    pos, r);
      return holy_error (holy_ERR_IO, "couldn't seek");
    }

  while (totl)
    {
      if (holy_ARC_FIRMWARE_VECTOR->write (last_handle, buf,
					   totl, &count))
	return holy_error (holy_ERR_WRITE_ERROR, N_("failure writing sector 0x%llx "
						    "to `%s'"),
			   (unsigned long long) sector,
			   disk->name);
      totl -= count;
      buf += count;
    }

  return holy_ERR_NONE;
}

static struct holy_disk_dev holy_arcdisk_dev =
  {
    .name = "arcdisk",
    .id = holy_DISK_DEVICE_ARCDISK_ID,
    .iterate = holy_arcdisk_iterate,
    .open = holy_arcdisk_open,
    .close = holy_arcdisk_close,
    .read = holy_arcdisk_read,
    .write = holy_arcdisk_write,
    .next = 0
  };

void
holy_arcdisk_init (void)
{
  holy_disk_dev_register (&holy_arcdisk_dev);
}

void
holy_arcdisk_fini (void)
{
  if (last_path)
    {
      holy_ARC_FIRMWARE_VECTOR->close (last_handle);
      holy_free (last_path);
      last_path = NULL;
      last_handle = 0;
    }

  holy_disk_dev_unregister (&holy_arcdisk_dev);
}
