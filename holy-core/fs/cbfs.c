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
#include <holy/cbfs_core.h>

holy_MOD_LICENSE ("GPLv2+");


struct holy_archelp_data
{
  holy_disk_t disk;
  holy_off_t hofs, next_hofs;
  holy_off_t dofs;
  holy_off_t size;
  holy_off_t cbfs_start;
  holy_off_t cbfs_end;
  holy_off_t cbfs_align;
};

static holy_err_t
holy_cbfs_find_file (struct holy_archelp_data *data, char **name,
		     holy_int32_t *mtime,
		     holy_uint32_t *mode)
{
  holy_size_t offset;
  for (;;
       data->dofs = data->hofs + offset,
	 data->next_hofs = ALIGN_UP (data->dofs + data->size, data->cbfs_align))
    {
      struct cbfs_file hd;
      holy_size_t namesize;

      data->hofs = data->next_hofs;

      if (data->hofs >= data->cbfs_end)
	{
	  *mode = holy_ARCHELP_ATTR_END;
	  return holy_ERR_NONE;
	}

      if (holy_disk_read (data->disk, 0, data->hofs, sizeof (hd), &hd))
	return holy_errno;

      if (holy_memcmp (hd.magic, CBFS_FILE_MAGIC, sizeof (hd.magic)) != 0)
	{
	  *mode = holy_ARCHELP_ATTR_END;
	  return holy_ERR_NONE;
	}
      data->size = holy_be_to_cpu32 (hd.len);
      (void) mtime;
      offset = holy_be_to_cpu32 (hd.offset);

      *mode = holy_ARCHELP_ATTR_FILE | holy_ARCHELP_ATTR_NOTIME;

      namesize = offset;
      if (namesize >= sizeof (hd))
	namesize -= sizeof (hd);
      if (namesize == 0)
	continue;
      *name = holy_malloc (namesize + 1);
      if (*name == NULL)
	return holy_errno;

      if (holy_disk_read (data->disk, 0, data->hofs + sizeof (hd),
			  namesize, *name))
	{
	  holy_free (*name);
	  return holy_errno;
	}

      if ((*name)[0] == '\0')
	{
	  holy_free (*name);
	  *name = NULL;
	  continue;
	}

      (*name)[namesize] = 0;

      data->dofs = data->hofs + offset;
      data->next_hofs = ALIGN_UP (data->dofs + data->size, data->cbfs_align);
      return holy_ERR_NONE;
    }
}

static void
holy_cbfs_rewind (struct holy_archelp_data *data)
{
  data->next_hofs = data->cbfs_start;
}

static struct holy_archelp_ops arcops =
  {
    .find_file = holy_cbfs_find_file,
    .rewind = holy_cbfs_rewind
  };

static int
validate_head (struct cbfs_header *head)
{
  return (head->magic == holy_cpu_to_be32_compile_time (CBFS_HEADER_MAGIC)
	  && (head->version
	      == holy_cpu_to_be32_compile_time (CBFS_HEADER_VERSION1)
	      || head->version
	      == holy_cpu_to_be32_compile_time (CBFS_HEADER_VERSION2))
	  && (holy_be_to_cpu32 (head->bootblocksize)
	      < holy_be_to_cpu32 (head->romsize))
	  && (holy_be_to_cpu32 (head->offset)
	      < holy_be_to_cpu32 (head->romsize))
	  && (holy_be_to_cpu32 (head->offset)
	      + holy_be_to_cpu32 (head->bootblocksize)
	      < holy_be_to_cpu32 (head->romsize))
	  && head->align != 0
	  && (head->align & (head->align - 1)) == 0
	  && head->romsize != 0);
}

static struct holy_archelp_data *
holy_cbfs_mount (holy_disk_t disk)
{
  struct cbfs_file hd;
  struct holy_archelp_data *data = NULL;
  holy_uint32_t ptr;
  holy_off_t header_off;
  struct cbfs_header head;

  if (holy_disk_get_size (disk) == holy_DISK_SIZE_UNKNOWN)
    goto fail;

  if (holy_disk_read (disk, holy_disk_get_size (disk) - 1,
		      holy_DISK_SECTOR_SIZE - sizeof (ptr),
		      sizeof (ptr), &ptr))
    goto fail;

  ptr = holy_cpu_to_le32 (ptr);
  header_off = (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS)
    + (holy_int32_t) ptr;

  if (holy_disk_read (disk, 0, header_off,
		      sizeof (head), &head))
    goto fail;

  if (!validate_head (&head))
    goto fail;

  data = (struct holy_archelp_data *) holy_zalloc (sizeof (*data));
  if (!data)
    goto fail;

  data->cbfs_start = (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS)
    - (holy_be_to_cpu32 (head.romsize) - holy_be_to_cpu32 (head.offset));
  data->cbfs_end = (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS)
    - holy_be_to_cpu32 (head.bootblocksize);
  data->cbfs_align = holy_be_to_cpu32 (head.align);

  if (data->cbfs_start >= (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS))
    goto fail;
  if (data->cbfs_end > (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS))
    data->cbfs_end = (holy_disk_get_size (disk) << holy_DISK_SECTOR_BITS);

  data->next_hofs = data->cbfs_start;

  if (holy_disk_read (disk, 0, data->cbfs_start, sizeof (hd), &hd))
    goto fail;

  if (holy_memcmp (hd.magic, CBFS_FILE_MAGIC, sizeof (CBFS_FILE_MAGIC) - 1))
    goto fail;

  data->disk = disk;

  return data;

fail:
  holy_free (data);
  holy_error (holy_ERR_BAD_FS, "not a cbfs filesystem");
  return 0;
}

static holy_err_t
holy_cbfs_dir (holy_device_t device, const char *path_in,
	       holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cbfs_mount (device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_dir (data, &arcops,
			  path_in, hook, hook_data);

  holy_free (data);

  return err;
}

static holy_err_t
holy_cbfs_open (holy_file_t file, const char *name_in)
{
  struct holy_archelp_data *data;
  holy_err_t err;

  data = holy_cbfs_mount (file->device->disk);
  if (!data)
    return holy_errno;

  err = holy_archelp_open (data, &arcops, name_in);
  if (err)
    {
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
holy_cbfs_read (holy_file_t file, char *buf, holy_size_t len)
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
holy_cbfs_close (holy_file_t file)
{
  struct holy_archelp_data *data;

  data = file->data;
  holy_free (data);

  return holy_errno;
}

#if (defined (__i386__) || defined (__x86_64__)) && !defined (holy_UTIL) \
  && !defined (holy_MACHINE_EMU) && !defined (holy_MACHINE_XEN)

static char *cbfsdisk_addr;
static holy_off_t cbfsdisk_size = 0;

static int
holy_cbfsdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		       holy_disk_pull_t pull)
{
  if (pull != holy_DISK_PULL_NONE)
    return 0;

  return hook ("cbfsdisk", hook_data);
}

static holy_err_t
holy_cbfsdisk_open (const char *name, holy_disk_t disk)
{
  if (holy_strcmp (name, "cbfsdisk"))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a cbfsdisk");

  disk->total_sectors = cbfsdisk_size / holy_DISK_SECTOR_SIZE;
  disk->max_agglomerate = holy_DISK_MAX_MAX_AGGLOMERATE;
  disk->id = 0;

  return holy_ERR_NONE;
}

static void
holy_cbfsdisk_close (holy_disk_t disk __attribute((unused)))
{
}

static holy_err_t
holy_cbfsdisk_read (holy_disk_t disk __attribute((unused)),
		    holy_disk_addr_t sector,
		    holy_size_t size, char *buf)
{
  holy_memcpy (buf, cbfsdisk_addr + (sector << holy_DISK_SECTOR_BITS),
	       size << holy_DISK_SECTOR_BITS);
  return 0;
}

static holy_err_t
holy_cbfsdisk_write (holy_disk_t disk __attribute__ ((unused)),
		     holy_disk_addr_t sector __attribute__ ((unused)),
		     holy_size_t size __attribute__ ((unused)),
		     const char *buf __attribute__ ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "rom flashing isn't implemented yet");
}

static struct holy_disk_dev holy_cbfsdisk_dev =
  {
    .name = "cbfsdisk",
    .id = holy_DISK_DEVICE_CBFSDISK_ID,
    .iterate = holy_cbfsdisk_iterate,
    .open = holy_cbfsdisk_open,
    .close = holy_cbfsdisk_close,
    .read = holy_cbfsdisk_read,
    .write = holy_cbfsdisk_write,
    .next = 0
  };

static void
init_cbfsdisk (void)
{
  holy_uint32_t ptr;
  struct cbfs_header *head;

  ptr = *(holy_uint32_t *) 0xfffffffc;
  head = (struct cbfs_header *) (holy_addr_t) ptr;
  holy_dprintf ("cbfs", "head=%p\n", head);

  /* coreboot current supports only ROMs <= 16 MiB. Bigger ROMs will
     have problems as RCBA is 18 MiB below end of 32-bit typically,
     so either memory map would have to be rearranged or we'd need to support
     reading ROMs through controller directly.
   */
  if (ptr < 0xff000000
      || 0xffffffff - ptr < (holy_uint32_t) sizeof (*head) + 0xf
      || !validate_head (head))
    return;

  cbfsdisk_size = ALIGN_UP (holy_be_to_cpu32 (head->romsize),
			    holy_DISK_SECTOR_SIZE);
  cbfsdisk_addr = (void *) (holy_addr_t) (0x100000000ULL - cbfsdisk_size);

  holy_disk_dev_register (&holy_cbfsdisk_dev);
}

static void
fini_cbfsdisk (void)
{
  if (! cbfsdisk_size)
    return;
  holy_disk_dev_unregister (&holy_cbfsdisk_dev);
}

#endif

static struct holy_fs holy_cbfs_fs = {
  .name = "cbfs",
  .dir = holy_cbfs_dir,
  .open = holy_cbfs_open,
  .read = holy_cbfs_read,
  .close = holy_cbfs_close,
#ifdef holy_UTIL
  .reserved_first_sector = 0,
  .blocklist_install = 0,
#endif
};

holy_MOD_INIT (cbfs)
{
#if (defined (__i386__) || defined (__x86_64__)) && !defined (holy_UTIL) && !defined (holy_MACHINE_EMU) && !defined (holy_MACHINE_XEN)
  init_cbfsdisk ();
#endif
  holy_fs_register (&holy_cbfs_fs);
}

holy_MOD_FINI (cbfs)
{
  holy_fs_unregister (&holy_cbfs_fs);
#if (defined (__i386__) || defined (__x86_64__)) && !defined (holy_UTIL) && !defined (holy_MACHINE_EMU) && !defined (holy_MACHINE_XEN)
  fini_cbfsdisk ();
#endif
}
