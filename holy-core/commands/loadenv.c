/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/partition.h>
#include <holy/lib/envblk.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] =
  {
    /* TRANSLATORS: This option is used to override default filename
       for loading and storing environment.  */
    {"file", 'f', 0, N_("Specify filename."), 0, ARG_TYPE_PATHNAME},
    {"skip-sig", 's', 0,
     N_("Skip signature-checking of the environment file."), 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

/* Opens 'filename' with compression filters disabled. Optionally disables the
   PUBKEY filter (that insists upon properly signed files) as well.  PUBKEY
   filter is restored before the function returns. */
static holy_file_t
open_envblk_file (char *filename, int untrusted)
{
  holy_file_t file;
  char *buf = 0;

  if (! filename)
    {
      const char *prefix;
      int len;

      prefix = holy_env_get ("prefix");
      if (! prefix)
        {
          holy_error (holy_ERR_FILE_NOT_FOUND, N_("variable `%s' isn't set"), "prefix");
          return 0;
        }

      len = holy_strlen (prefix);
      buf = holy_malloc (len + 1 + sizeof (holy_ENVBLK_DEFCFG));
      if (! buf)
        return 0;
      filename = buf;

      holy_strcpy (filename, prefix);
      filename[len] = '/';
      holy_strcpy (filename + len + 1, holy_ENVBLK_DEFCFG);
    }

  /* The filters that are disabled will be re-enabled by the call to
     holy_file_open() after this particular file is opened. */
  holy_file_filter_disable_compression ();
  if (untrusted)
    holy_file_filter_disable_pubkey ();

  file = holy_file_open (filename);

  holy_free (buf);
  return file;
}

static holy_envblk_t
read_envblk_file (holy_file_t file)
{
  holy_off_t offset = 0;
  char *buf;
  holy_size_t size = holy_file_size (file);
  holy_envblk_t envblk;

  buf = holy_malloc (size);
  if (! buf)
    return 0;

  while (size > 0)
    {
      holy_ssize_t ret;

      ret = holy_file_read (file, buf + offset, size);
      if (ret <= 0)
        {
          holy_free (buf);
          return 0;
        }

      size -= ret;
      offset += ret;
    }

  envblk = holy_envblk_open (buf, offset);
  if (! envblk)
    {
      holy_free (buf);
      holy_error (holy_ERR_BAD_FILE_TYPE, "invalid environment block");
      return 0;
    }

  return envblk;
}

struct holy_env_whitelist
{
  holy_size_t len;
  char **list;
};
typedef struct holy_env_whitelist holy_env_whitelist_t;

static int
test_whitelist_membership (const char* name,
                           const holy_env_whitelist_t* whitelist)
{
  holy_size_t i;

  for (i = 0; i < whitelist->len; i++)
    if (holy_strcmp (name, whitelist->list[i]) == 0)
      return 1;  /* found it */

  return 0;  /* not found */
}

/* Helper for holy_cmd_load_env.  */
static int
set_var (const char *name, const char *value, void *whitelist)
{
  if (! whitelist)
    {
      holy_env_set (name, value);
      return 0;
    }

  if (test_whitelist_membership (name,
				 (const holy_env_whitelist_t *) whitelist))
    holy_env_set (name, value);

  return 0;
}

static holy_err_t
holy_cmd_load_env (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  holy_file_t file;
  holy_envblk_t envblk;
  holy_env_whitelist_t whitelist;

  whitelist.len = argc;
  whitelist.list = args;

  /* state[0] is the -f flag; state[1] is the --skip-sig flag */
  file = open_envblk_file ((state[0].set) ? state[0].arg : 0, state[1].set);
  if (! file)
    return holy_errno;

  envblk = read_envblk_file (file);
  if (! envblk)
    goto fail;

  /* argc > 0 indicates caller provided a whitelist of variables to read. */
  holy_envblk_iterate (envblk, argc > 0 ? &whitelist : 0, set_var);
  holy_envblk_close (envblk);

 fail:
  holy_file_close (file);
  return holy_errno;
}

/* Print all variables in current context.  */
static int
print_var (const char *name, const char *value,
           void *hook_data __attribute__ ((unused)))
{
  holy_printf ("%s=%s\n", name, value);
  return 0;
}

static holy_err_t
holy_cmd_list_env (holy_extcmd_context_t ctxt,
		   int argc __attribute__ ((unused)),
		   char **args __attribute__ ((unused)))
{
  struct holy_arg_list *state = ctxt->state;
  holy_file_t file;
  holy_envblk_t envblk;

  file = open_envblk_file ((state[0].set) ? state[0].arg : 0, 0);
  if (! file)
    return holy_errno;

  envblk = read_envblk_file (file);
  if (! envblk)
    goto fail;

  holy_envblk_iterate (envblk, NULL, print_var);
  holy_envblk_close (envblk);

 fail:
  holy_file_close (file);
  return holy_errno;
}

/* Used to maintain a variable length of blocklists internally.  */
struct blocklist
{
  holy_disk_addr_t sector;
  unsigned offset;
  unsigned length;
  struct blocklist *next;
};

static void
free_blocklists (struct blocklist *p)
{
  struct blocklist *q;

  for (; p; p = q)
    {
      q = p->next;
      holy_free (p);
    }
}

static holy_err_t
check_blocklists (holy_envblk_t envblk, struct blocklist *blocklists,
                  holy_file_t file)
{
  holy_size_t total_length;
  holy_size_t index;
  holy_disk_t disk;
  holy_disk_addr_t part_start;
  struct blocklist *p;
  char *buf;

  /* Sanity checks.  */
  total_length = 0;
  for (p = blocklists; p; p = p->next)
    {
      struct blocklist *q;
      /* Check if any pair of blocks overlap.  */
      for (q = p->next; q; q = q->next)
        {
	  holy_disk_addr_t s1, s2;
	  holy_disk_addr_t e1, e2;

	  s1 = p->sector;
	  e1 = s1 + ((p->length + holy_DISK_SECTOR_SIZE - 1) >> holy_DISK_SECTOR_BITS);

	  s2 = q->sector;
	  e2 = s2 + ((q->length + holy_DISK_SECTOR_SIZE - 1) >> holy_DISK_SECTOR_BITS);

	  if (s1 < e2 && s2 < e1)
            {
              /* This might be actually valid, but it is unbelievable that
                 any filesystem makes such a silly allocation.  */
              return holy_error (holy_ERR_BAD_FS, "malformed file");
            }
        }

      total_length += p->length;
    }

  if (total_length != holy_file_size (file))
    {
      /* Maybe sparse, unallocated sectors. No way in holy.  */
      return holy_error (holy_ERR_BAD_FILE_TYPE, "sparse file not allowed");
    }

  /* One more sanity check. Re-read all sectors by blocklists, and compare
     those with the data read via a file.  */
  disk = file->device->disk;

  part_start = holy_partition_get_start (disk->partition);

  buf = holy_envblk_buffer (envblk);
  char *blockbuf = NULL;
  holy_size_t blockbuf_len = 0;
  for (p = blocklists, index = 0; p; index += p->length, p = p->next)
    {
      if (p->length > blockbuf_len)
	{
	  holy_free (blockbuf);
	  blockbuf_len = 2 * p->length;
	  blockbuf = holy_malloc (blockbuf_len);
	  if (!blockbuf)
	    return holy_errno;
	}

      if (holy_disk_read (disk, p->sector - part_start,
                          p->offset, p->length, blockbuf))
        return holy_errno;

      if (holy_memcmp (buf + index, blockbuf, p->length) != 0)
	return holy_error (holy_ERR_FILE_READ_ERROR, "invalid blocklist");
    }

  return holy_ERR_NONE;
}

static int
write_blocklists (holy_envblk_t envblk, struct blocklist *blocklists,
                  holy_file_t file)
{
  char *buf;
  holy_disk_t disk;
  holy_disk_addr_t part_start;
  struct blocklist *p;
  holy_size_t index;

  buf = holy_envblk_buffer (envblk);
  disk = file->device->disk;
  part_start = holy_partition_get_start (disk->partition);

  index = 0;
  for (p = blocklists; p; index += p->length, p = p->next)
    {
      if (holy_disk_write (disk, p->sector - part_start,
                           p->offset, p->length, buf + index))
        return 0;
    }

  return 1;
}

/* Context for holy_cmd_save_env.  */
struct holy_cmd_save_env_ctx
{
  struct blocklist *head, *tail;
};

/* Store blocklists in a linked list.  */
static void
save_env_read_hook (holy_disk_addr_t sector, unsigned offset, unsigned length,
		    void *data)
{
  struct holy_cmd_save_env_ctx *ctx = data;
  struct blocklist *block;

  block = holy_malloc (sizeof (*block));
  if (! block)
    return;

  block->sector = sector;
  block->offset = offset;
  block->length = length;

  /* Slightly complicated, because the list should be FIFO.  */
  block->next = 0;
  if (ctx->tail)
    ctx->tail->next = block;
  ctx->tail = block;
  if (! ctx->head)
    ctx->head = block;
}

static holy_err_t
holy_cmd_save_env (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  holy_file_t file;
  holy_envblk_t envblk;
  struct holy_cmd_save_env_ctx ctx = {
    .head = 0,
    .tail = 0
  };

  if (! argc)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no variable is specified");

  file = open_envblk_file ((state[0].set) ? state[0].arg : 0,
                           1 /* allow untrusted */);
  if (! file)
    return holy_errno;

  if (! file->device->disk)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_DEVICE, "disk device required");
    }

  file->read_hook = save_env_read_hook;
  file->read_hook_data = &ctx;
  envblk = read_envblk_file (file);
  file->read_hook = 0;
  if (! envblk)
    goto fail;

  if (check_blocklists (envblk, ctx.head, file))
    goto fail;

  while (argc)
    {
      const char *value;

      value = holy_env_get (args[0]);
      if (value)
        {
          if (! holy_envblk_set (envblk, args[0], value))
            {
              holy_error (holy_ERR_BAD_ARGUMENT, "environment block too small");
              goto fail;
            }
        }
      else
	holy_envblk_delete (envblk, args[0]);

      argc--;
      args++;
    }

  write_blocklists (envblk, ctx.head, file);

 fail:
  if (envblk)
    holy_envblk_close (envblk);
  free_blocklists (ctx.head);
  holy_file_close (file);
  return holy_errno;
}

static holy_extcmd_t cmd_load, cmd_list, cmd_save;

holy_MOD_INIT(loadenv)
{
  cmd_load =
    holy_register_extcmd ("load_env", holy_cmd_load_env, 0,
			  N_("[-f FILE] [-s|--skip-sig] [variable_name_to_whitelist] [...]"),
			  N_("Load variables from environment block file."),
			  options);
  cmd_list =
    holy_register_extcmd ("list_env", holy_cmd_list_env, 0, N_("[-f FILE]"),
			  N_("List variables from environment block file."),
			  options);
  cmd_save =
    holy_register_extcmd ("save_env", holy_cmd_save_env, 0,
			  N_("[-f FILE] variable_name [...]"),
			  N_("Save variables to environment block file."),
			  options);
}

holy_MOD_FINI(loadenv)
{
  holy_unregister_extcmd (cmd_load);
  holy_unregister_extcmd (cmd_list);
  holy_unregister_extcmd (cmd_save);
}
