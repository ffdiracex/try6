/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define FILETYPE_INO_MASK	0170000
#define FILETYPE_INO_REG	0100000
#define FILETYPE_INO_DIRECTORY	0040000
#define FILETYPE_INO_SYMLINK	0120000

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/fshelp.h>

holy_MOD_LICENSE ("GPLv2+");

#define NILFS_INODE_BMAP_SIZE	7

#define NILFS_SUPORT_REV	2

/* Magic value used to identify an nilfs2 filesystem.  */
#define	NILFS2_SUPER_MAGIC	0x3434
/* nilfs btree node flag.  */
#define NILFS_BTREE_NODE_ROOT   0x01

/* nilfs btree node level. */
#define NILFS_BTREE_LEVEL_DATA          0
#define NILFS_BTREE_LEVEL_NODE_MIN      (NILFS_BTREE_LEVEL_DATA + 1)

/* nilfs 1st super block posission from beginning of the partition
   in 512 block size */
#define NILFS_1ST_SUPER_BLOCK	2
/* nilfs 2nd super block posission from beginning of the partition
   in 512 block size */
#define NILFS_2ND_SUPER_BLOCK(devsize)	(((devsize >> 3) - 1) << 3)

#define LOG_INODE_SIZE 7
struct holy_nilfs2_inode
{
  holy_uint64_t i_blocks;
  holy_uint64_t i_size;
  holy_uint64_t i_ctime;
  holy_uint64_t i_mtime;
  holy_uint32_t i_ctime_nsec;
  holy_uint32_t i_mtime_nsec;
  holy_uint32_t i_uid;
  holy_uint32_t i_gid;
  holy_uint16_t i_mode;
  holy_uint16_t i_links_count;
  holy_uint32_t i_flags;
  holy_uint64_t i_bmap[NILFS_INODE_BMAP_SIZE];
#define i_device_code	i_bmap[0]
  holy_uint64_t i_xattr;
  holy_uint32_t i_generation;
  holy_uint32_t i_pad;
};

struct holy_nilfs2_super_root
{
  holy_uint32_t sr_sum;
  holy_uint16_t sr_bytes;
  holy_uint16_t sr_flags;
  holy_uint64_t sr_nongc_ctime;
  struct holy_nilfs2_inode sr_dat;
  struct holy_nilfs2_inode sr_cpfile;
  struct holy_nilfs2_inode sr_sufile;
};

struct holy_nilfs2_super_block
{
  holy_uint32_t s_rev_level;
  holy_uint16_t s_minor_rev_level;
  holy_uint16_t s_magic;
  holy_uint16_t s_bytes;
  holy_uint16_t s_flags;
  holy_uint32_t s_crc_seed;
  holy_uint32_t s_sum;
  holy_uint32_t s_log_block_size;
  holy_uint64_t s_nsegments;
  holy_uint64_t s_dev_size;
  holy_uint64_t s_first_data_block;
  holy_uint32_t s_blocks_per_segment;
  holy_uint32_t s_r_segments_percentage;
  holy_uint64_t s_last_cno;
  holy_uint64_t s_last_pseg;
  holy_uint64_t s_last_seq;
  holy_uint64_t s_free_blocks_count;
  holy_uint64_t s_ctime;
  holy_uint64_t s_mtime;
  holy_uint64_t s_wtime;
  holy_uint16_t s_mnt_count;
  holy_uint16_t s_max_mnt_count;
  holy_uint16_t s_state;
  holy_uint16_t s_errors;
  holy_uint64_t s_lastcheck;
  holy_uint32_t s_checkinterval;
  holy_uint32_t s_creator_os;
  holy_uint16_t s_def_resuid;
  holy_uint16_t s_def_resgid;
  holy_uint32_t s_first_ino;
  holy_uint16_t s_inode_size;
  holy_uint16_t s_dat_entry_size;
  holy_uint16_t s_checkpoint_size;
  holy_uint16_t s_segment_usage_size;
  holy_uint8_t s_uuid[16];
  char s_volume_name[80];
  holy_uint32_t s_c_interval;
  holy_uint32_t s_c_block_max;
  holy_uint32_t s_reserved[192];
};

struct holy_nilfs2_dir_entry
{
  holy_uint64_t inode;
  holy_uint16_t rec_len;
#define MAX_NAMELEN 255
  holy_uint8_t name_len;
  holy_uint8_t file_type;
#if 0				/* followed by file name. */
  char name[NILFS_NAME_LEN];
  char pad;
#endif
} holy_PACKED;

enum
{
  NILFS_FT_UNKNOWN,
  NILFS_FT_REG_FILE,
  NILFS_FT_DIR,
  NILFS_FT_CHRDEV,
  NILFS_FT_BLKDEV,
  NILFS_FT_FIFO,
  NILFS_FT_SOCK,
  NILFS_FT_SYMLINK,
  NILFS_FT_MAX
};

struct holy_nilfs2_finfo
{
  holy_uint64_t fi_ino;
  holy_uint64_t fi_cno;
  holy_uint32_t fi_nblocks;
  holy_uint32_t fi_ndatablk;
};

struct holy_nilfs2_binfo_v
{
  holy_uint64_t bi_vblocknr;
  holy_uint64_t bi_blkoff;
};

struct holy_nilfs2_binfo_dat
{
  holy_uint64_t bi_blkoff;
  holy_uint8_t bi_level;
  holy_uint8_t bi_pad[7];
};

union holy_nilfs2_binfo
{
  struct holy_nilfs2_binfo_v bi_v;
  struct holy_nilfs2_binfo_dat bi_dat;
};

struct holy_nilfs2_segment_summary
{
  holy_uint32_t ss_datasum;
  holy_uint32_t ss_sumsum;
  holy_uint32_t ss_magic;
  holy_uint16_t ss_bytes;
  holy_uint16_t ss_flags;
  holy_uint64_t ss_seq;
  holy_uint64_t ss_create;
  holy_uint64_t ss_next;
  holy_uint32_t ss_nblocks;
  holy_uint32_t ss_nfinfo;
  holy_uint32_t ss_sumbytes;
  holy_uint32_t ss_pad;
};

struct holy_nilfs2_btree_node
{
  holy_uint8_t bn_flags;
  holy_uint8_t bn_level;
  holy_uint16_t bn_nchildren;
  holy_uint32_t bn_pad;
  holy_uint64_t keys[0];
};

struct holy_nilfs2_palloc_group_desc
{
  holy_uint32_t pg_nfrees;
};

#define LOG_SIZE_GROUP_DESC 2

#define LOG_NILFS_DAT_ENTRY_SIZE 5
struct holy_nilfs2_dat_entry
{
  holy_uint64_t de_blocknr;
  holy_uint64_t de_start;
  holy_uint64_t de_end;
  holy_uint64_t de_rsv;
};

struct holy_nilfs2_snapshot_list
{
  holy_uint64_t ssl_next;
  holy_uint64_t ssl_prev;
};

struct holy_nilfs2_cpfile_header
{
  holy_uint64_t ch_ncheckpoints;
  holy_uint64_t ch_nsnapshots;
  struct holy_nilfs2_snapshot_list ch_snapshot_list;
};

struct holy_nilfs2_checkpoint
{
  holy_uint32_t cp_flags;
  holy_uint32_t cp_checkpoints_count;
  struct holy_nilfs2_snapshot_list cp_snapshot_list;
  holy_uint64_t cp_cno;
  holy_uint64_t cp_create;
  holy_uint64_t cp_nblk_inc;
  holy_uint64_t cp_inodes_count;
  holy_uint64_t cp_blocks_count;
  struct holy_nilfs2_inode cp_ifile_inode;
};


#define NILFS_BMAP_LARGE	0x1
#define NILFS_BMAP_SIZE	(NILFS_INODE_BMAP_SIZE * sizeof(holy_uint64_t))

/* nilfs extra padding for nonroot btree node. */
#define NILFS_BTREE_NODE_EXTRA_PAD_SIZE (sizeof(holy_uint64_t))
#define NILFS_BTREE_ROOT_SIZE	NILFS_BMAP_SIZE
#define NILFS_BTREE_ROOT_NCHILDREN_MAX \
 ((NILFS_BTREE_ROOT_SIZE - sizeof(struct nilfs_btree_node)) / \
  (sizeof(holy_uint64_t) + sizeof(holy_uint64_t)) )


struct holy_fshelp_node
{
  struct holy_nilfs2_data *data;
  struct holy_nilfs2_inode inode;
  holy_uint64_t ino;
  int inode_read;
};

struct holy_nilfs2_data
{
  struct holy_nilfs2_super_block sblock;
  struct holy_nilfs2_super_root sroot;
  struct holy_nilfs2_inode ifile;
  holy_disk_t disk;
  struct holy_nilfs2_inode *inode;
  struct holy_fshelp_node diropen;
};

/* Log2 size of nilfs2 block in 512 blocks.  */
#define LOG2_NILFS2_BLOCK_SIZE(data)			\
	(holy_le_to_cpu32 (data->sblock.s_log_block_size) + 1)

/* Log2 size of nilfs2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)					\
	(holy_le_to_cpu32 (data->sblock.s_log_block_size) + 10)

/* The size of an nilfs2 block in bytes.  */
#define NILFS2_BLOCK_SIZE(data)		(1 << LOG2_BLOCK_SIZE (data))

static holy_uint64_t
holy_nilfs2_dat_translate (struct holy_nilfs2_data *data, holy_uint64_t key);
static holy_dl_t my_mod;



static inline unsigned long
holy_nilfs2_log_palloc_entries_per_group (struct holy_nilfs2_data *data)
{
  return LOG2_BLOCK_SIZE (data) + 3;
}

static inline holy_uint64_t
holy_nilfs2_palloc_group (struct holy_nilfs2_data *data,
			  holy_uint64_t nr, holy_uint64_t * offset)
{
  *offset = nr & ((1 << holy_nilfs2_log_palloc_entries_per_group (data)) - 1);
  return nr >> holy_nilfs2_log_palloc_entries_per_group (data);
}

static inline holy_uint32_t
holy_nilfs2_palloc_log_groups_per_desc_block (struct holy_nilfs2_data *data)
{
  return LOG2_BLOCK_SIZE (data) - LOG_SIZE_GROUP_DESC;

  COMPILE_TIME_ASSERT (sizeof (struct holy_nilfs2_palloc_group_desc)
		       == (1 << LOG_SIZE_GROUP_DESC));
}

static inline holy_uint32_t
holy_nilfs2_log_entries_per_block_log (struct holy_nilfs2_data *data,
				       unsigned long log_entry_size)
{
  return LOG2_BLOCK_SIZE (data) - log_entry_size;
}


static inline holy_uint32_t
holy_nilfs2_blocks_per_group_log (struct holy_nilfs2_data *data,
				  unsigned long log_entry_size)
{
  return (1 << (holy_nilfs2_log_palloc_entries_per_group (data)
		- holy_nilfs2_log_entries_per_block_log (data,
							 log_entry_size))) + 1;
}

static inline holy_uint32_t
holy_nilfs2_blocks_per_desc_block_log (struct holy_nilfs2_data *data,
				       unsigned long log_entry_size)
{
  return(holy_nilfs2_blocks_per_group_log (data, log_entry_size)
	 << holy_nilfs2_palloc_log_groups_per_desc_block (data)) + 1;
}

static inline holy_uint32_t
holy_nilfs2_palloc_desc_block_offset_log (struct holy_nilfs2_data *data,
					  unsigned long group,
					  unsigned long log_entry_size)
{
  holy_uint32_t desc_block =
    group >> holy_nilfs2_palloc_log_groups_per_desc_block (data);
  return desc_block * holy_nilfs2_blocks_per_desc_block_log (data,
							     log_entry_size);
}

static inline holy_uint32_t
holy_nilfs2_palloc_bitmap_block_offset (struct holy_nilfs2_data *data,
					unsigned long group,
					unsigned long log_entry_size)
{
  unsigned long desc_offset = group
    & ((1 << holy_nilfs2_palloc_log_groups_per_desc_block (data)) - 1);

  return holy_nilfs2_palloc_desc_block_offset_log (data, group, log_entry_size)
    + 1
    + desc_offset * holy_nilfs2_blocks_per_group_log (data, log_entry_size);
}

static inline holy_uint32_t
holy_nilfs2_palloc_entry_offset_log (struct holy_nilfs2_data *data,
				     holy_uint64_t nr,
				     unsigned long log_entry_size)
{
  unsigned long group;
  holy_uint64_t group_offset;

  group = holy_nilfs2_palloc_group (data, nr, &group_offset);

  return holy_nilfs2_palloc_bitmap_block_offset (data, group,
						 log_entry_size) + 1 +
    (group_offset >> holy_nilfs2_log_entries_per_block_log (data,
							    log_entry_size));

}

static inline struct holy_nilfs2_btree_node *
holy_nilfs2_btree_get_root (struct holy_nilfs2_inode *inode)
{
  return (struct holy_nilfs2_btree_node *) &inode->i_bmap[0];
}

static inline int
holy_nilfs2_btree_get_level (struct holy_nilfs2_btree_node *node)
{
  return node->bn_level;
}

static inline holy_uint64_t *
holy_nilfs2_btree_node_dkeys (struct holy_nilfs2_btree_node *node)
{
  return (node->keys +
	  ((node->bn_flags & NILFS_BTREE_NODE_ROOT) ?
	   0 : (NILFS_BTREE_NODE_EXTRA_PAD_SIZE / sizeof (holy_uint64_t))));
}

static inline holy_uint64_t
holy_nilfs2_btree_node_get_key (struct holy_nilfs2_btree_node *node,
				int index)
{
  return holy_le_to_cpu64 (*(holy_nilfs2_btree_node_dkeys (node) + index));
}

static inline int
holy_nilfs2_btree_node_lookup (struct holy_nilfs2_btree_node *node,
			       holy_uint64_t key, int *indexp)
{
  holy_uint64_t nkey;
  int index, low, high, s;

  low = 0;
  high = holy_le_to_cpu16 (node->bn_nchildren) - 1;
  index = 0;
  s = 0;
  while (low <= high)
    {
      index = (low + high) / 2;
      nkey = holy_nilfs2_btree_node_get_key (node, index);
      if (nkey == key)
	{
	  *indexp = index;
	  return 1;
	}
      else if (nkey < key)
	{
	  low = index + 1;
	  s = -1;
	}
      else
	{
	  high = index - 1;
	  s = 1;
	}
    }

  if (node->bn_level > NILFS_BTREE_LEVEL_NODE_MIN)
    {
      if (s > 0 && index > 0)
	index--;
    }
  else if (s < 0)
    index++;

  *indexp = index;
  return s == 0;
}

static inline int
holy_nilfs2_btree_node_nchildren_max (struct holy_nilfs2_data *data,
				      struct holy_nilfs2_btree_node *node)
{
  int node_children_max = ((NILFS2_BLOCK_SIZE (data) -
			    sizeof (struct holy_nilfs2_btree_node) -
			    NILFS_BTREE_NODE_EXTRA_PAD_SIZE) /
			   (sizeof (holy_uint64_t) + sizeof (holy_uint64_t)));

  return (node->bn_flags & NILFS_BTREE_NODE_ROOT) ? 3 : node_children_max;
}

static inline holy_uint64_t *
holy_nilfs2_btree_node_dptrs (struct holy_nilfs2_data *data,
			      struct holy_nilfs2_btree_node *node)
{
  return (holy_uint64_t *) (holy_nilfs2_btree_node_dkeys (node) +
			    holy_nilfs2_btree_node_nchildren_max (data,
								  node));
}

static inline holy_uint64_t
holy_nilfs2_btree_node_get_ptr (struct holy_nilfs2_data *data,
				struct holy_nilfs2_btree_node *node,
				int index)
{
  return
    holy_le_to_cpu64 (*(holy_nilfs2_btree_node_dptrs (data, node) + index));
}

static inline int
holy_nilfs2_btree_get_nonroot_node (struct holy_nilfs2_data *data,
				    holy_uint64_t ptr, void *block)
{
  holy_disk_t disk = data->disk;
  unsigned int nilfs2_block_count = (1 << LOG2_NILFS2_BLOCK_SIZE (data));

  return holy_disk_read (disk, ptr * nilfs2_block_count, 0,
			 NILFS2_BLOCK_SIZE (data), block);
}

static holy_uint64_t
holy_nilfs2_btree_lookup (struct holy_nilfs2_data *data,
			  struct holy_nilfs2_inode *inode,
			  holy_uint64_t key, int need_translate)
{
  struct holy_nilfs2_btree_node *node;
  void *block;
  holy_uint64_t ptr;
  int level, found = 0, index;

  block = holy_malloc (NILFS2_BLOCK_SIZE (data));
  if (!block)
    return -1;

  node = holy_nilfs2_btree_get_root (inode);
  level = holy_nilfs2_btree_get_level (node);

  found = holy_nilfs2_btree_node_lookup (node, key, &index);
  ptr = holy_nilfs2_btree_node_get_ptr (data, node, index);
  if (need_translate)
    ptr = holy_nilfs2_dat_translate (data, ptr);

  for (level--; level >= NILFS_BTREE_LEVEL_NODE_MIN; level--)
    {
      holy_nilfs2_btree_get_nonroot_node (data, ptr, block);
      if (holy_errno)
	{
	  goto fail;
	}
      node = (struct holy_nilfs2_btree_node *) block;

      if (node->bn_level != level)
	{
	  holy_error (holy_ERR_BAD_FS, "btree level mismatch\n");
	  goto fail;
	}

      if (!found)
	found = holy_nilfs2_btree_node_lookup (node, key, &index);
      else
	index = 0;

      if (index < holy_nilfs2_btree_node_nchildren_max (data, node))
	{
	  ptr = holy_nilfs2_btree_node_get_ptr (data, node, index);
	  if (need_translate)
	    ptr = holy_nilfs2_dat_translate (data, ptr);
	}
      else
	{
	  holy_error (holy_ERR_BAD_FS, "btree corruption\n");
	  goto fail;
	}
    }

  holy_free (block);

  if (!found)
    return -1;

  return ptr;
 fail:
  holy_free (block);
  return -1;
}

static inline holy_uint64_t
holy_nilfs2_direct_lookup (struct holy_nilfs2_inode *inode, holy_uint64_t key)
{
  return holy_le_to_cpu64 (inode->i_bmap[1 + key]);
}

static inline holy_uint64_t
holy_nilfs2_bmap_lookup (struct holy_nilfs2_data *data,
			 struct holy_nilfs2_inode *inode,
			 holy_uint64_t key, int need_translate)
{
  struct holy_nilfs2_btree_node *root = holy_nilfs2_btree_get_root (inode);
  if (root->bn_flags & NILFS_BMAP_LARGE)
    return holy_nilfs2_btree_lookup (data, inode, key, need_translate);
  else
    {
      holy_uint64_t ptr;
      ptr = holy_nilfs2_direct_lookup (inode, key);
      if (need_translate)
	ptr = holy_nilfs2_dat_translate (data, ptr);
      return ptr;
    }
}

static holy_uint64_t
holy_nilfs2_dat_translate (struct holy_nilfs2_data *data, holy_uint64_t key)
{
  struct holy_nilfs2_dat_entry entry;
  holy_disk_t disk = data->disk;
  holy_uint64_t pptr;
  holy_uint64_t blockno, offset;
  unsigned int nilfs2_block_count = (1 << LOG2_NILFS2_BLOCK_SIZE (data));

  blockno = holy_nilfs2_palloc_entry_offset_log (data, key,
						 LOG_NILFS_DAT_ENTRY_SIZE);

  offset = ((key * sizeof (struct holy_nilfs2_dat_entry))
	    & ((1 << LOG2_BLOCK_SIZE (data)) - 1));

  pptr = holy_nilfs2_bmap_lookup (data, &data->sroot.sr_dat, blockno, 0);
  if (pptr == (holy_uint64_t) - 1)
    {
      holy_error (holy_ERR_BAD_FS, "btree lookup failure");
      return -1;
    }

  holy_disk_read (disk, pptr * nilfs2_block_count, offset,
		  sizeof (struct holy_nilfs2_dat_entry), &entry);

  return holy_le_to_cpu64 (entry.de_blocknr);
}


static holy_disk_addr_t
holy_nilfs2_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  struct holy_nilfs2_data *data = node->data;
  struct holy_nilfs2_inode *inode = &node->inode;
  holy_uint64_t pptr = -1;

  pptr = holy_nilfs2_bmap_lookup (data, inode, fileblock, 1);
  if (pptr == (holy_uint64_t) - 1)
    {
      holy_error (holy_ERR_BAD_FS, "btree lookup failure");
      return -1;
    }

  return pptr;
}

/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_nilfs2_read_file (holy_fshelp_node_t node,
		       holy_disk_read_hook_t read_hook, void *read_hook_data,
		       holy_off_t pos, holy_size_t len, char *buf)
{
  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_nilfs2_read_block,
				holy_le_to_cpu64 (node->inode.i_size),
				LOG2_NILFS2_BLOCK_SIZE (node->data), 0);

}

static holy_err_t
holy_nilfs2_read_checkpoint (struct holy_nilfs2_data *data,
			     holy_uint64_t cpno,
			     struct holy_nilfs2_checkpoint *cpp)
{
  holy_uint64_t blockno;
  holy_uint64_t offset;
  holy_uint64_t pptr;
  holy_disk_t disk = data->disk;
  unsigned int nilfs2_block_count = (1 << LOG2_NILFS2_BLOCK_SIZE (data));

  /* Assume sizeof(struct holy_nilfs2_cpfile_header) <
     sizeof(struct holy_nilfs2_checkpoint).
   */
  blockno = holy_divmod64 (cpno, NILFS2_BLOCK_SIZE (data) /
                          sizeof (struct holy_nilfs2_checkpoint), &offset);
  
  pptr = holy_nilfs2_bmap_lookup (data, &data->sroot.sr_cpfile, blockno, 1);
  if (pptr == (holy_uint64_t) - 1)
    {
      return holy_error (holy_ERR_BAD_FS, "btree lookup failure");
    }

  return holy_disk_read (disk, pptr * nilfs2_block_count,
			 offset * sizeof (struct holy_nilfs2_checkpoint),
			 sizeof (struct holy_nilfs2_checkpoint), cpp);
}

static inline holy_err_t
holy_nilfs2_read_last_checkpoint (struct holy_nilfs2_data *data,
				  struct holy_nilfs2_checkpoint *cpp)
{
  return holy_nilfs2_read_checkpoint (data,
				      holy_le_to_cpu64 (data->
							sblock.s_last_cno),
				      cpp);
}

/* Read the inode INO for the file described by DATA into INODE.  */
static holy_err_t
holy_nilfs2_read_inode (struct holy_nilfs2_data *data,
			holy_uint64_t ino, struct holy_nilfs2_inode *inodep)
{
  holy_uint64_t blockno;
  holy_uint64_t offset;
  holy_uint64_t pptr;
  holy_disk_t disk = data->disk;
  unsigned int nilfs2_block_count = (1 << LOG2_NILFS2_BLOCK_SIZE (data));

  blockno = holy_nilfs2_palloc_entry_offset_log (data, ino,
						 LOG_INODE_SIZE);

  offset = ((sizeof (struct holy_nilfs2_inode) * ino)
	    & ((1 << LOG2_BLOCK_SIZE (data)) - 1));
  pptr = holy_nilfs2_bmap_lookup (data, &data->ifile, blockno, 1);
  if (pptr == (holy_uint64_t) - 1)
    {
      return holy_error (holy_ERR_BAD_FS, "btree lookup failure");
    }

  return holy_disk_read (disk, pptr * nilfs2_block_count, offset,
			 sizeof (struct holy_nilfs2_inode), inodep);
}

static int
holy_nilfs2_valid_sb (struct holy_nilfs2_super_block *sbp)
{
  if (holy_le_to_cpu16 (sbp->s_magic) != NILFS2_SUPER_MAGIC)
    return 0;

  if (holy_le_to_cpu32 (sbp->s_rev_level) != NILFS_SUPORT_REV)
    return 0;

  /* 20 already means 1GiB blocks. We don't want to deal with blocks overflowing int32. */
  if (holy_le_to_cpu32 (sbp->s_log_block_size) > 20)
    return 0;

  return 1;
}

static holy_err_t
holy_nilfs2_load_sb (struct holy_nilfs2_data *data)
{
  holy_disk_t disk = data->disk;
  struct holy_nilfs2_super_block sb2;
  holy_uint64_t partition_size;
  int valid[2];
  int swp = 0;
  holy_err_t err;

  /* Read first super block. */
  err = holy_disk_read (disk, NILFS_1ST_SUPER_BLOCK, 0,
			sizeof (struct holy_nilfs2_super_block), &data->sblock);
  if (err)
    return err;
  /* Make sure if 1st super block is valid.  */
  valid[0] = holy_nilfs2_valid_sb (&data->sblock);

  if (valid[0])
    partition_size = (holy_le_to_cpu64 (data->sblock.s_dev_size)
		      >> holy_DISK_SECTOR_BITS);
  else
    partition_size = holy_disk_get_size (disk);
  if (partition_size != holy_DISK_SIZE_UNKNOWN)
    {
      /* Read second super block. */
      err = holy_disk_read (disk, NILFS_2ND_SUPER_BLOCK (partition_size), 0,
			    sizeof (struct holy_nilfs2_super_block), &sb2);
      if (err)
	{
	  valid[1] = 0;
	  holy_errno = holy_ERR_NONE;
	}
      else
	/* Make sure if 2nd super block is valid.  */
	valid[1] = holy_nilfs2_valid_sb (&sb2);
    }
  else
    /* 2nd super block may not exist, so it's invalid. */
    valid[1] = 0;

  if (!valid[0] && !valid[1])
    return holy_error (holy_ERR_BAD_FS, "not a nilfs2 filesystem");

  swp = valid[1] && (!valid[0] ||
		     holy_le_to_cpu64 (data->sblock.s_last_cno) <
		     holy_le_to_cpu64 (sb2.s_last_cno));

  /* swap if first super block is invalid or older than second one. */
  if (swp)
    holy_memcpy (&data->sblock, &sb2,
		 sizeof (struct holy_nilfs2_super_block));

  return holy_ERR_NONE;
}

static struct holy_nilfs2_data *
holy_nilfs2_mount (holy_disk_t disk)
{
  struct holy_nilfs2_data *data;
  struct holy_nilfs2_segment_summary ss;
  struct holy_nilfs2_checkpoint last_checkpoint;
  holy_uint64_t last_pseg;
  holy_uint32_t nblocks;
  unsigned int nilfs2_block_count;

  data = holy_malloc (sizeof (struct holy_nilfs2_data));
  if (!data)
    return 0;

  data->disk = disk;

  /* Read the superblock.  */
  holy_nilfs2_load_sb (data);
  if (holy_errno)
    goto fail;

  nilfs2_block_count = (1 << LOG2_NILFS2_BLOCK_SIZE (data));

  /* Read the last segment summary. */
  last_pseg = holy_le_to_cpu64 (data->sblock.s_last_pseg);
  holy_disk_read (disk, last_pseg * nilfs2_block_count, 0,
		  sizeof (struct holy_nilfs2_segment_summary), &ss);

  if (holy_errno)
    goto fail;

  /* Read the super root block. */
  nblocks = holy_le_to_cpu32 (ss.ss_nblocks);
  holy_disk_read (disk, (last_pseg + (nblocks - 1)) * nilfs2_block_count, 0,
		  sizeof (struct holy_nilfs2_super_root), &data->sroot);

  if (holy_errno)
    goto fail;

  holy_nilfs2_read_last_checkpoint (data, &last_checkpoint);

  if (holy_errno)
    goto fail;

  holy_memcpy (&data->ifile, &last_checkpoint.cp_ifile_inode,
	       sizeof (struct holy_nilfs2_inode));

  data->diropen.data = data;
  data->diropen.ino = 2;
  data->diropen.inode_read = 1;
  data->inode = &data->diropen.inode;

  holy_nilfs2_read_inode (data, 2, data->inode);

  return data;

fail:
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a nilfs2 filesystem");

  holy_free (data);
  return 0;
}

static char *
holy_nilfs2_read_symlink (holy_fshelp_node_t node)
{
  char *symlink;
  struct holy_fshelp_node *diro = node;

  if (!diro->inode_read)
    {
      holy_nilfs2_read_inode (diro->data, diro->ino, &diro->inode);
      if (holy_errno)
	return 0;
    }

  symlink = holy_malloc (holy_le_to_cpu64 (diro->inode.i_size) + 1);
  if (!symlink)
    return 0;

  holy_nilfs2_read_file (diro, 0, 0, 0,
			 holy_le_to_cpu64 (diro->inode.i_size), symlink);
  if (holy_errno)
    {
      holy_free (symlink);
      return 0;
    }

  symlink[holy_le_to_cpu64 (diro->inode.i_size)] = '\0';
  return symlink;
}

static int
holy_nilfs2_iterate_dir (holy_fshelp_node_t dir,
			 holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_off_t fpos = 0;
  struct holy_fshelp_node *diro = (struct holy_fshelp_node *) dir;

  if (!diro->inode_read)
    {
      holy_nilfs2_read_inode (diro->data, diro->ino, &diro->inode);
      if (holy_errno)
	return 0;
    }

  /* Iterate files.  */
  while (fpos < holy_le_to_cpu64 (diro->inode.i_size))
    {
      struct holy_nilfs2_dir_entry dirent;

      holy_nilfs2_read_file (diro, 0, 0, fpos,
			     sizeof (struct holy_nilfs2_dir_entry),
			     (char *) &dirent);
      if (holy_errno)
	return 0;

      if (dirent.rec_len == 0)
	return 0;

      if (dirent.name_len != 0)
	{
	  char filename[MAX_NAMELEN + 1];
	  struct holy_fshelp_node *fdiro;
	  enum holy_fshelp_filetype type = holy_FSHELP_UNKNOWN;

	  holy_nilfs2_read_file (diro, 0, 0,
				 fpos + sizeof (struct holy_nilfs2_dir_entry),
				 dirent.name_len, filename);
	  if (holy_errno)
	    return 0;

	  fdiro = holy_malloc (sizeof (struct holy_fshelp_node));
	  if (!fdiro)
	    return 0;

	  fdiro->data = diro->data;
	  fdiro->ino = holy_le_to_cpu64 (dirent.inode);

	  filename[dirent.name_len] = '\0';

	  if (dirent.file_type != NILFS_FT_UNKNOWN)
	    {
	      fdiro->inode_read = 0;

	      if (dirent.file_type == NILFS_FT_DIR)
		type = holy_FSHELP_DIR;
	      else if (dirent.file_type == NILFS_FT_SYMLINK)
		type = holy_FSHELP_SYMLINK;
	      else if (dirent.file_type == NILFS_FT_REG_FILE)
		type = holy_FSHELP_REG;
	    }
	  else
	    {
	      /* The filetype can not be read from the dirent, read
	         the inode to get more information.  */
	      holy_nilfs2_read_inode (diro->data,
				      holy_le_to_cpu64 (dirent.inode),
				      &fdiro->inode);
	      if (holy_errno)
		{
		  holy_free (fdiro);
		  return 0;
		}

	      fdiro->inode_read = 1;

	      if ((holy_le_to_cpu16 (fdiro->inode.i_mode)
		   & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
		type = holy_FSHELP_DIR;
	      else if ((holy_le_to_cpu16 (fdiro->inode.i_mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
		type = holy_FSHELP_SYMLINK;
	      else if ((holy_le_to_cpu16 (fdiro->inode.i_mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_REG)
		type = holy_FSHELP_REG;
	    }

	  if (hook (filename, type, fdiro, hook_data))
	    return 1;
	}

      fpos += holy_le_to_cpu16 (dirent.rec_len);
    }

  return 0;
}

/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_nilfs2_open (struct holy_file *file, const char *name)
{
  struct holy_nilfs2_data *data = NULL;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_nilfs2_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (name, &data->diropen, &fdiro,
			 holy_nilfs2_iterate_dir, holy_nilfs2_read_symlink,
			 holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  if (!fdiro->inode_read)
    {
      holy_nilfs2_read_inode (data, fdiro->ino, &fdiro->inode);
      if (holy_errno)
	goto fail;
    }

  holy_memcpy (data->inode, &fdiro->inode, sizeof (struct holy_nilfs2_inode));
  holy_free (fdiro);

  file->size = holy_le_to_cpu64 (data->inode->i_size);
  file->data = data;
  file->offset = 0;

  return 0;

fail:
  if (fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_nilfs2_close (holy_file_t file)
{
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static holy_ssize_t
holy_nilfs2_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_nilfs2_data *data = (struct holy_nilfs2_data *) file->data;

  return holy_nilfs2_read_file (&data->diropen,
				file->read_hook, file->read_hook_data,
				file->offset, len, buf);
}

/* Context for holy_nilfs2_dir.  */
struct holy_nilfs2_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
  struct holy_nilfs2_data *data;
};

/* Helper for holy_nilfs2_dir.  */
static int
holy_nilfs2_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		      holy_fshelp_node_t node, void *data)
{
  struct holy_nilfs2_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  if (!node->inode_read)
    {
      holy_nilfs2_read_inode (ctx->data, node->ino, &node->inode);
      if (!holy_errno)
	node->inode_read = 1;
      holy_errno = holy_ERR_NONE;
    }
  if (node->inode_read)
    {
      info.mtimeset = 1;
      info.mtime = holy_le_to_cpu64 (node->inode.i_mtime);
    }

  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_nilfs2_dir (holy_device_t device, const char *path,
		 holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_nilfs2_dir_ctx ctx = {
    .hook = hook,
    .hook_data = hook_data
  };
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  ctx.data = holy_nilfs2_mount (device->disk);
  if (!ctx.data)
    goto fail;

  holy_fshelp_find_file (path, &ctx.data->diropen, &fdiro,
			 holy_nilfs2_iterate_dir, holy_nilfs2_read_symlink,
			 holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_nilfs2_iterate_dir (fdiro, holy_nilfs2_dir_iter, &ctx);

fail:
  if (fdiro != &ctx.data->diropen)
    holy_free (fdiro);
  holy_free (ctx.data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_nilfs2_label (holy_device_t device, char **label)
{
  struct holy_nilfs2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_nilfs2_mount (disk);
  if (data)
    *label = holy_strndup (data->sblock.s_volume_name,
			   sizeof (data->sblock.s_volume_name));
  else
    *label = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_nilfs2_uuid (holy_device_t device, char **uuid)
{
  struct holy_nilfs2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_nilfs2_mount (disk);
  if (data)
    {
      *uuid =
	holy_xasprintf
	("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	 data->sblock.s_uuid[0], data->sblock.s_uuid[1],
	 data->sblock.s_uuid[2], data->sblock.s_uuid[3],
	 data->sblock.s_uuid[4], data->sblock.s_uuid[5],
	 data->sblock.s_uuid[6], data->sblock.s_uuid[7],
	 data->sblock.s_uuid[8], data->sblock.s_uuid[9],
	 data->sblock.s_uuid[10], data->sblock.s_uuid[11],
	 data->sblock.s_uuid[12], data->sblock.s_uuid[13],
	 data->sblock.s_uuid[14], data->sblock.s_uuid[15]);
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

/* Get mtime.  */
static holy_err_t
holy_nilfs2_mtime (holy_device_t device, holy_int32_t * tm)
{
  struct holy_nilfs2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_nilfs2_mount (disk);
  if (!data)
    *tm = 0;
  else
    *tm = (holy_int32_t) holy_le_to_cpu64 (data->sblock.s_wtime);

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}



static struct holy_fs holy_nilfs2_fs = {
  .name = "nilfs2",
  .dir = holy_nilfs2_dir,
  .open = holy_nilfs2_open,
  .read = holy_nilfs2_read,
  .close = holy_nilfs2_close,
  .label = holy_nilfs2_label,
  .uuid = holy_nilfs2_uuid,
  .mtime = holy_nilfs2_mtime,
#ifdef holy_UTIL
  .reserved_first_sector = 1,
  .blocklist_install = 0,
#endif
  .next = 0
};

holy_MOD_INIT (nilfs2)
{
  COMPILE_TIME_ASSERT ((1 << LOG_NILFS_DAT_ENTRY_SIZE)
		       == sizeof (struct
				  holy_nilfs2_dat_entry));
  COMPILE_TIME_ASSERT (1 << LOG_INODE_SIZE
		       == sizeof (struct holy_nilfs2_inode));
  holy_fs_register (&holy_nilfs2_fs);
  my_mod = mod;
}

holy_MOD_FINI (nilfs2)
{
  holy_fs_unregister (&holy_nilfs2_fs);
}
