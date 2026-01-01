/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_HFS_HEADER
#define holy_HFS_HEADER	1

#include <holy/types.h>

#define holy_HFS_MAGIC		0x4244

/* A single extent.  A file consists of one or more extents.  */
struct holy_hfs_extent
{
  /* The first physical block.  */
  holy_uint16_t first_block;
  holy_uint16_t count;
};

/* HFS stores extents in groups of 3.  */
typedef struct holy_hfs_extent holy_hfs_datarecord_t[3];

/* The HFS superblock (The official name is `Master Directory
   Block').  */
struct holy_hfs_sblock
{
  holy_uint16_t magic;
  holy_uint32_t ctime;
  holy_uint32_t mtime;
  holy_uint8_t unused[10];
  holy_uint32_t blksz;
  holy_uint8_t unused2[4];
  holy_uint16_t first_block;
  holy_uint8_t unused4[6];

  /* A pascal style string that holds the volumename.  */
  holy_uint8_t volname[28];

  holy_uint8_t unused5[28];

  holy_uint32_t ppc_bootdir;
  holy_uint32_t intel_bootfile;
  /* Folder opened when disk is mounted. Unused by holy. */
  holy_uint32_t showfolder;
  holy_uint32_t os9folder;
  holy_uint8_t unused6[4];
  holy_uint32_t osxfolder;

  holy_uint64_t num_serial;
  holy_uint16_t embed_sig;
  struct holy_hfs_extent embed_extent;
  holy_uint8_t unused7[4];
  holy_hfs_datarecord_t extent_recs;
  holy_uint32_t catalog_size;
  holy_hfs_datarecord_t catalog_recs;
} holy_PACKED;

#endif /* ! holy_HFS_HEADER */
