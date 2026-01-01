/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/ieee1275/ofdisk.h>
#include <holy/i18n.h>
#include <holy/time.h>

static char *last_devpath;
static holy_ieee1275_ihandle_t last_ihandle;

struct ofdisk_hash_ent
{
  char *devpath;
  char *open_path;
  char *holy_devpath;
  int is_boot;
  int is_removable;
  int block_size_fails;
  /* Pointer to shortest available name on nodes representing canonical names,
     otherwise NULL.  */
  const char *shortest;
  const char *holy_shortest;
  struct ofdisk_hash_ent *next;
};

static holy_err_t
holy_ofdisk_get_block_size (const char *device, holy_uint32_t *block_size,
			    struct ofdisk_hash_ent *op);

#define OFDISK_HASH_SZ	8
static struct ofdisk_hash_ent *ofdisk_hash[OFDISK_HASH_SZ];

static int
ofdisk_hash_fn (const char *devpath)
{
  int hash = 0;
  while (*devpath)
    hash ^= *devpath++;
  return (hash & (OFDISK_HASH_SZ - 1));
}

static struct ofdisk_hash_ent *
ofdisk_hash_find (const char *devpath)
{
  struct ofdisk_hash_ent *p = ofdisk_hash[ofdisk_hash_fn(devpath)];

  while (p)
    {
      if (!holy_strcmp (p->devpath, devpath))
	break;
      p = p->next;
    }
  return p;
}

static struct ofdisk_hash_ent *
ofdisk_hash_add_real (char *devpath)
{
  struct ofdisk_hash_ent *p;
  struct ofdisk_hash_ent **head = &ofdisk_hash[ofdisk_hash_fn(devpath)];
  const char *iptr;
  char *optr;

  p = holy_zalloc (sizeof (*p));
  if (!p)
    return NULL;

  p->devpath = devpath;

  p->holy_devpath = holy_malloc (sizeof ("ieee1275/")
				 + 2 * holy_strlen (p->devpath));

  if (!p->holy_devpath)
    {
      holy_free (p);
      return NULL;
    }

  if (! holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_PARTITION_0))
    {
      p->open_path = holy_malloc (holy_strlen (p->devpath) + 3);
      if (!p->open_path)
	{
	  holy_free (p->holy_devpath);
	  holy_free (p);
	  return NULL;
	}
      optr = holy_stpcpy (p->open_path, p->devpath);
      *optr++ = ':';
      *optr++ = '0';
      *optr = '\0';
    }
  else
    p->open_path = p->devpath;

  optr = holy_stpcpy (p->holy_devpath, "ieee1275/");
  for (iptr = p->devpath; *iptr; )
    {
      if (*iptr == ',')
	*optr++ = '\\';
      *optr++ = *iptr++;
    }
  *optr = 0;

  p->next = *head;
  *head = p;
  return p;
}

static int
check_string_removable (const char *str)
{
  const char *ptr = holy_strrchr (str, '/');

  if (ptr)
    ptr++;
  else
    ptr = str;
  return (holy_strncmp (ptr, "cdrom", 5) == 0 || holy_strncmp (ptr, "fd", 2) == 0);
}

static struct ofdisk_hash_ent *
ofdisk_hash_add (char *devpath, char *curcan)
{
  struct ofdisk_hash_ent *p, *pcan;

  p = ofdisk_hash_add_real (devpath);

  holy_dprintf ("disk", "devpath = %s, canonical = %s\n", devpath, curcan);

  if (!curcan)
    {
      p->shortest = p->devpath;
      p->holy_shortest = p->holy_devpath;
      if (check_string_removable (devpath))
	p->is_removable = 1;
      return p;
    }

  pcan = ofdisk_hash_find (curcan);
  if (!pcan)
    pcan = ofdisk_hash_add_real (curcan);
  else
    holy_free (curcan);

  if (check_string_removable (devpath) || check_string_removable (curcan))
    pcan->is_removable = 1;

  if (!pcan)
    holy_errno = holy_ERR_NONE;
  else
    {
      if (!pcan->shortest
	  || holy_strlen (pcan->shortest) > holy_strlen (devpath))
	{
	  pcan->shortest = p->devpath;
	  pcan->holy_shortest = p->holy_devpath;
	}
    }

  return p;
}

static void
dev_iterate_real (const char *name, const char *path)
{
  struct ofdisk_hash_ent *op;

  holy_dprintf ("disk", "disk name = %s, path = %s\n", name,
		path);

  op = ofdisk_hash_find (path);
  if (!op)
    {
      char *name_dup = holy_strdup (name);
      char *can = holy_strdup (path);
      if (!name_dup || !can)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_free (name_dup);
	  holy_free (can);
	  return;
	}
      op = ofdisk_hash_add (name_dup, can);
    }
  return;
}

static void
dev_iterate (const struct holy_ieee1275_devalias *alias)
{
  if (holy_strcmp (alias->type, "vscsi") == 0)
    {
      static holy_ieee1275_ihandle_t ihandle;
      struct set_color_args
      {
	struct holy_ieee1275_common_hdr common;
	holy_ieee1275_cell_t method;
	holy_ieee1275_cell_t ihandle;
	holy_ieee1275_cell_t catch_result;
	holy_ieee1275_cell_t nentries;
	holy_ieee1275_cell_t table;
      }
      args;
      char *buf, *bufptr;
      unsigned i;

      if (holy_ieee1275_open (alias->path, &ihandle))
	return;

      /* This method doesn't need memory allocation for the table. Open
         firmware takes care of all memory management and the result table
         stays in memory and is never freed. */
      INIT_IEEE1275_COMMON (&args.common, "call-method", 2, 3);
      args.method = (holy_ieee1275_cell_t) "vscsi-report-luns";
      args.ihandle = ihandle;
      args.table = 0;
      args.nentries = 0;

      if (IEEE1275_CALL_ENTRY_FN (&args) == -1 || args.catch_result)
	{
	  holy_ieee1275_close (ihandle);
	  return;
	}

      buf = holy_malloc (holy_strlen (alias->path) + 32);
      if (!buf)
	return;
      bufptr = holy_stpcpy (buf, alias->path);

      for (i = 0; i < args.nentries; i++)
	{
	  holy_uint64_t *ptr;

	  ptr = *(holy_uint64_t **) (args.table + 4 + 8 * i);
	  while (*ptr)
	    {
	      holy_snprintf (bufptr, 32, "/disk@%" PRIxholy_UINT64_T, *ptr++);
	      dev_iterate_real (buf, buf);
	    }
	}
      holy_ieee1275_close (ihandle);
      holy_free (buf);
      return;
    }
  else if (holy_strcmp (alias->type, "sas_ioa") == 0)
    {
      /* The method returns the number of disks and a table where
       * each ID is 64-bit long. Example of sas paths:
       *  /pci@80000002000001f/pci1014,034A@0/sas/disk@c05db70800
       *  /pci@80000002000001f/pci1014,034A@0/sas/disk@a05db70800
       *  /pci@80000002000001f/pci1014,034A@0/sas/disk@805db70800 */

      struct sas_children
        {
          struct holy_ieee1275_common_hdr common;
          holy_ieee1275_cell_t method;
          holy_ieee1275_cell_t ihandle;
          holy_ieee1275_cell_t max;
          holy_ieee1275_cell_t table;
          holy_ieee1275_cell_t catch_result;
          holy_ieee1275_cell_t nentries;
        }
      args;
      char *buf, *bufptr;
      unsigned i;
      holy_uint64_t *table;
      holy_uint16_t table_size;
      holy_ieee1275_ihandle_t ihandle;

      buf = holy_malloc (holy_strlen (alias->path) +
                         sizeof ("/disk@7766554433221100"));
      if (!buf)
        return;
      bufptr = holy_stpcpy (buf, alias->path);

      /* Power machines documentation specify 672 as maximum SAS disks in
         one system. Using a slightly larger value to be safe. */
      table_size = 768;
      table = holy_malloc (table_size * sizeof (holy_uint64_t));

      if (!table)
        {
          holy_free (buf);
          return;
        }

      if (holy_ieee1275_open (alias->path, &ihandle))
        {
          holy_free (buf);
          holy_free (table);
          return;
        }

      INIT_IEEE1275_COMMON (&args.common, "call-method", 4, 2);
      args.method = (holy_ieee1275_cell_t) "get-sas-children";
      args.ihandle = ihandle;
      args.max = table_size;
      args.table = (holy_ieee1275_cell_t) table;
      args.catch_result = 0;
      args.nentries = 0;

      if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
        {
          holy_ieee1275_close (ihandle);
          holy_free (table);
          holy_free (buf);
          return;
        }

      for (i = 0; i < args.nentries; i++)
        {
          holy_snprintf (bufptr, sizeof ("/disk@7766554433221100"),
                        "/disk@%" PRIxholy_UINT64_T, table[i]);
          dev_iterate_real (buf, buf);
        }

      holy_ieee1275_close (ihandle);
      holy_free (table);
      holy_free (buf);
    }

  if (!holy_ieee1275_test_flag (holy_IEEE1275_FLAG_NO_TREE_SCANNING_FOR_DISKS)
      && holy_strcmp (alias->type, "block") == 0)
    {
      dev_iterate_real (alias->path, alias->path);
      return;
    }

  {
    struct holy_ieee1275_devalias child;

    FOR_IEEE1275_DEVCHILDREN(alias->path, child)
      dev_iterate (&child);
  }
}

static void
scan (void)
{
  struct holy_ieee1275_devalias alias;
  FOR_IEEE1275_DEVALIASES(alias)
    {
      if (holy_strcmp (alias.type, "block") != 0)
	continue;
      dev_iterate_real (alias.name, alias.path);
    }

  FOR_IEEE1275_DEVCHILDREN("/", alias)
    dev_iterate (&alias);
}

static int
holy_ofdisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		     holy_disk_pull_t pull)
{
  unsigned i;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  scan ();
  
  for (i = 0; i < ARRAY_SIZE (ofdisk_hash); i++)
    {
      static struct ofdisk_hash_ent *ent;
      for (ent = ofdisk_hash[i]; ent; ent = ent->next)
	{
	  if (!ent->shortest)
	    continue;
	  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_OFDISK_SDCARD_ONLY))
	    {
	      holy_ieee1275_phandle_t dev;
	      char tmp[8];

	      if (holy_ieee1275_finddevice (ent->devpath, &dev))
		{
		  holy_dprintf ("disk", "finddevice (%s) failed\n",
				ent->devpath);
		  continue;
		}

	      if (holy_ieee1275_get_property (dev, "iconname", tmp,
					      sizeof tmp, 0))
		{
		  holy_dprintf ("disk", "get iconname failed\n");
		  continue;
		}

	      if (holy_strcmp (tmp, "sdmmc") != 0)
		{
		  holy_dprintf ("disk", "device is not an SD card\n");
		  continue;
		}
	    }

	  if (!ent->is_boot && ent->is_removable)
	    continue;

	  if (hook (ent->holy_shortest, hook_data))
	    return 1;
	}
    }	  
  return 0;
}

static char *
compute_dev_path (const char *name)
{
  char *devpath = holy_malloc (holy_strlen (name) + 3);
  char *p, c;

  if (!devpath)
    return NULL;

  /* Un-escape commas. */
  p = devpath;
  while ((c = *name++) != '\0')
    {
      if (c == '\\' && *name == ',')
	{
	  *p++ = ',';
	  name++;
	}
      else
	*p++ = c;
    }

  *p++ = '\0';

  return devpath;
}

static holy_err_t
holy_ofdisk_open (const char *name, holy_disk_t disk)
{
  holy_ieee1275_phandle_t dev;
  char *devpath;
  /* XXX: This should be large enough for any possible case.  */
  char prop[64];
  holy_ssize_t actual;
  holy_uint32_t block_size = 0;
  holy_err_t err;

  if (holy_strncmp (name, "ieee1275/", sizeof ("ieee1275/") - 1) != 0)
      return holy_error (holy_ERR_UNKNOWN_DEVICE,
			 "not IEEE1275 device");
  devpath = compute_dev_path (name + sizeof ("ieee1275/") - 1);
  if (! devpath)
    return holy_errno;

  holy_dprintf ("disk", "Opening `%s'.\n", devpath);

  if (holy_ieee1275_finddevice (devpath, &dev))
    {
      holy_free (devpath);
      return holy_error (holy_ERR_UNKNOWN_DEVICE,
			 "can't read device properties");
    }

  if (holy_ieee1275_get_property (dev, "device_type", prop, sizeof (prop),
				  &actual))
    {
      holy_free (devpath);
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "can't read the device type");
    }

  if (holy_strcmp (prop, "block"))
    {
      holy_free (devpath);
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a block device");
    }

  /* XXX: There is no property to read the number of blocks.  There
     should be a property `#blocks', but it is not there.  Perhaps it
     is possible to use seek for this.  */
  disk->total_sectors = holy_DISK_SIZE_UNKNOWN;

  {
    struct ofdisk_hash_ent *op;
    op = ofdisk_hash_find (devpath);
    if (!op)
      op = ofdisk_hash_add (devpath, NULL);
    if (!op)
      {
        holy_free (devpath);
        return holy_errno;
      }
    disk->id = (unsigned long) op;
    disk->data = op->open_path;

    err = holy_ofdisk_get_block_size (devpath, &block_size, op);
    if (err)
      {
        holy_free (devpath);
        return err;
      }
    if (block_size != 0)
      {
	for (disk->log_sector_size = 0;
	     (1U << disk->log_sector_size) < block_size;
	     disk->log_sector_size++);
      }
    else
      disk->log_sector_size = 9;
  }

  holy_free (devpath);
  return 0;
}

static void
holy_ofdisk_close (holy_disk_t disk)
{
  if (disk->data == last_devpath)
    {
      if (last_ihandle)
	holy_ieee1275_close (last_ihandle);
      last_ihandle = 0;
      last_devpath = NULL;
    }
  disk->data = 0;
}

static holy_err_t
holy_ofdisk_prepare (holy_disk_t disk, holy_disk_addr_t sector)
{
  holy_ssize_t status;
  unsigned long long pos;

  if (disk->data != last_devpath)
    {
      if (last_ihandle)
	holy_ieee1275_close (last_ihandle);
      last_ihandle = 0;
      last_devpath = NULL;

      holy_ieee1275_open (disk->data, &last_ihandle);
      if (! last_ihandle)
	return holy_error (holy_ERR_UNKNOWN_DEVICE, "can't open device");
      last_devpath = disk->data;      
    }

  pos = sector << disk->log_sector_size;

  holy_ieee1275_seek (last_ihandle, pos, &status);
  if (status < 0)
    return holy_error (holy_ERR_READ_ERROR,
		       "seek error, can't seek block %llu",
		       (long long) sector);
  return 0;
}

static holy_err_t
holy_ofdisk_read (holy_disk_t disk, holy_disk_addr_t sector,
		  holy_size_t size, char *buf)
{
  holy_err_t err;
  holy_ssize_t actual;
  err = holy_ofdisk_prepare (disk, sector);
  if (err)
    return err;
  holy_ieee1275_read (last_ihandle, buf, size  << disk->log_sector_size,
		      &actual);
  if (actual != (holy_ssize_t) (size  << disk->log_sector_size))
    return holy_error (holy_ERR_READ_ERROR, N_("failure reading sector 0x%llx "
					       "from `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return 0;
}

static holy_err_t
holy_ofdisk_write (holy_disk_t disk, holy_disk_addr_t sector,
		   holy_size_t size, const char *buf)
{
  holy_err_t err;
  holy_ssize_t actual;
  err = holy_ofdisk_prepare (disk, sector);
  if (err)
    return err;
  holy_ieee1275_write (last_ihandle, buf, size  << disk->log_sector_size,
		       &actual);
  if (actual != (holy_ssize_t) (size << disk->log_sector_size))
    return holy_error (holy_ERR_WRITE_ERROR, N_("failure writing sector 0x%llx "
						"to `%s'"),
		       (unsigned long long) sector,
		       disk->name);

  return 0;
}

static struct holy_disk_dev holy_ofdisk_dev =
  {
    .name = "ofdisk",
    .id = holy_DISK_DEVICE_OFDISK_ID,
    .iterate = holy_ofdisk_iterate,
    .open = holy_ofdisk_open,
    .close = holy_ofdisk_close,
    .read = holy_ofdisk_read,
    .write = holy_ofdisk_write,
    .next = 0
  };

static void
insert_bootpath (void)
{
  char *bootpath;
  holy_ssize_t bootpath_size;
  char *type;

  if (holy_ieee1275_get_property_length (holy_ieee1275_chosen, "bootpath",
					 &bootpath_size)
      || bootpath_size <= 0)
    {
      /* Should never happen.  */
      holy_printf ("/chosen/bootpath property missing!\n");
      return;
    }

  bootpath = (char *) holy_malloc ((holy_size_t) bootpath_size + 64);
  if (! bootpath)
    {
      holy_print_error ();
      return;
    }
  holy_ieee1275_get_property (holy_ieee1275_chosen, "bootpath", bootpath,
                              (holy_size_t) bootpath_size + 1, 0);
  bootpath[bootpath_size] = '\0';

  /* Transform an OF device path to a holy path.  */

  type = holy_ieee1275_get_device_type (bootpath);
  if (!(type && holy_strcmp (type, "network") == 0))
    {
      struct ofdisk_hash_ent *op;
      char *device = holy_ieee1275_get_devname (bootpath);
      op = ofdisk_hash_add (device, NULL);
      op->is_boot = 1;
    }
  holy_free (type);
  holy_free (bootpath);
}

void
holy_ofdisk_fini (void)
{
  if (last_ihandle)
    holy_ieee1275_close (last_ihandle);
  last_ihandle = 0;
  last_devpath = NULL;

  holy_disk_dev_unregister (&holy_ofdisk_dev);
}

void
holy_ofdisk_init (void)
{
  holy_disk_firmware_fini = holy_ofdisk_fini;

  insert_bootpath ();

  holy_disk_dev_register (&holy_ofdisk_dev);
}

static holy_err_t
holy_ofdisk_get_block_size (const char *device, holy_uint32_t *block_size,
			    struct ofdisk_hash_ent *op)
{
  struct size_args_ieee1275
    {
      struct holy_ieee1275_common_hdr common;
      holy_ieee1275_cell_t method;
      holy_ieee1275_cell_t ihandle;
      holy_ieee1275_cell_t result;
      holy_ieee1275_cell_t size1;
      holy_ieee1275_cell_t size2;
    } args_ieee1275;

  if (last_ihandle)
    holy_ieee1275_close (last_ihandle);

  last_ihandle = 0;
  last_devpath = NULL;

  holy_ieee1275_open (device, &last_ihandle);
  if (! last_ihandle)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "can't open device");

  *block_size = 0;

  if (op->block_size_fails >= 2)
    return holy_ERR_NONE;

  INIT_IEEE1275_COMMON (&args_ieee1275.common, "call-method", 2, 2);
  args_ieee1275.method = (holy_ieee1275_cell_t) "block-size";
  args_ieee1275.ihandle = last_ihandle;
  args_ieee1275.result = 1;

  if (IEEE1275_CALL_ENTRY_FN (&args_ieee1275) == -1)
    {
      holy_dprintf ("disk", "can't get block size: failed call-method\n");
      op->block_size_fails++;
    }
  else if (args_ieee1275.result)
    {
      holy_dprintf ("disk", "can't get block size: %lld\n",
		    (long long) args_ieee1275.result);
      op->block_size_fails++;
    }
  else if (args_ieee1275.size1
	   && !(args_ieee1275.size1 & (args_ieee1275.size1 - 1))
	   && args_ieee1275.size1 >= 512 && args_ieee1275.size1 <= 16384)
    {
      op->block_size_fails = 0;
      *block_size = args_ieee1275.size1;
    }

  return 0;
}
