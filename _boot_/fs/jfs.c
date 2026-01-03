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
#include <holy/charset.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_JFS_MAX_SYMLNK_CNT	8
#define holy_JFS_FILETYPE_MASK	0170000
#define holy_JFS_FILETYPE_REG	0100000
#define holy_JFS_FILETYPE_LNK	0120000
#define holy_JFS_FILETYPE_DIR	0040000

#define holy_JFS_SBLOCK		64
#define holy_JFS_AGGR_INODE	2
#define holy_JFS_FS1_INODE_BLK	104

#define holy_JFS_TREE_LEAF	2

struct holy_jfs_sblock
{
  /* The magic for JFS.  It should contain the string "JFS1".  */
  holy_uint8_t magic[4];
  holy_uint32_t version;
  holy_uint64_t ag_size;

  /* The size of a filesystem block in bytes.  XXX: currently only
     4096 was tested.  */
  holy_uint32_t blksz;
  holy_uint16_t log2_blksz;
  holy_uint8_t unused[14];
  holy_uint32_t flags;
  holy_uint8_t unused3[61];
  char volname[11];
  holy_uint8_t unused2[24];
  holy_uint8_t uuid[16];
  char volname2[16];
};

struct holy_jfs_extent
{
  /* The length of the extent in filesystem blocks.  */
  holy_uint16_t length;
  holy_uint8_t length2;

  /* The physical offset of the first block on the disk.  */
  holy_uint8_t blk1;
  holy_uint32_t blk2;
} holy_PACKED;

#define holy_JFS_IAG_INODES_OFFSET 3072
#define holy_JFS_IAG_INODES_COUNT 128

struct holy_jfs_iag
{
  holy_uint8_t unused[holy_JFS_IAG_INODES_OFFSET];
  struct holy_jfs_extent inodes[holy_JFS_IAG_INODES_COUNT];
} holy_PACKED;


/* The head of the tree used to find extents.  */
struct holy_jfs_treehead
{
  holy_uint64_t next;
  holy_uint64_t prev;

  holy_uint8_t flags;
  holy_uint8_t unused;

  holy_uint16_t count;
  holy_uint16_t max;
  holy_uint8_t unused2[10];
} holy_PACKED;

/* A node in the extent tree.  */
struct holy_jfs_tree_extent
{
  holy_uint8_t flags;
  holy_uint16_t unused;

  /* The offset is the key used to lookup an extent.  */
  holy_uint8_t offset1;
  holy_uint32_t offset2;

  struct holy_jfs_extent extent;
} holy_PACKED;

/* The tree of directory entries.  */
struct holy_jfs_tree_dir
{
  /* Pointers to the previous and next tree headers of other nodes on
     this level.  */
  holy_uint64_t nextb;
  holy_uint64_t prevb;

  holy_uint8_t flags;

  /* The amount of dirents in this node.  */
  holy_uint8_t count;
  holy_uint8_t freecnt;
  holy_uint8_t freelist;
  holy_uint8_t maxslot;

  /* The location of the sorted array of pointers to dirents.  */
  holy_uint8_t sindex;
  holy_uint8_t unused[10];
} holy_PACKED;

/* An internal node in the dirents tree.  */
struct holy_jfs_internal_dirent
{
  struct holy_jfs_extent ex;
  holy_uint8_t next;
  holy_uint8_t len;
  holy_uint16_t namepart[11];
} holy_PACKED;

/* A leaf node in the dirents tree.  */
struct holy_jfs_leaf_dirent
{
  /* The inode for this dirent.  */
  holy_uint32_t inode;
  holy_uint8_t next;

  /* The size of the name.  */
  holy_uint8_t len;
  holy_uint16_t namepart[11];
  holy_uint32_t index;
} holy_PACKED;

/* A leaf in the dirents tree.  This one is used if the previously
   dirent was not big enough to store the name.  */
struct holy_jfs_leaf_next_dirent
{
  holy_uint8_t next;
  holy_uint8_t len;
  holy_uint16_t namepart[15];
} holy_PACKED;

struct holy_jfs_time
{
  holy_int32_t sec;
  holy_int32_t nanosec;
} holy_PACKED;

struct holy_jfs_inode
{
  holy_uint32_t stamp;
  holy_uint32_t fileset;
  holy_uint32_t inode;
  holy_uint8_t unused[12];
  holy_uint64_t size;
  holy_uint8_t unused2[20];
  holy_uint32_t mode;
  struct holy_jfs_time atime;
  struct holy_jfs_time ctime;
  struct holy_jfs_time mtime;
  holy_uint8_t unused3[48];
  holy_uint8_t unused4[96];

  union
  {
    /* The tree describing the extents of the file.  */
    struct holy_PACKED
    {
      struct holy_jfs_treehead tree;
      struct holy_jfs_tree_extent extents[16];
    } file;
    union
    {
      /* The tree describing the dirents.  */
      struct
      {
	holy_uint8_t unused[16];
	holy_uint8_t flags;

	/* Amount of dirents in this node.  */
	holy_uint8_t count;
	holy_uint8_t freecnt;
	holy_uint8_t freelist;
	holy_uint32_t idotdot;
	holy_uint8_t sorted[8];
      } header;
      struct holy_jfs_leaf_dirent dirents[8];
    } holy_PACKED dir;
    /* Fast symlink.  */
    struct
    {
      holy_uint8_t unused[32];
      holy_uint8_t path[256];
    } symlink;
  } holy_PACKED;
} holy_PACKED;

struct holy_jfs_data
{
  struct holy_jfs_sblock sblock;
  holy_disk_t disk;
  struct holy_jfs_inode fileset;
  struct holy_jfs_inode currinode;
  int caseins;
  int pos;
  int linknest;
  int namecomponentlen;
} holy_PACKED;

struct holy_jfs_diropen
{
  int index;
  union
  {
    struct holy_jfs_tree_dir header;
    struct holy_jfs_leaf_dirent dirent[0];
    struct holy_jfs_leaf_next_dirent next_dirent[0];
    holy_uint8_t sorted[0];
  } holy_PACKED *dirpage;
  struct holy_jfs_data *data;
  struct holy_jfs_inode *inode;
  int count;
  holy_uint8_t *sorted;
  struct holy_jfs_leaf_dirent *leaf;
  struct holy_jfs_leaf_next_dirent *next_leaf;

  /* The filename and inode of the last read dirent.  */
  /* On-disk name is at most 255 UTF-16 codepoints.
     Every UTF-16 codepoint is at most 4 UTF-8 bytes.
   */
  char name[256 * holy_MAX_UTF8_PER_UTF16 + 1];
  holy_uint32_t ino;
} holy_PACKED;


static holy_dl_t my_mod;

static holy_err_t holy_jfs_lookup_symlink (struct holy_jfs_data *data, holy_uint32_t ino);

static holy_int64_t
getblk (struct holy_jfs_treehead *treehead,
	struct holy_jfs_tree_extent *extents,
	struct holy_jfs_data *data,
	holy_uint64_t blk)
{
  int found = -1;
  int i;

  for (i = 0; i < holy_le_to_cpu16 (treehead->count) - 2; i++)
    {
      if (treehead->flags & holy_JFS_TREE_LEAF)
	{
	  /* Read the leafnode.  */
	  if (holy_le_to_cpu32 (extents[i].offset2) <= blk
	      && ((holy_le_to_cpu16 (extents[i].extent.length))
		  + (extents[i].extent.length2 << 16)
		  + holy_le_to_cpu32 (extents[i].offset2)) > blk)
	    return (blk - holy_le_to_cpu32 (extents[i].offset2)
		    + holy_le_to_cpu32 (extents[i].extent.blk2));
	}
      else
	if (blk >= holy_le_to_cpu32 (extents[i].offset2))
	  found = i;
    }

  if (found != -1)
    {
      holy_int64_t ret = -1;
      struct
      {
	struct holy_jfs_treehead treehead;
	struct holy_jfs_tree_extent extents[254];
      } *tree;

      tree = holy_zalloc (sizeof (*tree));
      if (!tree)
	return -1;

      if (!holy_disk_read (data->disk,
			   ((holy_disk_addr_t) holy_le_to_cpu32 (extents[found].extent.blk2))
			   << (holy_le_to_cpu16 (data->sblock.log2_blksz)
			       - holy_DISK_SECTOR_BITS), 0,
			   sizeof (*tree), (char *) tree))
	ret = getblk (&tree->treehead, &tree->extents[0], data, blk);
      holy_free (tree);
      return ret;
    }

  return -1;
}

/* Get the block number for the block BLK in the node INODE in the
   mounted filesystem DATA.  */
static holy_int64_t
holy_jfs_blkno (struct holy_jfs_data *data, struct holy_jfs_inode *inode,
		holy_uint64_t blk)
{
  return getblk (&inode->file.tree, &inode->file.extents[0], data, blk);
}


static holy_err_t
holy_jfs_read_inode (struct holy_jfs_data *data, holy_uint32_t ino,
		     struct holy_jfs_inode *inode)
{
  struct holy_jfs_extent iag_inodes[holy_JFS_IAG_INODES_COUNT];
  holy_uint32_t iagnum = ino / 4096;
  unsigned inoext = (ino % 4096) / 32;
  unsigned inonum = (ino % 4096) % 32;
  holy_uint64_t iagblk;
  holy_uint64_t inoblk;

  iagblk = holy_jfs_blkno (data, &data->fileset, iagnum + 1);
  if (holy_errno)
    return holy_errno;

  /* Read in the IAG.  */
  if (holy_disk_read (data->disk,
		      iagblk << (holy_le_to_cpu16 (data->sblock.log2_blksz)
				 - holy_DISK_SECTOR_BITS),
		      holy_JFS_IAG_INODES_OFFSET,
		      sizeof (iag_inodes), &iag_inodes))
    return holy_errno;

  inoblk = holy_le_to_cpu32 (iag_inodes[inoext].blk2);
  inoblk <<= (holy_le_to_cpu16 (data->sblock.log2_blksz)
	      - holy_DISK_SECTOR_BITS);
  inoblk += inonum;

  if (holy_disk_read (data->disk, inoblk, 0,
		      sizeof (struct holy_jfs_inode), inode))
    return holy_errno;

  return 0;
}


static struct holy_jfs_data *
holy_jfs_mount (holy_disk_t disk)
{
  struct holy_jfs_data *data = 0;

  data = holy_malloc (sizeof (struct holy_jfs_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  if (holy_disk_read (disk, holy_JFS_SBLOCK, 0,
		      sizeof (struct holy_jfs_sblock), &data->sblock))
    goto fail;

  if (holy_strncmp ((char *) (data->sblock.magic), "JFS1", 4))
    {
      holy_error (holy_ERR_BAD_FS, "not a JFS filesystem");
      goto fail;
    }

  if (data->sblock.blksz == 0
      || holy_le_to_cpu32 (data->sblock.blksz)
      != (1U << holy_le_to_cpu16 (data->sblock.log2_blksz))
      || holy_le_to_cpu16 (data->sblock.log2_blksz) < holy_DISK_SECTOR_BITS)
    {
      holy_error (holy_ERR_BAD_FS, "not a JFS filesystem");
      goto fail;
    }

  data->disk = disk;
  data->pos = 0;
  data->linknest = 0;

  /* Read the inode of the first fileset.  */
  if (holy_disk_read (data->disk, holy_JFS_FS1_INODE_BLK, 0,
		      sizeof (struct holy_jfs_inode), &data->fileset))
    goto fail;

  if (data->sblock.flags & holy_cpu_to_le32_compile_time (0x00200000))
    data->namecomponentlen = 11;
  else
    data->namecomponentlen = 13;

  if (data->sblock.flags & holy_cpu_to_le32_compile_time (0x40000000))
    data->caseins = 1;
  else
    data->caseins = 0;

  return data;

 fail:
  holy_free (data);

  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a JFS filesystem");

  return 0;
}


static struct holy_jfs_diropen *
holy_jfs_opendir (struct holy_jfs_data *data, struct holy_jfs_inode *inode)
{
  struct holy_jfs_internal_dirent *de;
  struct holy_jfs_diropen *diro;
  holy_disk_addr_t blk;

  de = (struct holy_jfs_internal_dirent *) inode->dir.dirents;

  if (!((holy_le_to_cpu32 (inode->mode)
	 & holy_JFS_FILETYPE_MASK) == holy_JFS_FILETYPE_DIR))
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
      return 0;
    }

  diro = holy_zalloc (sizeof (struct holy_jfs_diropen));
  if (!diro)
    return 0;

  diro->data = data;
  diro->inode = inode;

  /* Check if the entire tree is contained within the inode.  */
  if (inode->file.tree.flags & holy_JFS_TREE_LEAF)
    {
      diro->leaf = inode->dir.dirents;
      diro->next_leaf = (struct holy_jfs_leaf_next_dirent *) de;
      diro->sorted = inode->dir.header.sorted;
      diro->count = inode->dir.header.count;

      return diro;
    }

  diro->dirpage = holy_malloc (holy_le_to_cpu32 (data->sblock.blksz));
  if (!diro->dirpage)
    {
      holy_free (diro);
      return 0;
    }

  blk = holy_le_to_cpu32 (de[inode->dir.header.sorted[0]].ex.blk2);
  blk <<= (holy_le_to_cpu16 (data->sblock.log2_blksz) - holy_DISK_SECTOR_BITS);

  /* Read in the nodes until we are on the leaf node level.  */
  do
    {
      int index;
      if (holy_disk_read (data->disk, blk, 0,
			  holy_le_to_cpu32 (data->sblock.blksz),
			  diro->dirpage->sorted))
	{
	  holy_free (diro->dirpage);
	  holy_free (diro);
	  return 0;
	}

      de = (struct holy_jfs_internal_dirent *) diro->dirpage->dirent;
      index = diro->dirpage->sorted[diro->dirpage->header.sindex * 32];
      blk = (holy_le_to_cpu32 (de[index].ex.blk2)
	     << (holy_le_to_cpu16 (data->sblock.log2_blksz)
		 - holy_DISK_SECTOR_BITS));
    } while (!(diro->dirpage->header.flags & holy_JFS_TREE_LEAF));

  diro->leaf = diro->dirpage->dirent;
  diro->next_leaf = diro->dirpage->next_dirent;
  diro->sorted = &diro->dirpage->sorted[diro->dirpage->header.sindex * 32];
  diro->count = diro->dirpage->header.count;

  return diro;
}


static void
holy_jfs_closedir (struct holy_jfs_diropen *diro)
{
  if (!diro)
    return;
  holy_free (diro->dirpage);
  holy_free (diro);
}

static void
le_to_cpu16_copy (holy_uint16_t *out, holy_uint16_t *in, holy_size_t len)
{
  while (len--)
    *out++ = holy_le_to_cpu16 (*in++);
}


/* Read in the next dirent from the directory described by DIRO.  */
static holy_err_t
holy_jfs_getent (struct holy_jfs_diropen *diro)
{
  int strpos = 0;
  struct holy_jfs_leaf_dirent *leaf;
  struct holy_jfs_leaf_next_dirent *next_leaf;
  int len;
  int nextent;
  holy_uint16_t filename[256];

  /* The last node, read in more.  */
  if (diro->index == diro->count)
    {
      holy_disk_addr_t next;

      /* If the inode contains the entry tree or if this was the last
	 node, there is nothing to read.  */
      if ((diro->inode->file.tree.flags & holy_JFS_TREE_LEAF)
	  || !holy_le_to_cpu64 (diro->dirpage->header.nextb))
	return holy_ERR_OUT_OF_RANGE;

      next = holy_le_to_cpu64 (diro->dirpage->header.nextb);
      next <<= (holy_le_to_cpu16 (diro->data->sblock.log2_blksz)
		- holy_DISK_SECTOR_BITS);

      if (holy_disk_read (diro->data->disk, next, 0,
			  holy_le_to_cpu32 (diro->data->sblock.blksz),
			  diro->dirpage->sorted))
	return holy_errno;

      diro->leaf = diro->dirpage->dirent;
      diro->next_leaf = diro->dirpage->next_dirent;
      diro->sorted = &diro->dirpage->sorted[diro->dirpage->header.sindex * 32];
      diro->count = diro->dirpage->header.count;
      diro->index = 0;
    }

  leaf = &diro->leaf[diro->sorted[diro->index]];
  next_leaf = &diro->next_leaf[diro->index];

  len = leaf->len;
  if (!len)
    {
      diro->index++;
      return holy_jfs_getent (diro);
    }

  le_to_cpu16_copy (filename + strpos, leaf->namepart, len < diro->data->namecomponentlen ? len
		    : diro->data->namecomponentlen);
  strpos += len < diro->data->namecomponentlen ? len
    : diro->data->namecomponentlen;
  diro->ino = holy_le_to_cpu32 (leaf->inode);
  len -= diro->data->namecomponentlen;

  /* Move down to the leaf level.  */
  nextent = leaf->next;
  if (leaf->next != 255)
    do
      {
 	next_leaf = &diro->next_leaf[nextent];
	le_to_cpu16_copy (filename + strpos, next_leaf->namepart, len < 15 ? len : 15);
	strpos += len < 15 ? len : 15;

	len -= 15;
	nextent = next_leaf->next;
      } while (next_leaf->next != 255 && len > 0);

  diro->index++;

  /* Convert the temporary UTF16 filename to UTF8.  */
  *holy_utf16_to_utf8 ((holy_uint8_t *) (diro->name), filename, strpos) = '\0';

  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_jfs_read_file (struct holy_jfs_data *data,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t pos, holy_size_t len, char *buf)
{
  holy_off_t i;
  holy_off_t blockcnt;

  blockcnt = (len + pos + holy_le_to_cpu32 (data->sblock.blksz) - 1)
    >> holy_le_to_cpu16 (data->sblock.log2_blksz);

  for (i = pos >> holy_le_to_cpu16 (data->sblock.log2_blksz); i < blockcnt; i++)
    {
      holy_disk_addr_t blknr;
      holy_uint32_t blockoff = pos & (holy_le_to_cpu32 (data->sblock.blksz) - 1);
      holy_uint32_t blockend = holy_le_to_cpu32 (data->sblock.blksz);

      holy_uint64_t skipfirst = 0;

      blknr = holy_jfs_blkno (data, &data->currinode, i);
      if (holy_errno)
	return -1;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) & (holy_le_to_cpu32 (data->sblock.blksz) - 1);

	  if (!blockend)
	    blockend = holy_le_to_cpu32 (data->sblock.blksz);
	}

      /* First block.  */
      if (i == (pos >> holy_le_to_cpu16 (data->sblock.log2_blksz)))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      data->disk->read_hook = read_hook;
      data->disk->read_hook_data = read_hook_data;
      holy_disk_read (data->disk,
		      blknr << (holy_le_to_cpu16 (data->sblock.log2_blksz)
				- holy_DISK_SECTOR_BITS),
		      skipfirst, blockend, buf);

      data->disk->read_hook = 0;
      if (holy_errno)
	return -1;

      buf += holy_le_to_cpu32 (data->sblock.blksz) - skipfirst;
    }

  return len;
}


/* Find the file with the pathname PATH on the filesystem described by
   DATA.  */
static holy_err_t
holy_jfs_find_file (struct holy_jfs_data *data, const char *path,
		    holy_uint32_t start_ino)
{
  const char *name;
  const char *next = path;
  struct holy_jfs_diropen *diro = NULL;

  if (holy_jfs_read_inode (data, start_ino, &data->currinode))
    return holy_errno;

  while (1)
    {
      name = next;
      while (*name == '/')
	name++;
      if (name[0] == 0)
	return holy_ERR_NONE;
      for (next = name; *next && *next != '/'; next++);

      if (name[0] == '.' && name + 1 == next)
	continue;

      if (name[0] == '.' && name[1] == '.' && name + 2 == next)
	{
	  holy_uint32_t ino = holy_le_to_cpu32 (data->currinode.dir.header.idotdot);

	  if (holy_jfs_read_inode (data, ino, &data->currinode))
	    return holy_errno;

	  continue;
	}

      diro = holy_jfs_opendir (data, &data->currinode);
      if (!diro)
	return holy_errno;

      for (;;)
	{
	  if (holy_jfs_getent (diro) == holy_ERR_OUT_OF_RANGE)
	    {
	      holy_jfs_closedir (diro);
	      return holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), path);
	    }

	  /* Check if the current direntry matches the current part of the
	     pathname.  */
	  if ((data->caseins ? holy_strncasecmp (name, diro->name, next - name) == 0
	       : holy_strncmp (name, diro->name, next - name) == 0) && !diro->name[next - name])
	    {
	      holy_uint32_t ino = diro->ino;
	      holy_uint32_t dirino = holy_le_to_cpu32 (data->currinode.inode);

	      holy_jfs_closedir (diro);
	      diro = 0;

	      if (holy_jfs_read_inode (data, ino, &data->currinode))
		break;

	      /* Check if this is a symlink.  */
	      if ((holy_le_to_cpu32 (data->currinode.mode)
		   & holy_JFS_FILETYPE_MASK) == holy_JFS_FILETYPE_LNK)
		{
		  holy_jfs_lookup_symlink (data, dirino);
		  if (holy_errno)
		    return holy_errno;
		}

	      break;
	    }
	}
    }
}


static holy_err_t
holy_jfs_lookup_symlink (struct holy_jfs_data *data, holy_uint32_t ino)
{
  holy_size_t size = holy_le_to_cpu64 (data->currinode.size);
  char *symlink;

  if (++data->linknest > holy_JFS_MAX_SYMLNK_CNT)
    return holy_error (holy_ERR_SYMLINK_LOOP, N_("too deep nesting of symlinks"));

  symlink = holy_malloc (size + 1);
  if (!symlink)
    return holy_errno;
  if (size <= sizeof (data->currinode.symlink.path))
    holy_memcpy (symlink, (char *) (data->currinode.symlink.path), size);
  else if (holy_jfs_read_file (data, 0, 0, 0, size, symlink) < 0)
    {
      holy_free (symlink);
      return holy_errno;
    }

  symlink[size] = '\0';

  /* The symlink is an absolute path, go back to the root inode.  */
  if (symlink[0] == '/')
    ino = 2;

  holy_jfs_find_file (data, symlink, ino);

  holy_free (symlink);

  return holy_errno;
}


static holy_err_t
holy_jfs_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_jfs_data *data = 0;
  struct holy_jfs_diropen *diro = 0;

  holy_dl_ref (my_mod);

  data = holy_jfs_mount (device->disk);
  if (!data)
    goto fail;

  if (holy_jfs_find_file (data, path, holy_JFS_AGGR_INODE))
    goto fail;

  diro = holy_jfs_opendir (data, &data->currinode);
  if (!diro)
    goto fail;

  /* Iterate over the dirents in the directory that was found.  */
  while (holy_jfs_getent (diro) != holy_ERR_OUT_OF_RANGE)
    {
      struct holy_jfs_inode inode;
      struct holy_dirhook_info info;
      holy_memset (&info, 0, sizeof (info));

      if (holy_jfs_read_inode (data, diro->ino, &inode))
	goto fail;

      info.dir = (holy_le_to_cpu32 (inode.mode)
		  & holy_JFS_FILETYPE_MASK) == holy_JFS_FILETYPE_DIR;
      info.mtimeset = 1;
      info.mtime = holy_le_to_cpu32 (inode.mtime.sec);
      if (hook (diro->name, &info, hook_data))
	goto fail;
    }

  /* XXX: holy_ERR_OUT_OF_RANGE is used for the last dirent.  */
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_errno = 0;

 fail:
  holy_jfs_closedir (diro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_jfs_open (struct holy_file *file, const char *name)
{
  struct holy_jfs_data *data;

  holy_dl_ref (my_mod);

  data = holy_jfs_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_jfs_find_file (data, name, holy_JFS_AGGR_INODE);
  if (holy_errno)
    goto fail;

  /* It is only possible for open regular files.  */
  if (! ((holy_le_to_cpu32 (data->currinode.mode)
	  & holy_JFS_FILETYPE_MASK) == holy_JFS_FILETYPE_REG))
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a regular file"));
      goto fail;
    }

  file->data = data;
  file->size = holy_le_to_cpu64 (data->currinode.size);

  return 0;

 fail:

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}


static holy_ssize_t
holy_jfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_jfs_data *data =
    (struct holy_jfs_data *) file->data;

  return holy_jfs_read_file (data, file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}


static holy_err_t
holy_jfs_close (holy_file_t file)
{
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

static holy_err_t
holy_jfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_jfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_jfs_mount (disk);
  if (data)
    {
      *uuid = holy_xasprintf ("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-"
			     "%02x%02x%02x%02x%02x%02x",
			     data->sblock.uuid[0], data->sblock.uuid[1],
			     data->sblock.uuid[2], data->sblock.uuid[3],
			     data->sblock.uuid[4], data->sblock.uuid[5],
			     data->sblock.uuid[6], data->sblock.uuid[7],
			     data->sblock.uuid[8], data->sblock.uuid[9],
			     data->sblock.uuid[10], data->sblock.uuid[11],
			     data->sblock.uuid[12], data->sblock.uuid[13],
			     data->sblock.uuid[14], data->sblock.uuid[15]);
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_jfs_label (holy_device_t device, char **label)
{
  struct holy_jfs_data *data;
  data = holy_jfs_mount (device->disk);

  if (data)
    {
      if (data->sblock.volname2[0] < ' ')
	{
	  char *ptr;
	  ptr = data->sblock.volname + sizeof (data->sblock.volname) - 1;
	  while (ptr >= data->sblock.volname && *ptr == ' ')
	    ptr--;
	  *label = holy_strndup (data->sblock.volname,
				 ptr - data->sblock.volname + 1);
	}
      else
	*label = holy_strndup (data->sblock.volname2,
			       sizeof (data->sblock.volname2));
    }
  else
    *label = 0;

  holy_free (data);

  return holy_errno;
}


static struct holy_fs holy_jfs_fs =
  {
    .name = "jfs",
    .dir = holy_jfs_dir,
    .open = holy_jfs_open,
    .read = holy_jfs_read,
    .close = holy_jfs_close,
    .label = holy_jfs_label,
    .uuid = holy_jfs_uuid,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(jfs)
{
  holy_fs_register (&holy_jfs_fs);
  my_mod = mod;
}

holy_MOD_FINI(jfs)
{
  holy_fs_unregister (&holy_jfs_fs);
}
