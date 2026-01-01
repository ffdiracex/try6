/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BIOSDISK_MACHINE_UTIL_HEADER
#define holy_BIOSDISK_MACHINE_UTIL_HEADER	1

#include <holy/disk.h>
#include <holy/partition.h>
#include <sys/types.h>
#include <holy/emu/hostfile.h>

holy_util_fd_t
holy_util_fd_open_device (const holy_disk_t disk, holy_disk_addr_t sector, int flags,
			  holy_disk_addr_t *max);

void holy_util_biosdisk_init (const char *dev_map);
void holy_util_biosdisk_fini (void);
char *holy_util_biosdisk_get_holy_dev (const char *os_dev);
const char *holy_util_biosdisk_get_osdev (holy_disk_t disk);
int holy_util_biosdisk_is_present (const char *name);
int holy_util_biosdisk_is_floppy (holy_disk_t disk);
const char *
holy_util_biosdisk_get_compatibility_hint (holy_disk_t disk);
holy_err_t holy_util_biosdisk_flush (struct holy_disk *disk);
holy_err_t
holy_cryptodisk_cheat_mount (const char *sourcedev, const char *cheat);
const char *
holy_util_cryptodisk_get_uuid (holy_disk_t disk);
char *
holy_util_get_ldm (holy_disk_t disk, holy_disk_addr_t start);
int
holy_util_is_ldm (holy_disk_t disk);
#ifdef holy_UTIL
holy_err_t
holy_util_ldm_embed (struct holy_disk *disk, unsigned int *nsectors,
		     unsigned int max_nsectors,
		     holy_embed_type_t embed_type,
		     holy_disk_addr_t **sectors);
#endif
const char *
holy_hostdisk_os_dev_to_holy_drive (const char *os_dev, int add);


char *
holy_util_get_os_disk (const char *os_dev);

int
holy_util_get_dm_node_linear_info (dev_t dev,
				   int *maj, int *min,
				   holy_disk_addr_t *st);


/* Supplied by hostdisk_*.c.  */
holy_int64_t
holy_util_get_fd_size_os (holy_util_fd_t fd, const char *name, unsigned *log_secsize);
/* REturns partition offset in 512B blocks.  */
holy_disk_addr_t
holy_hostdisk_find_partition_start_os (const char *dev);
void
holy_hostdisk_flush_initial_buffer (const char *os_dev);

#ifdef __GNU__
int
holy_util_hurd_get_disk_info (const char *dev, holy_uint32_t *secsize, holy_disk_addr_t *offset,
			      holy_disk_addr_t *size, char **parent);
#endif

struct holy_util_hostdisk_data
{
  char *dev;
  int access_mode;
  holy_util_fd_t fd;
  int is_disk;
  int device_map;
};

void holy_host_init (void);
void holy_host_fini (void);
void holy_hostfs_init (void);
void holy_hostfs_fini (void);

#endif /* ! holy_BIOSDISK_MACHINE_UTIL_HEADER */
