/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/archelp.h>
#include <holy/err.h>
#include <holy/fs.h>
#include <holy/disk.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

static inline void
canonicalize (char *name)
{
  char *iptr, *optr;
  for (iptr = name, optr = name; *iptr; )
    {
      while (*iptr == '/')
	iptr++;
      if (iptr[0] == '.' && (iptr[1] == '/' || iptr[1] == 0))
	{
	  iptr++;
	  continue;
	}
      if (iptr[0] == '.' && iptr[1] == '.' && (iptr[2] == '/' || iptr[2] == 0))
	{
	  iptr += 2;
	  if (optr == name)
	    continue;
	  for (optr -= 2; optr >= name && *optr != '/'; optr--);
	  optr++;
	  continue;
	}
      while (*iptr && *iptr != '/')
	*optr++ = *iptr++;
      if (*iptr)
	*optr++ = *iptr++;
    }
  *optr = 0;
}

static holy_err_t
handle_symlink (struct holy_archelp_data *data,
		struct holy_archelp_ops *arcops,
		const char *fn, char **name,
		holy_uint32_t mode, int *restart)
{
  holy_size_t flen;
  char *target;
  char *ptr;
  char *lastslash;
  holy_size_t prefixlen;
  char *rest;
  char *linktarget;
  holy_size_t linktarget_len;

  *restart = 0;

  if ((mode & holy_ARCHELP_ATTR_TYPE) != holy_ARCHELP_ATTR_LNK
      || !arcops->get_link_target)
    return holy_ERR_NONE;
  flen = holy_strlen (fn);
  if (holy_memcmp (*name, fn, flen) != 0
      || ((*name)[flen] != 0 && (*name)[flen] != '/'))
    return holy_ERR_NONE;
  rest = *name + flen;
  lastslash = rest;
  if (*rest)
    rest++;
  while (lastslash >= *name && *lastslash != '/')
    lastslash--;
  if (lastslash >= *name)
    prefixlen = lastslash - *name;
  else
    prefixlen = 0;

  if (prefixlen)
    prefixlen++;

  linktarget = arcops->get_link_target (data);
  if (!linktarget)
    return holy_errno;
  if (linktarget[0] == '\0')
    return holy_ERR_NONE;
  linktarget_len = holy_strlen (linktarget);
  target = holy_malloc (linktarget_len + holy_strlen (*name) + 2);
  if (!target)
    return holy_errno;

  holy_strcpy (target + prefixlen, linktarget);
  holy_free (linktarget);
  if (target[prefixlen] == '/')
    {
      ptr = holy_stpcpy (target, target + prefixlen);
      ptr = holy_stpcpy (ptr, rest);
      *ptr = 0;
      holy_dprintf ("archelp", "symlink redirected %s to %s\n",
		    *name, target);
      holy_free (*name);

      canonicalize (target);
      *name = target;
      *restart = 1;
      return holy_ERR_NONE;
    }
  if (prefixlen)
    {
      holy_memcpy (target, *name, prefixlen);
      target[prefixlen-1] = '/';
    }
  holy_strcpy (target + prefixlen + linktarget_len, rest);
  holy_dprintf ("archelp", "symlink redirected %s to %s\n",
		*name, target);
  holy_free (*name);
  canonicalize (target);
  *name = target;
  *restart = 1;
  return holy_ERR_NONE;
}

holy_err_t
holy_archelp_dir (struct holy_archelp_data *data,
		  struct holy_archelp_ops *arcops,
		  const char *path_in,
		  holy_fs_dir_hook_t hook, void *hook_data)
{
  char *prev, *name, *path, *ptr;
  holy_size_t len;
  int symlinknest = 0;

  path = holy_strdup (path_in + 1);
  if (!path)
    return holy_errno;
  canonicalize (path);
  for (ptr = path + holy_strlen (path) - 1; ptr >= path && *ptr == '/'; ptr--)
    *ptr = 0;

  prev = 0;

  len = holy_strlen (path);
  while (1)
    {
      holy_int32_t mtime;
      holy_uint32_t mode;
      holy_err_t err;

      if (arcops->find_file (data, &name, &mtime, &mode))
	goto fail;

      if (mode == holy_ARCHELP_ATTR_END)
	break;

      canonicalize (name);

      if (holy_memcmp (path, name, len) == 0
	  && (name[len] == 0 || name[len] == '/' || len == 0))
	{
	  char *p, *n;

	  n = name + len;
	  while (*n == '/')
	    n++;

	  p = holy_strchr (n, '/');
	  if (p)
	    *p = 0;

	  if (((!prev) || (holy_strcmp (prev, name) != 0)) && *n != 0)
	    {
	      struct holy_dirhook_info info;
	      holy_memset (&info, 0, sizeof (info));
	      info.dir = (p != NULL) || ((mode & holy_ARCHELP_ATTR_TYPE)
					 == holy_ARCHELP_ATTR_DIR);
	      if (!(mode & holy_ARCHELP_ATTR_NOTIME))
		{
		  info.mtime = mtime;
		  info.mtimeset = 1;
		}
	      if (hook (n, &info, hook_data))
		{
		  holy_free (name);
		  goto fail;
		}
	      holy_free (prev);
	      prev = name;
	    }
	  else
	    {
	      int restart = 0;
	      err = handle_symlink (data, arcops, name,
				    &path, mode, &restart);
	      holy_free (name);
	      if (err)
		goto fail;
	      if (restart)
		{
		  len = holy_strlen (path);
		  if (++symlinknest == 8)
		    {
		      holy_error (holy_ERR_SYMLINK_LOOP,
				  N_("too deep nesting of symlinks"));
		      goto fail;
		    }
		  arcops->rewind (data);
		}
	    }
	}
      else
	holy_free (name);
    }

fail:

  holy_free (path);
  holy_free (prev);

  return holy_errno;
}

holy_err_t
holy_archelp_open (struct holy_archelp_data *data,
		   struct holy_archelp_ops *arcops,
		   const char *name_in)
{
  char *fn;
  char *name = holy_strdup (name_in + 1);
  int symlinknest = 0;

  if (!name)
    return holy_errno;

  canonicalize (name);

  while (1)
    {
      holy_uint32_t mode;
      holy_int32_t mtime;
      int restart;
      
      if (arcops->find_file (data, &fn, &mtime, &mode))
	goto fail;

      if (mode == holy_ARCHELP_ATTR_END)
	{
	  holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), name_in);
	  break;
	}

      canonicalize (fn);

      if (handle_symlink (data, arcops, fn, &name, mode, &restart))
	{
	  holy_free (fn);
	  goto fail;
	}

      if (restart)
	{
	  arcops->rewind (data);
	  if (++symlinknest == 8)
	    {
	      holy_error (holy_ERR_SYMLINK_LOOP,
			  N_("too deep nesting of symlinks"));
	      goto fail;
	    }
	  goto no_match;
	}

      if (holy_strcmp (name, fn) != 0)
	goto no_match;

      holy_free (fn);
      holy_free (name);

      return holy_ERR_NONE;

    no_match:

      holy_free (fn);
    }

fail:
  holy_free (name);

  return holy_errno;
}
