/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_SCSICMD_H
#define	holy_SCSICMD_H	1

#include <holy/types.h>

#define holy_SCSI_DEVTYPE_MASK	31
#define holy_SCSI_REMOVABLE_BIT	7
#define holy_SCSI_LUN_SHIFT	5

struct holy_scsi_test_unit_ready
{
  holy_uint8_t opcode;
  holy_uint8_t lun; /* 7-5 LUN, 4-0 reserved */
  holy_uint8_t reserved1;
  holy_uint8_t reserved2;
  holy_uint8_t reserved3;
  holy_uint8_t control;
  holy_uint8_t pad[6]; /* To be ATAPI compatible */
} holy_PACKED;

struct holy_scsi_inquiry
{
  holy_uint8_t opcode;
  holy_uint8_t lun; /* 7-5 LUN, 4-1 reserved, 0 EVPD */
  holy_uint8_t page; /* page code if EVPD=1 */
  holy_uint8_t reserved;
  holy_uint8_t alloc_length;
  holy_uint8_t control;
  holy_uint8_t pad[6]; /* To be ATAPI compatible */
} holy_PACKED;

struct holy_scsi_inquiry_data
{
  holy_uint8_t devtype;
  holy_uint8_t rmb;
  holy_uint16_t reserved;
  holy_uint8_t length;
  holy_uint8_t reserved2[3];
  char vendor[8];
  char prodid[16];
  char prodrev[4];
} holy_PACKED;

struct holy_scsi_request_sense
{
  holy_uint8_t opcode;
  holy_uint8_t lun; /* 7-5 LUN, 4-0 reserved */
  holy_uint8_t reserved1;
  holy_uint8_t reserved2;
  holy_uint8_t alloc_length;
  holy_uint8_t control;
  holy_uint8_t pad[6]; /* To be ATAPI compatible */
} holy_PACKED;

struct holy_scsi_request_sense_data
{
  holy_uint8_t error_code; /* 7 Valid, 6-0 Err. code */
  holy_uint8_t segment_number;
  holy_uint8_t sense_key; /*7 FileMark, 6 EndOfMedia, 5 ILI, 4-0 sense key */
  holy_uint32_t information;
  holy_uint8_t additional_sense_length;
  holy_uint32_t cmd_specific_info;
  holy_uint8_t additional_sense_code;
  holy_uint8_t additional_sense_code_qualifier;
  holy_uint8_t field_replaceable_unit_code;
  holy_uint8_t sense_key_specific[3];
  /* there can be additional sense field */
} holy_PACKED;

struct holy_scsi_read_capacity10
{
  holy_uint8_t opcode;
  holy_uint8_t lun; /* 7-5 LUN, 4-1 reserved, 0 reserved */
  holy_uint32_t logical_block_addr; /* only if PMI=1 */
  holy_uint8_t reserved1;
  holy_uint8_t reserved2;
  holy_uint8_t PMI;
  holy_uint8_t control;
  holy_uint16_t pad; /* To be ATAPI compatible */
} holy_PACKED;

struct holy_scsi_read_capacity10_data
{
  holy_uint32_t last_block;
  holy_uint32_t blocksize;
} holy_PACKED;

struct holy_scsi_read_capacity16
{
  holy_uint8_t opcode;
  holy_uint8_t lun; /* 7-5 LUN, 4-0 0x10 */
  holy_uint64_t logical_block_addr; /* only if PMI=1 */
  holy_uint32_t alloc_len;
  holy_uint8_t PMI;
  holy_uint8_t control;
} holy_PACKED;

struct holy_scsi_read_capacity16_data
{
  holy_uint64_t last_block;
  holy_uint32_t blocksize;
  holy_uint8_t pad[20];
} holy_PACKED;

struct holy_scsi_read10
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint32_t lba;
  holy_uint8_t reserved;
  holy_uint16_t size;
  holy_uint8_t reserved2;
  holy_uint16_t pad;
} holy_PACKED;

struct holy_scsi_read12
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint32_t lba;
  holy_uint32_t size;
  holy_uint8_t reserved;
  holy_uint8_t control;
} holy_PACKED;

struct holy_scsi_read16
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint64_t lba;
  holy_uint32_t size;
  holy_uint8_t reserved;
  holy_uint8_t control;
} holy_PACKED;

struct holy_scsi_write10
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint32_t lba;
  holy_uint8_t reserved;
  holy_uint16_t size;
  holy_uint8_t reserved2;
  holy_uint16_t pad;
} holy_PACKED;

struct holy_scsi_write12
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint32_t lba;
  holy_uint32_t size;
  holy_uint8_t reserved;
  holy_uint8_t control;
} holy_PACKED;

struct holy_scsi_write16
{
  holy_uint8_t opcode;
  holy_uint8_t lun;
  holy_uint64_t lba;
  holy_uint32_t size;
  holy_uint8_t reserved;
  holy_uint8_t control;
} holy_PACKED;

typedef enum
  {
    holy_scsi_cmd_test_unit_ready = 0x00,
    holy_scsi_cmd_request_sense = 0x03,
    holy_scsi_cmd_inquiry = 0x12,
    holy_scsi_cmd_read_capacity10 = 0x25,
    holy_scsi_cmd_read10 = 0x28,
    holy_scsi_cmd_write10 = 0x2a,
    holy_scsi_cmd_read16 = 0x88,
    holy_scsi_cmd_write16 = 0x8a,
    holy_scsi_cmd_read_capacity16 = 0x9e,
    holy_scsi_cmd_read12 = 0xa8,
    holy_scsi_cmd_write12 = 0xaa,
  } holy_scsi_cmd_t;

typedef enum
  {
    holy_scsi_devtype_direct = 0x00,
    holy_scsi_devtype_cdrom = 0x05
  } holy_scsi_devtype_t;

#endif /* holy_SCSICMD_H */
