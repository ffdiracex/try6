/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BSDLABEL_PARTITION_HEADER
#define holy_BSDLABEL_PARTITION_HEADER	1

/* Constants for BSD disk label.  */
#define holy_PC_PARTITION_BSD_LABEL_SECTOR	1
#define holy_PC_PARTITION_BSD_LABEL_MAGIC	0x82564557

/* BSD partition types.  */
enum
  {
    holy_PC_PARTITION_BSD_TYPE_UNUSED  =  0,
    holy_PC_PARTITION_BSD_TYPE_SWAP    =  1,
    holy_PC_PARTITION_BSD_TYPE_V6      =  2,
    holy_PC_PARTITION_BSD_TYPE_V7      =  3,
    holy_PC_PARTITION_BSD_TYPE_SYSV    =  4,
    holy_PC_PARTITION_BSD_TYPE_V71K    =  5,
    holy_PC_PARTITION_BSD_TYPE_V8      =  6,
    holy_PC_PARTITION_BSD_TYPE_BSDFFS  =  7,
    holy_PC_PARTITION_BSD_TYPE_MSDOS   =  8,
    holy_PC_PARTITION_BSD_TYPE_BSDLFS  =  9,
    holy_PC_PARTITION_BSD_TYPE_OTHER   = 10,
    holy_PC_PARTITION_BSD_TYPE_HPFS    = 11,
    holy_PC_PARTITION_BSD_TYPE_ISO9660 = 12,
    holy_PC_PARTITION_BSD_TYPE_BOOT    = 13
  };

/* FreeBSD-specific types.  */
enum
  {
    holy_PC_PARTITION_FREEBSD_TYPE_VINUM = 14,
    holy_PC_PARTITION_FREEBSD_TYPE_RAID  = 15,
    holy_PC_PARTITION_FREEBSD_TYPE_JFS2  = 21
  };

/* NetBSD-specific types.  */
enum
  {
    holy_PC_PARTITION_NETBSD_TYPE_ADOS     = 14,
    holy_PC_PARTITION_NETBSD_TYPE_HFS      = 15,
    holy_PC_PARTITION_NETBSD_TYPE_FILECORE = 16,
    holy_PC_PARTITION_NETBSD_TYPE_EXT2FS   = 17,
    holy_PC_PARTITION_NETBSD_TYPE_NTFS     = 18,
    holy_PC_PARTITION_NETBSD_TYPE_RAID     = 19,
    holy_PC_PARTITION_NETBSD_TYPE_CCD      = 20,
    holy_PC_PARTITION_NETBSD_TYPE_JFS2     = 21,
    holy_PC_PARTITION_NETBSD_TYPE_APPLEUFS = 22
  };

/* OpenBSD-specific types.  */
enum
  {
    holy_PC_PARTITION_OPENBSD_TYPE_ADOS     = 14,
    holy_PC_PARTITION_OPENBSD_TYPE_HFS      = 15,
    holy_PC_PARTITION_OPENBSD_TYPE_FILECORE = 16,
    holy_PC_PARTITION_OPENBSD_TYPE_EXT2FS   = 17,
    holy_PC_PARTITION_OPENBSD_TYPE_NTFS     = 18,
    holy_PC_PARTITION_OPENBSD_TYPE_RAID     = 19
  };

#define holy_PC_PARTITION_BSD_LABEL_WHOLE_DISK_PARTITION 2

/* The BSD partition entry.  */
struct holy_partition_bsd_entry
{
  holy_uint32_t size;
  holy_uint32_t offset;
  holy_uint32_t fragment_size;
  holy_uint8_t fs_type;
  holy_uint8_t fs_fragments;
  holy_uint16_t fs_cylinders;
} holy_PACKED;

/* The BSD disk label. Only define members useful for holy.  */
struct holy_partition_bsd_disk_label
{
  holy_uint32_t magic;
  holy_uint16_t type;
  holy_uint8_t unused1[18];
  holy_uint8_t packname[16];
  holy_uint8_t unused2[92];
  holy_uint32_t magic2;
  holy_uint16_t checksum;
  holy_uint16_t num_partitions;
  holy_uint32_t boot_size;
  holy_uint32_t superblock_size;
} holy_PACKED;

#endif /* ! holy_PC_PARTITION_HEADER */
