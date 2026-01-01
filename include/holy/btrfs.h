/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BTRFS_H
#define holy_BTRFS_H	1

enum
  {
    holy_BTRFS_ITEM_TYPE_INODE_ITEM = 0x01,
    holy_BTRFS_ITEM_TYPE_INODE_REF = 0x0c,
    holy_BTRFS_ITEM_TYPE_DIR_ITEM = 0x54,
    holy_BTRFS_ITEM_TYPE_EXTENT_ITEM = 0x6c,
    holy_BTRFS_ITEM_TYPE_ROOT_ITEM = 0x84,
    holy_BTRFS_ITEM_TYPE_ROOT_BACKREF = 0x90,
    holy_BTRFS_ITEM_TYPE_DEVICE = 0xd8,
    holy_BTRFS_ITEM_TYPE_CHUNK = 0xe4
  };

enum
  {
    holy_BTRFS_ROOT_VOL_OBJECTID = 5,
    holy_BTRFS_TREE_ROOT_OBJECTID = 0x100,
  };

struct holy_btrfs_root_item
{
  holy_uint8_t dummy[0xb0];
  holy_uint64_t tree;
  holy_uint64_t inode;
};

struct holy_btrfs_key
{
  holy_uint64_t object_id;
  holy_uint8_t type;
  holy_uint64_t offset;
} holy_PACKED;


struct holy_btrfs_root_backref
{
  holy_uint64_t inode_id;
  holy_uint64_t seqnr;
  holy_uint16_t n;
  char name[0];
};

struct holy_btrfs_inode_ref
{
  holy_uint64_t idxid;
  holy_uint16_t n;
  char name[0];
};

#endif
