/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/types.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/device.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/fs.h>
#include <holy/partition.h>
#include <holy/msdos_partition.h>
#include <holy/gpt_partition.h>
#include <holy/emu/hostdisk.h>
#include <holy/emu/getroot.h>
#include <holy/term.h>
#include <holy/env.h>
#include <holy/diskfilter.h>
#include <holy/i18n.h>
#include <holy/emu/misc.h>
#include <holy/util/ofpath.h>
#include <holy/crypto.h>
#include <holy/cryptodisk.h>

#include <string.h>

/* Since OF path names can have "," characters in them, and holy
   internally uses "," to indicate partitions (unlike OF which uses
   ":" for this purpose) we escape such commas.  */
static char *
escape_of_path (const char *orig_path)
{
  char *new_path, *d, c;
  const char *p;

  if (!strchr (orig_path, ','))
    return (char *) xstrdup (orig_path);

  new_path = xmalloc (strlen (orig_path) * 2 + 1);

  p = orig_path;
  d = new_path;
  while ((c = *p++) != '\0')
    {
      if (c == ',')
	*d++ = '\\';
      *d++ = c;
    }
  *d = 0;

  return new_path;
}

char *
holy_util_guess_bios_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = holy_canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if ((ptr[0] == 's' || ptr[0] == 'h') && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("hd%d", num);
    }
  if (ptr[0] == 'f' && ptr[1] == 'd')
    {
      int num = atoi (ptr + 2);
      free (canon);
      return xasprintf ("fd%d", num);
    }
  free (canon);
  return NULL;
}

char *
holy_util_guess_efi_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = holy_canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if ((ptr[0] == 's' || ptr[0] == 'h') && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("hd%d", num);
    }
  if (ptr[0] == 'f' && ptr[1] == 'd')
    {
      int num = atoi (ptr + 2);
      free (canon);
      return xasprintf ("fd%d", num);
    }
  free (canon);
  return NULL;
}

char *
holy_util_guess_baremetal_drive (const char *orig_path)
{
  char *canon;
  char *ptr;
  canon = holy_canonicalize_file_name (orig_path);
  if (!canon)
    return NULL;
  ptr = strrchr (orig_path, '/');
  if (ptr)
    ptr++;
  else
    ptr = canon;
  if (ptr[0] == 'h' && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("ata%d", num);
    }
  if (ptr[0] == 's' && ptr[1] == 'd')
    {
      int num = ptr[2] - 'a';
      free (canon);
      return xasprintf ("ahci%d", num);
    }
  free (canon);
  return NULL;
}

void
holy_util_fprint_full_disk_name (FILE *f,
				 const char *drive, holy_device_t dev)
{
  char *dname = escape_of_path (drive);
  if (dev->disk->partition)
    {
      char *pname = holy_partition_get_name (dev->disk->partition);
      fprintf (f, "%s,%s", dname, pname);
      free (pname);
    }
  else
    fprintf (f, "%s", dname);
  free (dname);
} 
