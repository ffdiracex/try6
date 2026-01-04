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

#ifdef MODE_MINIX3
#define holy_MINIX_MAGIC	0x4D5A
#elif defined(MODE_MINIX2)
#define holy_MINIX_MAGIC	0x2468
#define holy_MINIX_MAGIC_30	0x2478
#else
#define holy_MINIX_MAGIC	0x137F
#define holy_MINIX_MAGIC_30	0x138F
#endif

#define holy_MINIX_INODE_DIR_BLOCKS	7
#define holy_MINIX_LOG2_BSIZE	1
#define holy_MINIX_ROOT_INODE	1
#define holy_MINIX_MAX_SYMLNK_CNT	8
#define holy_MINIX_SBLOCK	2

#define holy_MINIX_IFDIR	0040000U
#define holy_MINIX_IFLNK	0120000U

#ifdef MODE_BIGENDIAN
#define holy_cpu_to_minix16_compile_time holy_cpu_to_be16_compile_time
#define holy_minix_to_cpu16 holy_be_to_cpu16
#define holy_minix_to_cpu32 holy_be_to_cpu32
#else
#define holy_cpu_to_minix16_compile_time holy_cpu_to_le16_compile_time
#define holy_minix_to_cpu16 holy_le_to_cpu16
#define holy_minix_to_cpu32 holy_le_to_cpu32
#endif

#if defined(MODE_MINIX2) || defined(MODE_MINIX3)
typedef holy_uint32_t holy_minix_uintn_t;
#define holy_minix_to_cpu_n holy_minix_to_cpu32
#else
typedef holy_uint16_t holy_minix_uintn_t;
#define holy_minix_to_cpu_n holy_minix_to_cpu16
#endif

#ifdef MODE_MINIX3
typedef holy_uint32_t holy_minix_ino_t;
#define holy_minix_to_cpu_ino holy_minix_to_cpu32
#else
typedef holy_uint16_t holy_minix_ino_t;
#define holy_minix_to_cpu_ino holy_minix_to_cpu16
#endif

#define holy_MINIX_INODE_SIZE(data) (holy_minix_to_cpu32 (data->inode.size))
#define holy_MINIX_INODE_MODE(data) (holy_minix_to_cpu16 (data->inode.mode))
#define holy_MINIX_INODE_DIR_ZONES(data,blk) (holy_minix_to_cpu_n   \
					      (data->inode.dir_zones[blk]))
#define holy_MINIX_INODE_INDIR_ZONE(data)  (holy_minix_to_cpu_n \
					    (data->inode.indir_zone))
#define holy_MINIX_INODE_DINDIR_ZONE(data) (holy_minix_to_cpu_n \
					    (data->inode.double_indir_zone))


#ifdef MODE_MINIX3
struct holy_minix_sblock
{
  holy_uint32_t inode_cnt;
  holy_uint16_t zone_cnt;
  holy_uint16_t inode_bmap_size;
  holy_uint16_t zone_bmap_size;
  holy_uint16_t first_data_zone;
  holy_uint16_t log2_zone_size;
  holy_uint16_t pad;
  holy_uint32_t max_file_size;
  holy_uint32_t zones;
  holy_uint16_t magic;
  
  holy_uint16_t pad2;
  holy_uint16_t block_size;
  holy_uint8_t disk_version;
};
#else
struct holy_minix_sblock
{
  holy_uint16_t inode_cnt;
  holy_uint16_t zone_cnt;
  holy_uint16_t inode_bmap_size;
  holy_uint16_t zone_bmap_size;
  holy_uint16_t first_data_zone;
  holy_uint16_t log2_zone_size;
  holy_uint32_t max_file_size;
  holy_uint16_t magic;
};
#endif

#if defined(MODE_MINIX3) || defined(MODE_MINIX2)
struct holy_minix_inode
{
  holy_uint16_t mode;
  holy_uint16_t nlinks;
  holy_uint16_t uid;
  holy_uint16_t gid;
  holy_uint32_t size;
  holy_uint32_t atime;
  holy_uint32_t mtime;
  holy_uint32_t ctime;
  holy_uint32_t dir_zones[7];
  holy_uint32_t indir_zone;
  holy_uint32_t double_indir_zone;
  holy_uint32_t triple_indir_zone;
};
#else
struct holy_minix_inode
{
  holy_uint16_t mode;
  holy_uint16_t uid;
  holy_uint32_t size;
  holy_uint32_t mtime;
  holy_uint8_t gid;
  holy_uint8_t nlinks;
  holy_uint16_t dir_zones[7];
  holy_uint16_t indir_zone;
  holy_uint16_t double_indir_zone;
};

#endif

#if defined(MODE_MINIX3)
#define MAX_MINIX_FILENAME_SIZE 60
#else
#define MAX_MINIX_FILENAME_SIZE 30
#endif

/* Information about a "mounted" minix filesystem.  */
struct holy_minix_data
{
  struct holy_minix_sblock sblock;
  struct holy_minix_inode inode;
  holy_uint32_t block_per_zone;
  holy_minix_ino_t ino;
  int linknest;
  holy_disk_t disk;
  int filename_size;
  holy_size_t block_size;
};

static holy_dl_t my_mod;

static holy_err_t holy_minix_find_file (struct holy_minix_data *data,
					const char *path);

#ifdef MODE_MINIX3
static inline holy_disk_addr_t
holy_minix_zone2sect (struct holy_minix_data *data, holy_minix_uintn_t zone)
{
  return ((holy_disk_addr_t) zone) * data->block_size;
}
#else
static inline holy_disk_addr_t
holy_minix_zone2sect (struct holy_minix_data *data, holy_minix_uintn_t zone)
{
  int log2_zonesz = (holy_MINIX_LOG2_BSIZE
		     + holy_minix_to_cpu16 (data->sblock.log2_zone_size));
  return (((holy_disk_addr_t) zone) << log2_zonesz);
}
#endif


  /* Read the block pointer in ZONE, on the offset NUM.  */
static holy_minix_uintn_t
holy_get_indir (struct holy_minix_data *data,
		 holy_minix_uintn_t zone,
		 holy_minix_uintn_t num)
{
  holy_minix_uintn_t indirn;
  holy_disk_read (data->disk,
		  holy_minix_zone2sect(data, zone),
		  sizeof (holy_minix_uintn_t) * num,
		  sizeof (holy_minix_uintn_t), (char *) &indirn);
  return holy_minix_to_cpu_n (indirn);
}

static holy_minix_uintn_t
holy_minix_get_file_block (struct holy_minix_data *data, unsigned int blk)
{
  holy_minix_uintn_t indir;

  /* Direct block.  */
  if (blk < holy_MINIX_INODE_DIR_BLOCKS)
    return holy_MINIX_INODE_DIR_ZONES (data, blk);

  /* Indirect block.  */
  blk -= holy_MINIX_INODE_DIR_BLOCKS;
  if (blk < data->block_per_zone)
    {
      indir = holy_get_indir (data, holy_MINIX_INODE_INDIR_ZONE (data), blk);
      return indir;
    }

  /* Double indirect block.  */
  blk -= data->block_per_zone;
  if (blk < (holy_uint64_t) data->block_per_zone * (holy_uint64_t) data->block_per_zone)
    {
      indir = holy_get_indir (data, holy_MINIX_INODE_DINDIR_ZONE (data),
			      blk / data->block_per_zone);

      indir = holy_get_indir (data, indir, blk % data->block_per_zone);

      return indir;
    }

#if defined (MODE_MINIX3) || defined (MODE_MINIX2)
  blk -= data->block_per_zone * data->block_per_zone;
  if (blk < ((holy_uint64_t) data->block_per_zone * (holy_uint64_t) data->block_per_zone
	     * (holy_uint64_t) data->block_per_zone))
    {
      indir = holy_get_indir (data, holy_minix_to_cpu_n (data->inode.triple_indir_zone),
			      (blk / data->block_per_zone) / data->block_per_zone);
      indir = holy_get_indir (data, indir, (blk / data->block_per_zone) % data->block_per_zone);
      indir = holy_get_indir (data, indir, blk % data->block_per_zone);

      return indir;
    }
#endif

  /* This should never happen.  */
  holy_error (holy_ERR_OUT_OF_RANGE, "file bigger than maximum size");

  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_minix_read_file (struct holy_minix_data *data,
		      holy_disk_read_hook_t read_hook, void *read_hook_data,
		      holy_off_t pos, holy_size_t len, char *buf)
{
  holy_uint32_t i;
  holy_uint32_t blockcnt;
  holy_uint32_t posblock;
  holy_uint32_t blockoff;

  if (pos > holy_MINIX_INODE_SIZE (data))
    {
      holy_error (holy_ERR_OUT_OF_RANGE,
		  N_("attempt to read past the end of file"));
      return -1;
    }

  /* Adjust len so it we can't read past the end of the file.  */
  if (len + pos > holy_MINIX_INODE_SIZE (data))
    len = holy_MINIX_INODE_SIZE (data) - pos;
  if (len == 0)
    return 0;

  /* Files are at most 2G/4G - 1 bytes on minixfs. Avoid 64-bit division.  */
  blockcnt = ((holy_uint32_t) ((len + pos - 1)
	       >> holy_DISK_SECTOR_BITS)) / data->block_size + 1;
  posblock = (((holy_uint32_t) pos)
	      / (data->block_size << holy_DISK_SECTOR_BITS));
  blockoff = (((holy_uint32_t) pos)
	      % (data->block_size << holy_DISK_SECTOR_BITS));

  for (i = posblock; i < blockcnt; i++)
    {
      holy_minix_uintn_t blknr;
      holy_uint64_t blockend = data->block_size << holy_DISK_SECTOR_BITS;
      holy_off_t skipfirst = 0;

      blknr = holy_minix_get_file_block (data, i);
      if (holy_errno)
	return -1;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  /* len + pos < 4G (checked above), so it doesn't overflow.  */
	  blockend = (((holy_uint32_t) (len + pos))
		      % (data->block_size << holy_DISK_SECTOR_BITS));

	  if (!blockend)
	    blockend = data->block_size << holy_DISK_SECTOR_BITS;
	}

      /* First block.  */
      if (i == posblock)
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      data->disk->read_hook = read_hook;
      data->disk->read_hook_data = read_hook_data;
      holy_disk_read (data->disk,
		      holy_minix_zone2sect(data, blknr),
		      skipfirst, blockend, buf);
      data->disk->read_hook = 0;
      if (holy_errno)
	return -1;

      buf += (data->block_size << holy_DISK_SECTOR_BITS) - skipfirst;
    }

  return len;
}


/* Read inode INO from the mounted filesystem described by DATA.  This
   inode is used by default now.  */
static holy_err_t
holy_minix_read_inode (struct holy_minix_data *data, holy_minix_ino_t ino)
{
  struct holy_minix_sblock *sblock = &data->sblock;

  /* Block in which the inode is stored.  */
  holy_disk_addr_t block;
  data->ino = ino;

  /* The first inode in minix is inode 1.  */
  ino--;
  block = holy_minix_zone2sect (data,
				2 + holy_minix_to_cpu16 (sblock->inode_bmap_size)
				+ holy_minix_to_cpu16 (sblock->zone_bmap_size));
  block += ino / (holy_DISK_SECTOR_SIZE / sizeof (struct holy_minix_inode));
  int offs = (ino % (holy_DISK_SECTOR_SIZE
		     / sizeof (struct holy_minix_inode))
	      * sizeof (struct holy_minix_inode));
  
  holy_disk_read (data->disk, block, offs,
		  sizeof (struct holy_minix_inode), &data->inode);

  return holy_ERR_NONE;
}


/* Lookup the symlink the current inode points to.  INO is the inode
   number of the directory the symlink is relative to.  */
static holy_err_t
holy_minix_lookup_symlink (struct holy_minix_data *data, holy_minix_ino_t ino)
{
  char *symlink;
  holy_size_t sz = holy_MINIX_INODE_SIZE (data);

  if (++data->linknest > holy_MINIX_MAX_SYMLNK_CNT)
    return holy_error (holy_ERR_SYMLINK_LOOP, N_("too deep nesting of symlinks"));

  symlink = holy_malloc (sz + 1);
  if (!symlink)
    return holy_errno;
  if (holy_minix_read_file (data, 0, 0, 0, sz, symlink) < 0)
    return holy_errno;

  symlink[sz] = '\0';

  /* The symlink is an absolute path, go back to the root inode.  */
  if (symlink[0] == '/')
    ino = holy_MINIX_ROOT_INODE;

  /* Now load in the old inode.  */
  if (holy_minix_read_inode (data, ino))
    return holy_errno;

  holy_minix_find_file (data, symlink);

  return holy_errno;
}


/* Find the file with the pathname PATH on the filesystem described by
   DATA.  */
static holy_err_t
holy_minix_find_file (struct holy_minix_data *data, const char *path)
{
  const char *name;
  const char *next = path;
  unsigned int pos = 0;
  holy_minix_ino_t dirino;

  while (1)
    {
      name = next;
      /* Skip the first slash.  */
      while (*name == '/')
	name++;
      if (!*name)
	return holy_ERR_NONE;

      if ((holy_MINIX_INODE_MODE (data)
	   & holy_MINIX_IFDIR) != holy_MINIX_IFDIR)
	return holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));

      /* Extract the actual part from the pathname.  */
      for (next = name; *next && *next != '/'; next++);

      for (pos = 0; ; )
	{
	  holy_minix_ino_t ino;
	  char filename[MAX_MINIX_FILENAME_SIZE + 1];

	  if (pos >= holy_MINIX_INODE_SIZE (data))
	    {
	      holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), path);
	      return holy_errno;
	    }

	  if (holy_minix_read_file (data, 0, 0, pos, sizeof (ino),
				    (char *) &ino) < 0)
	    return holy_errno;
	  if (holy_minix_read_file (data, 0, 0, pos + sizeof (ino),
				    data->filename_size, (char *) filename)< 0)
	    return holy_errno;

	  pos += sizeof (ino) + data->filename_size;

	  filename[data->filename_size] = '\0';

	  /* Check if the current direntry matches the current part of the
	     pathname.  */
	  if (holy_strncmp (name, filename, next - name) == 0
	      && filename[next - name] == '\0')
	    {
	      dirino = data->ino;
	      holy_minix_read_inode (data, holy_minix_to_cpu_ino (ino));

	      /* Follow the symlink.  */
	      if ((holy_MINIX_INODE_MODE (data)
		   & holy_MINIX_IFLNK) == holy_MINIX_IFLNK)
		{
		  holy_minix_lookup_symlink (data, dirino);
		  if (holy_errno)
		    return holy_errno;
		}

	      break;
	    }
	}
    }
}


/* Mount the filesystem on the disk DISK.  */
static struct holy_minix_data *
holy_minix_mount (holy_disk_t disk)
{
  struct holy_minix_data *data;

  data = holy_malloc (sizeof (struct holy_minix_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  holy_disk_read (disk, holy_MINIX_SBLOCK, 0,
		  sizeof (struct holy_minix_sblock),&data->sblock);
  if (holy_errno)
    goto fail;

  if (data->sblock.magic == holy_cpu_to_minix16_compile_time (holy_MINIX_MAGIC))
    {
#if !defined(MODE_MINIX3)
      data->filename_size = 14;
#else
      data->filename_size = 60;
#endif
    }
#if !defined(MODE_MINIX3)
  else if (data->sblock.magic
	   == holy_cpu_to_minix16_compile_time (holy_MINIX_MAGIC_30))
    data->filename_size = 30;
#endif
  else
    goto fail;

  /* 20 means 1G zones. We could support up to 31 but already 1G isn't
     supported by anything else.  */
  if (holy_minix_to_cpu16 (data->sblock.log2_zone_size) >= 20)
    goto fail;

  data->disk = disk;
  data->linknest = 0;
#ifdef MODE_MINIX3
  /* These tests are endian-independent. No need to byteswap.  */
  if (data->sblock.block_size == 0xffff)
    data->block_size = 2;
  else
    {
      if ((data->sblock.block_size == holy_cpu_to_minix16_compile_time (0x200))
	  || (data->sblock.block_size == 0)
	  || (data->sblock.block_size & holy_cpu_to_minix16_compile_time (0x1ff)))
	goto fail;
      data->block_size = holy_minix_to_cpu16 (data->sblock.block_size)
	>> holy_DISK_SECTOR_BITS;
    }
#else
  data->block_size = 2;
#endif

  data->block_per_zone = (((holy_uint64_t) data->block_size <<	\
			   (holy_DISK_SECTOR_BITS + holy_minix_to_cpu16 (data->sblock.log2_zone_size)))
			  / sizeof (holy_minix_uintn_t));
  if (!data->block_per_zone)
    goto fail;

  return data;

 fail:
  holy_free (data);
#if defined(MODE_MINIX3)
  holy_error (holy_ERR_BAD_FS, "not a minix3 filesystem");
#elif defined(MODE_MINIX2)
  holy_error (holy_ERR_BAD_FS, "not a minix2 filesystem");
#else
  holy_error (holy_ERR_BAD_FS, "not a minix filesystem");
#endif
  return 0;
}

static holy_err_t
holy_minix_dir (holy_device_t device, const char *path,
		holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_minix_data *data = 0;
  unsigned int pos = 0;

  data = holy_minix_mount (device->disk);
  if (!data)
    return holy_errno;

  holy_minix_read_inode (data, holy_MINIX_ROOT_INODE);
  if (holy_errno)
    goto fail;

  holy_minix_find_file (data, path);
  if (holy_errno)
    goto fail;

  if ((holy_MINIX_INODE_MODE (data) & holy_MINIX_IFDIR) != holy_MINIX_IFDIR)
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
      goto fail;
    }

  while (pos < holy_MINIX_INODE_SIZE (data))
    {
      holy_minix_ino_t ino;
      char filename[MAX_MINIX_FILENAME_SIZE + 1];
      holy_minix_ino_t dirino = data->ino;
      struct holy_dirhook_info info;
      holy_memset (&info, 0, sizeof (info));


      if (holy_minix_read_file (data, 0, 0, pos, sizeof (ino),
				(char *) &ino) < 0)
	return holy_errno;

      if (holy_minix_read_file (data, 0, 0, pos + sizeof (ino),
				data->filename_size,
				(char *) filename) < 0)
	return holy_errno;
      filename[data->filename_size] = '\0';
      if (!ino)
	{
	  pos += sizeof (ino) + data->filename_size;
	  continue;
	}

      holy_minix_read_inode (data, holy_minix_to_cpu_ino (ino));
      info.dir = ((holy_MINIX_INODE_MODE (data)
		   & holy_MINIX_IFDIR) == holy_MINIX_IFDIR);
      info.mtimeset = 1;
      info.mtime = holy_minix_to_cpu32 (data->inode.mtime);

      if (hook (filename, &info, hook_data) ? 1 : 0)
	break;

      /* Load the old inode back in.  */
      holy_minix_read_inode (data, dirino);

      pos += sizeof (ino) + data->filename_size;
    }

 fail:
  holy_free (data);
  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_minix_open (struct holy_file *file, const char *name)
{
  struct holy_minix_data *data;
  data = holy_minix_mount (file->device->disk);
  if (!data)
    return holy_errno;

  /* Open the inode op the root directory.  */
  holy_minix_read_inode (data, holy_MINIX_ROOT_INODE);
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

  /* Traverse the directory tree to the node that should be
     opened.  */
  holy_minix_find_file (data, name);
  if (holy_errno)
    {
      holy_free (data);
      return holy_errno;
    }

  file->data = data;
  file->size = holy_MINIX_INODE_SIZE (data);

  return holy_ERR_NONE;
}


static holy_ssize_t
holy_minix_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_minix_data *data =
    (struct holy_minix_data *) file->data;

  return holy_minix_read_file (data, file->read_hook, file->read_hook_data,
			       file->offset, len, buf);
}


static holy_err_t
holy_minix_close (holy_file_t file)
{
  holy_free (file->data);

  return holy_ERR_NONE;
}



static struct holy_fs holy_minix_fs =
  {
#ifdef MODE_BIGENDIAN
#if defined(MODE_MINIX3)
    .name = "minix3_be",
#elif defined(MODE_MINIX2)
    .name = "minix2_be",
#else
    .name = "minix_be",
#endif
#else
#if defined(MODE_MINIX3)
    .name = "minix3",
#elif defined(MODE_MINIX2)
    .name = "minix2",
#else
    .name = "minix",
#endif
#endif
    .dir = holy_minix_dir,
    .open = holy_minix_open,
    .read = holy_minix_read,
    .close = holy_minix_close,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

#ifdef MODE_BIGENDIAN
#if defined(MODE_MINIX3)
holy_MOD_INIT(minix3_be)
#elif defined(MODE_MINIX2)
holy_MOD_INIT(minix2_be)
#else
holy_MOD_INIT(minix_be)
#endif
#else
#if defined(MODE_MINIX3)
holy_MOD_INIT(minix3)
#elif defined(MODE_MINIX2)
holy_MOD_INIT(minix2)
#else
holy_MOD_INIT(minix)
#endif
#endif
{
  holy_fs_register (&holy_minix_fs);
  my_mod = mod;
}

#ifdef MODE_BIGENDIAN
#if defined(MODE_MINIX3)
holy_MOD_FINI(minix3_be)
#elif defined(MODE_MINIX2)
holy_MOD_FINI(minix2_be)
#else
holy_MOD_FINI(minix_be)
#endif
#else
#if defined(MODE_MINIX3)
holy_MOD_FINI(minix3)
#elif defined(MODE_MINIX2)
holy_MOD_FINI(minix2)
#else
holy_MOD_FINI(minix)
#endif
#endif
{
  holy_fs_unregister (&holy_minix_fs);
}
