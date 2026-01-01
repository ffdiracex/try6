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
holy_util_part_to_disk (const char *dev,
			struct stat *st __attribute__ ((unused)),
			int *is_part)
{
  const char *dname;
  char *ret;
  struct DosList *dl;
  struct DeviceNode *dn;
  struct FileSysStartupMsg *fm;
  struct DosEnvec *envec;

  if (dev[0] != '/' || dev[1] != '/' || dev[2] != ':')
    return xstrdup (dev);

  dname = dev + 3;
  dl = LockDosList(LDF_READ);

  if (!dl)
    {
      return xstrdup (dev);
    }

  dn = (struct DeviceNode *) FindDosEntry (dl, (unsigned char *) dname,
					   LDF_DEVICES);
  UnLockDosList (LDF_READ);
  if (!dn)
    return xstrdup (dev);

  fm = (struct FileSysStartupMsg *) BADDR(dn->dn_Startup);
  envec = (struct DosEnvec *) fm->fssm_Environ;

  if (envec->de_LowCyl == 0)
    return xstrdup (dev);

  *is_part = 1;

  ret = xasprintf ("//:%s/%lx/%lx", fm->fssm_Device, fm->fssm_Unit, (unsigned long) fm->fssm_Flags);

  return ret;
}

enum holy_dev_abstraction_types
holy_util_get_dev_abstraction_os (const char *os_dev __attribute__((unused)))
{
  return holy_DEV_ABSTRACTION_NONE;
}

int
holy_util_pull_device_os (const char *os_dev __attribute__ ((unused)),
			  enum holy_dev_abstraction_types ab __attribute__ ((unused)))
{
  return 0;
}

char *
holy_util_get_holy_dev_os (const char *os_dev __attribute__ ((unused)))
{
  return NULL;
}


holy_disk_addr_t
holy_util_find_partition_start_os (const char *dev)
{
  const char *dname;
  struct DosList *dl;
  struct DeviceNode *dn;
  struct FileSysStartupMsg *fm;
  struct DosEnvec *envec;

  if (dev[0] != '/' || dev[1] != '/' || dev[2] != ':')
    return 0;

  dname = dev + 3;
  dl = LockDosList(LDF_READ);
  if (!dl)
    {
      return 0;
    }
  dn = (struct DeviceNode *) FindDosEntry (dl, (unsigned char *) dname,
					   LDF_DEVICES);
  UnLockDosList (LDF_READ);
  if (!dn)
    return 0;

  fm = (struct FileSysStartupMsg *) BADDR(dn->dn_Startup);
  envec = (struct DosEnvec *) fm->fssm_Environ;

  return (((holy_uint64_t) envec->de_Surfaces
	  * (holy_uint64_t) envec->de_BlocksPerTrack
	  * (holy_uint64_t) envec->de_LowCyl)
	  * (holy_uint64_t) envec->de_SizeBlock) >> 7;
}

char **
holy_guess_root_devices (const char *path)
{
  char **os_dev = NULL;
  struct InfoData id;
  BPTR lck;
  struct DeviceList *dl;
  struct DosList *dl2;
  size_t sz;
  const char *nm;

  lck = Lock ((const unsigned char *) path, SHARED_LOCK);
  if (!lck)
    holy_util_info ("Lock(%s) failed", path);
  if (!lck || !Info (lck, &id))
    {
      char *p;
      if (lck)
	UnLock (lck);
      holy_util_info ("Info(%s) failed", path);
      os_dev = xmalloc (2 * sizeof (os_dev[0]));
      sz = strlen (path);
      os_dev[0] = xmalloc (sz + 5);
      os_dev[0][0] = '/';
      os_dev[0][1] = '/';
      os_dev[0][2] = ':';
      memcpy (os_dev[0] + 3, path, sz);
      os_dev[0][sz + 3] = ':';
      os_dev[0][sz + 4] = '\0';
      p = strchr (os_dev[0] + 3, ':');
      *p = '\0';
      os_dev[1] = NULL;
      return os_dev;
    }
  dl = BADDR (id.id_VolumeNode);

  if (!dl->dl_Task)
    holy_util_error ("unsupported device %s", dl->dl_Name);

  holy_util_info ("dl=%p, dl->dl_Name=%s, dl->dl_Task=%p",
		  dl, dl->dl_Name,
		  dl->dl_Task);

  for (dl2 = LockDosList(LDF_READ | LDF_DEVICES);
       dl2;
       dl2 = NextDosEntry (dl2, LDF_DEVICES))
    {
      if (dl2->dol_Task == dl->dl_Task)
	break;
    }

  if (lck)
    UnLock (lck);

  if (dl2)
    nm = (char *) dl2->dol_Name;
  else
    nm = (char *) dl->dl_Name;

  holy_util_info ("dl2=%p, nm=%s", dl2, nm);

  os_dev = xmalloc (2 * sizeof (os_dev[0]));
  sz = strlen (nm);
  
  os_dev[0] = xmalloc (sz + 4);
  os_dev[0][0] = '/';
  os_dev[0][1] = '/';
  os_dev[0][2] = ':';
  memcpy (os_dev[0] + 3, nm, sz);
  os_dev[0][sz+3] = '\0';
  os_dev[1] = NULL;

  UnLockDosList (LDF_READ | LDF_DEVICES);

  return os_dev;
}

int
holy_util_biosdisk_is_floppy (holy_disk_t disk)
{
  const char *dname;

  dname = holy_util_biosdisk_get_osdev (disk);

  if (dname[0] != '/' || dname[1] != '/' || dname[2] != ':')
    return 0;

  dname += 3;

  if (strncmp (dname, TD_NAME, sizeof (TD_NAME) - 1) == 0
      && (TD_NAME[sizeof (TD_NAME) - 1] == '/'
	  || TD_NAME[sizeof (TD_NAME) - 1] == '\0'))
    return 1;
  return 0;
}

