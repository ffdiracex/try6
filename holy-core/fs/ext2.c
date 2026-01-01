/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define	EXT2_MAGIC		0xEF53
/* Amount of indirect blocks in an inode.  */
#define INDIRECT_BLOCKS		12

/* The good old revision and the default inode size.  */
#define EXT2_GOOD_OLD_REVISION		0
#define EXT2_GOOD_OLD_INODE_SIZE	128

/* Filetype used in directory entry.  */
#define	FILETYPE_UNKNOWN	0
#define	FILETYPE_REG		1
#define	FILETYPE_DIRECTORY	2
#define	FILETYPE_SYMLINK	7

/* Filetype information as used in inodes.  */
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

/* Log2 size of ext2 block in 512 blocks.  */
#define LOG2_EXT2_BLOCK_SIZE(data)			\
	(holy_le_to_cpu32 (data->sblock.log2_block_size) + 1)

/* Log2 size of ext2 block in bytes.  */
#define LOG2_BLOCK_SIZE(data)					\
	(holy_le_to_cpu32 (data->sblock.log2_block_size) + 10)

/* The size of an ext2 block in bytes.  */
#define EXT2_BLOCK_SIZE(data)		(1U << LOG2_BLOCK_SIZE (data))

/* The revision level.  */
#define EXT2_REVISION(data)	holy_le_to_cpu32 (data->sblock.revision_level)

/* The inode size.  */
#define EXT2_INODE_SIZE(data)	\
  (data->sblock.revision_level \
   == holy_cpu_to_le32_compile_time (EXT2_GOOD_OLD_REVISION)	\
         ? EXT2_GOOD_OLD_INODE_SIZE \
         : holy_le_to_cpu16 (data->sblock.inode_size))

/* Superblock filesystem feature flags (RW compatible)
 * A filesystem with any of these enabled can be read and written by a driver
 * that does not understand them without causing metadata/data corruption.  */
#define EXT2_FEATURE_COMPAT_DIR_PREALLOC	0x0001
#define EXT2_FEATURE_COMPAT_IMAGIC_INODES	0x0002
#define EXT3_FEATURE_COMPAT_HAS_JOURNAL		0x0004
#define EXT2_FEATURE_COMPAT_EXT_ATTR		0x0008
#define EXT2_FEATURE_COMPAT_RESIZE_INODE	0x0010
#define EXT2_FEATURE_COMPAT_DIR_INDEX		0x0020
/* Superblock filesystem feature flags (RO compatible)
 * A filesystem with any of these enabled can be safely read by a driver that
 * does not understand them, but should not be written to, usually because
 * additional metadata is required.  */
#define EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER	0x0001
#define EXT2_FEATURE_RO_COMPAT_LARGE_FILE	0x0002
#define EXT2_FEATURE_RO_COMPAT_BTREE_DIR	0x0004
#define EXT4_FEATURE_RO_COMPAT_GDT_CSUM		0x0010
#define EXT4_FEATURE_RO_COMPAT_DIR_NLINK	0x0020
#define EXT4_FEATURE_RO_COMPAT_EXTRA_ISIZE	0x0040
/* Superblock filesystem feature flags (back-incompatible)
 * A filesystem with any of these enabled should not be attempted to be read
 * by a driver that does not understand them, since they usually indicate
 * metadata format changes that might confuse the reader.  */
#define EXT2_FEATURE_INCOMPAT_COMPRESSION	0x0001
#define EXT2_FEATURE_INCOMPAT_FILETYPE		0x0002
#define EXT3_FEATURE_INCOMPAT_RECOVER		0x0004 /* Needs recovery */
#define EXT3_FEATURE_INCOMPAT_JOURNAL_DEV	0x0008 /* Volume is journal device */
#define EXT2_FEATURE_INCOMPAT_META_BG		0x0010
#define EXT4_FEATURE_INCOMPAT_EXTENTS		0x0040 /* Extents used */
#define EXT4_FEATURE_INCOMPAT_64BIT		0x0080
#define EXT4_FEATURE_INCOMPAT_MMP		0x0100
#define EXT4_FEATURE_INCOMPAT_FLEX_BG		0x0200

/* The set of back-incompatible features this driver DOES support. Add (OR)
 * flags here as the related features are implemented into the driver.  */
#define EXT2_DRIVER_SUPPORTED_INCOMPAT ( EXT2_FEATURE_INCOMPAT_FILETYPE \
                                       | EXT4_FEATURE_INCOMPAT_EXTENTS  \
                                       | EXT4_FEATURE_INCOMPAT_FLEX_BG \
                                       | EXT2_FEATURE_INCOMPAT_META_BG \
                                       | EXT4_FEATURE_INCOMPAT_64BIT)
/* List of rationales for the ignored "incompatible" features:
 * needs_recovery: Not really back-incompatible - was added as such to forbid
 *                 ext2 drivers from mounting an ext3 volume with a dirty
 *                 journal because they will ignore the journal, but the next
 *                 ext3 driver to mount the volume will find the journal and
 *                 replay it, potentially corrupting the metadata written by
 *                 the ext2 drivers. Safe to ignore for this RO driver.
 * mmp:            Not really back-incompatible - was added as such to
 *                 avoid multiple read-write mounts. Safe to ignore for this
 *                 RO driver.
 */
#define EXT2_DRIVER_IGNORED_INCOMPAT ( EXT3_FEATURE_INCOMPAT_RECOVER \
				     | EXT4_FEATURE_INCOMPAT_MMP)


#define EXT3_JOURNAL_MAGIC_NUMBER	0xc03b3998U

#define EXT3_JOURNAL_DESCRIPTOR_BLOCK	1
#define EXT3_JOURNAL_COMMIT_BLOCK	2
#define EXT3_JOURNAL_SUPERBLOCK_V1	3
#define EXT3_JOURNAL_SUPERBLOCK_V2	4
#define EXT3_JOURNAL_REVOKE_BLOCK	5

#define EXT3_JOURNAL_FLAG_ESCAPE	1
#define EXT3_JOURNAL_FLAG_SAME_UUID	2
#define EXT3_JOURNAL_FLAG_DELETED	4
#define EXT3_JOURNAL_FLAG_LAST_TAG	8

#define EXT4_EXTENTS_FLAG		0x80000

/* The ext2 superblock.  */
struct holy_ext2_sblock
{
  holy_uint32_t total_inodes;
  holy_uint32_t total_blocks;
  holy_uint32_t reserved_blocks;
  holy_uint32_t free_blocks;
  holy_uint32_t free_inodes;
  holy_uint32_t first_data_block;
  holy_uint32_t log2_block_size;
  holy_uint32_t log2_fragment_size;
  holy_uint32_t blocks_per_group;
  holy_uint32_t fragments_per_group;
  holy_uint32_t inodes_per_group;
  holy_uint32_t mtime;
  holy_uint32_t utime;
  holy_uint16_t mnt_count;
  holy_uint16_t max_mnt_count;
  holy_uint16_t magic;
  holy_uint16_t fs_state;
  holy_uint16_t error_handling;
  holy_uint16_t minor_revision_level;
  holy_uint32_t lastcheck;
  holy_uint32_t checkinterval;
  holy_uint32_t creator_os;
  holy_uint32_t revision_level;
  holy_uint16_t uid_reserved;
  holy_uint16_t gid_reserved;
  holy_uint32_t first_inode;
  holy_uint16_t inode_size;
  holy_uint16_t block_group_number;
  holy_uint32_t feature_compatibility;
  holy_uint32_t feature_incompat;
  holy_uint32_t feature_ro_compat;
  holy_uint16_t uuid[8];
  char volume_name[16];
  char last_mounted_on[64];
  holy_uint32_t compression_info;
  holy_uint8_t prealloc_blocks;
  holy_uint8_t prealloc_dir_blocks;
  holy_uint16_t reserved_gdt_blocks;
  holy_uint8_t journal_uuid[16];
  holy_uint32_t journal_inum;
  holy_uint32_t journal_dev;
  holy_uint32_t last_orphan;
  holy_uint32_t hash_seed[4];
  holy_uint8_t def_hash_version;
  holy_uint8_t jnl_backup_type;
  holy_uint16_t group_desc_size;
  holy_uint32_t default_mount_opts;
  holy_uint32_t first_meta_bg;
  holy_uint32_t mkfs_time;
  holy_uint32_t jnl_blocks[17];
};

/* The ext2 blockgroup.  */
struct holy_ext2_block_group
{
  holy_uint32_t block_id;
  holy_uint32_t inode_id;
  holy_uint32_t inode_table_id;
  holy_uint16_t free_blocks;
  holy_uint16_t free_inodes;
  holy_uint16_t used_dirs;
  holy_uint16_t pad;
  holy_uint32_t reserved[3];
  holy_uint32_t block_id_hi;
  holy_uint32_t inode_id_hi;
  holy_uint32_t inode_table_id_hi;
  holy_uint16_t free_blocks_hi;
  holy_uint16_t free_inodes_hi;
  holy_uint16_t used_dirs_hi;
  holy_uint16_t pad2;
  holy_uint32_t reserved2[3];
};

/* The ext2 inode.  */
struct holy_ext2_inode
{
  holy_uint16_t mode;
  holy_uint16_t uid;
  holy_uint32_t size;
  holy_uint32_t atime;
  holy_uint32_t ctime;
  holy_uint32_t mtime;
  holy_uint32_t dtime;
  holy_uint16_t gid;
  holy_uint16_t nlinks;
  holy_uint32_t blockcnt;  /* Blocks of 512 bytes!! */
  holy_uint32_t flags;
  holy_uint32_t osd1;
  union
  {
    struct datablocks
    {
      holy_uint32_t dir_blocks[INDIRECT_BLOCKS];
      holy_uint32_t indir_block;
      holy_uint32_t double_indir_block;
      holy_uint32_t triple_indir_block;
    } blocks;
    char symlink[60];
  };
  holy_uint32_t version;
  holy_uint32_t acl;
  holy_uint32_t size_high;
  holy_uint32_t fragment_addr;
  holy_uint32_t osd2[3];
};

/* The header of an ext2 directory entry.  */
struct ext2_dirent
{
  holy_uint32_t inode;
  holy_uint16_t direntlen;
#define MAX_NAMELEN 255
  holy_uint8_t namelen;
  holy_uint8_t filetype;
};

struct holy_ext3_journal_header
{
  holy_uint32_t magic;
  holy_uint32_t block_type;
  holy_uint32_t sequence;
};

struct holy_ext3_journal_revoke_header
{
  struct holy_ext3_journal_header header;
  holy_uint32_t count;
  holy_uint32_t data[0];
};

struct holy_ext3_journal_block_tag
{
  holy_uint32_t block;
  holy_uint32_t flags;
};

struct holy_ext3_journal_sblock
{
  struct holy_ext3_journal_header header;
  holy_uint32_t block_size;
  holy_uint32_t maxlen;
  holy_uint32_t first;
  holy_uint32_t sequence;
  holy_uint32_t start;
};

#define EXT4_EXT_MAGIC		0xf30a

struct holy_ext4_extent_header
{
  holy_uint16_t magic;
  holy_uint16_t entries;
  holy_uint16_t max;
  holy_uint16_t depth;
  holy_uint32_t generation;
};

struct holy_ext4_extent
{
  holy_uint32_t block;
  holy_uint16_t len;
  holy_uint16_t start_hi;
  holy_uint32_t start;
};

struct holy_ext4_extent_idx
{
  holy_uint32_t block;
  holy_uint32_t leaf;
  holy_uint16_t leaf_hi;
  holy_uint16_t unused;
};

struct holy_fshelp_node
{
  struct holy_ext2_data *data;
  struct holy_ext2_inode inode;
  int ino;
  int inode_read;
};

/* Information about a "mounted" ext2 filesystem.  */
struct holy_ext2_data
{
  struct holy_ext2_sblock sblock;
  int log_group_desc_size;
  holy_disk_t disk;
  struct holy_ext2_inode *inode;
  struct holy_fshelp_node diropen;
};

static holy_dl_t my_mod;



/* Check is a = b^x for some x.  */
static inline int
is_power_of (holy_uint64_t a, holy_uint32_t b)
{
  holy_uint64_t c;
  /* Prevent overflow assuming b < 8.  */
  if (a >= (1LL << 60))
    return 0;
  for (c = 1; c <= a; c *= b);
  return (c == a);
}


static inline int
group_has_super_block (struct holy_ext2_data *data, holy_uint64_t group)
{
  if (!(data->sblock.feature_ro_compat
	& holy_cpu_to_le32_compile_time(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER)))
    return 1;
  /* Algorithm looked up in Linux source.  */
  if (group <= 1)
    return 1;
  /* Even number is never a power of odd number.  */
  if (!(group & 1))
    return 0;
  return (is_power_of(group, 7) || is_power_of(group, 5) ||
	  is_power_of(group, 3));
}

/* Read into BLKGRP the blockgroup descriptor of blockgroup GROUP of
   the mounted filesystem DATA.  */
inline static holy_err_t
holy_ext2_blockgroup (struct holy_ext2_data *data, holy_uint64_t group,
		      struct holy_ext2_block_group *blkgrp)
{
  holy_uint64_t full_offset = (group << data->log_group_desc_size);
  holy_uint64_t block, offset;
  block = (full_offset >> LOG2_BLOCK_SIZE (data));
  offset = (full_offset & ((1 << LOG2_BLOCK_SIZE (data)) - 1));
  if ((data->sblock.feature_incompat
       & holy_cpu_to_le32_compile_time (EXT2_FEATURE_INCOMPAT_META_BG))
      && block >= holy_le_to_cpu32(data->sblock.first_meta_bg))
    {
      holy_uint64_t first_block_group;
      /* Find the first block group for which a descriptor
	 is stored in given block. */
      first_block_group = (block << (LOG2_BLOCK_SIZE (data)
				     - data->log_group_desc_size));

      block = (first_block_group
	       * holy_le_to_cpu32(data->sblock.blocks_per_group));

      if (group_has_super_block (data, first_block_group))
	block++;
    }
  else
    /* Superblock.  */
    block++;
  return holy_disk_read (data->disk,
                         ((holy_le_to_cpu32 (data->sblock.first_data_block)
			   + block)
                          << LOG2_EXT2_BLOCK_SIZE (data)), offset,
			 sizeof (struct holy_ext2_block_group), blkgrp);
}

static struct holy_ext4_extent_header *
holy_ext4_find_leaf (struct holy_ext2_data *data,
                     struct holy_ext4_extent_header *ext_block,
                     holy_uint32_t fileblock)
{
  struct holy_ext4_extent_idx *index;
  void *buf = NULL;

  while (1)
    {
      int i;
      holy_disk_addr_t block;

      index = (struct holy_ext4_extent_idx *) (ext_block + 1);

      if (ext_block->magic != holy_cpu_to_le16_compile_time (EXT4_EXT_MAGIC))
	goto fail;

      if (ext_block->depth == 0)
        return ext_block;

      for (i = 0; i < holy_le_to_cpu16 (ext_block->entries); i++)
        {
          if (fileblock < holy_le_to_cpu32(index[i].block))
            break;
        }

      if (--i < 0)
	goto fail;

      block = holy_le_to_cpu16 (index[i].leaf_hi);
      block = (block << 32) | holy_le_to_cpu32 (index[i].leaf);
      if (!buf)
	buf = holy_malloc (EXT2_BLOCK_SIZE(data));
      if (!buf)
	goto fail;
      if (holy_disk_read (data->disk,
                          block << LOG2_EXT2_BLOCK_SIZE (data),
                          0, EXT2_BLOCK_SIZE(data), buf))
	goto fail;

      ext_block = buf;
    }
 fail:
  holy_free (buf);
  return 0;
}

static holy_disk_addr_t
holy_ext2_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  struct holy_ext2_data *data = node->data;
  struct holy_ext2_inode *inode = &node->inode;
  unsigned int blksz = EXT2_BLOCK_SIZE (data);
  holy_disk_addr_t blksz_quarter = blksz / 4;
  int log2_blksz = LOG2_EXT2_BLOCK_SIZE (data);
  int log_perblock = log2_blksz + 9 - 2;
  holy_uint32_t indir;
  int shift;

  if (inode->flags & holy_cpu_to_le32_compile_time (EXT4_EXTENTS_FLAG))
    {
      struct holy_ext4_extent_header *leaf;
      struct holy_ext4_extent *ext;
      int i;
      holy_disk_addr_t ret;

      leaf = holy_ext4_find_leaf (data, (struct holy_ext4_extent_header *) inode->blocks.dir_blocks, fileblock);
      if (! leaf)
        {
          holy_error (holy_ERR_BAD_FS, "invalid extent");
          return -1;
        }

      ext = (struct holy_ext4_extent *) (leaf + 1);
      for (i = 0; i < holy_le_to_cpu16 (leaf->entries); i++)
        {
          if (fileblock < holy_le_to_cpu32 (ext[i].block))
            break;
        }

      if (--i >= 0)
        {
          fileblock -= holy_le_to_cpu32 (ext[i].block);
          if (fileblock >= holy_le_to_cpu16 (ext[i].len))
	    ret = 0;
          else
            {
              holy_disk_addr_t start;

              start = holy_le_to_cpu16 (ext[i].start_hi);
              start = (start << 32) + holy_le_to_cpu32 (ext[i].start);

              ret = fileblock + start;
            }
        }
      else
        {
          holy_error (holy_ERR_BAD_FS, "something wrong with extent");
	  ret = -1;
        }

      if (leaf != (struct holy_ext4_extent_header *) inode->blocks.dir_blocks)
	holy_free (leaf);

      return ret;
    }

  /* Direct blocks.  */
  if (fileblock < INDIRECT_BLOCKS)
    return holy_le_to_cpu32 (inode->blocks.dir_blocks[fileblock]);
  fileblock -= INDIRECT_BLOCKS;
  /* Indirect.  */
  if (fileblock < blksz_quarter)
    {
      indir = inode->blocks.indir_block;
      shift = 0;
      goto indirect;
    }
  fileblock -= blksz_quarter;
  /* Double indirect.  */
  if (fileblock < blksz_quarter * blksz_quarter)
    {
      indir = inode->blocks.double_indir_block;
      shift = 1;
      goto indirect;
    }
  fileblock -= blksz_quarter * blksz_quarter;
  /* Triple indirect.  */
  if (fileblock < blksz_quarter * blksz_quarter * (blksz_quarter + 1))
    {
      indir = inode->blocks.triple_indir_block;
      shift = 2;
      goto indirect;
    }
  holy_error (holy_ERR_BAD_FS,
	      "ext2fs doesn't support quadruple indirect blocks");
  return -1;

indirect:
  do {
    /* If the indirect block is zero, all child blocks are absent
       (i.e. filled with zeros.) */
    if (indir == 0)
      return 0;
    if (holy_disk_read (data->disk,
			((holy_disk_addr_t) holy_le_to_cpu32 (indir))
			<< log2_blksz,
			((fileblock >> (log_perblock * shift))
			 & ((1 << log_perblock) - 1))
			* sizeof (indir),
			sizeof (indir), &indir))
      return -1;
  } while (shift--);

  return holy_le_to_cpu32 (indir);
}

/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_ext2_read_file (holy_fshelp_node_t node,
		     holy_disk_read_hook_t read_hook, void *read_hook_data,
		     holy_off_t pos, holy_size_t len, char *buf)
{
  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_ext2_read_block,
				holy_cpu_to_le32 (node->inode.size)
				| (((holy_off_t) holy_cpu_to_le32 (node->inode.size_high)) << 32),
				LOG2_EXT2_BLOCK_SIZE (node->data), 0);

}


/* Read the inode INO for the file described by DATA into INODE.  */
static holy_err_t
holy_ext2_read_inode (struct holy_ext2_data *data,
		      int ino, struct holy_ext2_inode *inode)
{
  struct holy_ext2_block_group blkgrp;
  struct holy_ext2_sblock *sblock = &data->sblock;
  int inodes_per_block;
  unsigned int blkno;
  unsigned int blkoff;
  holy_disk_addr_t base;

  /* It is easier to calculate if the first inode is 0.  */
  ino--;

  holy_ext2_blockgroup (data,
                        ino / holy_le_to_cpu32 (sblock->inodes_per_group),
			&blkgrp);
  if (holy_errno)
    return holy_errno;

  inodes_per_block = EXT2_BLOCK_SIZE (data) / EXT2_INODE_SIZE (data);
  blkno = (ino % holy_le_to_cpu32 (sblock->inodes_per_group))
    / inodes_per_block;
  blkoff = (ino % holy_le_to_cpu32 (sblock->inodes_per_group))
    % inodes_per_block;

  base = holy_le_to_cpu32 (blkgrp.inode_table_id);
  if (data->log_group_desc_size >= 6)
    base |= (((holy_disk_addr_t) holy_le_to_cpu32 (blkgrp.inode_table_id_hi))
	     << 32);

  /* Read the inode.  */
  if (holy_disk_read (data->disk,
		      ((base + blkno) << LOG2_EXT2_BLOCK_SIZE (data)),
		      EXT2_INODE_SIZE (data) * blkoff,
		      sizeof (struct holy_ext2_inode), inode))
    return holy_errno;

  return 0;
}

static struct holy_ext2_data *
holy_ext2_mount (holy_disk_t disk)
{
  struct holy_ext2_data *data;

  data = holy_malloc (sizeof (struct holy_ext2_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  holy_disk_read (disk, 1 * 2, 0, sizeof (struct holy_ext2_sblock),
                  &data->sblock);
  if (holy_errno)
    goto fail;

  /* Make sure this is an ext2 filesystem.  */
  if (data->sblock.magic != holy_cpu_to_le16_compile_time (EXT2_MAGIC)
      || holy_le_to_cpu32 (data->sblock.log2_block_size) >= 16
      || data->sblock.inodes_per_group == 0
      /* 20 already means 1GiB blocks. We don't want to deal with blocks overflowing int32. */
      || holy_le_to_cpu32 (data->sblock.log2_block_size) > 20
      || EXT2_INODE_SIZE (data) == 0
      || EXT2_BLOCK_SIZE (data) / EXT2_INODE_SIZE (data) == 0)
    {
      holy_error (holy_ERR_BAD_FS, "not an ext2 filesystem");
      goto fail;
    }

  /* Check the FS doesn't have feature bits enabled that we don't support. */
  if (data->sblock.revision_level != holy_cpu_to_le32_compile_time (EXT2_GOOD_OLD_REVISION)
      && (data->sblock.feature_incompat
	  & holy_cpu_to_le32_compile_time (~(EXT2_DRIVER_SUPPORTED_INCOMPAT
					     | EXT2_DRIVER_IGNORED_INCOMPAT))))
    {
      holy_error (holy_ERR_BAD_FS, "filesystem has unsupported incompatible features");
      goto fail;
    }

  if (data->sblock.revision_level != holy_cpu_to_le32_compile_time (EXT2_GOOD_OLD_REVISION)
      && (data->sblock.feature_incompat
	  & holy_cpu_to_le32_compile_time (EXT4_FEATURE_INCOMPAT_64BIT))
      && data->sblock.group_desc_size != 0
      && ((data->sblock.group_desc_size & (data->sblock.group_desc_size - 1))
	  == 0)
      && (data->sblock.group_desc_size & holy_cpu_to_le16_compile_time (0x1fe0)))
    {
      holy_uint16_t b = holy_le_to_cpu16 (data->sblock.group_desc_size);
      for (data->log_group_desc_size = 0; b != (1 << data->log_group_desc_size);
	   data->log_group_desc_size++);
    }
  else
    data->log_group_desc_size = 5;

  data->disk = disk;

  data->diropen.data = data;
  data->diropen.ino = 2;
  data->diropen.inode_read = 1;

  data->inode = &data->diropen.inode;

  holy_ext2_read_inode (data, 2, data->inode);
  if (holy_errno)
    goto fail;

  return data;

 fail:
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not an ext2 filesystem");

  holy_free (data);
  return 0;
}

static char *
holy_ext2_read_symlink (holy_fshelp_node_t node)
{
  char *symlink;
  struct holy_fshelp_node *diro = node;

  if (! diro->inode_read)
    {
      holy_ext2_read_inode (diro->data, diro->ino, &diro->inode);
      if (holy_errno)
	return 0;
    }

  symlink = holy_malloc (holy_le_to_cpu32 (diro->inode.size) + 1);
  if (! symlink)
    return 0;

  /* If the filesize of the symlink is bigger than
     60 the symlink is stored in a separate block,
     otherwise it is stored in the inode.  */
  if (holy_le_to_cpu32 (diro->inode.size) <= sizeof (diro->inode.symlink))
    holy_memcpy (symlink,
		 diro->inode.symlink,
		 holy_le_to_cpu32 (diro->inode.size));
  else
    {
      holy_ext2_read_file (diro, 0, 0, 0,
			   holy_le_to_cpu32 (diro->inode.size),
			   symlink);
      if (holy_errno)
	{
	  holy_free (symlink);
	  return 0;
	}
    }

  symlink[holy_le_to_cpu32 (diro->inode.size)] = '\0';
  return symlink;
}

static int
holy_ext2_iterate_dir (holy_fshelp_node_t dir,
		       holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  unsigned int fpos = 0;
  struct holy_fshelp_node *diro = (struct holy_fshelp_node *) dir;

  if (! diro->inode_read)
    {
      holy_ext2_read_inode (diro->data, diro->ino, &diro->inode);
      if (holy_errno)
	return 0;
    }

  /* Search the file.  */
  while (fpos < holy_le_to_cpu32 (diro->inode.size))
    {
      struct ext2_dirent dirent;

      holy_ext2_read_file (diro, 0, 0, fpos, sizeof (struct ext2_dirent),
			   (char *) &dirent);
      if (holy_errno)
	return 0;

      if (dirent.direntlen == 0)
        return 0;

      if (dirent.inode != 0 && dirent.namelen != 0)
	{
	  char filename[MAX_NAMELEN + 1];
	  struct holy_fshelp_node *fdiro;
	  enum holy_fshelp_filetype type = holy_FSHELP_UNKNOWN;

	  holy_ext2_read_file (diro, 0, 0, fpos + sizeof (struct ext2_dirent),
			       dirent.namelen, filename);
	  if (holy_errno)
	    return 0;

	  fdiro = holy_malloc (sizeof (struct holy_fshelp_node));
	  if (! fdiro)
	    return 0;

	  fdiro->data = diro->data;
	  fdiro->ino = holy_le_to_cpu32 (dirent.inode);

	  filename[dirent.namelen] = '\0';

	  if (dirent.filetype != FILETYPE_UNKNOWN)
	    {
	      fdiro->inode_read = 0;

	      if (dirent.filetype == FILETYPE_DIRECTORY)
		type = holy_FSHELP_DIR;
	      else if (dirent.filetype == FILETYPE_SYMLINK)
		type = holy_FSHELP_SYMLINK;
	      else if (dirent.filetype == FILETYPE_REG)
		type = holy_FSHELP_REG;
	    }
	  else
	    {
	      /* The filetype can not be read from the dirent, read
		 the inode to get more information.  */
	      holy_ext2_read_inode (diro->data,
                                    holy_le_to_cpu32 (dirent.inode),
				    &fdiro->inode);
	      if (holy_errno)
		{
		  holy_free (fdiro);
		  return 0;
		}

	      fdiro->inode_read = 1;

	      if ((holy_le_to_cpu16 (fdiro->inode.mode)
		   & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
		type = holy_FSHELP_DIR;
	      else if ((holy_le_to_cpu16 (fdiro->inode.mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
		type = holy_FSHELP_SYMLINK;
	      else if ((holy_le_to_cpu16 (fdiro->inode.mode)
			& FILETYPE_INO_MASK) == FILETYPE_INO_REG)
		type = holy_FSHELP_REG;
	    }

	  if (hook (filename, type, fdiro, hook_data))
	    return 1;
	}

      fpos += holy_le_to_cpu16 (dirent.direntlen);
    }

  return 0;
}

/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_ext2_open (struct holy_file *file, const char *name)
{
  struct holy_ext2_data *data;
  struct holy_fshelp_node *fdiro = 0;
  holy_err_t err;

  holy_dl_ref (my_mod);

  data = holy_ext2_mount (file->device->disk);
  if (! data)
    {
      err = holy_errno;
      goto fail;
    }

  err = holy_fshelp_find_file (name, &data->diropen, &fdiro,
			       holy_ext2_iterate_dir,
			       holy_ext2_read_symlink, holy_FSHELP_REG);
  if (err)
    goto fail;

  if (! fdiro->inode_read)
    {
      err = holy_ext2_read_inode (data, fdiro->ino, &fdiro->inode);
      if (err)
	goto fail;
    }

  holy_memcpy (data->inode, &fdiro->inode, sizeof (struct holy_ext2_inode));
  holy_free (fdiro);

  file->size = holy_le_to_cpu32 (data->inode->size);
  file->size |= ((holy_off_t) holy_le_to_cpu32 (data->inode->size_high)) << 32;
  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return err;
}

static holy_err_t
holy_ext2_close (holy_file_t file)
{
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static holy_ssize_t
holy_ext2_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_ext2_data *data = (struct holy_ext2_data *) file->data;

  return holy_ext2_read_file (&data->diropen,
			      file->read_hook, file->read_hook_data,
			      file->offset, len, buf);
}


/* Context for holy_ext2_dir.  */
struct holy_ext2_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
  struct holy_ext2_data *data;
};

/* Helper for holy_ext2_dir.  */
static int
holy_ext2_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		    holy_fshelp_node_t node, void *data)
{
  struct holy_ext2_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  if (! node->inode_read)
    {
      holy_ext2_read_inode (ctx->data, node->ino, &node->inode);
      if (!holy_errno)
	node->inode_read = 1;
      holy_errno = holy_ERR_NONE;
    }
  if (node->inode_read)
    {
      info.mtimeset = 1;
      info.mtime = holy_le_to_cpu32 (node->inode.mtime);
    }

  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_ext2_dir (holy_device_t device, const char *path, holy_fs_dir_hook_t hook,
	       void *hook_data)
{
  struct holy_ext2_dir_ctx ctx = {
    .hook = hook,
    .hook_data = hook_data
  };
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  ctx.data = holy_ext2_mount (device->disk);
  if (! ctx.data)
    goto fail;

  holy_fshelp_find_file (path, &ctx.data->diropen, &fdiro,
			 holy_ext2_iterate_dir, holy_ext2_read_symlink,
			 holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_ext2_iterate_dir (fdiro, holy_ext2_dir_iter, &ctx);

 fail:
  if (fdiro != &ctx.data->diropen)
    holy_free (fdiro);
  holy_free (ctx.data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_ext2_label (holy_device_t device, char **label)
{
  struct holy_ext2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_ext2_mount (disk);
  if (data)
    *label = holy_strndup (data->sblock.volume_name,
			   sizeof (data->sblock.volume_name));
  else
    *label = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_ext2_uuid (holy_device_t device, char **uuid)
{
  struct holy_ext2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_ext2_mount (disk);
  if (data)
    {
      *uuid = holy_xasprintf ("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
			     holy_be_to_cpu16 (data->sblock.uuid[0]),
			     holy_be_to_cpu16 (data->sblock.uuid[1]),
			     holy_be_to_cpu16 (data->sblock.uuid[2]),
			     holy_be_to_cpu16 (data->sblock.uuid[3]),
			     holy_be_to_cpu16 (data->sblock.uuid[4]),
			     holy_be_to_cpu16 (data->sblock.uuid[5]),
			     holy_be_to_cpu16 (data->sblock.uuid[6]),
			     holy_be_to_cpu16 (data->sblock.uuid[7]));
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

/* Get mtime.  */
static holy_err_t
holy_ext2_mtime (holy_device_t device, holy_int32_t *tm)
{
  struct holy_ext2_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_ext2_mount (disk);
  if (!data)
    *tm = 0;
  else
    *tm = holy_le_to_cpu32 (data->sblock.utime);

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;

}



static struct holy_fs holy_ext2_fs =
  {
    .name = "ext2",
    .dir = holy_ext2_dir,
    .open = holy_ext2_open,
    .read = holy_ext2_read,
    .close = holy_ext2_close,
    .label = holy_ext2_label,
    .uuid = holy_ext2_uuid,
    .mtime = holy_ext2_mtime,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(ext2)
{
  holy_fs_register (&holy_ext2_fs);
  my_mod = mod;
}

holy_MOD_FINI(ext2)
{
  holy_fs_unregister (&holy_ext2_fs);
}
