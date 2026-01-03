/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/net.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/mm.h>
#include <holy/term.h>
#include <holy/i18n.h>

holy_fs_t holy_fs_list = 0;

holy_fs_autoload_hook_t holy_fs_autoload_hook = 0;

/* Helper for holy_fs_probe.  */
static int
probe_dummy_iter (const char *filename __attribute__ ((unused)),
		  const struct holy_dirhook_info *info __attribute__ ((unused)),
		  void *data __attribute__ ((unused)))
{
  return 1;
}

holy_fs_t
holy_fs_probe (holy_device_t device)
{
  holy_fs_t p;

  if (device->disk)
    {
      /* Make it sure not to have an infinite recursive calls.  */
      static int count = 0;

      for (p = holy_fs_list; p; p = p->next)
	{
	  holy_dprintf ("fs", "Detecting %s...\n", p->name);

	  /* This is evil: newly-created just mounted BtrFS after copying all
	     holy files has a very peculiar unrecoverable corruption which
	     will be fixed at sync but we'd rather not do a global sync and
	     syncing just files doesn't seem to help. Relax the check for
	     this time.  */
#ifdef holy_UTIL
	  if (holy_strcmp (p->name, "btrfs") == 0)
	    {
	      char *label = 0;
	      p->uuid (device, &label);
	      if (label)
		holy_free (label);
	    }
	  else
#endif
	    (p->dir) (device, "/", probe_dummy_iter, NULL);
	  if (holy_errno == holy_ERR_NONE)
	    return p;

	  holy_error_push ();
	  holy_dprintf ("fs", "%s detection failed.\n", p->name);
	  holy_error_pop ();

	  if (holy_errno != holy_ERR_BAD_FS
	      && holy_errno != holy_ERR_OUT_OF_RANGE)
	    return 0;

	  holy_errno = holy_ERR_NONE;
	}

      /* Let's load modules automatically.  */
      if (holy_fs_autoload_hook && count == 0)
	{
	  count++;

	  while (holy_fs_autoload_hook ())
	    {
	      p = holy_fs_list;

	      (p->dir) (device, "/", probe_dummy_iter, NULL);
	      if (holy_errno == holy_ERR_NONE)
		{
		  count--;
		  return p;
		}

	      if (holy_errno != holy_ERR_BAD_FS
		  && holy_errno != holy_ERR_OUT_OF_RANGE)
		{
		  count--;
		  return 0;
		}

	      holy_errno = holy_ERR_NONE;
	    }

	  count--;
	}
    }
  else if (device->net && device->net->fs)
    return device->net->fs;

  holy_error (holy_ERR_UNKNOWN_FS, N_("unknown filesystem"));
  return 0;
}



/* Block list support routines.  */

struct holy_fs_block
{
  holy_disk_addr_t offset;
  unsigned long length;
};

static holy_err_t
holy_fs_blocklist_open (holy_file_t file, const char *name)
{
  char *p = (char *) name;
  unsigned num = 0;
  unsigned i;
  holy_disk_t disk = file->device->disk;
  struct holy_fs_block *blocks;

  /* First, count the number of blocks.  */
  do
    {
      num++;
      p = holy_strchr (p, ',');
      if (p)
	p++;
    }
  while (p);

  /* Allocate a block list.  */
  blocks = holy_zalloc (sizeof (struct holy_fs_block) * (num + 1));
  if (! blocks)
    return 0;

  file->size = 0;
  p = (char *) name;
  for (i = 0; i < num; i++)
    {
      if (*p != '+')
	{
	  blocks[i].offset = holy_strtoull (p, &p, 0);
	  if (holy_errno != holy_ERR_NONE || *p != '+')
	    {
	      holy_error (holy_ERR_BAD_FILENAME,
			  N_("invalid file name `%s'"), name);
	      goto fail;
	    }
	}

      p++;
      blocks[i].length = holy_strtoul (p, &p, 0);
      if (holy_errno != holy_ERR_NONE
	  || blocks[i].length == 0
	  || (*p && *p != ',' && ! holy_isspace (*p)))
	{
	  holy_error (holy_ERR_BAD_FILENAME,
		      N_("invalid file name `%s'"), name);
	  goto fail;
	}

      if (disk->total_sectors < blocks[i].offset + blocks[i].length)
	{
	  holy_error (holy_ERR_BAD_FILENAME, "beyond the total sectors");
	  goto fail;
	}

      file->size += (blocks[i].length << holy_DISK_SECTOR_BITS);
      p++;
    }

  file->data = blocks;

  return holy_ERR_NONE;

 fail:
  holy_free (blocks);
  return holy_errno;
}

static holy_ssize_t
holy_fs_blocklist_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_fs_block *p;
  holy_disk_addr_t sector;
  holy_off_t offset;
  holy_ssize_t ret = 0;

  if (len > file->size - file->offset)
    len = file->size - file->offset;

  sector = (file->offset >> holy_DISK_SECTOR_BITS);
  offset = (file->offset & (holy_DISK_SECTOR_SIZE - 1));
  for (p = file->data; p->length && len > 0; p++)
    {
      if (sector < p->length)
	{
	  holy_size_t size;

	  size = len;
	  if (((size + offset + holy_DISK_SECTOR_SIZE - 1)
	       >> holy_DISK_SECTOR_BITS) > p->length - sector)
	    size = ((p->length - sector) << holy_DISK_SECTOR_BITS) - offset;

	  if (holy_disk_read (file->device->disk, p->offset + sector, offset,
			      size, buf) != holy_ERR_NONE)
	    return -1;

	  ret += size;
	  len -= size;
	  sector -= ((size + offset) >> holy_DISK_SECTOR_BITS);
	  offset = ((size + offset) & (holy_DISK_SECTOR_SIZE - 1));
	}
      else
	sector -= p->length;
    }

  return ret;
}

struct holy_fs holy_fs_blocklist =
  {
    .name = "blocklist",
    .dir = 0,
    .open = holy_fs_blocklist_open,
    .read = holy_fs_blocklist_read,
    .close = 0,
    .next = 0
  };
