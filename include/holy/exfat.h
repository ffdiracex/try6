/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EXFAT_H
#define holy_EXFAT_H	1

#include <holy/types.h>

struct holy_exfat_bpb
{
  holy_uint8_t jmp_boot[3];
  holy_uint8_t oem_name[8];
  holy_uint8_t mbz[53];
  holy_uint64_t num_hidden_sectors;
  holy_uint64_t num_total_sectors;
  holy_uint32_t num_reserved_sectors;
  holy_uint32_t sectors_per_fat;
  holy_uint32_t cluster_offset;
  holy_uint32_t cluster_count;
  holy_uint32_t root_cluster;
  holy_uint32_t num_serial;
  holy_uint16_t fs_revision;
  holy_uint16_t volume_flags;
  holy_uint8_t bytes_per_sector_shift;
  holy_uint8_t sectors_per_cluster_shift;
  holy_uint8_t num_fats;
  holy_uint8_t num_ph_drive;
  holy_uint8_t reserved[8];
} holy_PACKED;

#ifdef holy_UTIL
#include <holy/disk.h>

holy_disk_addr_t
holy_exfat_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn);
#endif

#endif
