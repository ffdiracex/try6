/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

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
#include <holy/i18n.h>
#include <holy/zfs/zfs.h>
#include <holy/emu/hostfile.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "progname.h"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#include "argp.h"
#pragma GCC diagnostic error "-Wmissing-prototypes"
#pragma GCC diagnostic error "-Wmissing-declarations"

static holy_err_t
execute_command (const char *name, int n, char **args)
{
  holy_command_t cmd;

  cmd = holy_command_find (name);
  if (! cmd)
    holy_util_error (_("can't find command `%s'"), name);

  return (cmd->func) (cmd, n, args);
}

enum {
  CMD_LS = 1,
  CMD_CP,
  CMD_CAT,
  CMD_CMP,
  CMD_HEX,
  CMD_CRC,
  CMD_BLOCKLIST,
  CMD_TESTLOAD,
  CMD_ZFSINFO,
  CMD_XNU_UUID
};
#define BUF_SIZE  32256

static holy_disk_addr_t skip, leng;
static int uncompress = 0;

static void
read_file (char *pathname, int (*hook) (holy_off_t ofs, char *buf, int len, void *hook_arg), void *hook_arg)
{
  static char buf[BUF_SIZE];
  holy_file_t file;

  if ((pathname[0] == '-') && (pathname[1] == 0))
    {
      holy_device_t dev;

      dev = holy_device_open (0);
      if ((! dev) || (! dev->disk))
        holy_util_error ("%s", holy_errmsg);

      holy_util_info ("total sectors : %" holy_HOST_PRIuLONG_LONG,
                      (unsigned long long) dev->disk->total_sectors);

      if (! leng)
        leng = (dev->disk->total_sectors << holy_DISK_SECTOR_BITS) - skip;

      while (leng)
        {
          holy_size_t len;

          len = (leng > BUF_SIZE) ? BUF_SIZE : leng;

          if (holy_disk_read (dev->disk, 0, skip, len, buf))
	    {
	      char *msg = holy_xasprintf (_("disk read fails at offset %lld, length %lld"),
					  (long long) skip, (long long) len);
	      holy_util_error ("%s", msg);
	    }

          if (hook (skip, buf, len, hook_arg))
            break;

          skip += len;
          leng -= len;
        }

      holy_device_close (dev);
      return;
    }

  if (uncompress == 0)
    holy_file_filter_disable_compression ();
  file = holy_file_open (pathname);
  if (!file)
    {
      holy_util_error (_("cannot open `%s': %s"), pathname,
		       holy_errmsg);
      return;
    }

  holy_util_info ("file size : %" holy_HOST_PRIuLONG_LONG,
		  (unsigned long long) file->size);

  if (skip > file->size)
    {
      char *msg = holy_xasprintf (_("invalid skip value %lld"),
				  (unsigned long long) skip);
      holy_util_error ("%s", msg);
      return;
    }

  {
    holy_off_t ofs, len;
    ofs = skip;
    len = file->size - skip;
    if ((leng) && (leng < len))
      len = leng;

    file->offset = skip;

    while (len)
      {
	holy_ssize_t sz;

	sz = holy_file_read (file, buf, (len > BUF_SIZE) ? BUF_SIZE : len);
	if (sz < 0)
	  {
	    char *msg = holy_xasprintf (_("read error at offset %llu: %s"),
					(unsigned long long) ofs, holy_errmsg);
	    holy_util_error ("%s", msg);
	    break;
	  }

	if ((sz == 0) || (hook (ofs, buf, sz, hook_arg)))
	  break;

	ofs += sz;
	len -= sz;
      }
  }

  holy_file_close (file);
}

struct cp_hook_ctx
{
  FILE *ff;
  const char *dest;
};

static int
cp_hook (holy_off_t ofs, char *buf, int len, void *_ctx)
{
  struct cp_hook_ctx *ctx = _ctx;
  (void) ofs;

  if ((int) fwrite (buf, 1, len, ctx->ff) != len)
    {
      holy_util_error (_("cannot write to `%s': %s"),
		       ctx->dest, strerror (errno));
      return 1;
    }

  return 0;
}

static void
cmd_cp (char *src, const char *dest)
{
  struct cp_hook_ctx ctx = 
    {
      .dest = dest
    };

  ctx.ff = holy_util_fopen (dest, "wb");
  if (ctx.ff == NULL)
    {
      holy_util_error (_("cannot open OS file `%s': %s"), dest,
		       strerror (errno));
      return;
    }
  read_file (src, cp_hook, &ctx);
  fclose (ctx.ff);
}

static int
cat_hook (holy_off_t ofs, char *buf, int len, void *_arg __attribute__ ((unused)))
{
  (void) ofs;

  if ((int) fwrite (buf, 1, len, stdout) != len)
    {
      holy_util_error (_("cannot write to the stdout: %s"),
		       strerror (errno));
      return 1;
    }

  return 0;
}

static void
cmd_cat (char *src)
{
  read_file (src, cat_hook, 0);
}

static int
cmp_hook (holy_off_t ofs, char *buf, int len, void *ff_in)
{
  FILE *ff = ff_in;
  static char buf_1[BUF_SIZE];
  if ((int) fread (buf_1, 1, len, ff) != len)
    {
      char *msg = holy_xasprintf (_("read error at offset %llu: %s"),
				  (unsigned long long) ofs, holy_errmsg);
      holy_util_error ("%s", msg);
      return 1;
    }

  if (holy_memcmp (buf, buf_1, len) != 0)
    {
      int i;

      for (i = 0; i < len; i++, ofs++)
	if (buf_1[i] != buf[i])
	  {
	    char *msg = holy_xasprintf (_("compare fail at offset %llu"),
					(unsigned long long) ofs);
	    holy_util_error ("%s", msg);
	    return 1;
	  }
    }
  return 0;
}


static void
cmd_cmp (char *src, char *dest)
{
  FILE *ff;

  if (holy_util_is_directory (dest))
    {
      holy_util_fd_dir_t dir = holy_util_fd_opendir (dest);
      holy_util_fd_dirent_t entry;
      if (dir == NULL)
	{
	  holy_util_error (_("OS file %s open error: %s"), dest,
			   holy_util_fd_strerror ());
	  return;
	}
      while ((entry = holy_util_fd_readdir (dir)))
	{
	  char *srcnew, *destnew;
	  char *ptr;
	  if (strcmp (entry->d_name, ".") == 0
	      || strcmp (entry->d_name, "..") == 0)
	    continue;
	  srcnew = xmalloc (strlen (src) + sizeof ("/")
			    + strlen (entry->d_name));
	  destnew = xmalloc (strlen (dest) + sizeof ("/")
			    + strlen (entry->d_name));
	  ptr = holy_stpcpy (srcnew, src);
	  *ptr++ = '/';
	  strcpy (ptr, entry->d_name);
	  ptr = holy_stpcpy (destnew, dest);
	  *ptr++ = '/';
	  strcpy (ptr, entry->d_name);

	  if (holy_util_is_special_file (destnew))
	    continue;

	  cmd_cmp (srcnew, destnew);
	}
      holy_util_fd_closedir (dir);
      return;
    }

  ff = holy_util_fopen (dest, "rb");
  if (ff == NULL)
    {
      holy_util_error (_("OS file %s open error: %s"), dest,
		       strerror (errno));
      return;
    }

  if ((skip) && (fseeko (ff, skip, SEEK_SET)))
    holy_util_error (_("cannot seek `%s': %s"), dest,
		     strerror (errno));

  read_file (src, cmp_hook, ff);

  {
    holy_uint64_t pre;
    pre = ftell (ff);
    fseek (ff, 0, SEEK_END);
    if (pre != ftell (ff))
      holy_util_error ("%s", _("unexpected end of file"));
  }
  fclose (ff);
}

static int
hex_hook (holy_off_t ofs, char *buf, int len, void *arg __attribute__ ((unused)))
{
  hexdump (ofs, buf, len);
  return 0;
}

static void
cmd_hex (char *pathname)
{
  read_file (pathname, hex_hook, 0);
}

static int
crc_hook (holy_off_t ofs, char *buf, int len, void *crc_ctx)
{
  (void) ofs;
  
  holy_MD_CRC32->write(crc_ctx, buf, len);
  return 0;
}

static void
cmd_crc (char *pathname)
{
  holy_uint8_t *crc32_context = xmalloc (holy_MD_CRC32->contextsize);
  holy_MD_CRC32->init(crc32_context);

  read_file (pathname, crc_hook, crc32_context);
  holy_MD_CRC32->final(crc32_context);
  printf ("%08x\n",
	  holy_be_to_cpu32 (holy_get_unaligned32 (holy_MD_CRC32->read (crc32_context))));
  free (crc32_context);
}

static const char *root = NULL;
static int args_count = 0;
static int nparm = 0;
static int num_disks = 1;
static char **images = NULL;
static int cmd = 0;
static char *debug_str = NULL;
static char **args = NULL;
static int mount_crypt = 0;

static void
fstest (int n)
{
  char *host_file;
  char *loop_name;
  int i;

  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];
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

  {
    if (mount_crypt)
      {
	char *argv[2] = { xstrdup ("-a"), NULL};
	if (execute_command ("cryptomount", 1, argv))
	  holy_util_error (_("`cryptomount' command fails: %s"),
			   holy_errmsg);
	free (argv[0]);
      }
  }

  holy_ldm_fini ();
  holy_lvm_fini ();
  holy_mdraid09_fini ();
  holy_mdraid1x_fini ();
  holy_diskfilter_fini ();
  holy_diskfilter_init ();
  holy_mdraid09_init ();
  holy_mdraid1x_init ();
  holy_lvm_init ();
  holy_ldm_init ();

  switch (cmd)
    {
    case CMD_LS:
      execute_command ("ls", n, args);
      break;
    case CMD_ZFSINFO:
      execute_command ("zfsinfo", n, args);
      break;
    case CMD_CP:
      cmd_cp (args[0], args[1]);
      break;
    case CMD_CAT:
      cmd_cat (args[0]);
      break;
    case CMD_CMP:
      cmd_cmp (args[0], args[1]);
      break;
    case CMD_HEX:
      cmd_hex (args[0]);
      break;
    case CMD_CRC:
      cmd_crc (args[0]);
      break;
    case CMD_BLOCKLIST:
      execute_command ("blocklist", n, args);
      holy_printf ("\n");
      break;
    case CMD_TESTLOAD:
      execute_command ("testload", n, args);
      holy_printf ("\n");
      break;
    case CMD_XNU_UUID:
      {
	holy_device_t dev;
	holy_fs_t fs;
	char *uuid = 0;
	char *argv[3] = { xstrdup ("-l"), NULL, NULL};
	dev = holy_device_open (n ? args[0] : 0);
	if (!dev)
	  holy_util_error ("%s", holy_errmsg);
	fs = holy_fs_probe (dev);
	if (!fs)
	  holy_util_error ("%s", holy_errmsg);
	if (!fs->uuid)
	  holy_util_error ("%s", _("couldn't retrieve UUID"));
	if (fs->uuid (dev, &uuid))
	  holy_util_error ("%s", holy_errmsg);
	if (!uuid)
	  holy_util_error ("%s", _("couldn't retrieve UUID"));
	argv[1] = uuid;
	execute_command ("xnu_uuid", 2, argv);
	holy_free (argv[0]);
	holy_free (uuid);
	holy_device_close (dev);
      }
    }
    
  for (i = 0; i < num_disks; i++)
    {
      char *argv[2];

      loop_name = holy_xasprintf ("loop%d", i);
      if (!loop_name)
	holy_util_error ("%s", holy_errmsg);

      argv[0] = xstrdup ("-d");
      argv[1] = loop_name;

      execute_command ("loopback", 2, argv);

      holy_free (loop_name);
      holy_free (argv[0]);
    }
}

static struct argp_option options[] = {
  {0,          0, 0      , OPTION_DOC, N_("Commands:"), 1},
  {N_("ls PATH"),  0, 0      , OPTION_DOC, N_("List files in PATH."), 1},
  {N_("cp FILE LOCAL"),  0, 0, OPTION_DOC, N_("Copy FILE to local file LOCAL."), 1},
  {N_("cat FILE"), 0, 0      , OPTION_DOC, N_("Copy FILE to standard output."), 1},
  {N_("cmp FILE LOCAL"), 0, 0, OPTION_DOC, N_("Compare FILE with local file LOCAL."), 1},
  {N_("hex FILE"), 0, 0      , OPTION_DOC, N_("Show contents of FILE in hex."), 1},
  {N_("crc FILE"), 0, 0     , OPTION_DOC, N_("Get crc32 checksum of FILE."), 1},
  {N_("blocklist FILE"), 0, 0, OPTION_DOC, N_("Display blocklist of FILE."), 1},
  {N_("xnu_uuid DEVICE"), 0, 0, OPTION_DOC, N_("Compute XNU UUID of the device."), 1},
  
  {"root",      'r', N_("DEVICE_NAME"), 0, N_("Set root device."),                 2},
  {"skip",      's', N_("NUM"),           0, N_("Skip N bytes from output file."),   2},
  {"length",    'n', N_("NUM"),           0, N_("Handle N bytes in output file."),   2},
  {"diskcount", 'c', N_("NUM"),           0, N_("Specify the number of input files."),                   2},
  {"debug",     'd', N_("STRING"),           0, N_("Set debug environment variable."),  2},
  {"crypto",   'C', NULL, 0, N_("Mount crypto devices."), 2},
  {"zfs-key",      'K',
   /* TRANSLATORS: "prompt" is a keyword.  */
   N_("FILE|prompt"), 0, N_("Load zfs crypto key."),                 2},
  {"verbose",   'v', NULL, 0, N_("print verbose messages."), 2},
  {"uncompress", 'u', NULL, 0, N_("Uncompress data."), 2},
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
  char *p;

  switch (key)
    {
    case 'r':
      root = arg;
      return 0;

    case 'K':
      if (strcmp (arg, "prompt") == 0)
	{
	  char buf[1024];	  
	  holy_puts_ (N_("Enter ZFS password: "));
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
	fclose (f);
	if (real_size < 0)
	  {
	    printf (_("%s: error:"), program_name);
	    printf (_("cannot read `%s': %s"), arg, strerror (errno));
	    printf ("\n");
	    return 0;
	  }
	holy_zfs_add_key (buf, real_size, 0);
      }
      return 0;

    case 'C':
      mount_crypt = 1;
      return 0;

    case 's':
      skip = holy_strtoul (arg, &p, 0);
      if (*p == 's')
	skip <<= holy_DISK_SECTOR_BITS;
      return 0;

    case 'n':
      leng = holy_strtoul (arg, &p, 0);
      if (*p == 's')
	leng <<= holy_DISK_SECTOR_BITS;
      return 0;

    case 'c':
      num_disks = holy_strtoul (arg, NULL, 0);
      if (num_disks < 1)
	{
	  fprintf (stderr, "%s", _("Invalid disk count.\n"));
	  argp_usage (state);
	}
      if (args_count != 0)
	{
	  /* TRANSLATORS: disk count is optional but if it's there it must
	     be before disk list. So please don't imply disk count as mandatory.
	   */
	  fprintf (stderr, "%s", _("Disk count must precede disks list.\n"));
	  argp_usage (state);
	}
      return 0;

    case 'd':
      debug_str = arg;
      return 0;

    case 'v':
      verbosity++;
      return 0;

    case 'u':
      uncompress = 1;
      return 0;

    case ARGP_KEY_END:
      if (args_count < num_disks)
	{
	  fprintf (stderr, "%s", _("No command is specified.\n"));
	  argp_usage (state);
	}
      if (args_count - 1 - num_disks < nparm)
	{
	  fprintf (stderr, "%s", _("Not enough parameters to command.\n"));
	  argp_usage (state);
	}
      return 0;

    case ARGP_KEY_ARG:
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  if (args_count < num_disks)
    {
      if (args_count == 0)
	images = xmalloc (num_disks * sizeof (images[0]));
      images[args_count] = holy_canonicalize_file_name (arg);
      args_count++;
      return 0;
    }

  if (args_count == num_disks)
    {
      if (!holy_strcmp (arg, "ls"))
        {
          cmd = CMD_LS;
        }
      else if (!holy_strcmp (arg, "zfsinfo"))
        {
          cmd = CMD_ZFSINFO;
        }
      else if (!holy_strcmp (arg, "cp"))
	{
	  cmd = CMD_CP;
          nparm = 2;
	}
      else if (!holy_strcmp (arg, "cat"))
	{
	  cmd = CMD_CAT;
	  nparm = 1;
	}
      else if (!holy_strcmp (arg, "cmp"))
	{
	  cmd = CMD_CMP;
          nparm = 2;
	}
      else if (!holy_strcmp (arg, "hex"))
	{
	  cmd = CMD_HEX;
          nparm = 1;
	}
      else if (!holy_strcmp (arg, "crc"))
	{
	  cmd = CMD_CRC;
          nparm = 1;
	}
      else if (!holy_strcmp (arg, "blocklist"))
	{
	  cmd = CMD_BLOCKLIST;
          nparm = 1;
	}
      else if (!holy_strcmp (arg, "testload"))
	{
	  cmd = CMD_TESTLOAD;
          nparm = 1;
	}
      else if (holy_strcmp (arg, "xnu_uuid") == 0)
	{
	  cmd = CMD_XNU_UUID;
	  nparm = 0;
	}
      else
	{
	  fprintf (stderr, _("Invalid command %s.\n"), arg);
	  argp_usage (state);
	}
      args_count++;
      return 0;
    }

  args[args_count - 1 - num_disks] = xstrdup (arg);
  args_count++;
  return 0;
}

struct argp argp = {
  options, argp_parser, N_("IMAGE_PATH COMMANDS"),
  N_("Debug tool for filesystem driver."), 
  NULL, NULL, NULL
};

int
main (int argc, char *argv[])
{
  const char *default_root;
  char *alloc_root;

  holy_util_host_init (&argc, &argv);

  args = xmalloc (argc * sizeof (args[0]));

  argp_parse (&argp, argc, argv, 0, 0, 0);

  /* Initialize all modules. */
  holy_init_all ();
  holy_gcry_init_all ();

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
  fstest (args_count - 1 - num_disks);

  /* Free resources.  */
  holy_gcry_fini_all ();
  holy_fini_all ();

  return 0;
}
