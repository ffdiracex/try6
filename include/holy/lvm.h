/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LVM_H
#define holy_LVM_H	1

#include <holy/types.h>
#include <holy/diskfilter.h>

/* Length of ID string, excluding terminating zero. */
#define holy_LVM_ID_STRLEN 38

#define holy_LVM_LABEL_SIZE holy_DISK_SECTOR_SIZE
#define holy_LVM_LABEL_SCAN_SECTORS 4L

#define holy_LVM_LABEL_ID "LABELONE"
#define holy_LVM_LVM2_LABEL "LVM2 001"

#define holy_LVM_ID_LEN 32

/* On disk - 32 bytes */
struct holy_lvm_label_header {
  holy_int8_t id[8];		/* LABELONE */
  holy_uint64_t sector_xl;	/* Sector number of this label */
  holy_uint32_t crc_xl;		/* From next field to end of sector */
  holy_uint32_t offset_xl;	/* Offset from start of struct to contents */
  holy_int8_t type[8];		/* LVM2 001 */
} holy_PACKED;

/* On disk */
struct holy_lvm_disk_locn {
  holy_uint64_t offset;		/* Offset in bytes to start sector */
  holy_uint64_t size;		/* Bytes */
} holy_PACKED;

/* Fields with the suffix _xl should be xlate'd wherever they appear */
/* On disk */
struct holy_lvm_pv_header {
  holy_int8_t pv_uuid[holy_LVM_ID_LEN];

  /* This size can be overridden if PV belongs to a VG */
  holy_uint64_t device_size_xl;	/* Bytes */

  /* NULL-terminated list of data areas followed by */
  /* NULL-terminated list of metadata area headers */
  struct holy_lvm_disk_locn disk_areas_xl[0];	/* Two lists */
} holy_PACKED;

#define holy_LVM_FMTT_MAGIC "\040\114\126\115\062\040\170\133\065\101\045\162\060\116\052\076"
#define holy_LVM_FMTT_VERSION 1
#define holy_LVM_MDA_HEADER_SIZE 512

/* On disk */
struct holy_lvm_raw_locn {
  holy_uint64_t offset;		/* Offset in bytes to start sector */
  holy_uint64_t size;		/* Bytes */
  holy_uint32_t checksum;
  holy_uint32_t filler;
} holy_PACKED;

/* On disk */
/* Structure size limited to one sector */
struct holy_lvm_mda_header {
  holy_uint32_t checksum_xl;	/* Checksum of rest of mda_header */
  holy_int8_t magic[16];	/* To aid scans for metadata */
  holy_uint32_t version;
  holy_uint64_t start;		/* Absolute start byte of mda_header */
  holy_uint64_t size;		/* Size of metadata area */

  struct holy_lvm_raw_locn raw_locns[0];	/* NULL-terminated list */
} holy_PACKED;


#endif /* ! holy_LVM_H */
