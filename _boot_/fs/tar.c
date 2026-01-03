/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/archelp.h>

#include <holy/file.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* tar support */
#define MAGIC	"ustar"
struct head
{
  char name[100];
  char mode[8];
  char uid[8];
  char gid[8];
  char size[12];
  char mtime[12];
  char chksum[8];
  char typeflag;
  char linkname[100];
  char magic[6];
  char version[2];
  char uname[32];
  char gname[32];
  char devmajor[8];
  char devminor[8];
  char prefix[155];
} holy_PACKED;

static inline unsigned long long
read_number (const char *str, holy_size_t size)
{
  unsigned long long ret = 0;
  while (size-- && *str >= '0' && *str <= '7')
    ret = (ret << 3) | (*str++ & 0xf);
  return ret;
}

struct holy_archelp_data
{
  holy_disk_t disk;
  holy_off_t hofs, next_hofs;
  holy_off_t dofs;
  holy_off_t size;
  char *linkname;
  holy_size_t linkname_alloc;
};

static holy_err_t
holy_cpio_find_file (struct holy_archelp_data *data, char **name,
		     holy_int32_t *mtime,
		     holy_uint32_t *mode)
{
  struct head hd;
  int reread = 0, have_longname = 0, have_longlink = 0;

  data->hofs = data->next_hofs;

  for (reread = 0; reread < 3; reread++)
    {
      if (holy_disk_read (data->disk, 0, data->hofs, sizeof (hd), &hd))
	return holy_errno;

      if (!hd.name[0] && !hd.prefix[0])
	{
	  *mode = holy_ARCHELP_ATTR_END;
	  return holy_ERR_NONE;
	}

      if (holy_memcmp (hd.magic, MAGIC, sizeof (MAGIC) - 1))
	return holy_error (holy_ERR_BAD_FS, "invalid tar archive");

      if (hd.typeflag == 'L')
	{
	  holy_err_t err;
	  holy_size_t namesize = read_number (hd.size, sizeof (hd.size));
	  *name = holy_malloc (namesize + 1);
	  if (*name == NULL)
	    return holy_errno;
	  err = holy_disk_read (data->disk, 0,
				data->hofs + holy_DISK_SECTOR_SIZE, namesize,
				*name);
	  (*name)[namesize] = 0;
	  if (err)
	    return err;
	  data->hofs += holy_DISK_SECTOR_SIZE
	    + ((namesize + holy_DISK_SECTOR_SIZE - 1) &
	       ~(holy_DISK_SECTOR_SIZE - 1));
	  have_longname = 1;
	  continue;
	}

      if (hd.typeflag == 'K')
	{
	  holy_err_t err;
	  holy_size_t linksize = read_number (hd.size, sizeof (hd.size));
	  if (data->linkname_alloc < linksize + 1)
	    {
	      char *n;
	      n = holy_malloc (2 * (linksize + 1));
	      if (!n)
		return holy_errno;
	      holy_free (data->linkname);
	      data->linkname = n;
	      data->linkname_alloc = 2 * (linksize + 1);
	    }

	  err = holy_disk_read (data->disk, 0,
				data->hofs + holy_DISK_SECTOR_SIZE, linksize,
				data->linkname);
	  if (err)
	    return err;
	  data->linkname[linksize] = 0;
	  data->hofs += holy_DISK_SECTOR_SIZE
	    + ((linksize + holy_DISK_SECTOR_SIZE - 1) &
	       ~(holy_DISK_SECTOR_SIZE - 1));
	  have_longlink = 1;
	  continue;
	}

      if (!have_longname)
	{
	  holy_size_t extra_size = 0;

	  while (extra_size < sizeof (hd.prefix)
		 && hd.prefix[extra_size])
	    extra_size++;
	  *name = holy_malloc (sizeof (hd.name) + extra_size + 2);
	  if (*name == NULL)
	    return holy_errno;
	  if (hd.prefix[0])
	    {
	      holy_memcpy (*name, hd.prefix, extra_size);
	      (*name)[extra_size++] = '/';
	    }
	  holy_memcpy (*name + extra_size, hd.name, sizeof (hd.name));
	  (*name)[extra_size + sizeof (hd.name)] = 0;
	}

      data->size = read_number (hd.size, sizeof (hd.size));
      data->dofs = data->hofs + holy_DISK_SECTOR_SIZE;
      data->next_hofs = data->dofs + ((data->size + holy_DISK_SECTOR_SIZE - 1) &
			   ~(holy_DISK_SECTOR_SIZE - 1));
      if (mtime)
	*mtime = read_number (hd.mtime, sizeof (hd.mtime));
      if (mode)
	{
	  *mode = read_number (hd.mode, sizeof (hd.mode));
	  switch (hd.typeflag)
	    {
	      /* Hardlink.  */
	    case '1':
	      /* Symlink.  */
	    case '2':
	      *mode |= holy_ARCHELP_ATTR_LNK;
	      break;
	    case '0':
	      *mode |= holy_ARCHELP_ATTR_FILE;
	      break;
	    case '5':
	      *mode |= holy_ARCHELP_ATTR_DIR;
	      break;
	    }
	}
      if (!have_longlink)
	{
	  if (data->linkname_alloc < 101)
	    {
	      char *n;
	      n = holy_malloc (101);
	      if (!n)
		return holy_errno;
	      holy_free (data->linkname);
	      data->linkname = n;
	      data->linkname_alloc = 101;
	    }
	  holy_memcpy (data->linkname, hd.linkname, sizeof (hd.linkname));
	  data->linkname[100] = 0;
	}
      return holy_ERR_NONE;
    }
  return holy_ERR_NONE;
}

static char *
holy_cpio_get_link_target (struct holy_archelp_data *data)
{
  return holy_strdup (data->linkname);
}

static void
holy_cpio_rewind (struct holy_archelp_data *data)
{
  data->next_hofs = 0;
}

static struct holy_archelp_ops arcops =
  {
    .find_file = holy_cpio_find_file,
    .get_link_target = holy_cpio_get_link_target,
    .rewind = holy_cpio_rewind
  };

static struct holy_archelp_data *
holy_cpio_mount (holy_disk_t disk)
{
  struct head hd;
  struct holy_archelp_data *data;

  if (holy_disk_read (disk, 0, 0, sizeof (hd), &hd))
    goto fail;

  if (holy_memcmp (hd.magic, MAGIC, sizeof (MAGIC) - 1))
    goto fail;

  data = (struct holy_archelp_data *) holy_zalloc (sizeof (*data));
  if (!data)
    goto fail;

  data->disk = disk;

  return data;

fail:
  holy_error (holy_ERR_BAD_FS, "not a tarfs filesystem");
  return 0;
}

static holy_err_t
holy_cpio_dir (holy_device_t device, const char *path_in,
	       holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cpio_mount (device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_dir (data, &arcops,
			  path_in, hook, hook_data);

  holy_free (data->linkname);
  holy_free (data);

  return err;
}

static holy_err_t
holy_cpio_open (holy_file_t file, const char *name_in)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cpio_mount (file->device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_open (data, &arcops, name_in);
  if (err)
    {
      holy_free (data->linkname);
      holy_free (data);
    }
  else
    {
      file->data = data;
      file->size = data->size;
    }
  return err;
}

static holy_ssize_t
holy_cpio_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_archelp_data *data;
  holy_ssize_t ret;

  data = file->data;

  data->disk->read_hook = file->read_hook;
  data->disk->read_hook_data = file->read_hook_data;
  ret = (holy_disk_read (data->disk, 0, data->dofs + file->offset,
			 len, buf)) ? -1 : (holy_ssize_t) len;
  data->disk->read_hook = 0;

  return ret;
}

static holy_err_t
holy_cpio_close (holy_file_t file)
{
  struct holy_archelp_data *data;

  data = file->data;
  holy_free (data->linkname);
  holy_free (data);

  return holy_errno;
}

static struct holy_fs holy_cpio_fs = {
  .name = "tarfs",
  .dir = holy_cpio_dir,
  .open = holy_cpio_open,
  .read = holy_cpio_read,
  .close = holy_cpio_close,
#ifdef holy_UTIL
  .reserved_first_sector = 0,
  .blocklist_install = 0,
#endif
};

holy_MOD_INIT (tar)
{
  holy_fs_register (&holy_cpio_fs);
}

holy_MOD_FINI (tar)
{
  holy_fs_unregister (&holy_cpio_fs);
}
