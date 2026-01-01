/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/fs.h>
#include <holy/env.h>
#include <holy/file.h>
#include <holy/device.h>
#include <holy/script_sh.h>

#include <regex.h>

static inline int isregexop (char ch);
static char ** merge (char **lhs, char **rhs);
static char *make_dir (const char *prefix, const char *start, const char *end);
static int make_regex (const char *regex_start, const char *regex_end,
		       regex_t *regexp);
static void split_path (const char *path, const char **suffix_end, const char **regex_end);
static char ** match_devices (const regex_t *regexp, int noparts);
static char ** match_files (const char *prefix, const char *suffix_start,
			    const char *suffix_end, const regex_t *regexp);

static holy_err_t wildcard_expand (const char *s, char ***strs);

struct holy_script_wildcard_translator holy_filename_translator = {
  .expand = wildcard_expand,
};

static char **
merge (char **dest, char **ps)
{
  int i;
  int j;
  char **p;

  if (! dest)
    return ps;

  if (! ps)
    return dest;

  for (i = 0; dest[i]; i++)
    ;
  for (j = 0; ps[j]; j++)
    ;

  p = holy_realloc (dest, sizeof (char*) * (i + j + 1));
  if (! p)
    {
      holy_free (dest);
      holy_free (ps);
      return 0;
    }

  dest = p;
  for (j = 0; ps[j]; j++)
    dest[i++] = ps[j];
  dest[i] = 0;

  holy_free (ps);
  return dest;
}

static inline int
isregexop (char ch)
{
  return holy_strchr ("*.\\|+{}[]?", ch) ? 1 : 0;
}

static char *
make_dir (const char *prefix, const char *start, const char *end)
{
  char ch;
  unsigned i;
  unsigned n;
  char *result;

  i = holy_strlen (prefix);
  n = i + end - start;

  result = holy_malloc (n + 1);
  if (! result)
    return 0;

  holy_strcpy (result, prefix);
  while (start < end && (ch = *start++))
    if (ch == '\\' && isregexop (*start))
      result[i++] = *start++;
    else
      result[i++] = ch;

  result[i] = '\0';
  return result;
}

static int
make_regex (const char *start, const char *end, regex_t *regexp)
{
  char ch;
  int i = 0;
  unsigned len = end - start;
  char *buffer = holy_malloc (len * 2 + 2 + 1); /* worst case size. */

  if (! buffer)
    return 1;

  buffer[i++] = '^';
  while (start < end)
    {
      /* XXX Only * and ? expansion for now.  */
      switch ((ch = *start++))
	{
	case '\\':
	  buffer[i++] = ch;
	  if (*start != '\0')
	    buffer[i++] = *start++;
	  break;

	case '.':
	case '(':
	case ')':
	case '@':
	case '+':
	case '|':
	case '{':
	case '}':
	case '[':
	case ']':
	  buffer[i++] = '\\';
	  buffer[i++] = ch;
	  break;

	case '*':
	  buffer[i++] = '.';
	  buffer[i++] = '*';
	  break;

	case '?':
	  buffer[i++] = '.';
	  break;

	default:
	  buffer[i++] = ch;
	}
    }
  buffer[i++] = '$';
  buffer[i] = '\0';
  holy_dprintf ("expand", "Regexp is %s\n", buffer);

  if (regcomp (regexp, buffer, RE_SYNTAX_GNU_AWK))
    {
      holy_free (buffer);
      return 1;
    }

  holy_free (buffer);
  return 0;
}

/* Split `str' into two parts: (1) dirname that is regexop free (2)
   dirname that has a regexop.  */
static void
split_path (const char *str, const char **noregexop, const char **regexop)
{
  char ch = 0;
  int regex = 0;

  const char *end;
  const char *split;  /* points till the end of dirnaname that doesn't
			 need expansion.  */

  split = end = str;
  while ((ch = *end))
    {
      if (ch == '\\' && end[1])
	end++;

      else if (ch == '*' || ch == '?')
	regex = 1;

      else if (ch == '/' && ! regex)
	split = end + 1;  /* forward to next regexop-free dirname */

      else if (ch == '/' && regex)
	break;  /* stop at the first dirname with a regexop */

      end++;
    }

  *regexop = end;
  if (! regex)
    *noregexop = end;
  else
    *noregexop = split;
}

/* Context for match_devices.  */
struct match_devices_ctx
{
  const regex_t *regexp;
  int noparts;
  int ndev;
  char **devs;
};

/* Helper for match_devices.  */
static int
match_devices_iter (const char *name, void *data)
{
  struct match_devices_ctx *ctx = data;
  char **t;
  char *buffer;

  /* skip partitions if asked to. */
  if (ctx->noparts && holy_strchr (name, ','))
    return 0;

  buffer = holy_xasprintf ("(%s)", name);
  if (! buffer)
    return 1;

  holy_dprintf ("expand", "matching: %s\n", buffer);
  if (regexec (ctx->regexp, buffer, 0, 0, 0))
    {
      holy_dprintf ("expand", "not matched\n");
      holy_free (buffer);
      return 0;
    }

  t = holy_realloc (ctx->devs, sizeof (char*) * (ctx->ndev + 2));
  if (! t)
    {
      holy_free (buffer);
      return 1;
    }

  ctx->devs = t;
  ctx->devs[ctx->ndev++] = buffer;
  ctx->devs[ctx->ndev] = 0;
  return 0;
}

static char **
match_devices (const regex_t *regexp, int noparts)
{
  struct match_devices_ctx ctx = {
    .regexp = regexp,
    .noparts = noparts,
    .ndev = 0,
    .devs = 0
  };
  int i;

  if (holy_device_iterate (match_devices_iter, &ctx))
    goto fail;

  return ctx.devs;

 fail:

  for (i = 0; ctx.devs && ctx.devs[i]; i++)
    holy_free (ctx.devs[i]);

  holy_free (ctx.devs);

  return 0;
}

/* Context for match_files.  */
struct match_files_ctx
{
  const regex_t *regexp;
  char **files;
  unsigned nfile;
  char *dir;
};

/* Helper for match_files.  */
static int
match_files_iter (const char *name,
		  const struct holy_dirhook_info *info __attribute__((unused)),
		  void *data)
{
  struct match_files_ctx *ctx = data;
  char **t;
  char *buffer;

  /* skip . and .. names */
  if (holy_strcmp(".", name) == 0 || holy_strcmp("..", name) == 0)
    return 0;

  holy_dprintf ("expand", "matching: %s in %s\n", name, ctx->dir);
  if (regexec (ctx->regexp, name, 0, 0, 0))
    return 0;

  holy_dprintf ("expand", "matched\n");

  buffer = holy_xasprintf ("%s%s", ctx->dir, name);
  if (! buffer)
    return 1;

  t = holy_realloc (ctx->files, sizeof (char*) * (ctx->nfile + 2));
  if (! t)
    {
      holy_free (buffer);
      return 1;
    }

  ctx->files = t;
  ctx->files[ctx->nfile++] = buffer;
  ctx->files[ctx->nfile] = 0;
  return 0;
}

static char **
match_files (const char *prefix, const char *suffix, const char *end,
	     const regex_t *regexp)
{
  struct match_files_ctx ctx = {
    .regexp = regexp,
    .nfile = 0,
    .files = 0
  };
  int i;
  const char *path;
  char *device_name;
  holy_fs_t fs;
  holy_device_t dev;

  dev = 0;
  device_name = 0;
  holy_error_push ();

  ctx.dir = make_dir (prefix, suffix, end);
  if (! ctx.dir)
    goto fail;

  device_name = holy_file_get_device_name (ctx.dir);
  dev = holy_device_open (device_name);
  if (! dev)
    goto fail;

  fs = holy_fs_probe (dev);
  if (! fs)
    goto fail;

  if (ctx.dir[0] == '(')
    {
      path = holy_strchr (ctx.dir, ')');
      if (!path)
	goto fail;
      path++;
    }
  else
    path = ctx.dir;

  if (fs->dir (dev, path, match_files_iter, &ctx))
    goto fail;

  holy_free (ctx.dir);
  holy_device_close (dev);
  holy_free (device_name);
  holy_error_pop ();
  return ctx.files;

 fail:

  holy_free (ctx.dir);

  for (i = 0; ctx.files && ctx.files[i]; i++)
    holy_free (ctx.files[i]);

  holy_free (ctx.files);

  if (dev)
    holy_device_close (dev);

  holy_free (device_name);

  holy_error_pop ();
  return 0;
}

/* Context for check_file.  */
struct check_file_ctx
{
  const char *basename;
  int found;
};

/* Helper for check_file.  */
static int
check_file_iter (const char *name, const struct holy_dirhook_info *info,
		 void *data)
{
  struct check_file_ctx *ctx = data;

  if (ctx->basename[0] == 0
      || (info->case_insensitive ? holy_strcasecmp (name, ctx->basename) == 0
	  : holy_strcmp (name, ctx->basename) == 0))
    {
      ctx->found = 1;
      return 1;
    }
  
  return 0;
}

static int
check_file (const char *dir, const char *basename)
{
  struct check_file_ctx ctx = {
    .basename = basename,
    .found = 0
  };
  holy_fs_t fs;
  holy_device_t dev;
  const char *device_name, *path;

  device_name = holy_file_get_device_name (dir);
  dev = holy_device_open (device_name);
  if (! dev)
    goto fail;

  fs = holy_fs_probe (dev);
  if (! fs)
    goto fail;

  if (dir[0] == '(')
    {
      path = holy_strchr (dir, ')');
      if (!path)
	goto fail;
      path++;
    }
  else
    path = dir;

  fs->dir (dev, path[0] ? path : "/", check_file_iter, &ctx);
  if (holy_errno == 0 && basename[0] == 0)
    ctx.found = 1;

 fail:
  holy_errno = 0;

  return ctx.found;
}

static void
unescape (char *out, const char *in, const char *end)
{
  char *optr;
  const char *iptr;

  for (optr = out, iptr = in; iptr < end;)
    {
      if (*iptr == '\\' && iptr + 1 < end)
	{
	  *optr++ = iptr[1];
	  iptr += 2;
	  continue;
	}
      if (*iptr == '\\')
	break;
      *optr++ = *iptr++;
    }
  *optr = 0;
}

static holy_err_t
wildcard_expand (const char *s, char ***strs)
{
  const char *start;
  const char *regexop;
  const char *noregexop;
  char **paths = 0;
  int had_regexp = 0;

  unsigned i;
  regex_t regexp;

  *strs = 0;
  if (s[0] != '/' && s[0] != '(' && s[0] != '*')
    return 0;

  start = s;
  while (*start)
    {
      split_path (start, &noregexop, &regexop);

      if (noregexop == regexop)
	{
	  holy_dprintf ("expand", "no expansion needed\n");
	  if (paths == 0)
	    {
	      paths = holy_malloc (sizeof (char *) * 2);
	      if (!paths)
		goto fail;
	      paths[0] = holy_malloc (regexop - start + 1);
	      if (!paths[0])
		goto fail;
	      unescape (paths[0], start, regexop);
	      paths[1] = 0;
	    }
	  else
	    {
	      int j = 0;
	      for (i = 0; paths[i]; i++)
		{
		  char *o, *oend;
		  char *n;
		  char *p;
		  o = paths[i];
		  oend = o + holy_strlen (o);
		  n = holy_malloc ((oend - o) + (regexop - start) + 1);
		  if (!n)
		    goto fail;
		  holy_memcpy (n, o, oend - o);

		  unescape (n + (oend - o), start, regexop);
		  if (had_regexp)
		    p = holy_strrchr (n, '/');
		  else
		    p = 0;
		  if (!p)
		    {
		      holy_free (o);
		      paths[j++] = n;
		      continue;
		    }
		  *p = 0;
		  if (!check_file (n, p + 1))
		    {
		      holy_dprintf ("expand", "file <%s> in <%s> not found\n",
				    p + 1, n);
		      holy_free (o);
		      holy_free (n);
			      continue;
		    }
		  *p = '/';
		  holy_free (o);
		  paths[j++] = n;
		}
	      if (j == 0)
		{
		  holy_free (paths);
		  paths = 0;
		  goto done;
		}
	      paths[j] = 0;
	    }
	  holy_dprintf ("expand", "paths[0] = `%s'\n", paths[0]);
	  start = regexop;
	  continue;
	}

      if (make_regex (noregexop, regexop, &regexp))
	goto fail;

      had_regexp = 1;

      if (paths == 0)
	{
	  if (start == noregexop) /* device part has regexop */
	    paths = match_devices (&regexp, *start != '(');

	  else  /* device part explicit wo regexop */
	    paths = match_files ("", start, noregexop, &regexp);
	}
      else
	{
	  char **r = 0;

	  for (i = 0; paths[i]; i++)
	    {
	      char **p;

	      p = match_files (paths[i], start, noregexop, &regexp);
	      holy_free (paths[i]);
	      if (! p)
		continue;

	      r = merge (r, p);
	      if (! r)
		goto fail;
	    }
	  holy_free (paths);
	  paths = r;
	}

      regfree (&regexp);
      if (! paths)
	goto done;

      start = regexop;
    }

 done:

  *strs = paths;
  return 0;

 fail:

  for (i = 0; paths && paths[i]; i++)
    holy_free (paths[i]);
  holy_free (paths);
  regfree (&regexp);
  return holy_errno;
}
