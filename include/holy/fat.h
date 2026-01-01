/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FAT_H
#define holy_FAT_H	1

#include <holy/types.h>

struct holy_fat_bpb
{
  holy_uint8_t jmp_boot[3];
  holy_uint8_t oem_name[8];
  holy_uint16_t bytes_per_sector;
  holy_uint8_t sectors_per_cluster;
  holy_uint16_t num_reserved_sectors;
  holy_uint8_t num_fats;
  /* 0x10 */
  holy_uint16_t num_root_entries;
  holy_uint16_t num_total_sectors_16;
  holy_uint8_t media;
  /*0 x15 */
  holy_uint16_t sectors_per_fat_16;
  holy_uint16_t sectors_per_track;
  /*0 x19 */
  holy_uint16_t num_heads;
  /*0 x1b */
  holy_uint32_t num_hidden_sectors;
  /* 0x1f */
  holy_uint32_t num_total_sectors_32;
  union
  {
    struct
    {
      holy_uint8_t num_ph_drive;
      holy_uint8_t reserved;
      holy_uint8_t boot_sig;
      holy_uint32_t num_serial;
      holy_uint8_t label[11];
      holy_uint8_t fstype[8];
    } holy_PACKED fat12_or_fat16;
    struct
    {
      holy_uint32_t sectors_per_fat_32;
      holy_uint16_t extended_flags;
      holy_uint16_t fs_version;
      holy_uint32_t root_cluster;
      holy_uint16_t fs_info;
      holy_uint16_t backup_boot_sector;
      holy_uint8_t reserved[12];
      holy_uint8_t num_ph_drive;
      holy_uint8_t reserved1;
      holy_uint8_t boot_sig;
      holy_uint32_t num_serial;
      holy_uint8_t label[11];
      holy_uint8_t fstype[8];
    } holy_PACKED fat32;
  } holy_PACKED version_specific;
} holy_PACKED;

#ifdef holy_UTIL
#include <holy/disk.h>

holy_disk_addr_t
holy_fat_get_cluster_sector (holy_disk_t disk, holy_uint64_t *sec_per_lcn);
#endif

#endif
