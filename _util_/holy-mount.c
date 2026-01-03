/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define FUSE_USE_VERSION 26
#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/misc.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/env.h>
#include <holy/term.h>
#include <holy/mm.h>
#include <holy/lib/hexdump.h>
#include <holy/crypto.h>
#include <holy/command.h>
#include <holy/zfs/zfs.h>
#include <holy/i18n.h>
#include <fuse/fuse.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include <argp.h>
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

#include "progname.h"

static const char *root = NULL;
holy_device_t dev = NULL;
holy_fs_t fs = NULL;
static char **images = NULL;
static char *debug_str = NULL;
static char **fuse_args = NULL;
static int fuse_argc = 0;
static int num_disks = 0;
static int mount_crypt = 0;

static holy_err_t
execute_command (const char *name, int n, char **args)
{
  holy_command_t cmd;

  cmd = holy_command_find (name);
  if (! cmd)
    holy_util_error (_("can't find command `%s'"), name);

  return (cmd->func) (cmd, n, args);
}

/* Translate holy error numbers into OS error numbers.  Print any unexpected
   errors.  */
static int
translate_error (void)
{
  int ret;

  switch (holy_errno)
    {
      case holy_ERR_NONE:
	ret = 0;
	break;

      case holy_ERR_OUT_OF_MEMORY:
	holy_print_error ();
	ret = -ENOMEM;
	break;

      case holy_ERR_BAD_FILE_TYPE:
	/* This could also be EISDIR.  Take a guess.  */
	ret = -ENOTDIR;
	break;

      case holy_ERR_FILE_NOT_FOUND:
	ret = -ENOENT;
	break;

      case holy_ERR_FILE_READ_ERROR:
      case holy_ERR_READ_ERROR:
      case holy_ERR_IO:
	holy_print_error ();
	ret = -EIO;
	break;

      case holy_ERR_SYMLINK_LOOP:
	ret = -ELOOP;
	break;

      default:
	holy_print_error ();
	ret = -EINVAL;
	break;
    }

  /* Any previous errors were handled.  */
  holy_errno = holy_ERR_NONE;

  return ret;
}

/* Context for fuse_getattr.  */
struct fuse_getattr_ctx
{
  char *filename;
  struct holy_dirhook_info file_info;
  int file_exists;
};

/* A hook for iterating directories. */
static int
fuse_getattr_find_file (const char *cur_filename,
			const struct holy_dirhook_info *info, void *data)
{
  struct fuse_getattr_ctx *ctx = data;

  if ((info->case_insensitive ? holy_strcasecmp (cur_filename, ctx->filename)
       : holy_strcmp (cur_filename, ctx->filename)) == 0)
    {
      ctx->file_info = *info;
      ctx->file_exists = 1;
      return 1;
    }
  return 0;
}

static int
fuse_getattr (const char *path, struct stat *st)
{
  struct fuse_getattr_ctx ctx;
  char *pathname, *path2;
  
  if (path[0] == '/' && path[1] == 0)
    {
      st->st_dev = 0;
      st->st_ino = 0;
      st->st_mode = 0555 | S_IFDIR;
      st->st_uid = 0;
      st->st_gid = 0;
      st->st_rdev = 0;
      st->st_size = 0;
      st->st_blksize = 512;
      st->st_blocks = (st->st_blksize + 511) >> 9;
      st->st_atime = st->st_mtime = st->st_ctime = 0;
      return 0;
    }

  ctx.file_exists = 0;

  pathname = xstrdup (path);
  
  /* Remove trailing '/'. */
  while (*pathname && pathname[holy_strlen (pathname) - 1] == '/')
    pathname[holy_strlen (pathname) - 1] = 0;

  /* Split into path and filename. */
  ctx.filename = holy_strrchr (pathname, '/');
  if (! ctx.filename)
    {
      path2 = holy_strdup ("/");
      ctx.filename = pathname;
    }
  else
    {
      ctx.filename++;
      path2 = holy_strdup (pathname);
      path2[ctx.filename - pathname] = 0;
    }

  /* It's the whole device. */
  (fs->dir) (dev, path2, fuse_getattr_find_file, &ctx);

  holy_free (path2);
  if (!ctx.file_exists)
    {
      holy_errno = holy_ERR_NONE;
      return -ENOENT;
    }
  st->st_dev = 0;
  st->st_ino = 0;
  st->st_mode = ctx.file_info.dir ? (0555 | S_IFDIR) : (0444 | S_IFREG);
  st->st_uid = 0;
  st->st_gid = 0;
  st->st_rdev = 0;
  st->st_size = 0;
  if (!ctx.file_info.dir)
    {
      holy_file_t file;
      file = holy_file_open (path);
      if (! file && holy_errno == holy_ERR_BAD_FILE_TYPE)
	{
	  holy_errno = holy_ERR_NONE;
	  st->st_mode = (0555 | S_IFDIR);
	}
      else if (! file)
	return translate_error ();
      else
	{
	  st->st_size = file->size;
	  holy_file_close (file);
	}
    }
  st->st_blksize = 512;
  st->st_blocks = (st->st_size + 511) >> 9;
  st->st_atime = st->st_mtime = st->st_ctime = ctx.file_info.mtimeset
    ? ctx.file_info.mtime : 0;
  holy_errno = holy_ERR_NONE;
  return 0;
}

static int
fuse_opendir (const char *path, struct fuse_file_info *fi) 
{
  return 0;
}

/* FIXME */
static holy_file_t files[65536];
static int first_fd = 1;

static int 
fuse_open (const char *path, struct fuse_file_info *fi __attribute__ ((unused)))
{
  holy_file_t file;
  file = holy_file_open (path);
  if (! file)
    return translate_error ();
  files[first_fd++] = file;
  fi->fh = first_fd;
  files[first_fd++] = file;
  holy_errno = holy_ERR_NONE;
  return 0;
} 

static int 
fuse_read (const char *path, char *buf, size_t sz, off_t off,
	   struct fuse_file_info *fi)
{
  holy_file_t file = files[fi->fh];
  holy_ssize_t size;

  if (off > file->size)
    return -EINVAL;

  file->offset = off;
  
  size = holy_file_read (file, buf, sz);
  if (size < 0)
    return translate_error ();
  else
    {
      holy_errno = holy_ERR_NONE;
      return size;
    }
} 

static int 
fuse_release (const char *path, struct fuse_file_info *fi)
{
  holy_file_close (files[fi->fh]);
  files[fi->fh] = NULL;
  holy_errno = holy_ERR_NONE;
  return 0;
}

/* Context for fuse_readdir.  */
struct fuse_readdir_ctx
{
  const char *path;
  void *buf;
  fuse_fill_dir_t fill;
};

/* Helper for fuse_readdir.  */
static int
fuse_readdir_call_fill (const char *filename,
			const struct holy_dirhook_info *info, void *data)
{
  struct fuse_readdir_ctx *ctx = data;
  struct stat st;

  holy_memset (&st, 0, sizeof (st));
  st.st_mode = info->dir ? (0555 | S_IFDIR) : (0444 | S_IFREG);
  if (!info->dir)
    {
      holy_file_t file;
      char *tmp;
      tmp = xasprintf ("%s/%s", ctx->path, filename);
      file = holy_file_open (tmp);
      free (tmp);
      /* Symlink to directory.  */
      if (! file && holy_errno == holy_ERR_BAD_FILE_TYPE)
	{
	  holy_errno = holy_ERR_NONE;
	  st.st_mode = (0555 | S_IFDIR);
	}
      else if (!file)
	{
	  holy_errno = holy_ERR_NONE;
	}
      else
	{
	  st.st_size = file->size;
	  holy_file_close (file);
	}
    }
  st.st_blksize = 512;
  st.st_blocks = (st.st_size + 511) >> 9;
  st.st_atime = st.st_mtime = st.st_ctime
    = info->mtimeset ? info->mtime : 0;
  ctx->fill (ctx->buf, filename, &st, 0);
  return 0;
}

static int 
fuse_readdir (const char *path, void *buf,
	      fuse_fill_dir_t fill, off_t off, struct fuse_file_info *fi)
{
  struct fuse_readdir_ctx ctx = {
    .path = path,
    .buf = buf,
    .fill = fill
  };
  char *pathname;

  pathname = xstrdup (path);
  
  /* Remove trailing '/'. */
  while (pathname [0] && pathname[1]
	 && pathname[holy_strlen (pathname) - 1] == '/')
    pathname[holy_strlen (pathname) - 1] = 0;

  (fs->dir) (dev, pathname, fuse_readdir_call_fill, &ctx);
  free (pathname);
  holy_errno = holy_ERR_NONE;
  return 0;
}

struct fuse_operations holy_opers = {
  .getattr = fuse_getattr,
  .open = fuse_open,
  .release = fuse_release,
  .opendir = fuse_opendir,
  .readdir = fuse_readdir,
  .read = fuse_read
};

static holy_err_t
fuse_init (void)
{
  int i;

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];
      char *host_file;
      char *loop_name;
      loop_name = holy_xasprintf ("loop%d", i);
      if (!loop_name)
	holy_util_error ("%s", holy_errmsg);

      host_file = holy_xasprintf ("(host)%s", images[i]);
      if (!host_file)
	holy_util_error ("%s", holy_errmsg);

      argv[0] = loop_name;
      argv[1] = host_file;

      if (execute_command ("loopback", 2, argv))
        holy_util_error (_("`loopback' command fails: %s"), holy_errmsg);

      holy_free (loop_name);
      holy_free (host_file);
    }

  if (mount_crypt)
    {
      char *argv[2] = { xstrdup ("-a"), NULL};
      if (execute_command ("cryptomount", 1, argv))
	  holy_util_error (_("`cryptomount' command fails: %s"),
			   holy_errmsg);
      free (argv[0]);
    }

  holy_lvm_fini ();
  holy_mdraid09_fini ();
  holy_mdraid1x_fini ();
  holy_diskfilter_fini ();
  holy_diskfilter_init ();
  holy_mdraid09_init ();
  holy_mdraid1x_init ();
  holy_lvm_init ();

  dev = holy_device_open (0);
  if (! dev)
    return holy_errno;

  fs = holy_fs_probe (dev);
  if (! fs)
    {
      holy_device_close (dev);
      return holy_errno;
    }

  if (fuse_main (fuse_argc, fuse_args, &holy_opers, NULL))
    holy_error (holy_ERR_IO, "fuse_main failed");

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];
      char *loop_name;

      loop_name = holy_xasprintf ("loop%d", i);
      if (!loop_name)
	holy_util_error ("%s", holy_errmsg);

      argv[0] = xstrdup ("-d");
      argv[1] = loop_name;

      execute_command ("loopback", 2, argv);

      holy_free (argv[0]);
      holy_free (loop_name);
    }

  return holy_errno;
}

static struct argp_option options[] = {  
  {"root",      'r', N_("DEVICE_NAME"), 0, N_("Set root device."),                 2},
  {"debug",     'd', N_("STRING"),           0, N_("Set debug environment variable."),  2},
  {"crypto",   'C', NULL, 0, N_("Mount crypto devices."), 2},
  {"zfs-key",      'K',
   /* TRANSLATORS: "prompt" is a keyword.  */
   N_("FILE|prompt"), 0, N_("Load zfs crypto key."),                 2},
  {"verbose",   'v', NULL, 0, N_("print verbose messages."), 2},
  {0, 0, 0, 0, 0, 0}
};

/* Print the version information.  */
static void
print_version (FILE *stream, struct argp_state *state)
{
  fprintf (stream, "%s (%s) %s\n", program_name, PACKAGE_NAME, PACKAGE_VERSION);
}
void (*argp_program_version_hook) (FILE *, struct argp_state *) = print_version;

static error_t 
argp_parser (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'r':
      root = arg;
      return 0;

    case 'K':
      if (strcmp (arg, "prompt") == 0)
	{
	  char buf[1024];	  
	  holy_printf ("%s", _("Enter ZFS password: "));
	  if (holy_password_get (buf, 1023))
	    {
	      holy_zfs_add_key ((holy_uint8_t *) buf, holy_strlen (buf), 1);
	    }
	}
      else
	{
	  FILE *f;
	  ssize_t real_size;
	  holy_uint8_t buf[1024];
	  f = holy_util_fopen (arg, "rb");
	  if (!f)
	    {
	      printf (_("%s: error:"), program_name);
	      printf (_("cannot open `%s': %s"), arg, strerror (errno));
	      printf ("\n");
	      return 0;
	    }
	  real_size = fread (buf, 1, 1024, f);
	  if (real_size < 0)
	    {
	      printf (_("%s: error:"), program_name);
	      printf (_("cannot read `%s': %s"), arg,
		      strerror (errno));
	      printf ("\n");
	      fclose (f);
	      return 0;
	    }
	  holy_zfs_add_key (buf, real_size, 0);
	  fclose (f);
	}
      return 0;

    case 'C':
      mount_crypt = 1;
      return 0;

    case 'd':
      debug_str = arg;
      return 0;

    case 'v':
      verbosity++;
      return 0;

    case ARGP_KEY_ARG:
      if (arg[0] != '-')
	break;

    /* FALLTHROUGH */
    default:
      if (!arg)
	return 0;

      fuse_args = xrealloc (fuse_args, (fuse_argc + 1) * sizeof (fuse_args[0]));
      fuse_args[fuse_argc] = xstrdup (arg);
      fuse_argc++;
      return 0;
    }

  images = xrealloc (images, (num_disks + 1) * sizeof (images[0]));
  images[num_disks] = holy_canonicalize_file_name (arg);
  num_disks++;

  return 0;
}

struct argp argp = {
  options, argp_parser, N_("IMAGE1 [IMAGE2 ...] MOUNTPOINT"),
  N_("Debug tool for filesystem driver."), 
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  const char *default_root;
  char *alloc_root;

  holy_util_host_init (&argc, &argv);

  fuse_args = xrealloc (fuse_args, (fuse_argc + 2) * sizeof (fuse_args[0]));
  fuse_args[fuse_argc] = xstrdup (argv[0]);
  fuse_argc++;
  /* Run single-threaded.  */
  fuse_args[fuse_argc] = xstrdup ("-s");
  fuse_argc++;

  argp_parse (&argp, argc, argv, 0, 0, 0);
  
  if (num_disks < 2)
    holy_util_error ("%s", _("need an image and mountpoint"));
  fuse_args = xrealloc (fuse_args, (fuse_argc + 2) * sizeof (fuse_args[0]));
  fuse_args[fuse_argc] = images[num_disks - 1];
  fuse_argc++;
  num_disks--;
  fuse_args[fuse_argc] = NULL;

  /* Initialize all modules. */
  holy_init_all ();

  if (debug_str)
    holy_env_set ("debug", debug_str);

  default_root = (num_disks == 1) ? "loop0" : "md0";
  alloc_root = 0;
  if (root)
    {
      if ((*root >= '0') && (*root <= '9'))
        {
          alloc_root = xmalloc (strlen (default_root) + strlen (root) + 2);

          sprintf (alloc_root, "%s,%s", default_root, root);
          root = alloc_root;
        }
    }
  else
    root = default_root;

  holy_env_set ("root", root);

  if (alloc_root)
    free (alloc_root);

  /* Do it.  */
  fuse_init ();
  if (holy_errno)
    {
      holy_print_error ();
      return 1;
    }

  /* Free resources.  */
  holy_fini_all ();

  return 0;
}
