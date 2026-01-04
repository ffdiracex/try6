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
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef MODE_UFS2
#define holy_UFS_MAGIC		0x19540119
#else
#define holy_UFS_MAGIC		0x11954
#endif

#define holy_UFS_INODE		2
#define holy_UFS_FILETYPE_DIR	4
#define holy_UFS_FILETYPE_LNK	10
#define holy_UFS_MAX_SYMLNK_CNT	8

#define holy_UFS_DIRBLKS	12
#define holy_UFS_INDIRBLKS	3

#define holy_UFS_ATTR_TYPE      0160000
#define holy_UFS_ATTR_FILE	0100000
#define holy_UFS_ATTR_DIR	0040000
#define holy_UFS_ATTR_LNK       0120000

#define holy_UFS_VOLNAME_LEN	32

#ifdef MODE_BIGENDIAN
#define holy_ufs_to_cpu16 holy_be_to_cpu16
#define holy_ufs_to_cpu32 holy_be_to_cpu32
#define holy_ufs_to_cpu64 holy_be_to_cpu64
#define holy_cpu_to_ufs32_compile_time holy_cpu_to_be32_compile_time
#else
#define holy_ufs_to_cpu16 holy_le_to_cpu16
#define holy_ufs_to_cpu32 holy_le_to_cpu32
#define holy_ufs_to_cpu64 holy_le_to_cpu64
#define holy_cpu_to_ufs32_compile_time holy_cpu_to_le32_compile_time
#endif

#ifdef MODE_UFS2
typedef holy_uint64_t holy_ufs_blk_t;
static inline holy_disk_addr_t
holy_ufs_to_cpu_blk (holy_ufs_blk_t blk)
{
  return holy_ufs_to_cpu64 (blk);
}
#else
typedef holy_uint32_t holy_ufs_blk_t;
static inline holy_disk_addr_t
holy_ufs_to_cpu_blk (holy_ufs_blk_t blk)
{
  return holy_ufs_to_cpu32 (blk);
}
#endif

/* Calculate in which group the inode can be found.  */
#define UFS_BLKSZ(sblock) (holy_ufs_to_cpu32 (sblock->bsize))
#define UFS_LOG_BLKSZ(sblock) (data->log2_blksz)

#ifdef MODE_UFS2
#define INODE_ENDIAN(data,field,bits1,bits2) holy_ufs_to_cpu##bits2 (data->inode.field)
#else
#define INODE_ENDIAN(data,field,bits1,bits2) holy_ufs_to_cpu##bits1 (data->inode.field)
#endif

#define INODE_SIZE(data) holy_ufs_to_cpu64 (data->inode.size)
#define INODE_MODE(data) holy_ufs_to_cpu16 (data->inode.mode)
#ifdef MODE_UFS2
#define LOG_INODE_BLKSZ 3
#else
#define LOG_INODE_BLKSZ 2
#endif
#ifdef MODE_UFS2
#define UFS_INODE_PER_BLOCK 2
#else
#define UFS_INODE_PER_BLOCK 4
#endif
#define INODE_DIRBLOCKS(data,blk) INODE_ENDIAN \
                                   (data,blocks.dir_blocks[blk],32,64)
#define INODE_INDIRBLOCKS(data,blk) INODE_ENDIAN \
                                     (data,blocks.indir_blocks[blk],32,64)

/* The blocks on which the superblock can be found.  */
static int sblocklist[] = { 128, 16, 0, 512, -1 };

struct holy_ufs_sblock
{
  holy_uint8_t unused[16];
  /* The offset of the inodes in the cylinder group.  */
  holy_uint32_t inoblk_offs;

  holy_uint8_t unused2[4];

  /* The start of the cylinder group.  */
  holy_uint32_t cylg_offset;
  holy_uint32_t cylg_mask;

  holy_uint32_t mtime;
  holy_uint8_t unused4[12];

  /* The size of a block in bytes.  */
  holy_int32_t bsize;
  holy_uint8_t unused5[48];

  /* The size of filesystem blocks to disk blocks.  */
  holy_uint32_t log2_blksz;
  holy_uint8_t unused6[40];
  holy_uint32_t uuidhi;
  holy_uint32_t uuidlow;
  holy_uint8_t unused7[32];

  /* Inodes stored per cylinder group.  */
  holy_uint32_t ino_per_group;

  /* The frags per cylinder group.  */
  holy_uint32_t frags_per_group;

  holy_uint8_t unused8[488];

  /* Volume name for UFS2.  */
  holy_uint8_t volume_name[holy_UFS_VOLNAME_LEN];
  holy_uint8_t unused9[360];

  holy_uint64_t mtime2;
  holy_uint8_t unused10[292];

  /* Magic value to check if this is really a UFS filesystem.  */
  holy_uint32_t magic;
};

#ifdef MODE_UFS2
/* UFS inode.  */
struct holy_ufs_inode
{
  holy_uint16_t mode;
  holy_uint16_t nlinks;
  holy_uint32_t uid;
  holy_uint32_t gid;
  holy_uint32_t blocksize;
  holy_uint64_t size;
  holy_int64_t nblocks;
  holy_uint64_t atime;
  holy_uint64_t mtime;
  holy_uint64_t ctime;
  holy_uint64_t create_time;
  holy_uint32_t atime_usec;
  holy_uint32_t mtime_usec;
  holy_uint32_t ctime_usec;
  holy_uint32_t create_time_sec;
  holy_uint32_t gen;
  holy_uint32_t kernel_flags;
  holy_uint32_t flags;
  holy_uint32_t extsz;
  holy_uint64_t ext[2];
  union
  {
    struct
    {
      holy_uint64_t dir_blocks[holy_UFS_DIRBLKS];
      holy_uint64_t indir_blocks[holy_UFS_INDIRBLKS];
    } blocks;
    holy_uint8_t symlink[(holy_UFS_DIRBLKS + holy_UFS_INDIRBLKS) * 8];
  };

  holy_uint8_t unused[24];
} holy_PACKED;
#else
/* UFS inode.  */
struct holy_ufs_inode
{
  holy_uint16_t mode;
  holy_uint16_t nlinks;
  holy_uint16_t uid;
  holy_uint16_t gid;
  holy_uint64_t size;
  holy_uint32_t atime;
  holy_uint32_t atime_usec;
  holy_uint32_t mtime;
  holy_uint32_t mtime_usec;
  holy_uint32_t ctime;
  holy_uint32_t ctime_usec;
  union
  {
    struct
    {
      holy_uint32_t dir_blocks[holy_UFS_DIRBLKS];
      holy_uint32_t indir_blocks[holy_UFS_INDIRBLKS];
    } blocks;
    holy_uint8_t symlink[(holy_UFS_DIRBLKS + holy_UFS_INDIRBLKS) * 4];
  };
  holy_uint32_t flags;
  holy_uint32_t nblocks;
  holy_uint32_t gen;
  holy_uint32_t unused;
  holy_uint8_t pad[12];
} holy_PACKED;
#endif

/* Directory entry.  */
struct holy_ufs_dirent
{
  holy_uint32_t ino;
  holy_uint16_t direntlen;
  union
  {
    holy_uint16_t namelen;
    struct
    {
      holy_uint8_t filetype_bsd;
      holy_uint8_t namelen_bsd;
    };
  };
} holy_PACKED;

/* Information about a "mounted" ufs filesystem.  */
struct holy_ufs_data
{
  struct holy_ufs_sblock sblock;
  holy_disk_t disk;
  struct holy_ufs_inode inode;
  int ino;
  int linknest;
  int log2_blksz;
};

static holy_dl_t my_mod;

/* Forward declaration.  */
static holy_err_t holy_ufs_find_file (struct holy_ufs_data *data,
				      const char *path);


static holy_disk_addr_t
holy_ufs_get_file_block (struct holy_ufs_data *data, holy_disk_addr_t blk)
{
  unsigned long indirsz;
  int log2_blksz, log_indirsz;

  /* Direct.  */
  if (blk < holy_UFS_DIRBLKS)
    return INODE_DIRBLOCKS (data, blk);

  log2_blksz = holy_ufs_to_cpu32 (data->sblock.log2_blksz);

  blk -= holy_UFS_DIRBLKS;

  log_indirsz = data->log2_blksz - LOG_INODE_BLKSZ;
  indirsz = 1 << log_indirsz;

  /* Single indirect block.  */
  if (blk < indirsz)
    {
      holy_ufs_blk_t indir;
      holy_disk_read (data->disk,
		      ((holy_disk_addr_t) INODE_INDIRBLOCKS (data, 0))
		      << log2_blksz,
		      blk * sizeof (indir), sizeof (indir), &indir);
      return indir;
    }
  blk -= indirsz;

  /* Double indirect block.  */
  if (blk < (holy_disk_addr_t) indirsz * (holy_disk_addr_t) indirsz)
    {
      holy_ufs_blk_t indir;

      holy_disk_read (data->disk,
		      ((holy_disk_addr_t) INODE_INDIRBLOCKS (data, 1))
		      << log2_blksz,
		      (blk >> log_indirsz) * sizeof (indir),
		      sizeof (indir), &indir);
      holy_disk_read (data->disk,
		      holy_ufs_to_cpu_blk (indir) << log2_blksz,
		      (blk & ((1 << log_indirsz) - 1)) * sizeof (indir),
		      sizeof (indir), &indir);

      return indir;
    }

  blk -= (holy_disk_addr_t) indirsz * (holy_disk_addr_t) indirsz;

  /* Triple indirect block.  */
  if (!(blk >> (3 * log_indirsz)))
    {
      holy_ufs_blk_t indir;

      holy_disk_read (data->disk,
		      ((holy_disk_addr_t) INODE_INDIRBLOCKS (data, 2))
		      << log2_blksz,
		      (blk >> (2 * log_indirsz)) * sizeof (indir),
		      sizeof (indir), &indir);
      holy_disk_read (data->disk,
		      holy_ufs_to_cpu_blk (indir) << log2_blksz,
		      ((blk >> log_indirsz)
		       & ((1 << log_indirsz) - 1)) * sizeof (indir),
		      sizeof (indir), &indir);

      holy_disk_read (data->disk,
		      holy_ufs_to_cpu_blk (indir) << log2_blksz,
		      (blk & ((1 << log_indirsz) - 1)) * sizeof (indir),
		      sizeof (indir), &indir);

      return indir;
    }

  holy_error (holy_ERR_BAD_FS,
	      "ufs does not support quadruple indirect blocks");
  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_ufs_read_file (struct holy_ufs_data *data,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t pos, holy_size_t len, char *buf)
{
  struct holy_ufs_sblock *sblock = &data->sblock;
  holy_off_t i;
  holy_off_t blockcnt;

  /* Adjust len so it we can't read past the end of the file.  */
  if (len + pos > INODE_SIZE (data))
    len = INODE_SIZE (data) - pos;

  blockcnt = (len + pos + UFS_BLKSZ (sblock) - 1) >> UFS_LOG_BLKSZ (sblock);

  for (i = pos >> UFS_LOG_BLKSZ (sblock); i < blockcnt; i++)
    {
      holy_disk_addr_t blknr;
      holy_off_t blockoff;
      holy_off_t blockend = UFS_BLKSZ (sblock);

      int skipfirst = 0;

      blockoff = pos & (UFS_BLKSZ (sblock) - 1);

      blknr = holy_ufs_get_file_block (data, i);
      if (holy_errno)
	return -1;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) & (UFS_BLKSZ (sblock) - 1);

	  if (!blockend)
	    blockend = UFS_BLKSZ (sblock);
	}

      /* First block.  */
      if (i == (pos >> UFS_LOG_BLKSZ (sblock)))
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* XXX: If the block number is 0 this block is not stored on
	 disk but is zero filled instead.  */
      if (blknr)
	{
	  data->disk->read_hook = read_hook;
	  data->disk->read_hook_data = read_hook_data;
	  holy_disk_read (data->disk,
			  blknr << holy_ufs_to_cpu32 (data->sblock.log2_blksz),
			  skipfirst, blockend, buf);
	  data->disk->read_hook = 0;
	  if (holy_errno)
	    return -1;
	}
      else
	holy_memset (buf, 0, blockend);

      buf += UFS_BLKSZ (sblock) - skipfirst;
    }

  return len;
}

/* Read inode INO from the mounted filesystem described by DATA.  This
   inode is used by default now.  */
static holy_err_t
holy_ufs_read_inode (struct holy_ufs_data *data, int ino, char *inode)
{
  struct holy_ufs_sblock *sblock = &data->sblock;

  /* Determine the group the inode is in.  */
  int group = ino / holy_ufs_to_cpu32 (sblock->ino_per_group);

  /* Determine the inode within the group.  */
  int grpino = ino % holy_ufs_to_cpu32 (sblock->ino_per_group);

  /* The first block of the group.  */
  int grpblk = group * (holy_ufs_to_cpu32 (sblock->frags_per_group));

#ifndef MODE_UFS2
  grpblk += holy_ufs_to_cpu32 (sblock->cylg_offset)
    * (group & (~holy_ufs_to_cpu32 (sblock->cylg_mask)));
#endif

  if (!inode)
    {
      inode = (char *) &data->inode;
      data->ino = ino;
    }

  holy_disk_read (data->disk,
		  ((holy_ufs_to_cpu32 (sblock->inoblk_offs) + grpblk)
		   << holy_ufs_to_cpu32 (data->sblock.log2_blksz))
		  + grpino / UFS_INODE_PER_BLOCK,
		  (grpino % UFS_INODE_PER_BLOCK)
		  * sizeof (struct holy_ufs_inode),
		  sizeof (struct holy_ufs_inode),
		  inode);

  return holy_errno;
}


/* Lookup the symlink the current inode points to.  INO is the inode
   number of the directory the symlink is relative to.  */
static holy_err_t
holy_ufs_lookup_symlink (struct holy_ufs_data *data, int ino)
{
  char *symlink;
  holy_size_t sz = INODE_SIZE (data);

  if (++data->linknest > holy_UFS_MAX_SYMLNK_CNT)
    return holy_error (holy_ERR_SYMLINK_LOOP, N_("too deep nesting of symlinks"));

  symlink = holy_malloc (sz + 1);
  if (!symlink)
    return holy_errno;
  /* Normally we should just check that data->inode.nblocks == 0.
     However old Linux doesn't maintain nblocks correctly and so it's always
     0. If size is bigger than inline space then the symlink is surely not
     inline.  */
  /* Check against zero is paylindromic, no need to swap.  */
  if (data->inode.nblocks == 0
      && INODE_SIZE (data) <= sizeof (data->inode.symlink))
    holy_strcpy (symlink, (char *) data->inode.symlink);
  else
    {
      if (holy_ufs_read_file (data, 0, 0, 0, sz, symlink) < 0)
	{
	  holy_free(symlink);
	  return holy_errno;
	}
    }
  symlink[sz] = '\0';

  /* The symlink is an absolute path, go back to the root inode.  */
  if (symlink[0] == '/')
    ino = holy_UFS_INODE;

  /* Now load in the old inode.  */
  if (holy_ufs_read_inode (data, ino, 0))
    {
      holy_free (symlink);
      return holy_errno;
    }

  holy_ufs_find_file (data, symlink);

  holy_free (symlink);

  return holy_errno;
}


/* Find the file with the pathname PATH on the filesystem described by
   DATA.  */
static holy_err_t
holy_ufs_find_file (struct holy_ufs_data *data, const char *path)
{
  const char *name;
  const char *next = path;
  unsigned int pos = 0;
  int dirino;
  char *filename;

  /* We reject filenames longer than the one we're looking
     for without reading, so this allocation is enough.  */
  filename = holy_malloc (holy_strlen (path) + 2);
  if (!filename)
    return holy_errno;

  while (1)
    {
      struct holy_ufs_dirent dirent;

      name = next;
      /* Skip the first slash.  */
      while (*name == '/')
	name++;
      if (*name == 0)
	{
	  holy_free (filename);
	  return holy_ERR_NONE;
	}

      if ((INODE_MODE(data) & holy_UFS_ATTR_TYPE)
	  != holy_UFS_ATTR_DIR)
	{
	  holy_error (holy_ERR_BAD_FILE_TYPE,
		      N_("not a directory"));
	  goto fail;
	}

      /* Extract the actual part from the pathname.  */
      for (next = name; *next && *next != '/'; next++);
      for (pos = 0;  ; pos += holy_ufs_to_cpu16 (dirent.direntlen))
	{
	  int namelen;

	  if (pos >= INODE_SIZE (data))
	    {
	      holy_error (holy_ERR_FILE_NOT_FOUND,
			  N_("file `%s' not found"),
			  path);
	      goto fail;
	    }

	  if (holy_ufs_read_file (data, 0, 0, pos, sizeof (dirent),
				  (char *) &dirent) < 0)
	    goto fail;

#ifdef MODE_UFS2
	  namelen = dirent.namelen_bsd;
#else
	  namelen = holy_ufs_to_cpu16 (dirent.namelen);
#endif
	  if (namelen < next - name)
	    continue;

	  if (holy_ufs_read_file (data, 0, 0, pos + sizeof (dirent),
				  next - name + (namelen != next - name),
				  filename) < 0)
	    goto fail;

	  if (holy_strncmp (name, filename, next - name) == 0
	      && (namelen == next - name || filename[next - name] == '\0'))
	    {
	      dirino = data->ino;
	      holy_ufs_read_inode (data, holy_ufs_to_cpu32 (dirent.ino), 0);
	      
	      if ((INODE_MODE(data) & holy_UFS_ATTR_TYPE)
		  == holy_UFS_ATTR_LNK)
		{
		  holy_ufs_lookup_symlink (data, dirino);
		  if (holy_errno)
		    goto fail;
		}

	      break;
	    }
	}
    }
 fail:
  holy_free (filename);
  return holy_errno;
}


/* Mount the filesystem on the disk DISK.  */
static struct holy_ufs_data *
holy_ufs_mount (holy_disk_t disk)
{
  struct holy_ufs_data *data;
  int *sblklist = sblocklist;

  data = holy_malloc (sizeof (struct holy_ufs_data));
  if (!data)
    return 0;

  /* Find a UFS sblock.  */
  while (*sblklist != -1)
    {
      holy_disk_read (disk, *sblklist, 0, sizeof (struct holy_ufs_sblock),
		      &data->sblock);
      if (holy_errno)
	goto fail;

      /* No need to byteswap bsize in this check. It works the same on both
	 endiannesses.  */
      if (data->sblock.magic == holy_cpu_to_ufs32_compile_time (holy_UFS_MAGIC)
	  && data->sblock.bsize != 0
	  && ((data->sblock.bsize & (data->sblock.bsize - 1)) == 0)
	  && data->sblock.ino_per_group != 0)
	{
	  for (data->log2_blksz = 0; 
	       (1U << data->log2_blksz) < holy_ufs_to_cpu32 (data->sblock.bsize);
	       data->log2_blksz++);

	  data->disk = disk;
	  data->linknest = 0;
	  return data;
	}
      sblklist++;
    }

 fail:

  if (holy_errno == holy_ERR_NONE || holy_errno == holy_ERR_OUT_OF_RANGE)
    {
#ifdef MODE_UFS2
      holy_error (holy_ERR_BAD_FS, "not an ufs2 filesystem");
#else
      holy_error (holy_ERR_BAD_FS, "not an ufs1 filesystem");
#endif
    }

  holy_free (data);

  return 0;
}


static holy_err_t
holy_ufs_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_ufs_data *data;
  unsigned int pos = 0;

  data = holy_ufs_mount (device->disk);
  if (!data)
    return holy_errno;

  holy_ufs_read_inode (data, holy_UFS_INODE, 0);
  if (holy_errno)
    return holy_errno;

  if (!path || path[0] != '/')
    {
      holy_error (holy_ERR_BAD_FILENAME, N_("invalid file name `%s'"), path);
      return holy_errno;
    }

  holy_ufs_find_file (data, path);
  if (holy_errno)
    goto fail;

  if ((INODE_MODE (data) & holy_UFS_ATTR_TYPE) != holy_UFS_ATTR_DIR)
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
      goto fail;
    }

  while (pos < INODE_SIZE (data))
    {
      struct holy_ufs_dirent dirent;
      int namelen;

      if (holy_ufs_read_file (data, 0, 0, pos, sizeof (dirent),
			      (char *) &dirent) < 0)
	break;

      if (dirent.direntlen == 0)
	break;

#ifdef MODE_UFS2
      namelen = dirent.namelen_bsd;
#else
      namelen = holy_ufs_to_cpu16 (dirent.namelen);
#endif

      char *filename = holy_malloc (namelen + 1);
      if (!filename)
	goto fail;
      struct holy_dirhook_info info;
      struct holy_ufs_inode inode;

      holy_memset (&info, 0, sizeof (info));

      if (holy_ufs_read_file (data, 0, 0, pos + sizeof (dirent),
			      namelen, filename) < 0)
	{
	  holy_free (filename);
	  break;
	}

      filename[namelen] = '\0';
      holy_ufs_read_inode (data, holy_ufs_to_cpu32 (dirent.ino),
			   (char *) &inode);

      info.dir = ((holy_ufs_to_cpu16 (inode.mode) & holy_UFS_ATTR_TYPE)
		  == holy_UFS_ATTR_DIR);
#ifdef MODE_UFS2
      info.mtime = holy_ufs_to_cpu64 (inode.mtime);
#else
      info.mtime = holy_ufs_to_cpu32 (inode.mtime);
#endif
      info.mtimeset = 1;

      if (hook (filename, &info, hook_data))
	{
	  holy_free (filename);
	  break;
	}

      holy_free (filename);

      pos += holy_ufs_to_cpu16 (dirent.direntlen);
    }

 fail:
  holy_free (data);

  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_ufs_open (struct holy_file *file, const char *name)
{
  struct holy_ufs_data *data;
  data = holy_ufs_mount (file->device->disk);
  if (!data)
    return holy_errno;

  holy_ufs_read_inode (data, 2, 0);
  if (holy_errno)
    {
      holy_free (data);
      return holy_errno;
    }

  if (!name || name[0] != '/')
    {
      holy_error (holy_ERR_BAD_FILENAME, N_("invalid file name `%s'"), name);
      return holy_errno;
    }

  holy_ufs_find_file (data, name);
  if (holy_errno)
    {
      holy_free (data);
      return holy_errno;
    }

  file->data = data;
  file->size = INODE_SIZE (data);

  return holy_ERR_NONE;
}


static holy_ssize_t
holy_ufs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_ufs_data *data =
    (struct holy_ufs_data *) file->data;

  return holy_ufs_read_file (data, file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}


static holy_err_t
holy_ufs_close (holy_file_t file)
{
  holy_free (file->data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_ufs_label (holy_device_t device, char **label)
{
  struct holy_ufs_data *data = 0;

  holy_dl_ref (my_mod);

  *label = 0;

  data = holy_ufs_mount (device->disk);
  if (data)
    *label = holy_strdup ((char *) data->sblock.volume_name);

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_ufs_uuid (holy_device_t device, char **uuid)
{
  struct holy_ufs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_ufs_mount (disk);
  if (data && (data->sblock.uuidhi != 0 || data->sblock.uuidlow != 0))
    *uuid = holy_xasprintf ("%08x%08x",
			   (unsigned) holy_ufs_to_cpu32 (data->sblock.uuidhi),
			   (unsigned) holy_ufs_to_cpu32 (data->sblock.uuidlow));
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}


/* Get mtime.  */
static holy_err_t
holy_ufs_mtime (holy_device_t device, holy_int32_t *tm)
{
  struct holy_ufs_data *data = 0;

  holy_dl_ref (my_mod);

  data = holy_ufs_mount (device->disk);
  if (!data)
    *tm = 0;
  else
    {
      *tm = holy_ufs_to_cpu32 (data->sblock.mtime);
#ifdef MODE_UFS2
      if (*tm < (holy_int64_t) holy_ufs_to_cpu64 (data->sblock.mtime2))
	*tm = holy_ufs_to_cpu64 (data->sblock.mtime2);
#endif
    }

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}



static struct holy_fs holy_ufs_fs =
  {
#ifdef MODE_UFS2
    .name = "ufs2",
#else
#ifdef MODE_BIGENDIAN
    .name = "ufs1_be",
#else
    .name = "ufs1",
#endif
#endif
    .dir = holy_ufs_dir,
    .open = holy_ufs_open,
    .read = holy_ufs_read,
    .close = holy_ufs_close,
    .label = holy_ufs_label,
    .uuid = holy_ufs_uuid,
    .mtime = holy_ufs_mtime,
    /* FIXME: set reserved_first_sector.  */
#ifdef holy_UTIL
    .blocklist_install = 1,
#endif
    .next = 0
  };

#ifdef MODE_UFS2
holy_MOD_INIT(ufs2)
#else
#ifdef MODE_BIGENDIAN
holy_MOD_INIT(ufs1_be)
#else
holy_MOD_INIT(ufs1)
#endif
#endif
{
  holy_fs_register (&holy_ufs_fs);
  my_mod = mod;
}

#ifdef MODE_UFS2
holy_MOD_FINI(ufs2)
#else
#ifdef MODE_BIGENDIAN
holy_MOD_FINI(ufs1_be)
#else
holy_MOD_FINI(ufs1)
#endif
#endif
{
  holy_fs_unregister (&holy_ufs_fs);
}

