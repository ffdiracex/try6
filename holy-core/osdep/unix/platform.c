/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/install.h>
#include <holy/emu/hostdisk.h>
#include <holy/util/misc.h>
#include <holy/misc.h>
#include <holy/i18n.h>
#include <holy/emu/exec.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>

static char *
get_ofpathname (const char *dev)
{
  size_t alloced = 4096;
  char *ret = xmalloc (alloced);
  size_t offset = 0;
  int fd;
  pid_t pid;

  pid = holy_util_exec_pipe ((const char * []){ "ofpathname", dev, NULL }, &fd);
  if (!pid)
    goto fail;

  FILE *fp = fdopen (fd, "r");
  if (!fp)
    goto fail;

  while (!feof (fp))
    {
      size_t r;
      if (alloced == offset)
       {
         alloced *= 2;
         ret = xrealloc (ret, alloced);
       }
      r = fread (ret + offset, 1, alloced - offset, fp);
      offset += r;
    }

  if (offset > 0 && ret[offset - 1] == '\n')
    offset--;
  if (offset > 0 && ret[offset - 1] == '\r')
    offset--;
  if (alloced == offset)
    {
      alloced++;
      ret = xrealloc (ret, alloced);
    }
  ret[offset] = '\0';

  fclose (fp);

  return ret;

 fail:
  holy_util_error (_("couldn't find IEEE1275 device path for %s.\nYou will have to set `boot-device' variable manually"),
		   dev);
}

static void
holy_install_remove_efi_entries_by_distributor (const char *efi_distributor)
{
  int fd;
  pid_t pid = holy_util_exec_pipe ((const char * []){ "efibootmgr", NULL }, &fd);
  char *line = NULL;
  size_t len = 0;

  if (!pid)
    {
      holy_util_warn (_("Unable to open stream from %s: %s"),
		      "efibootmgr", strerror (errno));
      return;
    }

  FILE *fp = fdopen (fd, "r");
  if (!fp)
    {
      holy_util_warn (_("Unable to open stream from %s: %s"),
		      "efibootmgr", strerror (errno));
      return;
    }

  line = xmalloc (80);
  len = 80;
  while (1)
    {
      int ret;
      char *bootnum;
      ret = getline (&line, &len, fp);
      if (ret == -1)
	break;
      if (holy_memcmp (line, "Boot", sizeof ("Boot") - 1) != 0
	  || line[sizeof ("Boot") - 1] < '0'
	  || line[sizeof ("Boot") - 1] > '9')
	continue;
      if (!strcasestr (line, efi_distributor))
	continue;
      bootnum = line + sizeof ("Boot") - 1;
      bootnum[4] = '\0';
      if (!verbosity)
	holy_util_exec ((const char * []){ "efibootmgr", "-q",
	      "-b", bootnum,  "-B", NULL });
      else
	holy_util_exec ((const char * []){ "efibootmgr",
	      "-b", bootnum, "-B", NULL });
    }

  free (line);
}

void
holy_install_register_efi (holy_device_t efidir_holy_dev,
			   const char *efifile_path,
			   const char *efi_distributor)
{
  const char * efidir_disk;
  int efidir_part;
  efidir_disk = holy_util_biosdisk_get_osdev (efidir_holy_dev->disk);
  efidir_part = efidir_holy_dev->disk->partition ? efidir_holy_dev->disk->partition->number + 1 : 1;

  if (holy_util_exec_redirect_null ((const char * []){ "efibootmgr", "--version", NULL }))
    {
      /* TRANSLATORS: This message is shown when required executable `%s'
	 isn't found.  */
      holy_util_error (_("%s: not found"), "efibootmgr");
    }

  /* On Linux, we need the efivars kernel modules.  */
#ifdef __linux__
  holy_util_exec ((const char * []){ "modprobe", "-q", "efivars", NULL });
#endif
  /* Delete old entries from the same distributor.  */
  holy_install_remove_efi_entries_by_distributor (efi_distributor);

  char *efidir_part_str = xasprintf ("%d", efidir_part);

  if (!verbosity)
    holy_util_exec ((const char * []){ "efibootmgr", "-q",
	  "-c", "-d", efidir_disk,
	  "-p", efidir_part_str, "-w",
	  "-L", efi_distributor, "-l", 
	  efifile_path, NULL });
  else
    holy_util_exec ((const char * []){ "efibootmgr",
	  "-c", "-d", efidir_disk,
	  "-p", efidir_part_str, "-w",
	  "-L", efi_distributor, "-l", 
	  efifile_path, NULL });
  free (efidir_part_str);
}

void
holy_install_register_ieee1275 (int is_prep, const char *install_device,
				int partno, const char *relpath)
{
  char *boot_device;

  if (holy_util_exec_redirect_null ((const char * []){ "ofpathname", "--version", NULL }))
    {
      /* TRANSLATORS: This message is shown when required executable `%s'
	 isn't found.  */
      holy_util_error (_("%s: not found"), "ofpathname");
    }

  /* Get the Open Firmware device tree path translation.  */
  if (!is_prep)
    {
      char *ptr;
      char *ofpath;
      const char *iptr;

      ofpath = get_ofpathname (install_device);
      boot_device = xmalloc (strlen (ofpath) + 1
			     + sizeof ("XXXXXXXXXXXXXXXXXXXX")
			     + 1 + strlen (relpath) + 1);
      ptr = holy_stpcpy (boot_device, ofpath);
      *ptr++ = ':';
      holy_snprintf (ptr, sizeof ("XXXXXXXXXXXXXXXXXXXX"), "%d",
		     partno);
      ptr += strlen (ptr);
      *ptr++ = ',';
      for (iptr = relpath; *iptr; iptr++, ptr++)
	{
	  if (*iptr == '/')
	    *ptr = '\\';
	  else
	    *ptr = *iptr;
	}
      *ptr = '\0';
    }
  else
    boot_device = get_ofpathname (install_device);

  if (holy_util_exec ((const char * []){ "nvsetenv", "boot-device",
	  boot_device, NULL }))
    {
      char *cmd = xasprintf ("setenv boot-device %s", boot_device);
      holy_util_error (_("`nvsetenv' failed. \nYou will have to set `boot-device' variable manually.  At the IEEE1275 prompt, type:\n  %s\n"),
		       cmd);
      free (cmd);
    }

  free (boot_device);
}

void
holy_install_sgi_setup (const char *install_device,
			const char *imgfile, const char *destname)
{
  holy_util_exec ((const char * []){ "dvhtool", "-d",
	install_device, "--unix-to-vh", 
	imgfile, destname, NULL });
  holy_util_warn ("%s", _("You will have to set `SystemPartition' and `OSLoader' manually."));
}
