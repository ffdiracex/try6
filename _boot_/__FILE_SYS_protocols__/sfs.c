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

/* The common header for a block.  */
struct holy_sfs_bheader
{
  holy_uint8_t magic[4];
  holy_uint32_t chksum;
  holy_uint32_t ipointtomyself;
} holy_PACKED;

/* The sfs rootblock.  */
struct holy_sfs_rblock
{
  struct holy_sfs_bheader header;
  holy_uint32_t version;
  holy_uint32_t createtime;
  holy_uint8_t flags;
  holy_uint8_t unused1[31];
  holy_uint32_t blocksize;
  holy_uint8_t unused2[40];
  holy_uint8_t unused3[8];
  holy_uint32_t rootobject;
  holy_uint32_t btree;
} holy_PACKED;

enum
  {
    FLAGS_CASE_SENSITIVE = 0x80
  };

/* A SFS object container.  */
struct holy_sfs_obj
{
  holy_uint8_t unused1[4];
  holy_uint32_t nodeid;
  holy_uint8_t unused2[4];
  union
  {
    struct
    {
      holy_uint32_t first_block;
      holy_uint32_t size;
    } holy_PACKED file;
    struct
    {
      holy_uint32_t hashtable;
      holy_uint32_t dir_objc;
    } holy_PACKED dir;
  } file_dir;
  holy_uint32_t mtime;
  holy_uint8_t type;
  holy_uint8_t filename[1];
  holy_uint8_t comment[1];
} holy_PACKED;

#define	holy_SFS_TYPE_DELETED	32
#define	holy_SFS_TYPE_SYMLINK	64
#define	holy_SFS_TYPE_DIR	128

/* A SFS object container.  */
struct holy_sfs_objc
{
  struct holy_sfs_bheader header;
  holy_uint32_t parent;
  holy_uint32_t next;
  holy_uint32_t prev;
  /* The amount of objects depends on the blocksize.  */
  struct holy_sfs_obj objects[1];
} holy_PACKED;

struct holy_sfs_btree_node
{
  holy_uint32_t key;
  holy_uint32_t data;
} holy_PACKED;

struct holy_sfs_btree_extent
{
  holy_uint32_t key;
  holy_uint32_t next;
  holy_uint32_t prev;
  holy_uint16_t size;
} holy_PACKED;

struct holy_sfs_btree
{
  struct holy_sfs_bheader header;
  holy_uint16_t nodes;
  holy_uint8_t leaf;
  holy_uint8_t nodesize;
  /* Normally this can be kind of node, but just extents are
     supported.  */
  struct holy_sfs_btree_node node[1];
} holy_PACKED;



struct cache_entry
{
  holy_uint32_t off;
  holy_uint32_t block;
};

struct holy_fshelp_node
{
  struct holy_sfs_data *data;
  holy_uint32_t block;
  holy_uint32_t size;
  holy_uint32_t mtime;
  holy_uint32_t cache_off;
  holy_uint32_t next_extent;
  holy_size_t cache_allocated;
  holy_size_t cache_size;
  struct cache_entry *cache;
};

/* Information about a "mounted" sfs filesystem.  */
struct holy_sfs_data
{
  struct holy_sfs_rblock rblock;
  struct holy_fshelp_node diropen;
  holy_disk_t disk;

  /* Log of blocksize in sectors.  */
  int log_blocksize;

  int fshelp_flags;

  /* Label of the filesystem.  */
  char *label;
};

static holy_dl_t my_mod;


/* Lookup the extent starting with BLOCK in the filesystem described
   by DATA.  Return the extent size in SIZE and the following extent
   in NEXTEXT.  */
static holy_err_t
holy_sfs_read_extent (struct holy_sfs_data *data, unsigned int block,
		      holy_uint32_t *size, holy_uint32_t *nextext)
{
  char *treeblock;
  struct holy_sfs_btree *tree;
  int i;
  holy_uint32_t next;
  holy_size_t blocksize = holy_DISK_SECTOR_SIZE << data->log_blocksize;

  treeblock = holy_malloc (blocksize);
  if (!treeblock)
    return holy_errno;

  next = holy_be_to_cpu32 (data->rblock.btree);
  tree = (struct holy_sfs_btree *) treeblock;

  /* Handle this level in the btree.  */
  do
    {
      holy_uint16_t nnodes;
      holy_disk_read (data->disk,
		      ((holy_disk_addr_t) next) << data->log_blocksize,
		      0, blocksize, treeblock);
      if (holy_errno)
	{
	  holy_free (treeblock);
	  return holy_errno;
	}

      nnodes = holy_be_to_cpu16 (tree->nodes);
      if (nnodes * (holy_uint32_t) (tree)->nodesize > blocksize)
	break;

      for (i = (int) nnodes - 1; i >= 0; i--)
	{

#define EXTNODE(tree, index)						\
	((struct holy_sfs_btree_node *) (((char *) &(tree)->node[0])	\
					 + (index) * (tree)->nodesize))

	  /* Follow the tree down to the leaf level.  */
	  if ((holy_be_to_cpu32 (EXTNODE(tree, i)->key) <= block)
	      && !tree->leaf)
	    {
	      next = holy_be_to_cpu32 (EXTNODE (tree, i)->data);
	      break;
	    }

	  /* If the leaf level is reached, just find the correct extent.  */
	  if (holy_be_to_cpu32 (EXTNODE (tree, i)->key) == block && tree->leaf)
	    {
	      struct holy_sfs_btree_extent *extent;
	      extent = (struct holy_sfs_btree_extent *) EXTNODE (tree, i);

	      /* We found a correct leaf.  */
	      *size = holy_be_to_cpu16 (extent->size);
	      *nextext = holy_be_to_cpu32 (extent->next);

	      holy_free (treeblock);
	      return 0;
	    }

#undef EXTNODE

	}
    } while (!tree->leaf);

  holy_free (treeblock);

  return holy_error (holy_ERR_FILE_READ_ERROR, "SFS extent not found");
}

static holy_disk_addr_t
holy_sfs_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  holy_uint32_t blk;
  holy_uint32_t size = 0;
  holy_uint32_t next = 0;
  holy_disk_addr_t off;
  struct holy_sfs_data *data = node->data;

  /* In case of the first block we don't have to lookup the
     extent, the minimum size is always 1.  */
  if (fileblock == 0)
    return node->block;

  if (!node->cache)
    {
      holy_size_t cache_size;
      /* Assume half-max extents (32768 sectors).  */
      cache_size = ((node->size >> (data->log_blocksize + holy_DISK_SECTOR_BITS
				    + 15))
		    + 3);
      if (cache_size < 8)
	cache_size = 8;

      node->cache_off = 0;
      node->next_extent = node->block;
      node->cache_size = 0;

      node->cache = holy_malloc (sizeof (node->cache[0]) * cache_size);
      if (!node->cache)
	{
	  holy_errno = 0;
	  node->cache_allocated = 0;
	}
      else
	{
	  node->cache_allocated = cache_size;
	  node->cache[0].off = 0;
	  node->cache[0].block = node->block;
	}
    }

  if (fileblock < node->cache_off)
    {
      unsigned int i = 0;
      int j, lg;
      for (lg = 0; node->cache_size >> lg; lg++);

      for (j = lg - 1; j >= 0; j--)
	if ((i | (1 << j)) < node->cache_size
	    && node->cache[(i | (1 << j))].off <= fileblock)
	  i |= (1 << j);
      return node->cache[i].block + fileblock - node->cache[i].off;
    }

  off = node->cache_off;
  blk = node->next_extent;

  while (blk)
    {
      holy_err_t err;

      err = holy_sfs_read_extent (node->data, blk, &size, &next);
      if (err)
	return 0;

      if (node->cache && node->cache_size >= node->cache_allocated)
	{
	  struct cache_entry *e = node->cache;
	  e = holy_realloc (node->cache,node->cache_allocated * 2
			    * sizeof (e[0]));
	  if (!e)
	    {
	      holy_errno = 0;
	      holy_free (node->cache);
	      node->cache = 0;
	    }
	  else
	    {
	      node->cache_allocated *= 2;
	      node->cache = e;
	    }
	}

      if (node->cache)
	{
	  node->cache_off = off + size;
	  node->next_extent = next;
	  node->cache[node->cache_size].off = off;
	  node->cache[node->cache_size].block = blk;
	  node->cache_size++;
	}

      if (fileblock - off < size)
	return fileblock - off + blk;

      off += size;

      blk = next;
    }

  holy_error (holy_ERR_FILE_READ_ERROR,
	      "reading a SFS block outside the extent");

  return 0;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_sfs_read_file (holy_fshelp_node_t node,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_off_t pos, holy_size_t len, char *buf)
{
  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_sfs_read_block,
				node->size, node->data->log_blocksize, 0);
}


static struct holy_sfs_data *
holy_sfs_mount (holy_disk_t disk)
{
  struct holy_sfs_data *data;
  struct holy_sfs_objc *rootobjc;
  char *rootobjc_data = 0;
  holy_uint32_t blk;

  data = holy_malloc (sizeof (*data));
  if (!data)
    return 0;

  /* Read the rootblock.  */
  holy_disk_read (disk, 0, 0, sizeof (struct holy_sfs_rblock),
		  &data->rblock);
  if (holy_errno)
    goto fail;

  /* Make sure this is a sfs filesystem.  */
  if (holy_strncmp ((char *) (data->rblock.header.magic), "SFS", 4)
      || data->rblock.blocksize == 0
      || (data->rblock.blocksize & (data->rblock.blocksize - 1)) != 0
      || (data->rblock.blocksize & holy_cpu_to_be32_compile_time (0xf00001ff)))
    {
      holy_error (holy_ERR_BAD_FS, "not a SFS filesystem");
      goto fail;
    }

  for (data->log_blocksize = 9;
       (1U << data->log_blocksize) < holy_be_to_cpu32 (data->rblock.blocksize);
       data->log_blocksize++);
  data->log_blocksize -= holy_DISK_SECTOR_BITS;
  if (data->rblock.flags & FLAGS_CASE_SENSITIVE)
    data->fshelp_flags = 0;
  else
    data->fshelp_flags = holy_FSHELP_CASE_INSENSITIVE;
  rootobjc_data = holy_malloc (holy_DISK_SECTOR_SIZE << data->log_blocksize);
  if (! rootobjc_data)
    goto fail;

  /* Read the root object container.  */
  holy_disk_read (disk, ((holy_disk_addr_t) holy_be_to_cpu32 (data->rblock.rootobject))
		  << data->log_blocksize, 0,
		  holy_DISK_SECTOR_SIZE << data->log_blocksize, rootobjc_data);
  if (holy_errno)
    goto fail;

  rootobjc = (struct holy_sfs_objc *) rootobjc_data;

  blk = holy_be_to_cpu32 (rootobjc->objects[0].file_dir.dir.dir_objc);
  data->diropen.size = 0;
  data->diropen.block = blk;
  data->diropen.data = data;
  data->diropen.cache = 0;
  data->disk = disk;
  data->label = holy_strdup ((char *) (rootobjc->objects[0].filename));

  holy_free (rootobjc_data);
  return data;

 fail:
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not an SFS filesystem");

  holy_free (data);
  holy_free (rootobjc_data);
  return 0;
}


static char *
holy_sfs_read_symlink (holy_fshelp_node_t node)
{
  struct holy_sfs_data *data = node->data;
  char *symlink;
  char *block;

  block = holy_malloc (holy_DISK_SECTOR_SIZE << data->log_blocksize);
  if (!block)
    return 0;

  holy_disk_read (data->disk, ((holy_disk_addr_t) node->block)
		  << data->log_blocksize,
		  0, holy_DISK_SECTOR_SIZE << data->log_blocksize, block);
  if (holy_errno)
    {
      holy_free (block);
      return 0;
    }

  /* This is just a wild guess, but it always worked for me.  How the
     SLNK block looks like is not documented in the SFS docs.  */
  symlink = holy_malloc (((holy_DISK_SECTOR_SIZE << data->log_blocksize)
			  - 24) * holy_MAX_UTF8_PER_LATIN1 + 1);
  if (!symlink)
    {
      holy_free (block);
      return 0;
    }
  *holy_latin1_to_utf8 ((holy_uint8_t *) symlink, (holy_uint8_t *) &block[24],
			(holy_DISK_SECTOR_SIZE << data->log_blocksize) - 24) = '\0';
  holy_free (block);
  return symlink;
}

/* Helper for holy_sfs_iterate_dir.  */
static int
holy_sfs_create_node (struct holy_fshelp_node **node,
		      struct holy_sfs_data *data,
		      const char *name,
		      holy_uint32_t block, holy_uint32_t size, int type,
		      holy_uint32_t mtime,
		      holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_size_t len = holy_strlen (name);
  holy_uint8_t *name_u8;
  int ret;
  *node = holy_malloc (sizeof (**node));
  if (!*node)
    return 1;
  name_u8 = holy_malloc (len * holy_MAX_UTF8_PER_LATIN1 + 1);
  if (!name_u8)
    {
      holy_free (*node);
      return 1;
    }

  (*node)->data = data;
  (*node)->size = size;
  (*node)->block = block;
  (*node)->mtime = mtime;
  (*node)->cache = 0;
  (*node)->cache_off = 0;
  (*node)->next_extent = block;
  (*node)->cache_size = 0;
  (*node)->cache_allocated = 0;

  *holy_latin1_to_utf8 (name_u8, (const holy_uint8_t *) name, len) = '\0';

  ret = hook ((char *) name_u8, type | data->fshelp_flags, *node, hook_data);
  holy_free (name_u8);
  return ret;
}

static int
holy_sfs_iterate_dir (holy_fshelp_node_t dir,
		      holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  struct holy_fshelp_node *node = 0;
  struct holy_sfs_data *data = dir->data;
  char *objc_data;
  struct holy_sfs_objc *objc;
  unsigned int next = dir->block;
  holy_uint32_t pos;

  objc_data = holy_malloc (holy_DISK_SECTOR_SIZE << data->log_blocksize);
  if (!objc_data)
    goto fail;

  /* The Object container can consist of multiple blocks, iterate over
     every block.  */
  while (next)
    {
      holy_disk_read (data->disk, ((holy_disk_addr_t) next)
		      << data->log_blocksize, 0,
		      holy_DISK_SECTOR_SIZE << data->log_blocksize, objc_data);
      if (holy_errno)
	goto fail;

      objc = (struct holy_sfs_objc *) objc_data;

      pos = (char *) &objc->objects[0] - (char *) objc;

      /* Iterate over all entries in this block.  */
      while (pos + sizeof (struct holy_sfs_obj)
	     < (1U << (holy_DISK_SECTOR_BITS + data->log_blocksize)))
	{
	  struct holy_sfs_obj *obj;
	  obj = (struct holy_sfs_obj *) ((char *) objc + pos);
	  const char *filename = (const char *) obj->filename;
	  holy_size_t len;
	  enum holy_fshelp_filetype type;
	  holy_uint32_t block;

	  /* The filename and comment dynamically increase the size of
	     the object.  */
	  len = holy_strlen (filename);
	  len += holy_strlen (filename + len + 1);

	  pos += sizeof (*obj) + len;
	  /* Round up to a multiple of two bytes.  */
	  pos = ((pos + 1) >> 1) << 1;

	  if (filename[0] == 0)
	    continue;

	  /* First check if the file was not deleted.  */
	  if (obj->type & holy_SFS_TYPE_DELETED)
	    continue;
	  else if (obj->type & holy_SFS_TYPE_SYMLINK)
	    type = holy_FSHELP_SYMLINK;
	  else if (obj->type & holy_SFS_TYPE_DIR)
	    type = holy_FSHELP_DIR;
	  else
	    type = holy_FSHELP_REG;

	  if (type == holy_FSHELP_DIR)
	    block = holy_be_to_cpu32 (obj->file_dir.dir.dir_objc);
	  else
	    block = holy_be_to_cpu32 (obj->file_dir.file.first_block);

	  if (holy_sfs_create_node (&node, data, filename, block,
				    holy_be_to_cpu32 (obj->file_dir.file.size),
				    type, holy_be_to_cpu32 (obj->mtime),
				    hook, hook_data))
	    {
	      holy_free (objc_data);
	      return 1;
	    }
	}

      next = holy_be_to_cpu32 (objc->next);
    }

 fail:
  holy_free (objc_data);
  return 0;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_sfs_open (struct holy_file *file, const char *name)
{
  struct holy_sfs_data *data;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_sfs_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (name, &data->diropen, &fdiro, holy_sfs_iterate_dir,
			 holy_sfs_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  file->size = fdiro->size;
  data->diropen = *fdiro;
  holy_free (fdiro);

  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (data && fdiro != &data->diropen)
    holy_free (fdiro);
  if (data)
    holy_free (data->label);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_err_t
holy_sfs_close (holy_file_t file)
{
  struct holy_sfs_data *data = (struct holy_sfs_data *) file->data;

  holy_free (data->diropen.cache);
  holy_free (data->label);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}


/* Read LEN bytes data from FILE into BUF.  */
static holy_ssize_t
holy_sfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_sfs_data *data = (struct holy_sfs_data *) file->data;

  return holy_sfs_read_file (&data->diropen,
			     file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}


/* Context for holy_sfs_dir.  */
struct holy_sfs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_sfs_dir.  */
static int
holy_sfs_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		   holy_fshelp_node_t node, void *data)
{
  struct holy_sfs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtime = node->mtime + 8 * 365 * 86400 + 86400 * 2;
  info.mtimeset = 1;
  holy_free (node->cache);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_sfs_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_sfs_dir_ctx ctx = { hook, hook_data };
  struct holy_sfs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_sfs_mount (device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (path, &data->diropen, &fdiro, holy_sfs_iterate_dir,
			holy_sfs_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_sfs_iterate_dir (fdiro, holy_sfs_dir_iter, &ctx);

 fail:
  if (data && fdiro != &data->diropen)
    holy_free (fdiro);
  if (data)
    holy_free (data->label);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_err_t
holy_sfs_label (holy_device_t device, char **label)
{
  struct holy_sfs_data *data;
  holy_disk_t disk = device->disk;

  data = holy_sfs_mount (disk);
  if (data)
    {
      holy_size_t len = holy_strlen (data->label);
      *label = holy_malloc (len * holy_MAX_UTF8_PER_LATIN1 + 1);
      if (*label)
	*holy_latin1_to_utf8 ((holy_uint8_t *) *label,
			      (const holy_uint8_t *) data->label,
			      len) = '\0';
      holy_free (data->label);
    }
  holy_free (data);

  return holy_errno;
}


static struct holy_fs holy_sfs_fs =
  {
    .name = "sfs",
    .dir = holy_sfs_dir,
    .open = holy_sfs_open,
    .read = holy_sfs_read,
    .close = holy_sfs_close,
    .label = holy_sfs_label,
#ifdef holy_UTIL
    .reserved_first_sector = 0,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(sfs)
{
  holy_fs_register (&holy_sfs_fs);
  my_mod = mod;
}

holy_MOD_FINI(sfs)
{
  holy_fs_unregister (&holy_sfs_fs);
}
