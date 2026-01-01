/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/util/install.h>
#include <holy/emu/exec.h>
#include <holy/emu/misc.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

#include <sys/utsname.h>

static int
is_not_empty_directory (const char *dir)
{
  DIR *d;
  struct dirent *de;

  d = opendir (dir);
  if (!d)
    return 0;
  while ((de = readdir (d)))
    {
      if (strcmp (de->d_name, ".") == 0
	  || strcmp (de->d_name, "..") == 0)
	continue;
      closedir (d);
      return 1;
    }

  closedir (d);
  return 0;
}

static int
is_64_kernel (void)
{
  struct utsname un;

  if (uname (&un) < 0)
    return 0;

  return strcmp (un.machine, "x86_64") == 0;
}

static int
read_platform_size (void)
{
  FILE *fp;
  char *buf = NULL;
  size_t len = 0;
  int ret = 0;

  /* Newer kernels can tell us directly about the size of the
   * underlying firmware - let's see if that interface is there. */
  fp = holy_util_fopen ("/sys/firmware/efi/fw_platform_size", "r");
  if (fp != NULL)
  {
    if (getline (&buf, &len, fp) >= 3) /* 2 digits plus newline */
      {
	if (strncmp (buf, "32", 2) == 0)
	  ret = 32;
	else if (strncmp (buf, "64", 2) == 0)
	  ret = 64;
      }
    free (buf);
    fclose (fp);
  }

  if (ret == 0)
    {
      /* Unrecognised - fall back to matching the kernel size
       * instead */
      if (is_64_kernel ())
	ret = 64;
      else
	ret = 32;
    }

  return ret;
}

const char *
holy_install_get_default_x86_platform (void)
{ 
  /*
     On Linux, we need the efivars kernel modules.
     If no EFI is available this module just does nothing
     besides a small hello and if we detect efi we'll load it
     anyway later. So it should be safe to
     try to load it here.
   */
  holy_util_exec_redirect_all ((const char * []){ "modprobe", "efivars", NULL },
			       NULL, NULL, "/dev/null");

  holy_util_info ("Looking for /sys/firmware/efi ..");
  if (is_not_empty_directory ("/sys/firmware/efi"))
    {
      holy_util_info ("...found");
      if (read_platform_size() == 64)
	return "x86_64-efi";
      else
	return "i386-efi";
    }

  holy_util_info ("... not found. Looking for /proc/device-tree ..");
  if (is_not_empty_directory ("/proc/device-tree"))
    {
      holy_util_info ("...found");
      return "i386-ieee1275";
    }

  holy_util_info ("... not found");
  return "i386-pc";
}
