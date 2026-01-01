/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/fshelp.h>

holy_MOD_LICENSE ("GPLv2+");

#define	XFS_INODE_EXTENTS	9

#define XFS_INODE_FORMAT_INO	1
#define XFS_INODE_FORMAT_EXT	2
#define XFS_INODE_FORMAT_BTREE	3

/* Superblock version field flags */
#define XFS_SB_VERSION_NUMBITS		0x000f
#define	XFS_SB_VERSION_ATTRBIT		0x0010
#define	XFS_SB_VERSION_NLINKBIT		0x0020
#define	XFS_SB_VERSION_QUOTABIT		0x0040
#define	XFS_SB_VERSION_ALIGNBIT		0x0080
#define	XFS_SB_VERSION_DALIGNBIT	0x0100
#define	XFS_SB_VERSION_LOGV2BIT		0x0400
#define	XFS_SB_VERSION_SECTORBIT	0x0800
#define	XFS_SB_VERSION_EXTFLGBIT	0x1000
#define	XFS_SB_VERSION_DIRV2BIT		0x2000
#define XFS_SB_VERSION_MOREBITSBIT	0x8000
#define XFS_SB_VERSION_BITS_SUPPORTED \
	(XFS_SB_VERSION_NUMBITS | \
	 XFS_SB_VERSION_ATTRBIT | \
	 XFS_SB_VERSION_NLINKBIT | \
	 XFS_SB_VERSION_QUOTABIT | \
	 XFS_SB_VERSION_ALIGNBIT | \
	 XFS_SB_VERSION_DALIGNBIT | \
	 XFS_SB_VERSION_LOGV2BIT | \
	 XFS_SB_VERSION_SECTORBIT | \
	 XFS_SB_VERSION_EXTFLGBIT | \
	 XFS_SB_VERSION_DIRV2BIT | \
	 XFS_SB_VERSION_MOREBITSBIT)

/* Recognized xfs format versions */
#define XFS_SB_VERSION_4		4	/* Good old XFS filesystem */
#define XFS_SB_VERSION_5		5	/* CRC enabled filesystem */

/* features2 field flags */
#define XFS_SB_VERSION2_LAZYSBCOUNTBIT	0x00000002	/* Superblk counters */
#define XFS_SB_VERSION2_ATTR2BIT	0x00000008	/* Inline attr rework */
#define XFS_SB_VERSION2_PROJID32BIT	0x00000080	/* 32-bit project ids */
#define XFS_SB_VERSION2_FTYPE		0x00000200	/* inode type in dir */
#define XFS_SB_VERSION2_BITS_SUPPORTED \
	(XFS_SB_VERSION2_LAZYSBCOUNTBIT | \
	 XFS_SB_VERSION2_ATTR2BIT | \
	 XFS_SB_VERSION2_PROJID32BIT | \
	 XFS_SB_VERSION2_FTYPE)

/* incompat feature flags */
#define XFS_SB_FEAT_INCOMPAT_FTYPE      (1 << 0)        /* filetype in dirent */
#define XFS_SB_FEAT_INCOMPAT_SPINODES   (1 << 1)        /* sparse inode chunks */
#define XFS_SB_FEAT_INCOMPAT_META_UUID  (1 << 2)        /* metadata UUID */

/* We do not currently verify metadata UUID so it is safe to read such filesystem */
#define XFS_SB_FEAT_INCOMPAT_SUPPORTED \
	(XFS_SB_FEAT_INCOMPAT_FTYPE | \
	 XFS_SB_FEAT_INCOMPAT_META_UUID)

struct holy_xfs_sblock
{
  holy_uint8_t magic[4];
  holy_uint32_t bsize;
  holy_uint8_t unused1[24];
  holy_uint16_t uuid[8];
  holy_uint8_t unused2[8];
  holy_uint64_t rootino;
  holy_uint8_t unused3[20];
  holy_uint32_t agsize;
  holy_uint8_t unused4[12];
  holy_uint16_t version;
  holy_uint8_t unused5[6];
  holy_uint8_t label[12];
  holy_uint8_t log2_bsize;
  holy_uint8_t log2_sect;
  holy_uint8_t log2_inode;
  holy_uint8_t log2_inop;
  holy_uint8_t log2_agblk;
  holy_uint8_t unused6[67];
  holy_uint8_t log2_dirblk;
  holy_uint8_t unused7[7];
  holy_uint32_t features2;
  holy_uint8_t unused8[4];
  holy_uint32_t sb_features_compat;
  holy_uint32_t sb_features_ro_compat;
  holy_uint32_t sb_features_incompat;
  holy_uint32_t sb_features_log_incompat;
} holy_PACKED;

struct holy_xfs_dir_header
{
  holy_uint8_t count;
  holy_uint8_t largeino;
  union
  {
    holy_uint32_t i4;
    holy_uint64_t i8;
  } holy_PACKED parent;
} holy_PACKED;

/* Structure for directory entry inlined in the inode */
struct holy_xfs_dir_entry
{
  holy_uint8_t len;
  holy_uint16_t offset;
  char name[1];
  /* Inode number follows, 32 / 64 bits.  */
} holy_PACKED;

/* Structure for directory entry in a block */
struct holy_xfs_dir2_entry
{
  holy_uint64_t inode;
  holy_uint8_t len;
} holy_PACKED;

struct holy_xfs_extent
{
  /* This should be a bitfield but bietfields are unportable, so just have
     a raw array and functions extracting useful info from it.
   */
  holy_uint32_t raw[4];
} holy_PACKED;

struct holy_xfs_btree_node
{
  holy_uint8_t magic[4];
  holy_uint16_t level;
  holy_uint16_t numrecs;
  holy_uint64_t left;
  holy_uint64_t right;
  /* In V5 here follow crc, uuid, etc. */
  /* Then follow keys and block pointers */
} holy_PACKED;

struct holy_xfs_btree_root
{
  holy_uint16_t level;
  holy_uint16_t numrecs;
  holy_uint64_t keys[1];
} holy_PACKED;

struct holy_xfs_time
{
  holy_uint32_t sec;
  holy_uint32_t nanosec;
} holy_PACKED;

struct holy_xfs_inode
{
  holy_uint8_t magic[2];
  holy_uint16_t mode;
  holy_uint8_t version;
  holy_uint8_t format;
  holy_uint8_t unused2[26];
  struct holy_xfs_time atime;
  struct holy_xfs_time mtime;
  struct holy_xfs_time ctime;
  holy_uint64_t size;
  holy_uint64_t nblocks;
  holy_uint32_t extsize;
  holy_uint32_t nextents;
  holy_uint16_t unused3;
  holy_uint8_t fork_offset;
  holy_uint8_t unused4[17];
} holy_PACKED;

#define XFS_V2_INODE_SIZE sizeof(struct holy_xfs_inode)
#define XFS_V3_INODE_SIZE (XFS_V2_INODE_SIZE + 76)

struct holy_xfs_dirblock_tail
{
  holy_uint32_t leaf_count;
  holy_uint32_t leaf_stale;
} holy_PACKED;

struct holy_fshelp_node
{
  struct holy_xfs_data *data;
  holy_uint64_t ino;
  int inode_read;
  struct holy_xfs_inode inode;
};

struct holy_xfs_data
{
  struct holy_xfs_sblock sblock;
  holy_disk_t disk;
  int pos;
  int bsize;
  holy_uint32_t agsize;
  unsigned int hasftype:1;
  unsigned int hascrc:1;
  struct holy_fshelp_node diropen;
};

static holy_dl_t my_mod;



static int holy_xfs_sb_hascrc(struct holy_xfs_data *data)
{
  return (data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_NUMBITS)) ==
	  holy_cpu_to_be16_compile_time(XFS_SB_VERSION_5);
}

static int holy_xfs_sb_hasftype(struct holy_xfs_data *data)
{
  if ((data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_NUMBITS)) ==
	holy_cpu_to_be16_compile_time(XFS_SB_VERSION_5) &&
      data->sblock.sb_features_incompat & holy_cpu_to_be32_compile_time(XFS_SB_FEAT_INCOMPAT_FTYPE))
    return 1;
  if (data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_MOREBITSBIT) &&
      data->sblock.features2 & holy_cpu_to_be32_compile_time(XFS_SB_VERSION2_FTYPE))
    return 1;
  return 0;
}

static int holy_xfs_sb_valid(struct holy_xfs_data *data)
{
  holy_dprintf("xfs", "Validating superblock\n");
  if (holy_strncmp ((char *) (data->sblock.magic), "XFSB", 4)
      || data->sblock.log2_bsize < holy_DISK_SECTOR_BITS
      || ((int) data->sblock.log2_bsize
	  + (int) data->sblock.log2_dirblk) >= 27)
    {
      holy_error (holy_ERR_BAD_FS, "not a XFS filesystem");
      return 0;
    }
  if ((data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_NUMBITS)) ==
       holy_cpu_to_be16_compile_time(XFS_SB_VERSION_5))
    {
      holy_dprintf("xfs", "XFS v5 superblock detected\n");
      if (data->sblock.sb_features_incompat &
          holy_cpu_to_be32_compile_time(~XFS_SB_FEAT_INCOMPAT_SUPPORTED))
        {
	  holy_error (holy_ERR_BAD_FS, "XFS filesystem has unsupported "
		      "incompatible features");
	  return 0;
        }
      return 1;
    }
  else if ((data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_NUMBITS)) ==
	   holy_cpu_to_be16_compile_time(XFS_SB_VERSION_4))
    {
      holy_dprintf("xfs", "XFS v4 superblock detected\n");
      if (!(data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_DIRV2BIT)))
	{
	  holy_error (holy_ERR_BAD_FS, "XFS filesystem without V2 directories "
		      "is unsupported");
	  return 0;
	}
      if (data->sblock.version & holy_cpu_to_be16_compile_time(~XFS_SB_VERSION_BITS_SUPPORTED) ||
	  (data->sblock.version & holy_cpu_to_be16_compile_time(XFS_SB_VERSION_MOREBITSBIT) &&
	   data->sblock.features2 & holy_cpu_to_be16_compile_time(~XFS_SB_VERSION2_BITS_SUPPORTED)))
	{
	  holy_error (holy_ERR_BAD_FS, "XFS filesystem has unsupported version "
		      "bits");
	  return 0;
	}
      return 1;
    }
  return 0;
}

/* Filetype information as used in inodes.  */
#define FILETYPE_INO_MASK	0170000
#define FILETYPE_INO_REG	0100000
#define FILETYPE_INO_DIRECTORY	0040000
#define FILETYPE_INO_SYMLINK	0120000

static inline int
holy_XFS_INO_AGBITS(struct holy_xfs_data *data)
{
  return ((data)->sblock.log2_agblk + (data)->sblock.log2_inop);
}

static inline holy_uint64_t
holy_XFS_INO_INOINAG (struct holy_xfs_data *data,
		      holy_uint64_t ino)
{
  return (ino & ((1LL << holy_XFS_INO_AGBITS (data)) - 1));
}

static inline holy_uint64_t
holy_XFS_INO_AG (struct holy_xfs_data *data,
		 holy_uint64_t ino)
{
  return (ino >> holy_XFS_INO_AGBITS (data));
}

static inline holy_disk_addr_t
holy_XFS_FSB_TO_BLOCK (struct holy_xfs_data *data, holy_disk_addr_t fsb)
{
  return ((fsb >> data->sblock.log2_agblk) * data->agsize
	  + (fsb & ((1LL << data->sblock.log2_agblk) - 1)));
}

static inline holy_uint64_t
holy_XFS_EXTENT_OFFSET (struct holy_xfs_extent *exts, int ex)
{
  return ((holy_be_to_cpu32 (exts[ex].raw[0]) & ~(1 << 31)) << 23
	  | holy_be_to_cpu32 (exts[ex].raw[1]) >> 9);
}

static inline holy_uint64_t
holy_XFS_EXTENT_BLOCK (struct holy_xfs_extent *exts, int ex)
{
  return ((holy_uint64_t) (holy_be_to_cpu32 (exts[ex].raw[1])
			   & (0x1ff)) << 43
	  | (holy_uint64_t) holy_be_to_cpu32 (exts[ex].raw[2]) << 11
	  | holy_be_to_cpu32 (exts[ex].raw[3]) >> 21);
}

static inline holy_uint64_t
holy_XFS_EXTENT_SIZE (struct holy_xfs_extent *exts, int ex)
{
  return (holy_be_to_cpu32 (exts[ex].raw[3]) & ((1 << 21) - 1));
}


static inline holy_uint64_t
holy_xfs_inode_block (struct holy_xfs_data *data,
		      holy_uint64_t ino)
{
  long long int inoinag = holy_XFS_INO_INOINAG (data, ino);
  long long ag = holy_XFS_INO_AG (data, ino);
  long long block;

  block = (inoinag >> data->sblock.log2_inop) + ag * data->agsize;
  block <<= (data->sblock.log2_bsize - holy_DISK_SECTOR_BITS);
  return block;
}


static inline int
holy_xfs_inode_offset (struct holy_xfs_data *data,
		       holy_uint64_t ino)
{
  int inoag = holy_XFS_INO_INOINAG (data, ino);
  return ((inoag & ((1 << data->sblock.log2_inop) - 1)) <<
	  data->sblock.log2_inode);
}

static inline holy_size_t
holy_xfs_inode_size(struct holy_xfs_data *data)
{
  return (holy_size_t)1 << data->sblock.log2_inode;
}

/*
 * Returns size occupied by XFS inode stored in memory - we store struct
 * holy_fshelp_node there but on disk inode size may be actually larger than
 * struct holy_xfs_inode so we need to account for that so that we can read
 * from disk directly into in-memory structure.
 */
static inline holy_size_t
holy_xfs_fshelp_size(struct holy_xfs_data *data)
{
  return sizeof (struct holy_fshelp_node) - sizeof (struct holy_xfs_inode)
	       + holy_xfs_inode_size(data);
}

/* This should return void * but XFS code is error-prone with alignment, so
   return char to retain cast-align.
 */
static char *
holy_xfs_inode_data(struct holy_xfs_inode *inode)
{
	if (inode->version <= 2)
		return ((char *)inode) + XFS_V2_INODE_SIZE;
	return ((char *)inode) + XFS_V3_INODE_SIZE;
}

static struct holy_xfs_dir_entry *
holy_xfs_inline_de(struct holy_xfs_dir_header *head)
{
  /*
    With small inode numbers the header is 4 bytes smaller because of
    smaller parent pointer
  */
  return (struct holy_xfs_dir_entry *)
    (((char *) head) + sizeof(struct holy_xfs_dir_header) -
     (head->largeino ? 0 : sizeof(holy_uint32_t)));
}

static holy_uint8_t *
holy_xfs_inline_de_inopos(struct holy_xfs_data *data,
			  struct holy_xfs_dir_entry *de)
{
  return ((holy_uint8_t *)(de + 1)) + de->len - 1 + (data->hasftype ? 1 : 0);
}

static struct holy_xfs_dir_entry *
holy_xfs_inline_next_de(struct holy_xfs_data *data,
			struct holy_xfs_dir_header *head,
			struct holy_xfs_dir_entry *de)
{
  char *p = (char *)de + sizeof(struct holy_xfs_dir_entry) - 1 + de->len;

  p += head->largeino ? sizeof(holy_uint64_t) : sizeof(holy_uint32_t);
  if (data->hasftype)
    p++;

  return (struct holy_xfs_dir_entry *)p;
}

static struct holy_xfs_dirblock_tail *
holy_xfs_dir_tail(struct holy_xfs_data *data, void *dirblock)
{
  int dirblksize = 1 << (data->sblock.log2_bsize + data->sblock.log2_dirblk);

  return (struct holy_xfs_dirblock_tail *)
    ((char *)dirblock + dirblksize - sizeof (struct holy_xfs_dirblock_tail));
}

static struct holy_xfs_dir2_entry *
holy_xfs_first_de(struct holy_xfs_data *data, void *dirblock)
{
  if (data->hascrc)
    return (struct holy_xfs_dir2_entry *)((char *)dirblock + 64);
  return (struct holy_xfs_dir2_entry *)((char *)dirblock + 16);
}

static struct holy_xfs_dir2_entry *
holy_xfs_next_de(struct holy_xfs_data *data, struct holy_xfs_dir2_entry *de)
{
  int size = sizeof (struct holy_xfs_dir2_entry) + de->len + 2 /* Tag */;

  if (data->hasftype)
    size++;		/* File type */
  return (struct holy_xfs_dir2_entry *)(((char *)de) + ALIGN_UP(size, 8));
}

/* This should return void * but XFS code is error-prone with alignment, so
   return char to retain cast-align.
 */
static char *
holy_xfs_btree_keys(struct holy_xfs_data *data,
		    struct holy_xfs_btree_node *leaf)
{
  char *keys = (char *)(leaf + 1);

  if (data->hascrc)
    keys += 48;	/* skip crc, uuid, ... */
  return keys;
}

static holy_err_t
holy_xfs_read_inode (struct holy_xfs_data *data, holy_uint64_t ino,
		     struct holy_xfs_inode *inode)
{
  holy_uint64_t block = holy_xfs_inode_block (data, ino);
  int offset = holy_xfs_inode_offset (data, ino);

  holy_dprintf("xfs", "Reading inode (%"PRIuholy_UINT64_T") - %"PRIuholy_UINT64_T", %d\n",
	       ino, block, offset);
  /* Read the inode.  */
  if (holy_disk_read (data->disk, block, offset, holy_xfs_inode_size(data),
		      inode))
    return holy_errno;

  if (holy_strncmp ((char *) inode->magic, "IN", 2))
    return holy_error (holy_ERR_BAD_FS, "not a correct XFS inode");

  return 0;
}

static holy_uint64_t
get_fsb (const void *keys, int idx)
{
  const char *p = (const char *) keys + sizeof(holy_uint64_t) * idx;
  return holy_be_to_cpu64 (holy_get_unaligned64 (p));
}

static holy_disk_addr_t
holy_xfs_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  struct holy_xfs_btree_node *leaf = 0;
  int ex, nrec;
  struct holy_xfs_extent *exts;
  holy_uint64_t ret = 0;

  if (node->inode.format == XFS_INODE_FORMAT_BTREE)
    {
      struct holy_xfs_btree_root *root;
      const char *keys;
      int recoffset;

      leaf = holy_malloc (node->data->bsize);
      if (leaf == 0)
        return 0;

      root = (struct holy_xfs_btree_root *) holy_xfs_inode_data(&node->inode);
      nrec = holy_be_to_cpu16 (root->numrecs);
      keys = (char *) &root->keys[0];
      if (node->inode.fork_offset)
	recoffset = (node->inode.fork_offset - 1) / 2;
      else
	recoffset = (holy_xfs_inode_size(node->data)
		     - ((char *) keys - (char *) &node->inode))
				/ (2 * sizeof (holy_uint64_t));
      do
        {
          int i;

          for (i = 0; i < nrec; i++)
            {
              if (fileblock < get_fsb(keys, i))
                break;
            }

          /* Sparse block.  */
          if (i == 0)
            {
              holy_free (leaf);
              return 0;
            }

          if (holy_disk_read (node->data->disk,
                              holy_XFS_FSB_TO_BLOCK (node->data, get_fsb (keys, i - 1 + recoffset)) << (node->data->sblock.log2_bsize - holy_DISK_SECTOR_BITS),
                              0, node->data->bsize, leaf))
            return 0;

	  if ((!node->data->hascrc &&
	       holy_strncmp ((char *) leaf->magic, "BMAP", 4)) ||
	      (node->data->hascrc &&
	       holy_strncmp ((char *) leaf->magic, "BMA3", 4)))
            {
              holy_free (leaf);
              holy_error (holy_ERR_BAD_FS, "not a correct XFS BMAP node");
              return 0;
            }

          nrec = holy_be_to_cpu16 (leaf->numrecs);
          keys = holy_xfs_btree_keys(node->data, leaf);
	  recoffset = ((node->data->bsize - ((char *) keys
					     - (char *) leaf))
		       / (2 * sizeof (holy_uint64_t)));
	}
      while (leaf->level);
      exts = (struct holy_xfs_extent *) keys;
    }
  else if (node->inode.format == XFS_INODE_FORMAT_EXT)
    {
      nrec = holy_be_to_cpu32 (node->inode.nextents);
      exts = (struct holy_xfs_extent *) holy_xfs_inode_data(&node->inode);
    }
  else
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "XFS does not support inode format %d yet",
		  node->inode.format);
      return 0;
    }

  /* Iterate over each extent to figure out which extent has
     the block we are looking for.  */
  for (ex = 0; ex < nrec; ex++)
    {
      holy_uint64_t start = holy_XFS_EXTENT_BLOCK (exts, ex);
      holy_uint64_t offset = holy_XFS_EXTENT_OFFSET (exts, ex);
      holy_uint64_t size = holy_XFS_EXTENT_SIZE (exts, ex);

      /* Sparse block.  */
      if (fileblock < offset)
        break;
      else if (fileblock < offset + size)
        {
          ret = (fileblock - offset + start);
          break;
        }
    }

  holy_free (leaf);

  return holy_XFS_FSB_TO_BLOCK(node->data, ret);
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_xfs_read_file (holy_fshelp_node_t node,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t pos, holy_size_t len, char *buf, holy_uint32_t header_size)
{
  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_xfs_read_block,
				holy_be_to_cpu64 (node->inode.size) + header_size,
				node->data->sblock.log2_bsize
				- holy_DISK_SECTOR_BITS, 0);
}


static char *
holy_xfs_read_symlink (holy_fshelp_node_t node)
{
  holy_ssize_t size = holy_be_to_cpu64 (node->inode.size);

  if (size < 0)
    {
      holy_error (holy_ERR_BAD_FS, "invalid symlink");
      return 0;
    }

  switch (node->inode.format)
    {
    case XFS_INODE_FORMAT_INO:
      return holy_strndup (holy_xfs_inode_data(&node->inode), size);

    case XFS_INODE_FORMAT_EXT:
      {
	char *symlink;
	holy_ssize_t numread;
	int off = 0;

	if (node->data->hascrc)
	  off = 56;

	symlink = holy_malloc (size + 1);
	if (!symlink)
	  return 0;

	node->inode.size = holy_be_to_cpu64 (size + off);
	numread = holy_xfs_read_file (node, 0, 0, off, size, symlink, off);
	if (numread != size)
	  {
	    holy_free (symlink);
	    return 0;
	  }
	symlink[size] = '\0';
	return symlink;
      }
    }

  return 0;
}


static enum holy_fshelp_filetype
holy_xfs_mode_to_filetype (holy_uint16_t mode)
{
  if ((holy_be_to_cpu16 (mode)
       & FILETYPE_INO_MASK) == FILETYPE_INO_DIRECTORY)
    return holy_FSHELP_DIR;
  else if ((holy_be_to_cpu16 (mode)
	    & FILETYPE_INO_MASK) == FILETYPE_INO_SYMLINK)
    return holy_FSHELP_SYMLINK;
  else if ((holy_be_to_cpu16 (mode)
	    & FILETYPE_INO_MASK) == FILETYPE_INO_REG)
    return holy_FSHELP_REG;
  return holy_FSHELP_UNKNOWN;
}


/* Context for holy_xfs_iterate_dir.  */
struct holy_xfs_iterate_dir_ctx
{
  holy_fshelp_iterate_dir_hook_t hook;
  void *hook_data;
  struct holy_fshelp_node *diro;
};

/* Helper for holy_xfs_iterate_dir.  */
static int iterate_dir_call_hook (holy_uint64_t ino, const char *filename,
				  struct holy_xfs_iterate_dir_ctx *ctx)
{
  struct holy_fshelp_node *fdiro;
  holy_err_t err;

  fdiro = holy_malloc (holy_xfs_fshelp_size(ctx->diro->data) + 1);
  if (!fdiro)
    {
      holy_print_error ();
      return 0;
    }

  /* The inode should be read, otherwise the filetype can
     not be determined.  */
  fdiro->ino = ino;
  fdiro->inode_read = 1;
  fdiro->data = ctx->diro->data;
  err = holy_xfs_read_inode (ctx->diro->data, ino, &fdiro->inode);
  if (err)
    {
      holy_print_error ();
      return 0;
    }

  return ctx->hook (filename, holy_xfs_mode_to_filetype (fdiro->inode.mode),
		    fdiro, ctx->hook_data);
}

static int
holy_xfs_iterate_dir (holy_fshelp_node_t dir,
		      holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  struct holy_fshelp_node *diro = (struct holy_fshelp_node *) dir;
  struct holy_xfs_iterate_dir_ctx ctx = {
    .hook = hook,
    .hook_data = hook_data,
    .diro = diro
  };

  switch (diro->inode.format)
    {
    case XFS_INODE_FORMAT_INO:
      {
	struct holy_xfs_dir_header *head = (struct holy_xfs_dir_header *) holy_xfs_inode_data(&diro->inode);
	struct holy_xfs_dir_entry *de = holy_xfs_inline_de(head);
	int smallino = !head->largeino;
	int i;
	holy_uint64_t parent;

	/* If small inode numbers are used to pack the direntry, the
	   parent inode number is small too.  */
	if (smallino)
	  parent = holy_be_to_cpu32 (head->parent.i4);
	else
	  parent = holy_be_to_cpu64 (head->parent.i8);

	/* Synthesize the direntries for `.' and `..'.  */
	if (iterate_dir_call_hook (diro->ino, ".", &ctx))
	  return 1;

	if (iterate_dir_call_hook (parent, "..", &ctx))
	  return 1;

	for (i = 0; i < head->count; i++)
	  {
	    holy_uint64_t ino;
	    holy_uint8_t *inopos = holy_xfs_inline_de_inopos(dir->data, de);
	    holy_uint8_t c;

	    /* inopos might be unaligned.  */
	    if (smallino)
	      ino = (((holy_uint32_t) inopos[0]) << 24)
		| (((holy_uint32_t) inopos[1]) << 16)
		| (((holy_uint32_t) inopos[2]) << 8)
		| (((holy_uint32_t) inopos[3]) << 0);
	    else
	      ino = (((holy_uint64_t) inopos[0]) << 56)
		| (((holy_uint64_t) inopos[1]) << 48)
		| (((holy_uint64_t) inopos[2]) << 40)
		| (((holy_uint64_t) inopos[3]) << 32)
		| (((holy_uint64_t) inopos[4]) << 24)
		| (((holy_uint64_t) inopos[5]) << 16)
		| (((holy_uint64_t) inopos[6]) << 8)
		| (((holy_uint64_t) inopos[7]) << 0);

	    c = de->name[de->len];
	    de->name[de->len] = '\0';
	    if (iterate_dir_call_hook (ino, de->name, &ctx))
	      {
		de->name[de->len] = c;
		return 1;
	      }
	    de->name[de->len] = c;

	    de = holy_xfs_inline_next_de(dir->data, head, de);
	  }
	break;
      }

    case XFS_INODE_FORMAT_BTREE:
    case XFS_INODE_FORMAT_EXT:
      {
	holy_ssize_t numread;
	char *dirblock;
	holy_uint64_t blk;
        int dirblk_size, dirblk_log2;

        dirblk_log2 = (dir->data->sblock.log2_bsize
                       + dir->data->sblock.log2_dirblk);
        dirblk_size = 1 << dirblk_log2;

	dirblock = holy_malloc (dirblk_size);
	if (! dirblock)
	  return 0;

	/* Iterate over every block the directory has.  */
	for (blk = 0;
	     blk < (holy_be_to_cpu64 (dir->inode.size)
		    >> dirblk_log2);
	     blk++)
	  {
	    struct holy_xfs_dir2_entry *direntry =
					holy_xfs_first_de(dir->data, dirblock);
	    int entries;
	    struct holy_xfs_dirblock_tail *tail =
					holy_xfs_dir_tail(dir->data, dirblock);

	    numread = holy_xfs_read_file (dir, 0, 0,
					  blk << dirblk_log2,
					  dirblk_size, dirblock, 0);
	    if (numread != dirblk_size)
	      return 0;

	    entries = (holy_be_to_cpu32 (tail->leaf_count)
		       - holy_be_to_cpu32 (tail->leaf_stale));

	    /* Iterate over all entries within this block.  */
	    while ((char *)direntry < (char *)tail)
	      {
		holy_uint8_t *freetag;
		char *filename;

		freetag = (holy_uint8_t *) direntry;

		if (holy_get_unaligned16 (freetag) == 0XFFFF)
		  {
		    holy_uint8_t *skip = (freetag + sizeof (holy_uint16_t));

		    /* This entry is not used, go to the next one.  */
		    direntry = (struct holy_xfs_dir2_entry *)
				(((char *)direntry) +
				holy_be_to_cpu16 (holy_get_unaligned16 (skip)));

		    continue;
		  }

		filename = (char *)(direntry + 1);
		/* The byte after the filename is for the filetype, padding, or
		   tag, which is not used by holy.  So it can be overwritten. */
		filename[direntry->len] = '\0';

		if (iterate_dir_call_hook (holy_be_to_cpu64(direntry->inode),
					   filename, &ctx))
		  {
		    holy_free (dirblock);
		    return 1;
		  }

		/* Check if last direntry in this block is
		   reached.  */
		entries--;
		if (!entries)
		  break;

		/* Select the next directory entry.  */
		direntry = holy_xfs_next_de(dir->data, direntry);
	      }
	  }
	holy_free (dirblock);
	break;
      }

    default:
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "XFS does not support inode format %d yet",
		  diro->inode.format);
    }
  return 0;
}


static struct holy_xfs_data *
holy_xfs_mount (holy_disk_t disk)
{
  struct holy_xfs_data *data = 0;

  data = holy_zalloc (sizeof (struct holy_xfs_data));
  if (!data)
    return 0;

  holy_dprintf("xfs", "Reading sb\n");
  /* Read the superblock.  */
  if (holy_disk_read (disk, 0, 0,
		      sizeof (struct holy_xfs_sblock), &data->sblock))
    goto fail;

  if (!holy_xfs_sb_valid(data))
    goto fail;

  data = holy_realloc (data,
		       sizeof (struct holy_xfs_data)
		       - sizeof (struct holy_xfs_inode)
		       + holy_xfs_inode_size(data) + 1);

  if (! data)
    goto fail;

  data->diropen.data = data;
  data->diropen.ino = holy_be_to_cpu64(data->sblock.rootino);
  data->diropen.inode_read = 1;
  data->bsize = holy_be_to_cpu32 (data->sblock.bsize);
  data->agsize = holy_be_to_cpu32 (data->sblock.agsize);
  data->hasftype = holy_xfs_sb_hasftype(data);
  data->hascrc = holy_xfs_sb_hascrc(data);

  data->disk = disk;
  data->pos = 0;
  holy_dprintf("xfs", "Reading root ino %"PRIuholy_UINT64_T"\n",
	       holy_cpu_to_be64(data->sblock.rootino));

  holy_xfs_read_inode (data, data->diropen.ino, &data->diropen.inode);

  return data;
 fail:

  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not an XFS filesystem");

  holy_free (data);

  return 0;
}


/* Context for holy_xfs_dir.  */
struct holy_xfs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_xfs_dir.  */
static int
holy_xfs_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		   holy_fshelp_node_t node, void *data)
{
  struct holy_xfs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  if (node->inode_read)
    {
      info.mtimeset = 1;
      info.mtime = holy_be_to_cpu32 (node->inode.mtime.sec);
    }
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_xfs_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_xfs_dir_ctx ctx = { hook, hook_data };
  struct holy_xfs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_xfs_mount (device->disk);
  if (!data)
    goto mount_fail;

  holy_fshelp_find_file (path, &data->diropen, &fdiro, holy_xfs_iterate_dir,
			 holy_xfs_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_xfs_iterate_dir (fdiro, holy_xfs_dir_iter, &ctx);

 fail:
  if (fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

 mount_fail:

  holy_dl_unref (my_mod);

  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_xfs_open (struct holy_file *file, const char *name)
{
  struct holy_xfs_data *data;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_xfs_mount (file->device->disk);
  if (!data)
    goto mount_fail;

  holy_fshelp_find_file (name, &data->diropen, &fdiro, holy_xfs_iterate_dir,
			 holy_xfs_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  if (!fdiro->inode_read)
    {
      holy_xfs_read_inode (data, fdiro->ino, &fdiro->inode);
      if (holy_errno)
	goto fail;
    }

  if (fdiro != &data->diropen)
    {
      holy_memcpy (&data->diropen, fdiro, holy_xfs_fshelp_size(data));
      holy_free (fdiro);
    }

  file->size = holy_be_to_cpu64 (data->diropen.inode.size);
  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

 mount_fail:
  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_ssize_t
holy_xfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_xfs_data *data =
    (struct holy_xfs_data *) file->data;

  return holy_xfs_read_file (&data->diropen,
			     file->read_hook, file->read_hook_data,
			     file->offset, len, buf, 0);
}


static holy_err_t
holy_xfs_close (holy_file_t file)
{
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}


static holy_err_t
holy_xfs_label (holy_device_t device, char **label)
{
  struct holy_xfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_xfs_mount (disk);
  if (data)
    *label = holy_strndup ((char *) (data->sblock.label), 12);
  else
    *label = 0;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_xfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_xfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_xfs_mount (disk);
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



static struct holy_fs holy_xfs_fs =
  {
    .name = "xfs",
    .dir = holy_xfs_dir,
    .open = holy_xfs_open,
    .read = holy_xfs_read,
    .close = holy_xfs_close,
    .label = holy_xfs_label,
    .uuid = holy_xfs_uuid,
#ifdef holy_UTIL
    .reserved_first_sector = 0,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(xfs)
{
  holy_fs_register (&holy_xfs_fs);
  my_mod = mod;
}

holy_MOD_FINI(xfs)
{
  holy_fs_unregister (&holy_xfs_fs);
}
