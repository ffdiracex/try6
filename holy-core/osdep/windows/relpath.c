/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <holy/types.h>

#include <holy/util/misc.h>

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/charset.h>
#include <holy/util/windows.h>
#include <windows.h>
#include <winioctl.h>

static size_t
tclen (const TCHAR *s)
{
  const TCHAR *s0 = s;
  while (*s)
      s++;
  return s - s0;
}

char *
holy_make_system_path_relative_to_its_root (const char *path)
{
  TCHAR *dirwindows, *mntpointwindows;
  TCHAR *ptr;
  size_t offset, flen;
  TCHAR *ret;
  char *cret;

  dirwindows = holy_util_get_windows_path (path);
  if (!dirwindows)
    return xstrdup (path);

  mntpointwindows = holy_get_mount_point (dirwindows);

  if (!mntpointwindows)
    {
      offset = 0;
      if (dirwindows[0] && dirwindows[1] == ':')
	offset = 2;
    }
  offset = tclen (mntpointwindows);
  free (mntpointwindows);
  flen = tclen (dirwindows);
  if (offset > flen)
    {
      offset = 0;
      if (dirwindows[0] && dirwindows[1] == ':')
	offset = 2;
    }
  ret = xmalloc (sizeof (ret[0]) * (flen - offset + 2));
  if (dirwindows[offset] != '\\'
      && dirwindows[offset] != '/'
      && dirwindows[offset])
    {
      ret[0] = '\\';
      memcpy (ret + 1, dirwindows + offset, (flen - offset + 1) * sizeof (ret[0]));
    }
  else
    memcpy (ret, dirwindows + offset, (flen - offset + 1) * sizeof (ret[0]));

  free (dirwindows);

  for (ptr = ret; *ptr; ptr++)
    if (*ptr == '\\')
      *ptr = '/';

  cret = holy_util_tchar_to_utf8 (ret);
  free (ret);

  return cret;
}
