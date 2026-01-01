/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PC_PARTITION_HEADER
#define holy_PC_PARTITION_HEADER	1

#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/disk.h>
#include <holy/partition.h>

/* The signature.  */
#define holy_PC_PARTITION_SIGNATURE		0xaa55

/* This is not a flag actually, but used as if it were a flag.  */
#define holy_PC_PARTITION_TYPE_HIDDEN_FLAG	0x10

/* DOS partition types.  */
#define holy_PC_PARTITION_TYPE_NONE		0
#define holy_PC_PARTITION_TYPE_FAT12		1
#define holy_PC_PARTITION_TYPE_FAT16_LT32M	4
#define holy_PC_PARTITION_TYPE_EXTENDED		5
#define holy_PC_PARTITION_TYPE_FAT16_GT32M	6
#define holy_PC_PARTITION_TYPE_NTFS	        7
#define holy_PC_PARTITION_TYPE_FAT32		0xb
#define holy_PC_PARTITION_TYPE_FAT32_LBA	0xc
#define holy_PC_PARTITION_TYPE_FAT16_LBA	0xe
#define holy_PC_PARTITION_TYPE_WIN95_EXTENDED	0xf
#define holy_PC_PARTITION_TYPE_PLAN9            0x39
#define holy_PC_PARTITION_TYPE_LDM		0x42
#define holy_PC_PARTITION_TYPE_EZD		0x55
#define holy_PC_PARTITION_TYPE_MINIX		0x80
#define holy_PC_PARTITION_TYPE_LINUX_MINIX	0x81
#define holy_PC_PARTITION_TYPE_LINUX_SWAP	0x82
#define holy_PC_PARTITION_TYPE_EXT2FS		0x83
#define holy_PC_PARTITION_TYPE_LINUX_EXTENDED	0x85
#define holy_PC_PARTITION_TYPE_VSTAFS		0x9e
#define holy_PC_PARTITION_TYPE_FREEBSD		0xa5
#define holy_PC_PARTITION_TYPE_OPENBSD		0xa6
#define holy_PC_PARTITION_TYPE_NETBSD		0xa9
#define holy_PC_PARTITION_TYPE_HFS		0xaf
#define holy_PC_PARTITION_TYPE_GPT_DISK		0xee
#define holy_PC_PARTITION_TYPE_LINUX_RAID	0xfd

/* The partition entry.  */
struct holy_msdos_partition_entry
{
  /* If active, 0x80, otherwise, 0x00.  */
  holy_uint8_t flag;

  /* The head of the start.  */
  holy_uint8_t start_head;

  /* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
     is the cylinder of the start. Note that S is counted from one.  */
  holy_uint8_t start_sector;

  /* (C & 0xFF) where C is the cylinder of the start.  */
  holy_uint8_t start_cylinder;

  /* The partition type.  */
  holy_uint8_t type;

  /* The end versions of start_head, start_sector and start_cylinder,
     respectively.  */
  holy_uint8_t end_head;
  holy_uint8_t end_sector;
  holy_uint8_t end_cylinder;

  /* The start sector. Note that this is counted from zero.  */
  holy_uint32_t start;

  /* The length in sector units.  */
  holy_uint32_t length;
} holy_PACKED;

/* The structure of MBR.  */
struct holy_msdos_partition_mbr
{
  /* The code area (actually, including BPB).  */
  holy_uint8_t code[446];

  /* Four partition entries.  */
  struct holy_msdos_partition_entry entries[4];

  /* The signature 0xaa55.  */
  holy_uint16_t signature;
} holy_PACKED;



static inline int
holy_msdos_partition_is_empty (int type)
{
  return (type == holy_PC_PARTITION_TYPE_NONE);
}

static inline int
holy_msdos_partition_is_extended (int type)
{
  return (type == holy_PC_PARTITION_TYPE_EXTENDED
	  || type == holy_PC_PARTITION_TYPE_WIN95_EXTENDED
	  || type == holy_PC_PARTITION_TYPE_LINUX_EXTENDED);
}

holy_err_t
holy_partition_msdos_iterate (holy_disk_t disk,
			      holy_partition_iterate_hook_t hook,
			      void *hook_data);

#endif /* ! holy_PC_PARTITION_HEADER */
