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
#include <holy/charset.h>

holy_MOD_LICENSE ("GPLv2+");

/* The affs bootblock.  */
struct holy_affs_bblock
{
  holy_uint8_t type[3];
  holy_uint8_t flags;
  holy_uint32_t checksum;
  holy_uint32_t rootblock;
} holy_PACKED;

/* Set if the filesystem is a AFFS filesystem.  Otherwise this is an
   OFS filesystem.  */
#define holy_AFFS_FLAG_FFS	1

/* The affs rootblock.  */
struct holy_affs_rblock
{
  holy_uint32_t type;
  holy_uint8_t unused1[8];
  holy_uint32_t htsize;
  holy_uint32_t unused2;
  holy_uint32_t checksum;
  holy_uint32_t hashtable[1];
} holy_PACKED;

struct holy_affs_time
{
  holy_int32_t day;
  holy_uint32_t min;
  holy_uint32_t hz;
} holy_PACKED;

/* The second part of a file header block.  */
struct holy_affs_file
{
  holy_uint8_t unused1[12];
  holy_uint32_t size;
  holy_uint8_t unused2[92];
  struct holy_affs_time mtime;
  holy_uint8_t namelen;
  holy_uint8_t name[30];
  holy_uint8_t unused3[5];
  holy_uint32_t hardlink;
  holy_uint32_t unused4[6];
  holy_uint32_t next;
  holy_uint32_t parent;
  holy_uint32_t extension;
  holy_uint32_t type;
} holy_PACKED;

/* The location of `struct holy_affs_file' relative to the end of a
   file header block.  */
#define	holy_AFFS_FILE_LOCATION		200

/* The offset in both the rootblock and the file header block for the
   hashtable, symlink and block pointers (all synonyms).  */
#define holy_AFFS_HASHTABLE_OFFSET	24
#define holy_AFFS_BLOCKPTR_OFFSET	24
#define holy_AFFS_SYMLINK_OFFSET	24

enum
  {
    holy_AFFS_FILETYPE_DIR = 2,
    holy_AFFS_FILETYPE_SYMLINK = 3,
    holy_AFFS_FILETYPE_HARDLINK = 0xfffffffc,
    holy_AFFS_FILETYPE_REG = 0xfffffffd
  };

#define AFFS_MAX_LOG_BLOCK_SIZE 4
#define AFFS_MAX_SUPERBLOCK 1



struct holy_fshelp_node
{
  struct holy_affs_data *data;
  holy_uint32_t block;
  struct holy_fshelp_node *parent;
  struct holy_affs_file di;
  holy_uint32_t *block_cache;
  holy_uint32_t last_block_cache;
};

/* Information about a "mounted" affs filesystem.  */
struct holy_affs_data
{
  struct holy_affs_bblock bblock;
  struct holy_fshelp_node diropen;
  holy_disk_t disk;

  /* Log blocksize in sectors.  */
  int log_blocksize;

  /* The number of entries in the hashtable.  */
  unsigned int htsize;
};

static holy_dl_t my_mod;


static holy_disk_addr_t
holy_affs_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  holy_uint32_t target, curblock;
  holy_uint32_t pos;
  struct holy_affs_file file;
  struct holy_affs_data *data = node->data;
  holy_uint64_t mod;

  if (!node->block_cache)
    {
      node->block_cache = holy_malloc (((holy_be_to_cpu32 (node->di.size)
					 >> (9 + node->data->log_blocksize))
					/ data->htsize + 2)
				       * sizeof (node->block_cache[0]));
      if (!node->block_cache)
	return -1;
      node->last_block_cache = 0;
      node->block_cache[0] = node->block;
    }

  /* Files are at most 2G on AFFS, so no need for 64-bit division.  */
  target = (holy_uint32_t) fileblock / data->htsize;
  mod = (holy_uint32_t) fileblock % data->htsize;
  /* Find the block that points to the fileblock we are looking up by
     following the chain until the right table is reached.  */
  for (curblock = node->last_block_cache + 1; curblock < target + 1; curblock++)
    {
      holy_disk_read (data->disk,
		      (((holy_uint64_t) node->block_cache[curblock - 1] + 1)
		       << data->log_blocksize) - 1,
		      holy_DISK_SECTOR_SIZE - holy_AFFS_FILE_LOCATION,
		      sizeof (file), &file);
      if (holy_errno)
	return 0;

      node->block_cache[curblock] = holy_be_to_cpu32 (file.extension);
      node->last_block_cache = curblock;
    }

  /* Translate the fileblock to the block within the right table.  */
  holy_disk_read (data->disk, (holy_uint64_t) node->block_cache[target]
		  << data->log_blocksize,
		  holy_AFFS_BLOCKPTR_OFFSET
		  + (data->htsize - mod - 1) * sizeof (pos),
		  sizeof (pos), &pos);
  if (holy_errno)
    return 0;

  return holy_be_to_cpu32 (pos);
}

static struct holy_affs_data *
holy_affs_mount (holy_disk_t disk)
{
  struct holy_affs_data *data;
  holy_uint32_t *rootblock = 0;
  struct holy_affs_rblock *rblock = 0;
  int log_blocksize = 0;
  int bsnum = 0;

  data = holy_zalloc (sizeof (struct holy_affs_data));
  if (!data)
    return 0;

  for (bsnum = 0; bsnum < AFFS_MAX_SUPERBLOCK + 1; bsnum++)
    {
      /* Read the bootblock.  */
      holy_disk_read (disk, bsnum, 0, sizeof (struct holy_affs_bblock),
		      &data->bblock);
      if (holy_errno)
	goto fail;

      /* Make sure this is an affs filesystem.  */
      if (holy_strncmp ((char *) (data->bblock.type), "DOS", 3) != 0
	  /* Test if the filesystem is a OFS filesystem.  */
	  || !(data->bblock.flags & holy_AFFS_FLAG_FFS))
	continue;

      /* No sane person uses more than 8KB for a block.  At least I hope
	 for that person because in that case this won't work.  */
      if (!rootblock)
	rootblock = holy_malloc (holy_DISK_SECTOR_SIZE
				 << AFFS_MAX_LOG_BLOCK_SIZE);
      if (!rootblock)
	goto fail;

      rblock = (struct holy_affs_rblock *) rootblock;

      /* The filesystem blocksize is not stored anywhere in the filesystem
	 itself.  One way to determine it is try reading blocks for the
	 rootblock until the checksum is correct.  */
      for (log_blocksize = 0; log_blocksize <= AFFS_MAX_LOG_BLOCK_SIZE;
	   log_blocksize++)
	{
	  holy_uint32_t *currblock = rootblock;
	  unsigned int i;
	  holy_uint32_t checksum = 0;

	  /* Read the rootblock.  */
	  holy_disk_read (disk,
			  (holy_uint64_t) holy_be_to_cpu32 (data->bblock.rootblock)
			  << log_blocksize, 0,
			  holy_DISK_SECTOR_SIZE << log_blocksize, rootblock);
	  if (holy_errno == holy_ERR_OUT_OF_RANGE)
	    {
	      holy_errno = 0;
	      break;
	    }
	  if (holy_errno)
	    goto fail;

	  if (rblock->type != holy_cpu_to_be32_compile_time (2)
	      || rblock->htsize == 0
	      || currblock[(holy_DISK_SECTOR_SIZE << log_blocksize)
			   / sizeof (*currblock) - 1]
	      != holy_cpu_to_be32_compile_time (1))
	    continue;

	  for (i = 0; i < (holy_DISK_SECTOR_SIZE << log_blocksize)
		 / sizeof (*currblock);
	       i++)
	    checksum += holy_be_to_cpu32 (currblock[i]);

	  if (checksum == 0)
	    {
	      data->log_blocksize = log_blocksize;
	      data->disk = disk;
	      data->htsize = holy_be_to_cpu32 (rblock->htsize);
	      data->diropen.data = data;
	      data->diropen.block = holy_be_to_cpu32 (data->bblock.rootblock);
	      data->diropen.parent = NULL;
	      holy_memcpy (&data->diropen.di, rootblock,
			   sizeof (data->diropen.di));
	      holy_free (rootblock);

	      return data;
	    }
	}
    }

 fail:
  if (holy_errno == holy_ERR_NONE || holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not an AFFS filesystem");

  holy_free (data);
  holy_free (rootblock);
  return 0;
}


static char *
holy_affs_read_symlink (holy_fshelp_node_t node)
{
  struct holy_affs_data *data = node->data;
  holy_uint8_t *latin1, *utf8;
  const holy_size_t symlink_size = ((holy_DISK_SECTOR_SIZE
				     << data->log_blocksize) - holy_AFFS_SYMLINK_OFFSET);

  latin1 = holy_malloc (symlink_size + 1);
  if (!latin1)
    return 0;

  holy_disk_read (data->disk,
		  (holy_uint64_t) node->block << data->log_blocksize,
		  holy_AFFS_SYMLINK_OFFSET,
		  symlink_size, latin1);
  if (holy_errno)
    {
      holy_free (latin1);
      return 0;
    }
  latin1[symlink_size] = 0;
  utf8 = holy_malloc (symlink_size * holy_MAX_UTF8_PER_LATIN1 + 1);
  if (!utf8)
    {
      holy_free (latin1);
      return 0;
    }
  *holy_latin1_to_utf8 (utf8, latin1, symlink_size) = '\0';
  holy_dprintf ("affs", "Symlink: `%s'\n", utf8);
  holy_free (latin1);
  if (utf8[0] == ':')
    utf8[0] = '/';
  return (char *) utf8;
}


/* Helper for holy_affs_iterate_dir.  */
static int
holy_affs_create_node (holy_fshelp_node_t dir,
		       holy_fshelp_iterate_dir_hook_t hook, void *hook_data,
		       struct holy_fshelp_node **node,
		       holy_uint32_t **hashtable,
		       holy_uint32_t block, const struct holy_affs_file *fil)
{
  struct holy_affs_data *data = dir->data;
  int type = holy_FSHELP_REG;
  holy_uint8_t name_u8[sizeof (fil->name) * holy_MAX_UTF8_PER_LATIN1 + 1];
  holy_size_t len;
  unsigned int nest;

  *node = holy_zalloc (sizeof (**node));
  if (!*node)
    {
      holy_free (*hashtable);
      return 1;
    }

  (*node)->data = data;
  (*node)->block = block;
  (*node)->parent = dir;

  len = fil->namelen;
  if (len > sizeof (fil->name))
    len = sizeof (fil->name);
  *holy_latin1_to_utf8 (name_u8, fil->name, len) = '\0';
  
  (*node)->di = *fil;
  for (nest = 0; nest < 8; nest++)
    {
      switch ((*node)->di.type)
	{
	case holy_cpu_to_be32_compile_time (holy_AFFS_FILETYPE_REG):
	  type = holy_FSHELP_REG;
	  break;
	case holy_cpu_to_be32_compile_time (holy_AFFS_FILETYPE_DIR):
	  type = holy_FSHELP_DIR;
	  break;
	case holy_cpu_to_be32_compile_time (holy_AFFS_FILETYPE_SYMLINK):
	  type = holy_FSHELP_SYMLINK;
	  break;
	case holy_cpu_to_be32_compile_time (holy_AFFS_FILETYPE_HARDLINK):
	  {
	    holy_err_t err;
	    (*node)->block = holy_be_to_cpu32 ((*node)->di.hardlink);
	    err = holy_disk_read (data->disk,
				  (((holy_uint64_t) (*node)->block + 1) << data->log_blocksize)
				  - 1,
				  holy_DISK_SECTOR_SIZE - holy_AFFS_FILE_LOCATION,
				  sizeof ((*node)->di), (char *) &(*node)->di);
	    if (err)
	      return 1;
	    continue;
	  }
	default:
	  return 0;
	}
      break;
    }

  if (nest == 8)
    return 0;

  type |= holy_FSHELP_CASE_INSENSITIVE;

  if (hook ((char *) name_u8, type, *node, hook_data))
    {
      holy_free (*hashtable);
      *node = 0;
      return 1;
    }
  *node = 0;
  return 0;
}

static int
holy_affs_iterate_dir (holy_fshelp_node_t dir,
		       holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  unsigned int i;
  struct holy_affs_file file;
  struct holy_fshelp_node *node = 0;
  struct holy_affs_data *data = dir->data;
  holy_uint32_t *hashtable;

  /* Create the directory entries for `.' and `..'.  */
  node = holy_zalloc (sizeof (*node));
  if (!node)
    return 1;
    
  *node = *dir;
  if (hook (".", holy_FSHELP_DIR, node, hook_data))
    return 1;
  if (dir->parent)
    {
      node = holy_zalloc (sizeof (*node));
      if (!node)
	return 1;
      *node = *dir->parent;
      if (hook ("..", holy_FSHELP_DIR, node, hook_data))
	return 1;
    }

  hashtable = holy_zalloc (data->htsize * sizeof (*hashtable));
  if (!hashtable)
    return 1;

  holy_disk_read (data->disk,
		  (holy_uint64_t) dir->block << data->log_blocksize,
		  holy_AFFS_HASHTABLE_OFFSET,
		  data->htsize * sizeof (*hashtable), (char *) hashtable);
  if (holy_errno)
    goto fail;

  for (i = 0; i < data->htsize; i++)
    {
      holy_uint32_t next;

      if (!hashtable[i])
	continue;

      /* Every entry in the hashtable can be chained.  Read the entire
	 chain.  */
      next = holy_be_to_cpu32 (hashtable[i]);

      while (next)
	{
	  holy_disk_read (data->disk,
			  (((holy_uint64_t) next + 1) << data->log_blocksize)
			  - 1,
			  holy_DISK_SECTOR_SIZE - holy_AFFS_FILE_LOCATION,
			  sizeof (file), (char *) &file);
	  if (holy_errno)
	    goto fail;

	  if (holy_affs_create_node (dir, hook, hook_data, &node, &hashtable,
				     next, &file))
	    return 1;

	  next = holy_be_to_cpu32 (file.next);
	}
    }

  holy_free (hashtable);
  return 0;

 fail:
  holy_free (node);
  holy_free (hashtable);
  return 0;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_affs_open (struct holy_file *file, const char *name)
{
  struct holy_affs_data *data;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_affs_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (name, &data->diropen, &fdiro, holy_affs_iterate_dir,
			 holy_affs_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  file->size = holy_be_to_cpu32 (fdiro->di.size);
  data->diropen = *fdiro;
  holy_free (fdiro);

  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (data && fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}

static holy_err_t
holy_affs_close (holy_file_t file)
{
  struct holy_affs_data *data =
    (struct holy_affs_data *) file->data;

  holy_free (data->diropen.block_cache);
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static holy_ssize_t
holy_affs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_affs_data *data =
    (struct holy_affs_data *) file->data;

  return holy_fshelp_read_file (data->diropen.data->disk, &data->diropen,
				file->read_hook, file->read_hook_data,
				file->offset, len, buf, holy_affs_read_block,
				holy_be_to_cpu32 (data->diropen.di.size),
				data->log_blocksize, 0);
}

static holy_int32_t
aftime2ctime (const struct holy_affs_time *t)
{
  return holy_be_to_cpu32 (t->day) * 86400
    + holy_be_to_cpu32 (t->min) * 60
    + holy_be_to_cpu32 (t->hz) / 50
    + 8 * 365 * 86400 + 86400 * 2;
}

/* Context for holy_affs_dir.  */
struct holy_affs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_affs_dir.  */
static int
holy_affs_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		    holy_fshelp_node_t node, void *data)
{
  struct holy_affs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = 1;
  info.mtime = aftime2ctime (&node->di.mtime);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_affs_dir (holy_device_t device, const char *path,
	       holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_affs_dir_ctx ctx = { hook, hook_data };
  struct holy_affs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_affs_mount (device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (path, &data->diropen, &fdiro, holy_affs_iterate_dir,
			 holy_affs_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_affs_iterate_dir (fdiro, holy_affs_dir_iter, &ctx);

 fail:
  if (data && fdiro != &data->diropen)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_err_t
holy_affs_label (holy_device_t device, char **label)
{
  struct holy_affs_data *data;
  struct holy_affs_file file;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_affs_mount (disk);
  if (data)
    {
      holy_size_t len;
      /* The rootblock maps quite well on a file header block, it's
	 something we can use here.  */
      holy_disk_read (data->disk,
		      (((holy_uint64_t)
			holy_be_to_cpu32 (data->bblock.rootblock) + 1)
		       << data->log_blocksize) - 1,
		      holy_DISK_SECTOR_SIZE - holy_AFFS_FILE_LOCATION,
		      sizeof (file), &file);
      if (holy_errno)
	return holy_errno;

      len = file.namelen;
      if (len > sizeof (file.name))
	len = sizeof (file.name);
      *label = holy_malloc (len * holy_MAX_UTF8_PER_LATIN1 + 1);
      if (*label)
	*holy_latin1_to_utf8 ((holy_uint8_t *) *label, file.name, len) = '\0';
    }
  else
    *label = 0;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_affs_mtime (holy_device_t device, holy_int32_t *t)
{
  struct holy_affs_data *data;
  holy_disk_t disk = device->disk;
  struct holy_affs_time af_time;

  *t = 0;

  holy_dl_ref (my_mod);

  data = holy_affs_mount (disk);
  if (!data)
    {
      holy_dl_unref (my_mod);
      return holy_errno;
    }

  holy_disk_read (data->disk,
		  (((holy_uint64_t)
		    holy_be_to_cpu32 (data->bblock.rootblock) + 1)
		   << data->log_blocksize) - 1,
		  holy_DISK_SECTOR_SIZE - 40,
		  sizeof (af_time), &af_time);
  if (holy_errno)
    {
      holy_dl_unref (my_mod);
      holy_free (data);
      return holy_errno;
    }

  *t = aftime2ctime (&af_time);
  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_ERR_NONE;
}


static struct holy_fs holy_affs_fs =
  {
    .name = "affs",
    .dir = holy_affs_dir,
    .open = holy_affs_open,
    .read = holy_affs_read,
    .close = holy_affs_close,
    .label = holy_affs_label,
    .mtime = holy_affs_mtime,

#ifdef holy_UTIL
    .reserved_first_sector = 0,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(affs)
{
  holy_fs_register (&holy_affs_fs);
  my_mod = mod;
}

holy_MOD_FINI(affs)
{
  holy_fs_unregister (&holy_affs_fs);
}
