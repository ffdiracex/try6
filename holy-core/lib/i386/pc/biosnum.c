/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/env.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/machine/biosnum.h>

static int
holy_get_root_biosnumber_default (void)
{
  const char *biosnum;
  int ret = -1;
  holy_device_t dev;

  biosnum = holy_env_get ("biosnum");

  if (biosnum)
    return holy_strtoul (biosnum, 0, 0);

  dev = holy_device_open (0);
  if (dev && dev->disk && dev->disk->dev
      && dev->disk->dev->id == holy_DISK_DEVICE_BIOSDISK_ID)
    ret = (int) dev->disk->id;

  if (dev)
    holy_device_close (dev);

  return ret;
}

int (*holy_get_root_biosnumber) (void) = holy_get_root_biosnumber_default;
