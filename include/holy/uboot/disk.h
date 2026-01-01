/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UBOOT_DISK_HEADER
#define holy_UBOOT_DISK_HEADER	1

#include <holy/symbol.h>
#include <holy/disk.h>
#include <holy/uboot/uboot.h>

void holy_ubootdisk_init (void);
void holy_ubootdisk_fini (void);

enum disktype
{ cd, fd, hd };

struct ubootdisk_data
{
  void *cookie;
  struct device_info *dev;
  int opencount;
  enum disktype type;
  holy_uint32_t block_size;
};

holy_err_t holy_ubootdisk_register (struct device_info *newdev);

#endif /* ! holy_UBOOT_DISK_HEADER */
