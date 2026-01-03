/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/zfs/zfs.h>
#include <holy/device.h>
#include <holy/file.h>
#include <holy/command.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static inline void
print_tabs (int n)
{
  int i;

  for (i = 0; i < n; i++)
    holy_printf (" ");
}

static holy_err_t
print_state (char *nvlist, int tab)
{
  holy_uint64_t ival;
  int isok = 1;

  print_tabs (tab);

  if (holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_REMOVED, &ival))
    {
      holy_puts_ (N_("Virtual device is removed"));
      isok = 0;
    }

  if (holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_FAULTED, &ival))
    {
      holy_puts_ (N_("Virtual device is faulted"));
      isok = 0;
    }

  if (holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_OFFLINE, &ival))
    {
      holy_puts_ (N_("Virtual device is offline"));
      isok = 0;
    }

  if (holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_FAULTED, &ival))
    /* TRANSLATORS: degraded doesn't mean broken but that some of
       component are missing but virtual device as whole is still usable.  */
    holy_puts_ (N_("Virtual device is degraded"));

  if (isok)
    holy_puts_ (N_("Virtual device is online"));
  holy_xputs ("\n");

  return holy_ERR_NONE;
}

static holy_err_t
print_vdev_info (char *nvlist, int tab)
{
  char *type = 0;

  type = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_TYPE);

  if (!type)
    {
      print_tabs (tab);
      holy_puts_ (N_("Incorrect virtual device: no type available"));
      return holy_errno;
    }

  if (holy_strcmp (type, VDEV_TYPE_DISK) == 0)
    {
      char *bootpath = 0;
      char *path = 0;
      char *devid = 0;

      print_tabs (tab);
      /* TRANSLATORS: The virtual devices form a tree (in graph-theoretical
	 sense). The nodes like mirror or raidz have children: member devices.
	 The "real" devices which actually store data are called "leafs"
	 (again borrowed from graph theory) and can be either disks
	 (or partitions) or files.  */
      holy_puts_ (N_("Leaf virtual device (file or disk)"));

      print_state (nvlist, tab);

      bootpath =
	holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_PHYS_PATH);
      print_tabs (tab);
      if (!bootpath)
	holy_puts_ (N_("Bootpath: unavailable\n"));
      else
	holy_printf_ (N_("Bootpath: %s\n"), bootpath);

      path = holy_zfs_nvlist_lookup_string (nvlist, "path");
      print_tabs (tab);
      if (!path)
	holy_puts_ (N_("Path: unavailable"));
      else
	holy_printf_ (N_("Path: %s\n"), path);

      devid = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_DEVID);
      print_tabs (tab);
      if (!devid)
	holy_puts_ (N_("Devid: unavailable"));
      else
	holy_printf_ (N_("Devid: %s\n"), devid);
      holy_free (bootpath);
      holy_free (devid);
      holy_free (path);
      holy_free (type);
      return holy_ERR_NONE;
    }
  char is_mirror=(holy_strcmp(type,VDEV_TYPE_MIRROR) == 0);
  char is_raidz=(holy_strcmp(type,VDEV_TYPE_RAIDZ) == 0);
  holy_free (type);

  if (is_mirror || is_raidz)
    {
      int nelm, i;

      nelm = holy_zfs_nvlist_lookup_nvlist_array_get_nelm
	(nvlist, ZPOOL_CONFIG_CHILDREN);

      if(is_mirror){
	 holy_puts_ (N_("This VDEV is a mirror"));
      }
      else if(is_raidz){
	 holy_uint64_t parity;
	 holy_zfs_nvlist_lookup_uint64(nvlist,"nparity",&parity);
	 holy_printf_ (N_("This VDEV is a RAIDZ%llu\n"),(unsigned long long)parity);
      }
      print_tabs (tab);
      if (nelm <= 0)
	{
	  holy_puts_ (N_("Incorrect VDEV"));
	  return holy_ERR_NONE;
	}
      holy_printf_ (N_("VDEV with %d children\n"), nelm);
      print_state (nvlist, tab);
      for (i = 0; i < nelm; i++)
	{
	  char *child;

	  child = holy_zfs_nvlist_lookup_nvlist_array
	    (nvlist, ZPOOL_CONFIG_CHILDREN, i);

	  print_tabs (tab);
	  if (!child)
	    {
	      /* TRANSLATORS: it's the element carying the number %d, not
		 total element number. And the number itself is fine,
		 only the element isn't.
	      */
	      holy_printf_ (N_("VDEV element number %d isn't correct\n"), i);
	      continue;
	    }

	  /* TRANSLATORS: it's the element carying the number %d, not
	     total element number. This is used in enumeration
	     "Element number 1", "Element number 2", ... */
	  holy_printf_ (N_("VDEV element number %d:\n"), i);
	  print_vdev_info (child, tab + 1);

	  holy_free (child);
	}
      return holy_ERR_NONE;
    }

  print_tabs (tab);
  holy_printf_ (N_("Unknown virtual device type: %s\n"), type);

  return holy_ERR_NONE;
}

static holy_err_t
get_bootpath (char *nvlist, char **bootpath, char **devid)
{
  char *type = 0;

  type = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_TYPE);

  if (!type)
    return holy_errno;

  if (holy_strcmp (type, VDEV_TYPE_DISK) == 0)
    {
      *bootpath = holy_zfs_nvlist_lookup_string (nvlist,
						 ZPOOL_CONFIG_PHYS_PATH);
      *devid = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_DEVID);
      if (!*bootpath || !*devid)
	{
	  holy_free (*bootpath);
	  holy_free (*devid);
	  *bootpath = 0;
	  *devid = 0;
	}
      return holy_ERR_NONE;
    }

  if (holy_strcmp (type, VDEV_TYPE_MIRROR) == 0)
    {
      int nelm, i;

      nelm = holy_zfs_nvlist_lookup_nvlist_array_get_nelm
	(nvlist, ZPOOL_CONFIG_CHILDREN);

      for (i = 0; i < nelm; i++)
	{
	  char *child;

	  child = holy_zfs_nvlist_lookup_nvlist_array (nvlist,
						       ZPOOL_CONFIG_CHILDREN,
						       i);

	  get_bootpath (child, bootpath, devid);

	  holy_free (child);

	  if (*bootpath && *devid)
	    return holy_ERR_NONE;
	}
    }

  return holy_ERR_NONE;
}

static const char *poolstates[] = {
  /* TRANSLATORS: Here we speak about ZFS pools it's semi-marketing,
     semi-technical term by Sun/Oracle and should be translated in sync with
     other ZFS-related software and documentation.  */
  [POOL_STATE_ACTIVE] = N_("Pool state: active"),
  [POOL_STATE_EXPORTED] = N_("Pool state: exported"),
  [POOL_STATE_DESTROYED] = N_("Pool state: destroyed"),
  [POOL_STATE_SPARE] = N_("Pool state: reserved for hot spare"),
  [POOL_STATE_L2CACHE] = N_("Pool state: level 2 ARC device"),
  [POOL_STATE_UNINITIALIZED] = N_("Pool state: uninitialized"),
  [POOL_STATE_UNAVAIL] = N_("Pool state: unavailable"),
  [POOL_STATE_POTENTIALLY_ACTIVE] = N_("Pool state: potentially active")
};

static holy_err_t
holy_cmd_zfsinfo (holy_command_t cmd __attribute__ ((unused)), int argc,
		  char **args)
{
  holy_device_t dev;
  char *devname;
  holy_err_t err;
  char *nvlist = 0;
  char *nv = 0;
  char *poolname;
  holy_uint64_t guid;
  holy_uint64_t pool_state;
  int found;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  if (args[0][0] == '(' && args[0][holy_strlen (args[0]) - 1] == ')')
    {
      devname = holy_strdup (args[0] + 1);
      if (devname)
	devname[holy_strlen (devname) - 1] = 0;
    }
  else
    devname = holy_strdup (args[0]);
  if (!devname)
    return holy_errno;

  dev = holy_device_open (devname);
  holy_free (devname);
  if (!dev)
    return holy_errno;

  err = holy_zfs_fetch_nvlist (dev, &nvlist);

  holy_device_close (dev);

  if (err)
    return err;

  poolname = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_POOL_NAME);
  if (!poolname)
    holy_puts_ (N_("Pool name: unavailable"));
  else
    holy_printf_ (N_("Pool name: %s\n"), poolname);

  found =
    holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_POOL_GUID, &guid);
  if (!found)
    holy_puts_ (N_("Pool GUID: unavailable"));
  else
    holy_printf_ (N_("Pool GUID: %016llx\n"), (long long unsigned) guid);

  found = holy_zfs_nvlist_lookup_uint64 (nvlist, ZPOOL_CONFIG_POOL_STATE,
					 &pool_state);
  if (!found)
    holy_puts_ (N_("Unable to retrieve pool state"));
  else if (pool_state >= ARRAY_SIZE (poolstates))
    holy_puts_ (N_("Unrecognized pool state"));
  else
    holy_puts_ (poolstates[pool_state]);

  nv = holy_zfs_nvlist_lookup_nvlist (nvlist, ZPOOL_CONFIG_VDEV_TREE);

  if (!nv)
    /* TRANSLATORS: There are undetermined number of virtual devices
       in a device tree, not just one.
     */
    holy_puts_ (N_("No virtual device tree available"));
  else
    print_vdev_info (nv, 1);

  holy_free (nv);
  holy_free (nvlist);

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_zfs_bootfs (holy_command_t cmd __attribute__ ((unused)), int argc,
		     char **args)
{
  holy_device_t dev;
  char *devname;
  holy_err_t err;
  char *nvlist = 0;
  char *nv = 0;
  char *bootpath = 0, *devid = 0;
  char *fsname;
  char *bootfs;
  char *poolname;
  holy_uint64_t mdnobj;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  devname = holy_file_get_device_name (args[0]);
  if (holy_errno)
    return holy_errno;

  dev = holy_device_open (devname);
  holy_free (devname);
  if (!dev)
    return holy_errno;

  err = holy_zfs_fetch_nvlist (dev, &nvlist);

  fsname = holy_strchr (args[0], ')');
  if (fsname)
    fsname++;
  else
    fsname = args[0];

  if (!err)
    err = holy_zfs_getmdnobj (dev, fsname, &mdnobj);

  holy_device_close (dev);

  if (err)
    return err;

  poolname = holy_zfs_nvlist_lookup_string (nvlist, ZPOOL_CONFIG_POOL_NAME);
  if (!poolname)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_FS, "No poolname found");
      return holy_errno;
    }

  nv = holy_zfs_nvlist_lookup_nvlist (nvlist, ZPOOL_CONFIG_VDEV_TREE);

  if (nv)
    get_bootpath (nv, &bootpath, &devid);

  holy_free (nv);
  holy_free (nvlist);

  bootfs = holy_xasprintf ("zfs-bootfs=%s/%llu%s%s%s%s%s%s",
			   poolname, (unsigned long long) mdnobj,
			   bootpath ? ",bootpath=\"" : "",
			   bootpath ? : "",
			   bootpath ? "\"" : "",
			   devid ? ",diskdevid=\"" : "",
			   devid ? : "",
			   devid ? "\"" : "");
  if (!bootfs)
    return holy_errno;
  if (argc >= 2)
    holy_env_set (args[1], bootfs);
  else
    holy_printf ("%s\n", bootfs);

  holy_free (bootfs);
  holy_free (poolname);
  holy_free (bootpath);
  holy_free (devid);

  return holy_ERR_NONE;
}


static holy_command_t cmd_info, cmd_bootfs;

holy_MOD_INIT (zfsinfo)
{
  cmd_info = holy_register_command ("zfsinfo", holy_cmd_zfsinfo,
				    N_("DEVICE"),
				    N_("Print ZFS info about DEVICE."));
  cmd_bootfs = holy_register_command ("zfs-bootfs", holy_cmd_zfs_bootfs,
				      N_("FILESYSTEM [VARIABLE]"),
				      N_("Print ZFS-BOOTFSOBJ or store it into VARIABLE"));
}

holy_MOD_FINI (zfsinfo)
{
  holy_unregister_command (cmd_info);
  holy_unregister_command (cmd_bootfs);
}
