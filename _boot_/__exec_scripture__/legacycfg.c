/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/command.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/normal.h>
#include <holy/script_sh.h>
#include <holy/i18n.h>
#include <holy/term.h>
#include <holy/legacy_parse.h>
#include <holy/crypto.h>
#include <holy/auth.h>
#include <holy/disk.h>
#include <holy/partition.h>

holy_MOD_LICENSE ("GPLv2+");

/* Helper for legacy_file.  */
static holy_err_t
legacy_file_getline (char **line, int cont __attribute__ ((unused)),
		     void *data __attribute__ ((unused)))
{
  *line = 0;
  return holy_ERR_NONE;
}

static holy_err_t
legacy_file (const char *filename)
{
  holy_file_t file;
  char *entryname = NULL, *entrysrc = NULL;
  holy_menu_t menu;
  char *suffix = holy_strdup ("");

  if (!suffix)
    return holy_errno;

  file = holy_file_open (filename);
  if (! file)
    {
      holy_free (suffix);
      return holy_errno;
    }

  menu = holy_env_get_menu ();
  if (! menu)
    {
      menu = holy_zalloc (sizeof (*menu));
      if (! menu)
	{
	  holy_free (suffix);
	  return holy_errno;
	}

      holy_env_set_menu (menu);
    }

  while (1)
    {
      char *buf = holy_file_getline (file);
      char *parsed = NULL;

      if (!buf && holy_errno)
	{
	  holy_file_close (file);
	  holy_free (suffix);
	  return holy_errno;
	}

      if (!buf)
	break;

      {
	char *oldname = NULL;
	char *newsuffix;
	char *ptr;

	for (ptr = buf; *ptr && holy_isspace (*ptr); ptr++);

	oldname = entryname;
	parsed = holy_legacy_parse (ptr, &entryname, &newsuffix);
	holy_free (buf);
	buf = NULL;
	if (newsuffix)
	  {
	    char *t;
	    
	    t = suffix;
	    suffix = holy_realloc (suffix, holy_strlen (suffix)
				   + holy_strlen (newsuffix) + 1);
	    if (!suffix)
	      {
		holy_free (t);
		holy_free (entrysrc);
		holy_free (parsed);
		holy_free (newsuffix);
		holy_free (suffix);
		return holy_errno;
	      }
	    holy_memcpy (suffix + holy_strlen (suffix), newsuffix,
			 holy_strlen (newsuffix) + 1);
	    holy_free (newsuffix);
	    newsuffix = NULL;
	  }
	if (oldname != entryname && oldname)
	  {
	    const char **args = holy_malloc (sizeof (args[0]));
	    if (!args)
	      {
		holy_file_close (file);
		return holy_errno;
	      }
	    args[0] = oldname;
	    holy_normal_add_menu_entry (1, args, NULL, NULL, "legacy",
					NULL, NULL,
					entrysrc, 0);
	    holy_free (args);
	    entrysrc[0] = 0;
	    holy_free (oldname);
	  }
      }

      if (parsed && !entryname)
	{
	  holy_normal_parse_line (parsed, legacy_file_getline, NULL);
	  holy_print_error ();
	  holy_free (parsed);
	  parsed = NULL;
	}
      else if (parsed)
	{
	  if (!entrysrc)
	    entrysrc = parsed;
	  else
	    {
	      char *t;

	      t = entrysrc;
	      entrysrc = holy_realloc (entrysrc, holy_strlen (entrysrc)
				       + holy_strlen (parsed) + 1);
	      if (!entrysrc)
		{
		  holy_free (t);
		  holy_free (parsed);
		  holy_free (suffix);
		  return holy_errno;
		}
	      holy_memcpy (entrysrc + holy_strlen (entrysrc), parsed,
			   holy_strlen (parsed) + 1);
	      holy_free (parsed);
	      parsed = NULL;
	    }
	}
    }
  holy_file_close (file);

  if (entryname)
    {
      const char **args = holy_malloc (sizeof (args[0]));
      if (!args)
	{
	  holy_file_close (file);
	  holy_free (suffix);
	  holy_free (entrysrc);
	  return holy_errno;
	}
      args[0] = entryname;
      holy_normal_add_menu_entry (1, args, NULL, NULL, NULL,
				  NULL, NULL, entrysrc, 0);
      holy_free (args);
    }

  holy_normal_parse_line (suffix, legacy_file_getline, NULL);
  holy_print_error ();
  holy_free (suffix);
  holy_free (entrysrc);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_legacy_source (struct holy_command *cmd,
			int argc, char **args)
{
  int new_env, extractor;
  holy_err_t ret;

  if (argc != 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  extractor = (cmd->name[0] == 'e');
  new_env = (cmd->name[extractor ? (sizeof ("extract_legacy_entries_") - 1)
		       : (sizeof ("legacy_") - 1)] == 'c');

  if (new_env)
    holy_cls ();

  if (new_env && !extractor)
    holy_env_context_open ();
  if (extractor)
    holy_env_extractor_open (!new_env);

  ret = legacy_file (args[0]);

  if (new_env)
    {
      holy_menu_t menu;
      menu = holy_env_get_menu ();
      if (menu && menu->size)
	holy_show_menu (menu, 1, 0);
      if (!extractor)
	holy_env_context_close ();
    }
  if (extractor)
    holy_env_extractor_close (!new_env);

  return ret;
}

static enum
  { 
    GUESS_IT, LINUX, MULTIBOOT, KFREEBSD, KNETBSD, KOPENBSD 
  } kernel_type;

static holy_err_t
holy_cmd_legacy_kernel (struct holy_command *mycmd __attribute__ ((unused)),
			int argc, char **args)
{
  int i;
#ifdef TODO
  int no_mem_option = 0;
#endif
  struct holy_command *cmd;
  char **cutargs;
  int cutargc;
  holy_err_t err = holy_ERR_NONE;
  
  for (i = 0; i < 2; i++)
    {
      /* FIXME: really support this.  */
      if (argc >= 1 && holy_strcmp (args[0], "--no-mem-option") == 0)
	{
#ifdef TODO
	  no_mem_option = 1;
#endif
	  argc--;
	  args++;
	  continue;
	}

      /* linux16 handles both zImages and bzImages.   */
      if (argc >= 1 && (holy_strcmp (args[0], "--type=linux") == 0
			|| holy_strcmp (args[0], "--type=biglinux") == 0))
	{
	  kernel_type = LINUX;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && holy_strcmp (args[0], "--type=multiboot") == 0)
	{
	  kernel_type = MULTIBOOT;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && holy_strcmp (args[0], "--type=freebsd") == 0)
	{
	  kernel_type = KFREEBSD;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && holy_strcmp (args[0], "--type=openbsd") == 0)
	{
	  kernel_type = KOPENBSD;
	  argc--;
	  args++;
	  continue;
	}

      if (argc >= 1 && holy_strcmp (args[0], "--type=netbsd") == 0)
	{
	  kernel_type = KNETBSD;
	  argc--;
	  args++;
	  continue;
	}
    }

  if (argc < 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("filename expected"));

  cutargs = holy_malloc (sizeof (cutargs[0]) * (argc - 1));
  if (!cutargs)
    return holy_errno;
  cutargc = argc - 1;
  holy_memcpy (cutargs + 1, args + 2, sizeof (cutargs[0]) * (argc - 2));
  cutargs[0] = args[0];

  do
    {
      /* First try Linux.  */
      if (kernel_type == GUESS_IT || kernel_type == LINUX)
	{
#ifdef holy_MACHINE_PCBIOS
	  cmd = holy_command_find ("linux16");
#else
	  cmd = holy_command_find ("linux");
#endif
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, cutargc, cutargs))
		{
		  kernel_type = LINUX;
		  goto out;
		}
	    }
	  holy_errno = holy_ERR_NONE;
	}

      /* Then multiboot.  */
      if (kernel_type == GUESS_IT || kernel_type == MULTIBOOT)
	{
	  cmd = holy_command_find ("multiboot");
	  if (cmd)
	    {
	      if (!(cmd->func) (cmd, argc, args))
		{
		  kernel_type = MULTIBOOT;
		  goto out;
		}
	    }
	  holy_errno = holy_ERR_NONE;
	}

      {
	int bsd_device = -1;
	int bsd_slice = -1;
	int bsd_part = -1;
	{
	  holy_device_t dev;
	  const char *hdbiasstr;
	  int hdbias = 0;
	  hdbiasstr = holy_env_get ("legacy_hdbias");
	  if (hdbiasstr)
	    {
	      hdbias = holy_strtoul (hdbiasstr, 0, 0);
	      holy_errno = holy_ERR_NONE;
	    }
	  dev = holy_device_open (0);
	  if (dev && dev->disk
	      && dev->disk->dev->id == holy_DISK_DEVICE_BIOSDISK_ID
	      && dev->disk->id >= 0x80 && dev->disk->id <= 0x90)
	    {
	      struct holy_partition *part = dev->disk->partition;
	      bsd_device = dev->disk->id - 0x80 - hdbias;
	      if (part && (holy_strcmp (part->partmap->name, "netbsd") == 0
			   || holy_strcmp (part->partmap->name, "openbsd") == 0
			   || holy_strcmp (part->partmap->name, "bsd") == 0))
		{
		  bsd_part = part->number;
		  part = part->parent;
		}
	      if (part && holy_strcmp (part->partmap->name, "msdos") == 0)
		bsd_slice = part->number;
	    }
	  if (dev)
	    holy_device_close (dev);
	}
	
	/* k*BSD didn't really work well with holy-legacy.  */
	if (kernel_type == GUESS_IT || kernel_type == KFREEBSD)
	  {
	    char buf[sizeof("adXXXXXXXXXXXXsXXXXXXXXXXXXYYY")];
	    if (bsd_device != -1)
	      {
		if (bsd_slice != -1 && bsd_part != -1)
		  holy_snprintf(buf, sizeof(buf), "ad%ds%d%c", bsd_device,
				bsd_slice, 'a' + bsd_part);
		else if (bsd_slice != -1)
		  holy_snprintf(buf, sizeof(buf), "ad%ds%d", bsd_device,
				bsd_slice);
		else
		  holy_snprintf(buf, sizeof(buf), "ad%d", bsd_device);
		holy_env_set ("kFreeBSD.vfs.root.mountfrom", buf);
	      }
	    else
	      holy_env_unset ("kFreeBSD.vfs.root.mountfrom");
	    cmd = holy_command_find ("kfreebsd");
	    if (cmd)
	      {
		if (!(cmd->func) (cmd, cutargc, cutargs))
		  {
		    kernel_type = KFREEBSD;
		    goto out;
		  }
	      }
	    holy_errno = holy_ERR_NONE;
	  }
	{
	  char **bsdargs;
	  int bsdargc;
	  char bsddevname[sizeof ("wdXXXXXXXXXXXXY")];
	  int found = 0;

	  if (bsd_device == -1)
	    {
	      bsdargs = cutargs;
	      bsdargc = cutargc;
	    }
	  else
	    {
	      char rbuf[3] = "-r";
	      bsdargc = cutargc + 2;
	      bsdargs = holy_malloc (sizeof (bsdargs[0]) * bsdargc);
	      if (!bsdargs)
		{
		  err = holy_errno;
		  goto out;
		}
	      holy_memcpy (bsdargs, args, argc * sizeof (bsdargs[0]));
	      bsdargs[argc] = rbuf;
	      bsdargs[argc + 1] = bsddevname;
	      holy_snprintf (bsddevname, sizeof (bsddevname),
			     "wd%d%c", bsd_device,
			     bsd_part != -1 ? bsd_part + 'a' : 'c');
	    }
	  if (kernel_type == GUESS_IT || kernel_type == KNETBSD)
	    {
	      cmd = holy_command_find ("knetbsd");
	      if (cmd)
		{
		  if (!(cmd->func) (cmd, bsdargc, bsdargs))
		    {
		      kernel_type = KNETBSD;
		      found = 1;
		      goto free_bsdargs;
		    }
		}
	      holy_errno = holy_ERR_NONE;
	    }
	  if (kernel_type == GUESS_IT || kernel_type == KOPENBSD)
	    {
	      cmd = holy_command_find ("kopenbsd");
	      if (cmd)
		{
		  if (!(cmd->func) (cmd, bsdargc, bsdargs))
		    {
		      kernel_type = KOPENBSD;
		      found = 1;
		      goto free_bsdargs;
		    }
		}
	      holy_errno = holy_ERR_NONE;
	    }

free_bsdargs:
	  if (bsdargs != cutargs)
	    holy_free (bsdargs);
	  if (found)
	    goto out;
	}
      }
    }
  while (0);

  err = holy_error (holy_ERR_BAD_OS, "couldn't load file %s",
		    args[0]);
out:
  holy_free (cutargs);
  return err;
}

static holy_err_t
holy_cmd_legacy_initrd (struct holy_command *mycmd __attribute__ ((unused)),
			int argc, char **args)
{
  struct holy_command *cmd;

  if (kernel_type == LINUX)
    {
#ifdef holy_MACHINE_PCBIOS
      cmd = holy_command_find ("initrd16");
#else
      cmd = holy_command_find ("initrd");
#endif
      if (!cmd)
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("can't find command `%s'"),
#ifdef holy_MACHINE_PCBIOS
			   "initrd16"
#else
			   "initrd"
#endif
			   );

      return cmd->func (cmd, argc ? 1 : 0, args);
    }
  if (kernel_type == MULTIBOOT)
    {
      cmd = holy_command_find ("module");
      if (!cmd)
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("can't find command `%s'"),
			   "module");

      return cmd->func (cmd, argc, args);
    }

  return holy_error (holy_ERR_BAD_ARGUMENT,
		     N_("you need to load the kernel first"));
}

static holy_err_t
holy_cmd_legacy_initrdnounzip (struct holy_command *mycmd __attribute__ ((unused)),
			       int argc, char **args)
{
  struct holy_command *cmd;

  if (kernel_type == LINUX)
    {
      cmd = holy_command_find ("initrd16");
      if (!cmd)
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("can't find command `%s'"),
			   "initrd16");

      return cmd->func (cmd, argc, args);
    }
  if (kernel_type == MULTIBOOT)
    {
      char **newargs;
      holy_err_t err;
      char nounzipbuf[10] = "--nounzip";

      cmd = holy_command_find ("module");
      if (!cmd)
	return holy_error (holy_ERR_BAD_ARGUMENT, N_("can't find command `%s'"),
			   "module");

      newargs = holy_malloc ((argc + 1) * sizeof (newargs[0]));
      if (!newargs)
	return holy_errno;
      holy_memcpy (newargs + 1, args, argc * sizeof (newargs[0]));
      newargs[0] = nounzipbuf;

      err = cmd->func (cmd, argc + 1, newargs);
      holy_free (newargs);
      return err;
    }

  return holy_error (holy_ERR_BAD_ARGUMENT,
		     N_("you need to load the kernel first"));
}

static holy_err_t
check_password_deny (const char *user __attribute__ ((unused)),
		     const char *entered  __attribute__ ((unused)),
		     void *password __attribute__ ((unused)))
{
  return holy_ACCESS_DENIED;
}

#define MD5_HASHLEN 16

struct legacy_md5_password
{
  holy_uint8_t *salt;
  int saltlen;
  holy_uint8_t hash[MD5_HASHLEN];
};

static int
check_password_md5_real (const char *entered,
			 struct legacy_md5_password *pw)
{
  holy_size_t enteredlen = holy_strlen (entered);
  unsigned char alt_result[MD5_HASHLEN];
  unsigned char *digest;
  holy_uint8_t *ctx;
  holy_size_t i;
  int ret;

  ctx = holy_zalloc (holy_MD_MD5->contextsize);
  if (!ctx)
    return 0;

  holy_MD_MD5->init (ctx);
  holy_MD_MD5->write (ctx, entered, enteredlen);
  holy_MD_MD5->write (ctx, pw->salt + 3, pw->saltlen - 3);
  holy_MD_MD5->write (ctx, entered, enteredlen);
  digest = holy_MD_MD5->read (ctx);
  holy_MD_MD5->final (ctx);
  holy_memcpy (alt_result, digest, MD5_HASHLEN);
  
  holy_MD_MD5->init (ctx);
  holy_MD_MD5->write (ctx, entered, enteredlen);
  holy_MD_MD5->write (ctx, pw->salt, pw->saltlen); /* include the $1$ header */
  for (i = enteredlen; i > 16; i -= 16)
    holy_MD_MD5->write (ctx, alt_result, 16);
  holy_MD_MD5->write (ctx, alt_result, i);

  for (i = enteredlen; i > 0; i >>= 1)
    holy_MD_MD5->write (ctx, entered + ((i & 1) ? enteredlen : 0), 1);
  digest = holy_MD_MD5->read (ctx);
  holy_MD_MD5->final (ctx);

  for (i = 0; i < 1000; i++)
    {
      holy_memcpy (alt_result, digest, 16);

      holy_MD_MD5->init (ctx);
      if ((i & 1) != 0)
	holy_MD_MD5->write (ctx, entered, enteredlen);
      else
	holy_MD_MD5->write (ctx, alt_result, 16);
      
      if (i % 3 != 0)
	holy_MD_MD5->write (ctx, pw->salt + 3, pw->saltlen - 3);

      if (i % 7 != 0)
	holy_MD_MD5->write (ctx, entered, enteredlen);

      if ((i & 1) != 0)
	holy_MD_MD5->write (ctx, alt_result, 16);
      else
	holy_MD_MD5->write (ctx, entered, enteredlen);
      digest = holy_MD_MD5->read (ctx);
      holy_MD_MD5->final (ctx);
    }

  ret = (holy_crypto_memcmp (digest, pw->hash, MD5_HASHLEN) == 0);
  holy_free (ctx);
  return ret;
}

static holy_err_t
check_password_md5 (const char *user,
		    const char *entered,
		    void *password)
{
  if (!check_password_md5_real (entered, password))
    return holy_ACCESS_DENIED;

  holy_auth_authenticate (user);

  return holy_ERR_NONE;
}

static inline int
ib64t (char c)
{
  if (c == '.')
    return 0;
  if (c == '/')
    return 1;
  if (c >= '0' && c <= '9')
    return c - '0' + 2;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 12;
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 38;
  return -1;
}

static struct legacy_md5_password *
parse_legacy_md5 (int argc, char **args)
{
  const char *salt, *saltend;
  struct legacy_md5_password *pw = NULL;
  int i;
  const char *p;

  if (holy_memcmp (args[0], "--md5", sizeof ("--md5")) != 0)
    goto fail;
  if (argc == 1)
    goto fail;
  if (holy_strlen(args[1]) <= 3)
    goto fail;
  salt = args[1];
  saltend = holy_strchr (salt + 3, '$');
  if (!saltend)
    goto fail;
  pw = holy_malloc (sizeof (*pw));
  if (!pw)
    goto fail;

  p = saltend + 1;
  for (i = 0; i < 5; i++)
    {
      int n;
      holy_uint32_t w = 0;

      for (n = 0; n < 4; n++)
	{
	  int ww = ib64t(*p++);
	  if (ww == -1)
	    goto fail;
	  w |= ww << (n * 6);
	}
      pw->hash[i == 4 ? 5 : 12+i] = w & 0xff;
      pw->hash[6+i] = (w >> 8) & 0xff;
      pw->hash[i] = (w >> 16) & 0xff;
    }
  {
    int n;
    holy_uint32_t w = 0;
    for (n = 0; n < 2; n++)
      {
	int ww = ib64t(*p++);
	if (ww == -1)
	  goto fail;
	w |= ww << (6 * n);
      }
    if (w >= 0x100)
      goto fail;
    pw->hash[11] = w;
  }

  pw->saltlen = saltend - salt;
  pw->salt = (holy_uint8_t *) holy_strndup (salt, pw->saltlen);
  if (!pw->salt)
    goto fail;

  return pw;

 fail:
  holy_free (pw);
  return NULL;
}

static holy_err_t
holy_cmd_legacy_password (struct holy_command *mycmd __attribute__ ((unused)),
			  int argc, char **args)
{
  struct legacy_md5_password *pw = NULL;

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  if (args[0][0] != '-' || args[0][1] != '-')
    return holy_normal_set_password ("legacy", args[0]);

  pw = parse_legacy_md5 (argc, args);

  if (pw)
    return holy_auth_register_authentication ("legacy", check_password_md5, pw);
  else
    /* This is to imitate minor difference between holy-legacy in holy2.
       If 2 password commands are executed in a row and second one fails
       on holy2 the password of first one is used, whereas in holy-legacy
       authenthication is denied. In case of no password command was executed
       early both versions deny any access.  */
    return holy_auth_register_authentication ("legacy", check_password_deny,
					      NULL);
}

int
holy_legacy_check_md5_password (int argc, char **args,
				char *entered)
{
  struct legacy_md5_password *pw = NULL;
  int ret;

  if (args[0][0] != '-' || args[0][1] != '-')
    {
      char correct[holy_AUTH_MAX_PASSLEN];

      holy_memset (correct, 0, sizeof (correct));
      holy_strncpy (correct, args[0], sizeof (correct));

      return holy_crypto_memcmp (entered, correct, holy_AUTH_MAX_PASSLEN) == 0;
    }

  pw = parse_legacy_md5 (argc, args);

  if (!pw)
    return 0;

  ret = check_password_md5_real (entered, pw);
  holy_free (pw);
  return ret;
}

static holy_err_t
holy_cmd_legacy_check_password (struct holy_command *mycmd __attribute__ ((unused)),
				int argc, char **args)
{
  char entered[holy_AUTH_MAX_PASSLEN];

  if (argc == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  holy_puts_ (N_("Enter password: "));
  if (!holy_password_get (entered, holy_AUTH_MAX_PASSLEN))
    return holy_ACCESS_DENIED;

  if (!holy_legacy_check_md5_password (argc, args,
				       entered))
    return holy_ACCESS_DENIED;

  return holy_ERR_NONE;
}

static holy_command_t cmd_source, cmd_configfile;
static holy_command_t cmd_source_extract, cmd_configfile_extract;
static holy_command_t cmd_kernel, cmd_initrd, cmd_initrdnounzip;
static holy_command_t cmd_password, cmd_check_password;

holy_MOD_INIT(legacycfg)
{
  cmd_source
    = holy_register_command ("legacy_source",
			     holy_cmd_legacy_source,
			     N_("FILE"),
			     /* TRANSLATORS: "legacy config" means
				"config as used by holy-legacy".  */
			     N_("Parse legacy config in same context"));
  cmd_configfile
    = holy_register_command ("legacy_configfile",
			     holy_cmd_legacy_source,
			     N_("FILE"),
			     N_("Parse legacy config in new context"));
  cmd_source_extract
    = holy_register_command ("extract_legacy_entries_source",
			     holy_cmd_legacy_source,
			     N_("FILE"),
			     N_("Parse legacy config in same context taking only menu entries"));
  cmd_configfile_extract
    = holy_register_command ("extract_legacy_entries_configfile",
			     holy_cmd_legacy_source,
			     N_("FILE"),
			     N_("Parse legacy config in new context taking only menu entries"));

  cmd_kernel = holy_register_command ("legacy_kernel",
				      holy_cmd_legacy_kernel,
				      N_("[--no-mem-option] [--type=TYPE] FILE [ARG ...]"),
				      N_("Simulate holy-legacy `kernel' command"));

  cmd_initrd = holy_register_command ("legacy_initrd",
				      holy_cmd_legacy_initrd,
				      N_("FILE [ARG ...]"),
				      N_("Simulate holy-legacy `initrd' command"));
  cmd_initrdnounzip = holy_register_command ("legacy_initrd_nounzip",
					     holy_cmd_legacy_initrdnounzip,
					     N_("FILE [ARG ...]"),
					     N_("Simulate holy-legacy `modulenounzip' command"));

  cmd_password = holy_register_command ("legacy_password",
					holy_cmd_legacy_password,
					N_("[--md5] PASSWD [FILE]"),
					N_("Simulate holy-legacy `password' command"));

  cmd_check_password = holy_register_command ("legacy_check_password",
					      holy_cmd_legacy_check_password,
					      N_("[--md5] PASSWD [FILE]"),
					      N_("Simulate holy-legacy `password' command in menu entry mode"));

}

holy_MOD_FINI(legacycfg)
{
  holy_unregister_command (cmd_source);
  holy_unregister_command (cmd_configfile);
  holy_unregister_command (cmd_source_extract);
  holy_unregister_command (cmd_configfile_extract);

  holy_unregister_command (cmd_kernel);
  holy_unregister_command (cmd_initrd);
  holy_unregister_command (cmd_initrdnounzip);

  holy_unregister_command (cmd_password);
  holy_unregister_command (cmd_check_password);
}
