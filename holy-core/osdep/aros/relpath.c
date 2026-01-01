/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <holy/util/misc.h>

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>

#include <string.h>
#include <dos/dos.h>
#include <dos/filesystem.h>
#include <dos/exall.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <devices/trackdisk.h>

char *
holy_make_system_path_relative_to_its_root (const char *path)
{
  char *p;
  unsigned char *tmp;
  char *ret;
  BPTR lck;

  if (path[0] == '/' && path[1] == '/' && path[2] == ':')
    return xstrdup (path);

  tmp = xmalloc (2048);

  lck = Lock ((const unsigned char *) path, SHARED_LOCK);
  if (!lck || !NameFromLock (lck, tmp, 2040))
    {
      free (tmp);
      tmp = (unsigned char *) xstrdup (path);
    }
  if (lck)
    UnLock (lck);
  p = strchr ((char *) tmp, ':');
  if (!p)
    return (char *) tmp;
  if (p[1] == '/' || p[1] == '\0')
    {
      ret = xstrdup (p + 1);
    }
  else
    {
      ret = xmalloc (strlen (p + 1) + 2);
      ret[0] = '/';
      strcpy (ret + 1, p + 1);
    }

  free (tmp);
  return ret;
}
