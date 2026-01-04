/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <holy/util/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/mm.h>

/* This function never prints trailing slashes (so that its output
   can be appended a slash unconditionally).  */
char *
holy_make_system_path_relative_to_its_root (const char *path)
{
  struct stat st;
  char *p, *buf, *buf2, *buf3, *ret;
  uintptr_t offset = 0;
  dev_t num;
  size_t len;
  char *poolfs = NULL;

  /* canonicalize.  */
  p = holy_canonicalize_file_name (path);
  if (p == NULL)
    holy_util_error (_("failed to get canonical path of `%s'"), path);

#ifdef __linux__
  ret = holy_make_system_path_relative_to_its_root_os (p);
  if (ret)
    {
      free (p);
      return ret;
    }
#endif

  /* For ZFS sub-pool filesystems.  */
#ifndef __HAIKU__
  {
    char *dummy;
    holy_find_zpool_from_dir (p, &dummy, &poolfs);
  }
#endif

  len = strlen (p) + 1;
  buf = xstrdup (p);
  free (p);

  if (stat (buf, &st) < 0)
    holy_util_error (_("cannot stat `%s': %s"), buf, strerror (errno));

  buf2 = xstrdup (buf);
  num = st.st_dev;

  /* This loop sets offset to the number of chars of the root
     directory we're inspecting.  */
  while (1)
    {
      p = strrchr (buf, '/');
      if (p == NULL)
	/* This should never happen.  */
	holy_util_error ("%s",
			 /* TRANSLATORS: canonical pathname is the
			    complete one e.g. /etc/fstab. It has
			    to contain `/' normally, if it doesn't
			    we're in trouble and throw this error.  */
			 _("no `/' in canonical filename"));
      if (p != buf)
	*p = 0;
      else
	*++p = 0;

      if (stat (buf, &st) < 0)
	holy_util_error (_("cannot stat `%s': %s"), buf, strerror (errno));

      /* buf is another filesystem; we found it.  */
      if (st.st_dev != num)
	{
	  /* offset == 0 means path given is the mount point.
	     This works around special-casing of "/" in Un*x.  This function never
	     prints trailing slashes (so that its output can be appended a slash
	     unconditionally).  Each slash in is considered a preceding slash, and
	     therefore the root directory is an empty string.  */
	  if (offset == 0)
	    {
	      free (buf);
	      free (buf2);
	      if (poolfs)
		return xasprintf ("/%s/@", poolfs);
	      return xstrdup ("");
	    }
	  else
	    break;
	}

      offset = p - buf;
      /* offset == 1 means root directory.  */
      if (offset == 1)
	{
	  /* Include leading slash.  */
	  offset = 0;
	  break;
	}
    }
  free (buf);
  buf3 = xstrdup (buf2 + offset);
  buf2[offset] = 0;
  
  free (buf2);

  /* Remove trailing slashes, return empty string if root directory.  */
  len = strlen (buf3);
  while (len > 0 && buf3[len - 1] == '/')
    {
      buf3[len - 1] = '\0';
      len--;
    }

  if (poolfs)
    {
      ret = xasprintf ("/%s/@%s", poolfs, buf3);
      free (buf3);
    }
  else
    ret = buf3;

  return ret;
}
