/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <config-util.h>

#include <holy/dl.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/emu/hostdisk.h>

int holy_disk_host_i_want_a_reference;

static int
holy_host_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
		   holy_disk_pull_t pull)
{
  if (pull != holy_DISK_PULL_NONE)
    return 0;

  if (hook ("host", hook_data))
    return 1;
  return 0;
}

static holy_err_t
holy_host_open (const char *name, holy_disk_t disk)
{
  if (holy_strcmp (name, "host"))
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "not a host disk");

  disk->total_sectors = 0;
  disk->id = 0;

  disk->data = 0;

  return holy_ERR_NONE;
}

static void
holy_host_close (holy_disk_t disk __attribute((unused)))
{
}

static holy_err_t
holy_host_read (holy_disk_t disk __attribute((unused)),
		holy_disk_addr_t sector __attribute((unused)),
		holy_size_t size __attribute((unused)),
		char *buf __attribute((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

static holy_err_t
holy_host_write (holy_disk_t disk __attribute ((unused)),
		     holy_disk_addr_t sector __attribute ((unused)),
		     holy_size_t size __attribute ((unused)),
		     const char *buf __attribute ((unused)))
{
  return holy_ERR_OUT_OF_RANGE;
}

static struct holy_disk_dev holy_host_dev =
  {
    /* The only important line in this file :-) */
    .name = "host",
    .id = holy_DISK_DEVICE_HOST_ID,
    .iterate = holy_host_iterate,
    .open = holy_host_open,
    .close = holy_host_close,
    .read = holy_host_read,
    .write = holy_host_write,
    .next = 0
  };

holy_MOD_INIT(host)
{
  holy_disk_dev_register (&holy_host_dev);
}

holy_MOD_FINI(host)
{
  holy_disk_dev_unregister (&holy_host_dev);
}
