/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_nand_data
{
  holy_ieee1275_ihandle_t handle;
  holy_uint32_t block_size;
};

static int
holy_nand_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		   holy_disk_pull_t pull)
{
  static int have_nand = -1;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  if (have_nand == -1)
    {
      struct holy_ieee1275_devalias alias;

      have_nand = 0;
      FOR_IEEE1275_DEVALIASES(alias)
	if (holy_strcmp (alias.name, "nand") == 0)
	  {
	    have_nand = 1;
	    break;
	  }
      holy_ieee1275_devalias_free (&alias);
    }

  if (have_nand)
    return hook ("nand", hook_data);

  return 0;
}

static holy_err_t
holy_nand_read (holy_disk_t disk, holy_disk_addr_t sector,
                holy_size_t size, char *buf);

static holy_err_t
holy_nand_open (const char *name, holy_disk_t disk)
{
  holy_ieee1275_ihandle_t dev_ihandle = 0;
  struct holy_nand_data *data = 0;
  const char *devname;
  struct size_args
    {
      struct holy_ieee1275_common_hdr common;
      holy_ieee1275_cell_t method;
      holy_ieee1275_cell_t ihandle;
      holy_ieee1275_cell_t result;
      holy_ieee1275_cell_t size1;
      holy_ieee1275_cell_t size2;
    } args;

  if (holy_memcmp (name, "nand/", sizeof ("nand/") - 1) == 0)
    devname = name + sizeof ("nand/") - 1;
  else if (holy_strcmp (name, "nand") == 0)
    devname = name;
  else
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a NAND device");

  data = holy_malloc (sizeof (*data));
  if (! data)
    goto fail;

  holy_ieee1275_open (devname, &dev_ihandle);
  if (! dev_ihandle)
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, "can't open device");
      goto fail;
    }

  data->handle = dev_ihandle;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 2, 2);
  args.method = (holy_ieee1275_cell_t) "block-size";
  args.ihandle = dev_ihandle;
  args.result = 1;

  if ((IEEE1275_CALL_ENTRY_FN (&args) == -1) || (args.result))
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, "can't get block size");
      goto fail;
    }

  data->block_size = (args.size1 >> holy_DISK_SECTOR_BITS);
  if (!data->block_size)
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, "invalid block size");
      goto fail;
    }

  INIT_IEEE1275_COMMON (&args.common, "call-method", 2, 3);
  args.method = (holy_ieee1275_cell_t) "size";
  args.ihandle = dev_ihandle;
  args.result = 1;

  if ((IEEE1275_CALL_ENTRY_FN (&args) == -1) || (args.result))
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, "can't get disk size");
      goto fail;
    }

  disk->total_sectors = args.size1;
  disk->total_sectors <<= 32;
  disk->total_sectors += args.size2;
  disk->total_sectors >>= holy_DISK_SECTOR_BITS;

  disk->id = dev_ihandle;

  disk->data = data;

  return 0;

fail:
  if (dev_ihandle)
    holy_ieee1275_close (dev_ihandle);
  holy_free (data);
  return holy_errno;
}

static void
holy_nand_close (holy_disk_t disk)
{
  holy_ieee1275_close (((struct holy_nand_data *) disk->data)->handle);
  holy_free (disk->data);
}

static holy_err_t
holy_nand_read (holy_disk_t disk, holy_disk_addr_t sector,
                holy_size_t size, char *buf)
{
  struct holy_nand_data *data = disk->data;
  holy_size_t bsize, ofs;

  struct read_args
    {
      struct holy_ieee1275_common_hdr common;
      holy_ieee1275_cell_t method;
      holy_ieee1275_cell_t ihandle;
      holy_ieee1275_cell_t ofs;
      holy_ieee1275_cell_t page;
      holy_ieee1275_cell_t len;
      holy_ieee1275_cell_t buf;
      holy_ieee1275_cell_t result;
    } args;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = (holy_ieee1275_cell_t) "pio-read";
  args.ihandle = data->handle;
  args.buf = (holy_ieee1275_cell_t) buf;
  args.page = (holy_ieee1275_cell_t) ((holy_size_t) sector / data->block_size);

  ofs = ((holy_size_t) sector % data->block_size) << holy_DISK_SECTOR_BITS;
  size <<= holy_DISK_SECTOR_BITS;
  bsize = (data->block_size << holy_DISK_SECTOR_BITS);

  do
    {
      holy_size_t len;

      len = (ofs + size > bsize) ? (bsize - ofs) : size;

      args.len = (holy_ieee1275_cell_t) len;
      args.ofs = (holy_ieee1275_cell_t) ofs;
      args.result = 1;

      if ((IEEE1275_CALL_ENTRY_FN (&args) == -1) || (args.result))
        return holy_error (holy_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
						   "from `%s'"),
			   (unsigned long long) sector,
			   disk->name);

      ofs = 0;
      size -= len;
      args.buf += len;
      args.page++;
    } while (size);

  return holy_ERR_NONE;
}

static holy_err_t
holy_nand_write (holy_disk_t disk __attribute ((unused)),
                 holy_disk_addr_t sector __attribute ((unused)),
                 holy_size_t size __attribute ((unused)),
                 const char *buf __attribute ((unused)))
{
  return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		     "nand write is not supported");
}

static struct holy_disk_dev holy_nand_dev =
  {
    .name = "nand",
    .id = holy_DISK_DEVICE_NAND_ID,
    .iterate = holy_nand_iterate,
    .open = holy_nand_open,
    .close = holy_nand_close,
    .read = holy_nand_read,
    .write = holy_nand_write,
    .next = 0
  };

holy_MOD_INIT(nand)
{
  holy_disk_dev_register (&holy_nand_dev);
}

holy_MOD_FINI(nand)
{
  holy_disk_dev_unregister (&holy_nand_dev);
}
