/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <holy/emu/getroot.h>
#include <holy/mm.h>

#ifdef HAVE_DEVICE_MAPPER

#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if defined(MAJOR_IN_MKDEV)
#include <sys/mkdev.h>
#elif defined(MAJOR_IN_SYSMACROS)
#include <sys/sysmacros.h>
#endif

#include <libdevmapper.h>

#include <holy/types.h>
#include <holy/util/misc.h>

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/emu/hostdisk.h>

static int
holy_util_open_dm (const char *os_dev, struct dm_tree **tree,
		   struct dm_tree_node **node)
{
  uint32_t maj, min;
  struct stat st;

  *node = NULL;
  *tree = NULL;

  if (stat (os_dev, &st) < 0)
    return 0;

  maj = major (st.st_rdev);
  min = minor (st.st_rdev);

  if (!dm_is_dm_major (maj))
    return 0;

  *tree = dm_tree_create ();
  if (! *tree)
    {
      holy_puts_ (N_("Failed to create `device-mapper' tree"));
      holy_dprintf ("hostdisk", "dm_tree_create failed\n");
      return 0;
    }

  if (! dm_tree_add_dev (*tree, maj, min))
    {
      holy_dprintf ("hostdisk", "dm_tree_add_dev failed\n");
      dm_tree_free (*tree);
      *tree = NULL;
      return 0;
    }

  *node = dm_tree_find_node (*tree, maj, min);
  if (! *node)
    {
      holy_dprintf ("hostdisk", "dm_tree_find_node failed\n");
      dm_tree_free (*tree);
      *tree = NULL;
      return 0;
    }
  return 1;
}

static char *
get_dm_uuid (const char *os_dev)
{
  struct dm_tree *tree;
  struct dm_tree_node *node;
  const char *node_uuid;
  char *ret;

  if (!holy_util_open_dm (os_dev, &tree, &node))
    return NULL;

  node_uuid = dm_tree_node_get_uuid (node);
  if (! node_uuid)
    {
      holy_dprintf ("hostdisk", "%s has no DM uuid\n", os_dev);
      dm_tree_free (tree);
      return NULL;
    }

  ret = holy_strdup (node_uuid);

  dm_tree_free (tree);

  return ret;
}

enum holy_dev_abstraction_types
holy_util_get_dm_abstraction (const char *os_dev)
{
  char *uuid;

  uuid = get_dm_uuid (os_dev);

  if (uuid == NULL)
    return holy_DEV_ABSTRACTION_NONE;

  if (strncmp (uuid, "LVM-", 4) == 0)
    {
      holy_free (uuid);
      return holy_DEV_ABSTRACTION_LVM;
    }
  if (strncmp (uuid, "CRYPT-LUKS1-", 12) == 0)
    {
      holy_free (uuid);
      return holy_DEV_ABSTRACTION_LUKS;
    }

  holy_free (uuid);
  return holy_DEV_ABSTRACTION_NONE;
}

void
holy_util_pull_devmapper (const char *os_dev)
{
  struct dm_tree *tree;
  struct dm_tree_node *node;
  struct dm_tree_node *child;
  void *handle = NULL;
  char *lastsubdev = NULL;
  char *uuid;

  uuid = get_dm_uuid (os_dev);

  if (!holy_util_open_dm (os_dev, &tree, &node))
    {
      holy_free (uuid);
      return;
    }

  while ((child = dm_tree_next_child (&handle, node, 0)))
    {
      const struct dm_info *dm = dm_tree_node_get_info (child);
      char *subdev;
      if (!dm)
	continue;
      subdev = holy_find_device ("/dev", makedev (dm->major, dm->minor));
      if (subdev)
	{
	  lastsubdev = subdev;
	  holy_util_pull_device (subdev);
	}
    }
  if (uuid && strncmp (uuid, "CRYPT-LUKS1-", sizeof ("CRYPT-LUKS1-") - 1) == 0
      && lastsubdev)
    {
      char *grdev = holy_util_get_holy_dev (lastsubdev);
      dm_tree_free (tree);
      if (grdev)
	{
	  holy_err_t err;
	  err = holy_cryptodisk_cheat_mount (grdev, os_dev);
	  if (err)
	    holy_util_error (_("can't mount encrypted volume `%s': %s"),
			     lastsubdev, holy_errmsg);
	}
      holy_free (grdev);
    }
  else
    dm_tree_free (tree);
  holy_free (uuid);
}

char *
holy_util_devmapper_part_to_disk (struct stat *st,
				  int *is_part, const char *path)
{
  int major, minor;

  if (holy_util_get_dm_node_linear_info (st->st_rdev,
					 &major, &minor, 0))
    {
      *is_part = 1;
      return holy_find_device ("/dev", makedev (major, minor));
    }
  *is_part = 0;
  return xstrdup (path);
}

char *
holy_util_get_devmapper_holy_dev (const char *os_dev)
{
  char *uuid, *optr;
  char *holy_dev;

  uuid = get_dm_uuid (os_dev);
  if (!uuid)
    return NULL;

  switch (holy_util_get_dev_abstraction (os_dev))
    {
    case holy_DEV_ABSTRACTION_LVM:
      {
	unsigned i;
	int dashes[] = { 0, 6, 10, 14, 18, 22, 26, 32, 38, 42, 46, 50, 54, 58};

	holy_dev = xmalloc (holy_strlen (uuid) + 40);
	optr = holy_stpcpy (holy_dev, "lvmid/");
	for (i = 0; i < ARRAY_SIZE (dashes) - 1; i++)
	  {
	    memcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i],
		    dashes[i+1] - dashes[i]);
	    optr += dashes[i+1] - dashes[i];
	    *optr++ = '-';
	  }
	optr = stpcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i]);
	*optr = '\0';
	holy_dev[sizeof("lvmid/xxxxxx-xxxx-xxxx-xxxx-xxxx-xxxx-xxxxxx") - 1]
	  = '/';
	free (uuid);
	return holy_dev;
      }

    case holy_DEV_ABSTRACTION_LUKS:
      {
	char *dash;

	dash = holy_strchr (uuid + sizeof ("CRYPT-LUKS1-") - 1, '-');
	if (dash)
	  *dash = 0;
	holy_dev = holy_xasprintf ("cryptouuid/%s",
				   uuid + sizeof ("CRYPT-LUKS1-") - 1);
	holy_free (uuid);
	return holy_dev;
      }

    default:
      holy_free (uuid);
      return NULL;
    }
}

char *
holy_util_get_vg_uuid (const char *os_dev)
{
  char *uuid, *vgid;
  int dashes[] = { 0, 6, 10, 14, 18, 22, 26, 32};
  unsigned i;
  char *optr;

  uuid = get_dm_uuid (os_dev);
  if (!uuid)
    return NULL;

  vgid = xmalloc (holy_strlen (uuid));
  optr = vgid;
  for (i = 0; i < ARRAY_SIZE (dashes) - 1; i++)
    {
      memcpy (optr, uuid + sizeof ("LVM-") - 1 + dashes[i],
	      dashes[i+1] - dashes[i]);
      optr += dashes[i+1] - dashes[i];
      *optr++ = '-';
    }
  optr--;
  *optr = '\0';
  holy_free (uuid);
  return vgid;
}

void
holy_util_devmapper_cleanup (void)
{
  dm_lib_release ();
}

#else
void
holy_util_pull_devmapper (const char *os_dev __attribute__ ((unused)))
{
  return;
}

void
holy_util_devmapper_cleanup (void)
{
}

enum holy_dev_abstraction_types
holy_util_get_dm_abstraction (const char *os_dev __attribute__ ((unused)))
{
  return holy_DEV_ABSTRACTION_NONE;
}

char *
holy_util_get_vg_uuid (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

char *
holy_util_devmapper_part_to_disk (struct stat *st __attribute__ ((unused)),
				  int *is_part __attribute__ ((unused)),
				  const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

char *
holy_util_get_devmapper_holy_dev (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}

#endif
