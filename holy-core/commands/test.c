/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/env.h>
#include <holy/fs.h>
#include <holy/device.h>
#include <holy/file.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* A simple implementation for signed numbers. */
static int
holy_strtosl (char *arg, char **end, int base)
{
  if (arg[0] == '-')
    return -holy_strtoul (arg + 1, end, base);
  return holy_strtoul (arg, end, base);
}

/* Context for test_parse.  */
struct test_parse_ctx
{
  int invert;
  int or, and;
  int file_exists;
  struct holy_dirhook_info file_info;
  char *filename;
};

/* Take care of discarding and inverting. */
static void
update_val (int val, struct test_parse_ctx *ctx)
{
  ctx->and = ctx->and && (ctx->invert ? ! val : val);
  ctx->invert = 0;
}

/* A hook for iterating directories. */
static int
find_file (const char *cur_filename, const struct holy_dirhook_info *info,
	   void *data)
{
  struct test_parse_ctx *ctx = data;

  if ((info->case_insensitive ? holy_strcasecmp (cur_filename, ctx->filename)
       : holy_strcmp (cur_filename, ctx->filename)) == 0)
    {
      ctx->file_info = *info;
      ctx->file_exists = 1;
      return 1;
    }
  return 0;
}

/* Check if file exists and fetch its information. */
static void
get_fileinfo (char *path, struct test_parse_ctx *ctx)
{
  char *pathname;
  char *device_name;
  holy_fs_t fs;
  holy_device_t dev;

  ctx->file_exists = 0;
  device_name = holy_file_get_device_name (path);
  dev = holy_device_open (device_name);
  if (! dev)
    {
      holy_free (device_name);
      return;
    }

  fs = holy_fs_probe (dev);
  if (! fs)
    {
      holy_free (device_name);
      holy_device_close (dev);
      return;
    }

  pathname = holy_strchr (path, ')');
  if (! pathname)
    pathname = path;
  else
    pathname++;

  /* Remove trailing '/'. */
  while (*pathname && pathname[holy_strlen (pathname) - 1] == '/')
    pathname[holy_strlen (pathname) - 1] = 0;

  /* Split into path and filename. */
  ctx->filename = holy_strrchr (pathname, '/');
  if (! ctx->filename)
    {
      path = holy_strdup ("/");
      ctx->filename = pathname;
    }
  else
    {
      ctx->filename++;
      path = holy_strdup (pathname);
      path[ctx->filename - pathname] = 0;
    }

  /* It's the whole device. */
  if (! *pathname)
    {
      ctx->file_exists = 1;
      holy_memset (&ctx->file_info, 0, sizeof (ctx->file_info));
      /* Root is always a directory. */
      ctx->file_info.dir = 1;

      /* Fetch writing time. */
      ctx->file_info.mtimeset = 0;
      if (fs->mtime)
	{
	  if (! fs->mtime (dev, &ctx->file_info.mtime))
	    ctx->file_info.mtimeset = 1;
	  holy_errno = holy_ERR_NONE;
	}
    }
  else
    (fs->dir) (dev, path, find_file, ctx);

  holy_device_close (dev);
  holy_free (path);
  holy_free (device_name);
}

/* Parse a test expression starting from *argn. */
static int
test_parse (char **args, int *argn, int argc)
{
  struct test_parse_ctx ctx = {
    .and = 1,
    .or = 0,
    .invert = 0
  };

  /* Here we have the real parsing. */
  while (*argn < argc)
    {
      /* First try 3 argument tests. */
      if (*argn + 2 < argc)
	{
	  /* String tests. */
	  if (holy_strcmp (args[*argn + 1], "=") == 0
	      || holy_strcmp (args[*argn + 1], "==") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) == 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "!=") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) != 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  /* holy extension: lexicographical sorting. */
	  if (holy_strcmp (args[*argn + 1], "<") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) < 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "<=") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) <= 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], ">") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) > 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], ">=") == 0)
	    {
	      update_val (holy_strcmp (args[*argn], args[*argn + 2]) >= 0,
			  &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  /* Number tests. */
	  if (holy_strcmp (args[*argn + 1], "-eq") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
			  == holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "-ge") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
			  >= holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "-gt") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
			  > holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "-le") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
		      <= holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "-lt") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
			  < holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  if (holy_strcmp (args[*argn + 1], "-ne") == 0)
	    {
	      update_val (holy_strtosl (args[*argn], 0, 0)
			  != holy_strtosl (args[*argn + 2], 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  /* holy extension: compare numbers skipping prefixes.
	     Useful for comparing versions. E.g. vmlinuz-2 -plt vmlinuz-11. */
	  if (holy_strcmp (args[*argn + 1], "-pgt") == 0
	      || holy_strcmp (args[*argn + 1], "-plt") == 0)
	    {
	      int i;
	      /* Skip common prefix. */
	      for (i = 0; args[*argn][i] == args[*argn + 2][i]
		     && args[*argn][i]; i++);

	      /* Go the digits back. */
	      i--;
	      while (holy_isdigit (args[*argn][i]) && i > 0)
		i--;
	      i++;

	      if (holy_strcmp (args[*argn + 1], "-pgt") == 0)
		update_val (holy_strtoul (args[*argn] + i, 0, 0)
			    > holy_strtoul (args[*argn + 2] + i, 0, 0), &ctx);
	      else
		update_val (holy_strtoul (args[*argn] + i, 0, 0)
			    < holy_strtoul (args[*argn + 2] + i, 0, 0), &ctx);
	      (*argn) += 3;
	      continue;
	    }

	  /* -nt and -ot tests. holy extension: when doing -?t<bias> bias
	     will be added to the first mtime. */
	  if (holy_memcmp (args[*argn + 1], "-nt", 3) == 0
	      || holy_memcmp (args[*argn + 1], "-ot", 3) == 0)
	    {
	      struct holy_dirhook_info file1;
	      int file1exists;
	      int bias = 0;

	      /* Fetch fileinfo. */
	      get_fileinfo (args[*argn], &ctx);
	      file1 = ctx.file_info;
	      file1exists = ctx.file_exists;
	      get_fileinfo (args[*argn + 2], &ctx);

	      if (args[*argn + 1][3])
		bias = holy_strtosl (args[*argn + 1] + 3, 0, 0);

	      if (holy_memcmp (args[*argn + 1], "-nt", 3) == 0)
		update_val ((file1exists && ! ctx.file_exists)
			    || (file1.mtimeset && ctx.file_info.mtimeset
				&& file1.mtime + bias > ctx.file_info.mtime),
			    &ctx);
	      else
		update_val ((! file1exists && ctx.file_exists)
			    || (file1.mtimeset && ctx.file_info.mtimeset
				&& file1.mtime + bias < ctx.file_info.mtime),
			    &ctx);
	      (*argn) += 3;
	      continue;
	    }
	}

      /* Two-argument tests. */
      if (*argn + 1 < argc)
	{
	  /* File tests. */
	  if (holy_strcmp (args[*argn], "-d") == 0)
	    {
	      get_fileinfo (args[*argn + 1], &ctx);
	      update_val (ctx.file_exists && ctx.file_info.dir, &ctx);
	      (*argn) += 2;
	      continue;
	    }

	  if (holy_strcmp (args[*argn], "-e") == 0)
	    {
	      get_fileinfo (args[*argn + 1], &ctx);
	      update_val (ctx.file_exists, &ctx);
	      (*argn) += 2;
	      continue;
	    }

	  if (holy_strcmp (args[*argn], "-f") == 0)
	    {
	      get_fileinfo (args[*argn + 1], &ctx);
	      /* FIXME: check for other types. */
	      update_val (ctx.file_exists && ! ctx.file_info.dir, &ctx);
	      (*argn) += 2;
	      continue;
	    }

	  if (holy_strcmp (args[*argn], "-s") == 0)
	    {
	      holy_file_t file;
	      holy_file_filter_disable_compression ();
	      file = holy_file_open (args[*argn + 1]);
	      update_val (file && (holy_file_size (file) != 0), &ctx);
	      if (file)
		holy_file_close (file);
	      holy_errno = holy_ERR_NONE;
	      (*argn) += 2;
	      continue;
	    }

	  /* String tests. */
	  if (holy_strcmp (args[*argn], "-n") == 0)
	    {
	      update_val (args[*argn + 1][0], &ctx);

	      (*argn) += 2;
	      continue;
	    }
	  if (holy_strcmp (args[*argn], "-z") == 0)
	    {
	      update_val (! args[*argn + 1][0], &ctx);
	      (*argn) += 2;
	      continue;
	    }
	}

      /* Special modifiers. */

      /* End of expression. return to parent. */
      if (holy_strcmp (args[*argn], ")") == 0)
	{
	  (*argn)++;
	  return ctx.or || ctx.and;
	}
      /* Recursively invoke if parenthesis. */
      if (holy_strcmp (args[*argn], "(") == 0)
	{
	  (*argn)++;
	  update_val (test_parse (args, argn, argc), &ctx);
	  continue;
	}

      if (holy_strcmp (args[*argn], "!") == 0)
	{
	  ctx.invert = ! ctx.invert;
	  (*argn)++;
	  continue;
	}
      if (holy_strcmp (args[*argn], "-a") == 0)
	{
	  (*argn)++;
	  continue;
	}
      if (holy_strcmp (args[*argn], "-o") == 0)
	{
	  ctx.or = ctx.or || ctx.and;
	  ctx.and = 1;
	  (*argn)++;
	  continue;
	}

      /* No test found. Interpret if as just a string. */
      update_val (args[*argn][0], &ctx);
      (*argn)++;
    }
  return ctx.or || ctx.and;
}

static holy_err_t
holy_cmd_test (holy_command_t cmd __attribute__ ((unused)),
	       int argc, char **args)
{
  int argn = 0;

  if (argc >= 1 && holy_strcmp (args[argc - 1], "]") == 0)
    argc--;

  return test_parse (args, &argn, argc) ? holy_ERR_NONE
    : holy_error (holy_ERR_TEST_FAILURE, N_("false"));
}

static holy_command_t cmd_1, cmd_2;

holy_MOD_INIT(test)
{
  cmd_1 = holy_register_command ("[", holy_cmd_test,
				 N_("EXPRESSION ]"), N_("Evaluate an expression."));
  cmd_1->flags |= holy_COMMAND_FLAG_EXTRACTOR;
  cmd_2 = holy_register_command ("test", holy_cmd_test,
				 N_("EXPRESSION"), N_("Evaluate an expression."));
  cmd_2->flags |= holy_COMMAND_FLAG_EXTRACTOR;
}

holy_MOD_FINI(test)
{
  holy_unregister_command (cmd_1);
  holy_unregister_command (cmd_2);
}
