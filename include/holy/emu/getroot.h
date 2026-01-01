/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_UTIL_GETROOT_HEADER
#define holy_UTIL_GETROOT_HEADER	1

#include <holy/types.h>
#include <holy/device.h>

#include <sys/types.h>
#include <stdio.h>

enum holy_dev_abstraction_types {
  holy_DEV_ABSTRACTION_NONE,
  holy_DEV_ABSTRACTION_LVM,
  holy_DEV_ABSTRACTION_RAID,
  holy_DEV_ABSTRACTION_LUKS,
  holy_DEV_ABSTRACTION_GELI,
};

char *holy_find_device (const char *dir, dev_t dev);
void holy_util_pull_device (const char *osname);
char **holy_guess_root_devices (const char *dir);
int holy_util_get_dev_abstraction (const char *os_dev);
char *holy_make_system_path_relative_to_its_root (const char *path);
char *
holy_make_system_path_relative_to_its_root_os (const char *path);
char *holy_util_get_holy_dev (const char *os_dev);
#if defined (__FreeBSD__) || defined(__FreeBSD_kernel__)
void holy_util_follow_gpart_up (const char *name, holy_disk_addr_t *off_out,
				char **name_out);
#endif

#include <sys/stat.h>

#ifdef __linux__
char **
holy_find_root_devices_from_mountinfo (const char *dir, char **relroot);
#endif

/* Devmapper functions provided by getroot_devmapper.c.  */
void
holy_util_pull_devmapper (const char *os_dev);
int
holy_util_device_is_mapped_stat (struct stat *st);
void holy_util_devmapper_cleanup (void);
enum holy_dev_abstraction_types
holy_util_get_dm_abstraction (const char *os_dev);
char *
holy_util_get_vg_uuid (const char *os_dev);
char *
holy_util_devmapper_part_to_disk (struct stat *st,
				  int *is_part, const char *os_dev);
char *
holy_util_get_devmapper_holy_dev (const char *os_dev);

void
holy_util_pull_lvm_by_command (const char *os_dev);
char **
holy_util_find_root_devices_from_poolname (char *poolname);

holy_disk_addr_t
holy_util_find_partition_start (const char *dev);

/* OS-specific functions provided by getroot_*.c.  */
enum holy_dev_abstraction_types
holy_util_get_dev_abstraction_os (const char *os_dev);
char *
holy_util_part_to_disk (const char *os_dev, struct stat *st,
			int *is_part);
int
holy_util_pull_device_os (const char *osname,
			  enum holy_dev_abstraction_types ab);
char *
holy_util_get_holy_dev_os (const char *os_dev);
holy_disk_addr_t
holy_util_find_partition_start_os (const char *dev);

char *
holy_util_guess_bios_drive (const char *orig_path);
char *
holy_util_guess_efi_drive (const char *orig_path);
char *
holy_util_guess_baremetal_drive (const char *orig_path);
void
holy_util_fprint_full_disk_name (FILE *f,
				 const char *drive, holy_device_t dev);

#endif /* ! holy_UTIL_GETROOT_HEADER */
