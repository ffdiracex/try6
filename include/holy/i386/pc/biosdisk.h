/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BIOSDISK_MACHINE_HEADER
#define holy_BIOSDISK_MACHINE_HEADER	1

#include <holy/symbol.h>
#include <holy/types.h>

#define holy_BIOSDISK_FLAG_LBA	1
#define holy_BIOSDISK_FLAG_CDROM 2

#define holy_BIOSDISK_CDTYPE_NO_EMUL	0
#define holy_BIOSDISK_CDTYPE_1_2_M	1
#define holy_BIOSDISK_CDTYPE_1_44_M	2
#define holy_BIOSDISK_CDTYPE_2_88_M	3
#define holy_BIOSDISK_CDTYPE_HARDDISK	4

#define holy_BIOSDISK_CDTYPE_MASK	0xF

struct holy_biosdisk_data
{
  int drive;
  unsigned long cylinders;
  unsigned long heads;
  unsigned long sectors;
  unsigned long flags;
};

/* Drive Parameters.  */
struct holy_biosdisk_drp
{
  holy_uint16_t size;
  holy_uint16_t flags;
  holy_uint32_t cylinders;
  holy_uint32_t heads;
  holy_uint32_t sectors;
  holy_uint64_t total_sectors;
  holy_uint16_t bytes_per_sector;
  /* ver 2.0 or higher */

  union
  {
    holy_uint32_t EDD_configuration_parameters;

    /* Pointer to the Device Parameter Table Extension (ver 3.0+).  */
    holy_uint32_t dpte_pointer;
  };

  /* ver 3.0 or higher */
  holy_uint16_t signature_dpi;
  holy_uint8_t length_dpi;
  holy_uint8_t reserved[3];
  holy_uint8_t name_of_host_bus[4];
  holy_uint8_t name_of_interface_type[8];
  holy_uint8_t interface_path[8];
  holy_uint8_t device_path[16];
  holy_uint8_t reserved2;
  holy_uint8_t checksum;

  /* XXX: This is necessary, because the BIOS of Thinkpad X20
     writes a garbage to the tail of drive parameters,
     regardless of a size specified in a caller.  */
  holy_uint8_t dummy[16];
} holy_PACKED;

struct holy_biosdisk_cdrp
{
  holy_uint8_t size;
  holy_uint8_t media_type;
  holy_uint8_t drive_no;
  holy_uint8_t controller_no;
  holy_uint32_t image_lba;
  holy_uint16_t device_spec;
  holy_uint16_t cache_seg;
  holy_uint16_t load_seg;
  holy_uint16_t length_sec512;
  holy_uint8_t cylinders;
  holy_uint8_t sectors;
  holy_uint8_t heads;
  holy_uint8_t dummy[16];
} holy_PACKED;

/* Disk Address Packet.  */
struct holy_biosdisk_dap
{
  holy_uint8_t length;
  holy_uint8_t reserved;
  holy_uint16_t blocks;
  holy_uint32_t buffer;
  holy_uint64_t block;
} holy_PACKED;

#endif /* ! holy_BIOSDISK_MACHINE_HEADER */
