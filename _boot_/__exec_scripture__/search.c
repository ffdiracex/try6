/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/device.h>
#include <holy/file.h>
#include <holy/env.h>
#include <holy/command.h>
#include <holy/search.h>
#include <holy/i18n.h>
#include <holy/disk.h>
#include <holy/partition.h>
#if defined(DO_SEARCH_PART_UUID) || defined(DO_SEARCH_PART_LABEL) || \
    defined(DO_SEARCH_DISK_UUID)
#include <holy/gpt_partition.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

struct cache_entry
{
  struct cache_entry *next;
  char *key;
  char *value;
};

static struct cache_entry *cache;

/* Context for FUNC_NAME.  */
struct search_ctx
{
  const char *key;
  const char *var;
  int no_floppy;
  char **hints;
  unsigned nhints;
  int count;
  int is_cache;
};

/* Helper for FUNC_NAME.  */
static int
iterate_device (const char *name, void *data)
{
  struct search_ctx *ctx = data;
  int found = 0;

  /* Skip floppy drives when requested.  */
  if (ctx->no_floppy &&
      name[0] == 'f' && name[1] == 'd' && name[2] >= '0' && name[2] <= '9')
    return 1;

#if defined(DO_SEARCH_FS_UUID) || defined(DO_SEARCH_DISK_UUID)
#define compare_fn holy_strcasecmp
#else
#define compare_fn holy_strcmp
#endif

#ifdef DO_SEARCH_FILE
    {
      char *buf;
      holy_file_t file;

      buf = holy_xasprintf ("(%s)%s", name, ctx->key);
      if (! buf)
	return 1;

      holy_file_filter_disable_compression ();
      file = holy_file_open (buf);
      if (file)
	{
	  found = 1;
	  holy_file_close (file);
	}
      holy_free (buf);
    }
#elif defined(DO_SEARCH_PART_UUID)
    {
      holy_device_t dev;
      char *quid;

      dev = holy_device_open (name);
      if (dev)
	{
	  if (holy_gpt_part_uuid (dev, &quid) == holy_ERR_NONE)
	    {
	      if (holy_strcasecmp (quid, ctx->key) == 0)
		    found = 1;

	      holy_free (quid);
	    }

	  holy_device_close (dev);
	}
    }
#elif defined(DO_SEARCH_PART_LABEL)
    {
      holy_device_t dev;
      char *quid;

      dev = holy_device_open (name);
      if (dev)
	{
	  if (holy_gpt_part_label (dev, &quid) == holy_ERR_NONE)
	    {
	      if (holy_strcmp (quid, ctx->key) == 0)
		    found = 1;

	      holy_free (quid);
	    }

	  holy_device_close (dev);
	}
    }
#elif defined(DO_SEARCH_DISK_UUID)
    {
      holy_device_t dev;
      char *quid;

      dev = holy_device_open (name);
      if (dev)
	{
	  if (holy_gpt_disk_uuid (dev, &quid) == holy_ERR_NONE)
	    {
	      if (holy_strcmp (quid, ctx->key) == 0)
		found = 1;

	      holy_free (quid);
	    }

	  holy_device_close (dev);
	}
    }
#else
    {
      /* SEARCH_FS_UUID or SEARCH_LABEL */
      holy_device_t dev;
      holy_fs_t fs;
      char *quid;

      dev = holy_device_open (name);
      if (dev)
	{
	  fs = holy_fs_probe (dev);

#ifdef DO_SEARCH_FS_UUID
#define read_fn uuid
#else
#define read_fn label
#endif

	  if (fs && fs->read_fn)
	    {
	      fs->read_fn (dev, &quid);

	      if (holy_errno == holy_ERR_NONE && quid)
		{
		  if (compare_fn (quid, ctx->key) == 0)
		    found = 1;

		  holy_free (quid);
		}
	    }

	  holy_device_close (dev);
	}
    }
#endif

  if (!ctx->is_cache && found && ctx->count == 0)
    {
      struct cache_entry *cache_ent;
      cache_ent = holy_malloc (sizeof (*cache_ent));
      if (cache_ent)
	{
	  cache_ent->key = holy_strdup (ctx->key);
	  cache_ent->value = holy_strdup (name);
	  if (cache_ent->value && cache_ent->key)
	    {
	      cache_ent->next = cache;
	      cache = cache_ent;
	    }
	  else
	    {
	      holy_free (cache_ent->value);
	      holy_free (cache_ent->key);
	      holy_free (cache_ent);
	      holy_errno = holy_ERR_NONE;
	    }
	}
      else
	holy_errno = holy_ERR_NONE;
    }

  if (found)
    {
      ctx->count++;
      if (ctx->var)
	holy_env_set (ctx->var, name);
      else
	holy_printf (" %s", name);
    }

  holy_errno = holy_ERR_NONE;
  return (found && ctx->var);
}

/* Helper for FUNC_NAME.  */
static int
part_hook (holy_disk_t disk, const holy_partition_t partition, void *data)
{
  struct search_ctx *ctx = data;
  char *partition_name, *devname;
  int ret;

  partition_name = holy_partition_get_name (partition);
  if (! partition_name)
    return 1;

  devname = holy_xasprintf ("%s,%s", disk->name, partition_name);
  holy_free (partition_name);
  if (!devname)
    return 1;
  ret = iterate_device (devname, ctx);
  holy_free (devname);

  return ret;
}

/* Helper for FUNC_NAME.  */
static void
try (struct search_ctx *ctx)    
{
  unsigned i;
  struct cache_entry **prev;
  struct cache_entry *cache_ent;

  for (prev = &cache, cache_ent = *prev; cache_ent;
       prev = &cache_ent->next, cache_ent = *prev)
    if (compare_fn (cache_ent->key, ctx->key) == 0)
      break;
  if (cache_ent)
    {
      ctx->is_cache = 1;
      if (iterate_device (cache_ent->value, ctx))
	{
	  ctx->is_cache = 0;
	  return;
	}
      ctx->is_cache = 0;
      /* Cache entry was outdated. Remove it.  */
      if (!ctx->count)
	{
	  *prev = cache_ent->next;
	  holy_free (cache_ent->key);
	  holy_free (cache_ent->value);
	  holy_free (cache_ent);
	}
    }

  for (i = 0; i < ctx->nhints; i++)
    {
      char *end;
      if (!ctx->hints[i][0])
	continue;
      end = ctx->hints[i] + holy_strlen (ctx->hints[i]) - 1;
      if (*end == ',')
	*end = 0;
      if (iterate_device (ctx->hints[i], ctx))
	{
	  if (!*end)
	    *end = ',';
	  return;
	}
      if (!*end)
	{
	  holy_device_t dev;
	  int ret;
	  dev = holy_device_open (ctx->hints[i]);
	  if (!dev)
	    {
	      if (!*end)
		*end = ',';
	      continue;
	    }
	  if (!dev->disk)
	    {
	      holy_device_close (dev);
	      if (!*end)
		*end = ',';
	      continue;
	    }
	  ret = holy_partition_iterate (dev->disk, part_hook, ctx);
	  if (!*end)
	    *end = ',';
	  holy_device_close (dev);
	  if (ret)
	    return;
	}
    }
  holy_device_iterate (iterate_device, ctx);
}

void
FUNC_NAME (const char *key, const char *var, int no_floppy,
	   char **hints, unsigned nhints)
{
  struct search_ctx ctx = {
    .key = key,
    .var = var,
    .no_floppy = no_floppy,
    .hints = hints,
    .nhints = nhints,
    .count = 0,
    .is_cache = 0
  };
  holy_fs_autoload_hook_t saved_autoload;

  /* First try without autoloading if we're setting variable. */
  if (var)
    {
      saved_autoload = holy_fs_autoload_hook;
      holy_fs_autoload_hook = 0;
      try (&ctx);

      /* Restore autoload hook.  */
      holy_fs_autoload_hook = saved_autoload;

      /* Retry with autoload if nothing found.  */
      if (holy_errno == holy_ERR_NONE && ctx.count == 0)
	try (&ctx);
    }
  else
    try (&ctx);

  if (holy_errno == holy_ERR_NONE && ctx.count == 0)
    holy_error (holy_ERR_FILE_NOT_FOUND, "no such device: %s", key);
}

static holy_err_t
holy_cmd_do_search (holy_command_t cmd __attribute__ ((unused)), int argc,
		    char **args)
{
  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  FUNC_NAME (args[0], argc == 1 ? 0 : args[1], 0, (args + 2),
	     argc > 2 ? argc - 2 : 0);

  return holy_errno;
}

static holy_command_t cmd;

#ifdef DO_SEARCH_FILE
holy_MOD_INIT(search_fs_file)
#elif defined(DO_SEARCH_PART_UUID)
holy_MOD_INIT(search_part_uuid)
#elif defined(DO_SEARCH_PART_LABEL)
holy_MOD_INIT(search_part_label)
#elif defined (DO_SEARCH_FS_UUID)
holy_MOD_INIT(search_fs_uuid)
#elif defined (DO_SEARCH_DISK_UUID)
holy_MOD_INIT(search_disk_uuid)
#else
holy_MOD_INIT(search_label)
#endif
{
  cmd =
    holy_register_command (COMMAND_NAME, holy_cmd_do_search,
			   N_("NAME [VARIABLE] [HINTS]"),
			   HELP_MESSAGE);
}

#ifdef DO_SEARCH_FILE
holy_MOD_FINI(search_fs_file)
#elif defined(DO_SEARCH_PART_UUID)
holy_MOD_FINI(search_part_uuid)
#elif defined(DO_SEARCH_PART_LABEL)
holy_MOD_FINI(search_part_label)
#elif defined (DO_SEARCH_FS_UUID)
holy_MOD_FINI(search_fs_uuid)
#elif defined (DO_SEARCH_DISK_UUID)
holy_MOD_FINI(search_disk_uuid)
#else
holy_MOD_FINI(search_label)
#endif
{
  holy_unregister_command (cmd);
}
