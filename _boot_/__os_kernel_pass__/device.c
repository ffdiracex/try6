/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/device.h>
#include <holy/disk.h>
#include <holy/net.h>
#include <holy/fs.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/partition.h>
#include <holy/i18n.h>

holy_net_t (*holy_net_open) (const char *name) = NULL;

holy_device_t
holy_device_open (const char *name)
{
  holy_device_t dev = 0;

  if (! name)
    {
      name = holy_env_get ("root");
      if (name == NULL || *name == '\0')
	{
	  holy_error (holy_ERR_BAD_DEVICE,  N_("variable `%s' isn't set"), "root");
	  goto fail;
	}
    }

  dev = holy_malloc (sizeof (*dev));
  if (! dev)
    goto fail;

  dev->net = NULL;
  /* Try to open a disk.  */
  dev->disk = holy_disk_open (name);
  if (dev->disk)
    return dev;
  if (holy_net_open && holy_errno == holy_ERR_UNKNOWN_DEVICE)
    {
      holy_errno = holy_ERR_NONE;
      dev->net = holy_net_open (name);
    }

  if (dev->net)
    return dev;

 fail:
  holy_free (dev);

  return 0;
}

holy_err_t
holy_device_close (holy_device_t device)
{
  if (device->disk)
    holy_disk_close (device->disk);

  if (device->net)
    {
      holy_free (device->net->server);
      holy_free (device->net);
    }

  holy_free (device);

  return holy_errno;
}

struct part_ent
{
  struct part_ent *next;
  char *name;
};

/* Context for holy_device_iterate.  */
struct holy_device_iterate_ctx
{
  holy_device_iterate_hook_t hook;
  void *hook_data;
  struct part_ent *ents;
};

/* Helper for holy_device_iterate.  */
static int
iterate_partition (holy_disk_t disk, const holy_partition_t partition,
		   void *data)
{
  struct holy_device_iterate_ctx *ctx = data;
  struct part_ent *p;
  char *part_name;

  p = holy_malloc (sizeof (*p));
  if (!p)
    {
      return 1;
    }

  part_name = holy_partition_get_name (partition);
  if (!part_name)
    {
      holy_free (p);
      return 1;
    }
  p->name = holy_xasprintf ("%s,%s", disk->name, part_name);
  holy_free (part_name);
  if (!p->name)
    {
      holy_free (p);
      return 1;
    }

  p->next = ctx->ents;
  ctx->ents = p;

  return 0;
}

/* Helper for holy_device_iterate.  */
static int
iterate_disk (const char *disk_name, void *data)
{
  struct holy_device_iterate_ctx *ctx = data;
  holy_device_t dev;

  if (ctx->hook (disk_name, ctx->hook_data))
    return 1;

  dev = holy_device_open (disk_name);
  if (! dev)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }

  if (dev->disk)
    {
      struct part_ent *p;
      int ret = 0;

      ctx->ents = NULL;
      (void) holy_partition_iterate (dev->disk, iterate_partition, ctx);
      holy_device_close (dev);

      holy_errno = holy_ERR_NONE;

      p = ctx->ents;
      while (p != NULL)
	{
	  struct part_ent *next = p->next;

	  if (!ret)
	    ret = ctx->hook (p->name, ctx->hook_data);
	  holy_free (p->name);
	  holy_free (p);
	  p = next;
	}

      return ret;
    }

  holy_device_close (dev);
  return 0;
}

int
holy_device_iterate (holy_device_iterate_hook_t hook, void *hook_data)
{
  struct holy_device_iterate_ctx ctx = { hook, hook_data, NULL };

  /* Only disk devices are supported at the moment.  */
  return holy_disk_dev_iterate (iterate_disk, &ctx);
}
