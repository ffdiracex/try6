/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/disk.h>

#define holy_HFSPLUS_MAGIC 0x482B
#define holy_HFSPLUSX_MAGIC 0x4858
#define holy_HFSPLUS_SBLOCK 2

/* A HFS+ extent.  */
struct holy_hfsplus_extent
{
  /* The first block of a file on disk.  */
  holy_uint32_t start;
  /* The amount of blocks described by this extent.  */
  holy_uint32_t count;
} holy_PACKED;

/* The descriptor of a fork.  */
struct holy_hfsplus_forkdata
{
  holy_uint64_t size;
  holy_uint32_t clumpsize;
  holy_uint32_t blocks;
  struct holy_hfsplus_extent extents[8];
} holy_PACKED;

/* The HFS+ Volume Header.  */
struct holy_hfsplus_volheader
{
  holy_uint16_t magic;
  holy_uint16_t version;
  holy_uint32_t attributes;
  holy_uint8_t unused1[12];
  holy_uint32_t utime;
  holy_uint8_t unused2[16];
  holy_uint32_t blksize;
  holy_uint8_t unused3[36];

  holy_uint32_t ppc_bootdir;
  holy_uint32_t intel_bootfile;
  /* Folder opened when disk is mounted. Unused by holy. */
  holy_uint32_t showfolder;
  holy_uint32_t os9folder;
  holy_uint8_t unused4[4];
  holy_uint32_t osxfolder;

  holy_uint64_t num_serial;
  struct holy_hfsplus_forkdata allocations_file;
  struct holy_hfsplus_forkdata extents_file;
  struct holy_hfsplus_forkdata catalog_file;
  struct holy_hfsplus_forkdata attr_file;
  struct holy_hfsplus_forkdata startup_file;
} holy_PACKED;

struct holy_hfsplus_compress_index
{
  holy_uint32_t start;
  holy_uint32_t size;
};

struct holy_hfsplus_file
{
  struct holy_hfsplus_data *data;
  struct holy_hfsplus_extent extents[8];
  struct holy_hfsplus_extent resource_extents[8];
  holy_uint64_t size;
  holy_uint64_t resource_size;
  holy_uint32_t fileid;
  holy_int32_t mtime;
  int compressed;
  char *cbuf;
  void *file;
  struct holy_hfsplus_compress_index *compress_index;
  holy_uint32_t cbuf_block;
  holy_uint32_t compress_index_size;
};

struct holy_hfsplus_btree
{
  holy_uint32_t root;
  holy_size_t nodesize;

  /* Catalog file node.  */
  struct holy_hfsplus_file file;
};

/* Information about a "mounted" HFS+ filesystem.  */
struct holy_hfsplus_data
{
  struct holy_hfsplus_volheader volheader;
  holy_disk_t disk;

  unsigned int log2blksize;

  struct holy_hfsplus_btree catalog_tree;
  struct holy_hfsplus_btree extoverflow_tree;
  struct holy_hfsplus_btree attr_tree;

  struct holy_hfsplus_file dirroot;
  struct holy_hfsplus_file opened_file;

  /* This is the offset into the physical disk for an embedded HFS+
     filesystem (one inside a plain HFS wrapper).  */
  holy_disk_addr_t embedded_offset;
  int case_sensitive;
};

/* Internal representation of a catalog key.  */
struct holy_hfsplus_catkey_internal
{
  holy_uint32_t parent;
  const holy_uint16_t *name;
  holy_size_t namelen;
};

/* Internal representation of an extent overflow key.  */
struct holy_hfsplus_extkey_internal
{
  holy_uint32_t fileid;
  holy_uint32_t start;
  holy_uint8_t type;
};

struct holy_hfsplus_attrkey
{
  holy_uint16_t keylen;
  holy_uint16_t unknown1[1];
  holy_uint32_t cnid;
  holy_uint16_t unknown2[2];
  holy_uint16_t namelen;
  holy_uint16_t name[0];
} holy_PACKED;

struct holy_hfsplus_attrkey_internal
{
  holy_uint32_t cnid;
  const holy_uint16_t *name;
  holy_size_t namelen;
};

struct holy_hfsplus_key_internal
{
  union
  {
    struct holy_hfsplus_extkey_internal extkey;
    struct holy_hfsplus_catkey_internal catkey;
    struct holy_hfsplus_attrkey_internal attrkey;
  };
};

/* The on disk layout of a catalog key.  */
struct holy_hfsplus_catkey
{
  holy_uint16_t keylen;
  holy_uint32_t parent;
  holy_uint16_t namelen;
  holy_uint16_t name[0];
} holy_PACKED;

/* The on disk layout of an extent overflow file key.  */
struct holy_hfsplus_extkey
{
  holy_uint16_t keylen;
  holy_uint8_t type;
  holy_uint8_t unused;
  holy_uint32_t fileid;
  holy_uint32_t start;
} holy_PACKED;

struct holy_hfsplus_key
{
  union
  {
    struct holy_hfsplus_extkey extkey;
    struct holy_hfsplus_catkey catkey;
    struct holy_hfsplus_attrkey attrkey;
    holy_uint16_t keylen;
  };
} holy_PACKED;

struct holy_hfsplus_btnode
{
  holy_uint32_t next;
  holy_uint32_t prev;
  holy_int8_t type;
  holy_uint8_t height;
  holy_uint16_t count;
  holy_uint16_t unused;
} holy_PACKED;

/* Return the offset of the record with the index INDEX, in the node
   NODE which is part of the B+ tree BTREE.  */
static inline holy_uint16_t
holy_hfsplus_btree_recoffset (struct holy_hfsplus_btree *btree,
			   struct holy_hfsplus_btnode *node, unsigned index)
{
  char *cnode = (char *) node;
  void *recptr;
  if (btree->nodesize < index * sizeof (holy_uint16_t) + 2)
    index = 0;
  recptr = (&cnode[btree->nodesize - index * sizeof (holy_uint16_t) - 2]);
  return holy_be_to_cpu16 (holy_get_unaligned16 (recptr));
}

/* Return a pointer to the record with the index INDEX, in the node
   NODE which is part of the B+ tree BTREE.  */
static inline struct holy_hfsplus_key *
holy_hfsplus_btree_recptr (struct holy_hfsplus_btree *btree,
			   struct holy_hfsplus_btnode *node, unsigned index)
{
  char *cnode = (char *) node;
  holy_uint16_t offset;
  offset = holy_hfsplus_btree_recoffset (btree, node, index);
  if (offset > btree->nodesize - sizeof (struct holy_hfsplus_key))
    offset = 0;
  return (struct holy_hfsplus_key *) &cnode[offset];
}

extern holy_err_t (*holy_hfsplus_open_compressed) (struct holy_hfsplus_file *node);
extern holy_ssize_t (*holy_hfsplus_read_compressed) (struct holy_hfsplus_file *node,
						     holy_off_t pos,
						     holy_size_t len,
						     char *buf);

holy_ssize_t
holy_hfsplus_read_file (struct holy_hfsplus_file *node,
			holy_disk_read_hook_t read_hook, void *read_hook_data,
			holy_off_t pos, holy_size_t len, char *buf);
holy_err_t
holy_hfsplus_btree_search (struct holy_hfsplus_btree *btree,
			   struct holy_hfsplus_key_internal *key,
			   int (*compare_keys) (struct holy_hfsplus_key *keya,
						struct holy_hfsplus_key_internal *keyb),
			   struct holy_hfsplus_btnode **matchnode,
			   holy_off_t *keyoffset);
holy_err_t
holy_mac_bless_inode (holy_device_t dev, holy_uint32_t inode, int is_dir,
		      int intel);
holy_err_t
holy_mac_bless_file (holy_device_t dev, const char *path_in, int intel);
