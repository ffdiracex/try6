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
#include <holy/deflate.h>
#include <minilzo.h>

#include "xz.h"
#include "xz_stream.h"

holy_MOD_LICENSE ("GPLv2+");

/*
  object         format      Pointed by
  superblock     RAW         Fixed offset (0)
  data           RAW ?       Fixed offset (60)
  inode table    Chunk       superblock
  dir table      Chunk       superblock
  fragment table Chunk       unk1
  unk1           RAW, Chunk  superblock
  unk2           RAW         superblock
  UID/GID        Chunk       exttblptr
  exttblptr      RAW         superblock

  UID/GID table is the array ot uint32_t
  unk1 contains pointer to fragment table followed by some chunk.
  unk2 containts one uint64_t
*/

struct holy_squash_super
{
  holy_uint32_t magic;
#define SQUASH_MAGIC 0x73717368
  holy_uint32_t dummy1;
  holy_uint32_t creation_time;
  holy_uint32_t block_size;
  holy_uint32_t dummy2;
  holy_uint16_t compression;
  holy_uint16_t dummy3;
  holy_uint64_t dummy4;
  holy_uint16_t root_ino_offset;
  holy_uint32_t root_ino_chunk;
  holy_uint16_t dummy5;
  holy_uint64_t total_size;
  holy_uint64_t exttbloffset;
  holy_uint64_t dummy6;
  holy_uint64_t inodeoffset;
  holy_uint64_t diroffset;
  holy_uint64_t unk1offset;
  holy_uint64_t unk2offset;
} holy_PACKED;

/* Chunk-based */
struct holy_squash_inode
{
  /* Same values as direlem types. */
  holy_uint16_t type;
  holy_uint16_t dummy[3];
  holy_uint32_t mtime;
  holy_uint32_t dummy2;
  union
  {
    struct {
      holy_uint32_t chunk;
      holy_uint32_t fragment;
      holy_uint32_t offset;
      holy_uint32_t size;
      holy_uint32_t block_size[0];
    }  holy_PACKED file;
    struct {
      holy_uint64_t chunk;
      holy_uint64_t size;
      holy_uint32_t dummy1[3];
      holy_uint32_t fragment;
      holy_uint32_t offset;
      holy_uint32_t dummy3;
      holy_uint32_t block_size[0];
    }  holy_PACKED long_file;
    struct {
      holy_uint32_t chunk;
      holy_uint32_t dummy;
      holy_uint16_t size;
      holy_uint16_t offset;
    } holy_PACKED dir;
    struct {
      holy_uint32_t dummy1;
      holy_uint32_t size;
      holy_uint32_t chunk;
      holy_uint32_t dummy2;
      holy_uint16_t dummy3;
      holy_uint16_t offset;
    } holy_PACKED long_dir;
    struct {
      holy_uint32_t dummy;
      holy_uint32_t namelen;
      char name[0];
    } holy_PACKED symlink;
  }  holy_PACKED;
} holy_PACKED;

struct holy_squash_cache_inode
{
  struct holy_squash_inode ino;
  holy_disk_addr_t ino_chunk;
  holy_uint16_t	ino_offset;
  holy_uint32_t *block_sizes;
  holy_disk_addr_t *cumulated_block_sizes;
};

/* Chunk-based.  */
struct holy_squash_dirent_header
{
  /* Actually the value is the number of elements - 1.  */
  holy_uint32_t nelems;
  holy_uint32_t ino_chunk;
  holy_uint32_t dummy;
} holy_PACKED;

struct holy_squash_dirent
{
  holy_uint16_t ino_offset;
  holy_uint16_t dummy;
  holy_uint16_t type;
  /* Actually the value is the length of name - 1.  */
  holy_uint16_t namelen;
  char name[0];
} holy_PACKED;

enum
  {
    SQUASH_TYPE_DIR = 1,
    SQUASH_TYPE_REGULAR = 2,
    SQUASH_TYPE_SYMLINK = 3,
    SQUASH_TYPE_LONG_DIR = 8,
    SQUASH_TYPE_LONG_REGULAR = 9,
  };


struct holy_squash_frag_desc
{
  holy_uint64_t offset;
  holy_uint32_t size;
  holy_uint32_t dummy;
} holy_PACKED;

enum
  {
    SQUASH_CHUNK_FLAGS = 0x8000,
    SQUASH_CHUNK_UNCOMPRESSED = 0x8000
  };

enum
  {
    SQUASH_BLOCK_FLAGS = 0x1000000,
    SQUASH_BLOCK_UNCOMPRESSED = 0x1000000
  };

enum
  {
    COMPRESSION_ZLIB = 1,
    COMPRESSION_LZO = 3,
    COMPRESSION_XZ = 4,
  };


#define SQUASH_CHUNK_SIZE 0x2000
#define XZBUFSIZ 0x2000

struct holy_squash_data
{
  holy_disk_t disk;
  struct holy_squash_super sb;
  struct holy_squash_cache_inode ino;
  holy_uint64_t fragments;
  int log2_blksz;
  holy_size_t blksz;
  holy_ssize_t (*decompress) (char *inbuf, holy_size_t insize, holy_off_t off,
			      char *outbuf, holy_size_t outsize,
			      struct holy_squash_data *data);
  struct xz_dec *xzdec;
  char *xzbuf;
};

struct holy_fshelp_node
{
  struct holy_squash_data *data;
  struct holy_squash_inode ino;
  holy_size_t stsize;
  struct 
  {
    holy_disk_addr_t ino_chunk;
    holy_uint16_t ino_offset;
  } stack[1];
};

static holy_err_t
read_chunk (struct holy_squash_data *data, void *buf, holy_size_t len,
	    holy_uint64_t chunk_start, holy_off_t offset)
{
  while (len > 0)
    {
      holy_uint64_t csize;
      holy_uint16_t d;
      holy_err_t err;
      while (1)
	{
	  err = holy_disk_read (data->disk,
				chunk_start >> holy_DISK_SECTOR_BITS,
				chunk_start & (holy_DISK_SECTOR_SIZE - 1),
				sizeof (d), &d);
	  if (err)
	    return err;
	  if (offset < SQUASH_CHUNK_SIZE)
	    break;
	  offset -= SQUASH_CHUNK_SIZE;
	  chunk_start += 2 + (holy_le_to_cpu16 (d) & ~SQUASH_CHUNK_FLAGS);
	}

      csize = SQUASH_CHUNK_SIZE - offset;
      if (csize > len)
	csize = len;
  
      if (holy_le_to_cpu16 (d) & SQUASH_CHUNK_UNCOMPRESSED)
	{
	  holy_disk_addr_t a = chunk_start + 2 + offset;
	  err = holy_disk_read (data->disk, (a >> holy_DISK_SECTOR_BITS),
				a & (holy_DISK_SECTOR_SIZE - 1),
				csize, buf);
	  if (err)
	    return err;
	}
      else
	{
	  char *tmp;
	  holy_size_t bsize = holy_le_to_cpu16 (d) & ~SQUASH_CHUNK_FLAGS;
	  holy_disk_addr_t a = chunk_start + 2;
	  tmp = holy_malloc (bsize);
	  if (!tmp)
	    return holy_errno;
	  /* FIXME: buffer uncompressed data.  */
	  err = holy_disk_read (data->disk, (a >> holy_DISK_SECTOR_BITS),
				a & (holy_DISK_SECTOR_SIZE - 1),
				bsize, tmp);
	  if (err)
	    {
	      holy_free (tmp);
	      return err;
	    }

	  if (data->decompress (tmp, bsize, offset,
				buf, csize, data) < 0)
	    {
	      holy_free (tmp);
	      return holy_errno;
	    }
	  holy_free (tmp);
	}
      len -= csize;
      offset += csize;
      buf = (char *) buf + csize;
    }
  return holy_ERR_NONE;
}

static holy_ssize_t
zlib_decompress (char *inbuf, holy_size_t insize, holy_off_t off,
		 char *outbuf, holy_size_t outsize,
		 struct holy_squash_data *data __attribute__ ((unused)))
{
  return holy_zlib_decompress (inbuf, insize, off, outbuf, outsize);
}

static holy_ssize_t
lzo_decompress (char *inbuf, holy_size_t insize, holy_off_t off,
		char *outbuf, holy_size_t len, struct holy_squash_data *data)
{
  lzo_uint usize = data->blksz;
  holy_uint8_t *udata;

  if (usize < 8192)
    usize = 8192;

  udata = holy_malloc (usize);
  if (!udata)
    return -1;

  if (lzo1x_decompress_safe ((holy_uint8_t *) inbuf,
			     insize, udata, &usize, NULL) != LZO_E_OK)
    {
      holy_error (holy_ERR_BAD_FS, "incorrect compressed chunk");
      holy_free (udata);
      return -1;
    }
  holy_memcpy (outbuf, udata + off, len);
  holy_free (udata);
  return len;
}

static holy_ssize_t
xz_decompress (char *inbuf, holy_size_t insize, holy_off_t off,
	       char *outbuf, holy_size_t len, struct holy_squash_data *data)
{
  holy_size_t ret = 0;
  holy_off_t pos = 0;
  struct xz_buf buf;

  xz_dec_reset (data->xzdec);
  buf.in = (holy_uint8_t *) inbuf;
  buf.in_pos = 0;
  buf.in_size = insize;
  buf.out = (holy_uint8_t *) data->xzbuf;
  buf.out_pos = 0;
  buf.out_size = XZBUFSIZ;

  while (len)
    {
      enum xz_ret xzret;
      
      buf.out_pos = 0;

      xzret = xz_dec_run (data->xzdec, &buf);

      if (xzret != XZ_OK && xzret != XZ_STREAM_END)
	{
	  holy_error (holy_ERR_BAD_COMPRESSED_DATA, "invalid xz chunk");
	  return -1;
	}
      if (pos + buf.out_pos >= off)
	{
	  holy_ssize_t outoff = pos - off;
	  holy_size_t l;
	  if (outoff >= 0)
	    {
	      l = buf.out_pos;
	      if (l > len)
		l = len;
	      holy_memcpy (outbuf + outoff, buf.out, l);
	    }
	  else
	    {
	      outoff = -outoff;
	      l = buf.out_pos - outoff;
	      if (l > len)
		l = len;
	      holy_memcpy (outbuf, buf.out + outoff, l);
	    }
	  ret += l;
	  len -= l;
	}
      pos += buf.out_pos;
      if (xzret == XZ_STREAM_END)
	break;
    }
  return ret;
}

static struct holy_squash_data *
squash_mount (holy_disk_t disk)
{
  struct holy_squash_super sb;
  holy_err_t err;
  struct holy_squash_data *data;
  holy_uint64_t frag;

  err = holy_disk_read (disk, 0, 0, sizeof (sb), &sb);
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a squash4");
  if (err)
    return NULL;
  if (sb.magic != holy_cpu_to_le32_compile_time (SQUASH_MAGIC)
      || sb.block_size == 0
      || ((sb.block_size - 1) & sb.block_size))
    {
      holy_error (holy_ERR_BAD_FS, "not squash4");
      return NULL;
    }

  err = holy_disk_read (disk,
			holy_le_to_cpu64 (sb.unk1offset)
			>> holy_DISK_SECTOR_BITS,
			holy_le_to_cpu64 (sb.unk1offset)
			& (holy_DISK_SECTOR_SIZE - 1), sizeof (frag), &frag);
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a squash4");
  if (err)
    return NULL;

  data = holy_zalloc (sizeof (*data));
  if (!data)
    return NULL;
  data->sb = sb;
  data->disk = disk;
  data->fragments = holy_le_to_cpu64 (frag);

  switch (sb.compression)
    {
    case holy_cpu_to_le16_compile_time (COMPRESSION_ZLIB):
      data->decompress = zlib_decompress;
      break;
    case holy_cpu_to_le16_compile_time (COMPRESSION_LZO):
      data->decompress = lzo_decompress;
      break;
    case holy_cpu_to_le16_compile_time (COMPRESSION_XZ):
      data->decompress = xz_decompress;
      data->xzbuf = holy_malloc (XZBUFSIZ);
      if (!data->xzbuf)
	{
	  holy_free (data);
	  return NULL;
	}
      data->xzdec = xz_dec_init (1 << 16);
      if (!data->xzdec)
	{
	  holy_free (data->xzbuf);
	  holy_free (data);
	  return NULL;
	}
      break;
    default:
      holy_free (data);
      holy_error (holy_ERR_BAD_FS, "unsupported compression %d",
		  holy_le_to_cpu16 (sb.compression));
      return NULL;
    }

  data->blksz = holy_le_to_cpu32 (data->sb.block_size);
  for (data->log2_blksz = 0; 
       (1U << data->log2_blksz) < data->blksz;
       data->log2_blksz++);

  return data;
}

static char *
holy_squash_read_symlink (holy_fshelp_node_t node)
{
  char *ret;
  holy_err_t err;
  ret = holy_malloc (holy_le_to_cpu32 (node->ino.symlink.namelen) + 1);

  err = read_chunk (node->data, ret,
		    holy_le_to_cpu32 (node->ino.symlink.namelen),
		    holy_le_to_cpu64 (node->data->sb.inodeoffset)
		    + node->stack[node->stsize - 1].ino_chunk,
		    node->stack[node->stsize - 1].ino_offset
		    + (node->ino.symlink.name - (char *) &node->ino));
  if (err)
    {
      holy_free (ret);
      return NULL;
    }
  ret[holy_le_to_cpu32 (node->ino.symlink.namelen)] = 0;
  return ret;
}

static int
holy_squash_iterate_dir (holy_fshelp_node_t dir,
			 holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_uint32_t off;
  holy_uint32_t endoff;
  holy_uint64_t chunk;
  unsigned i;

  /* FIXME: why - 3 ? */
  switch (dir->ino.type)
    {
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_DIR):
      off = holy_le_to_cpu16 (dir->ino.dir.offset);
      endoff = holy_le_to_cpu16 (dir->ino.dir.size) + off - 3;
      chunk = holy_le_to_cpu32 (dir->ino.dir.chunk);
      break;
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_DIR):
      off = holy_le_to_cpu16 (dir->ino.long_dir.offset);
      endoff = holy_le_to_cpu16 (dir->ino.long_dir.size) + off - 3;
      chunk = holy_le_to_cpu32 (dir->ino.long_dir.chunk);
      break;
    default:
      holy_error (holy_ERR_BAD_FS, "unexpected ino type 0x%x",
		  holy_le_to_cpu16 (dir->ino.type));
      return 0;
    }

  {
    holy_fshelp_node_t node;
    node = holy_malloc (sizeof (*node) + dir->stsize * sizeof (dir->stack[0]));
    if (!node)
      return 0;
    holy_memcpy (node, dir,
		 sizeof (*node) + dir->stsize * sizeof (dir->stack[0]));
    if (hook (".", holy_FSHELP_DIR, node, hook_data))
      return 1;

    if (dir->stsize != 1)
      {
	holy_err_t err;

	node = holy_malloc (sizeof (*node) + dir->stsize * sizeof (dir->stack[0]));
	if (!node)
	  return 0;

	holy_memcpy (node, dir,
		     sizeof (*node) + dir->stsize * sizeof (dir->stack[0]));

	node->stsize--;
	err = read_chunk (dir->data, &node->ino, sizeof (node->ino),
			  holy_le_to_cpu64 (dir->data->sb.inodeoffset)
			  + node->stack[node->stsize - 1].ino_chunk,
			  node->stack[node->stsize - 1].ino_offset);
	if (err)
	  return 0;

	if (hook ("..", holy_FSHELP_DIR, node, hook_data))
	  return 1;
      }
  }

  while (off < endoff)
    {
      struct holy_squash_dirent_header dh;
      holy_err_t err;

      err = read_chunk (dir->data, &dh, sizeof (dh),
			holy_le_to_cpu64 (dir->data->sb.diroffset)
			+ chunk, off);
      if (err)
	return 0;
      off += sizeof (dh);
      for (i = 0; i < (unsigned) holy_le_to_cpu32 (dh.nelems) + 1; i++)
	{
	  char *buf;
	  int r;
	  struct holy_fshelp_node *node;
	  enum holy_fshelp_filetype filetype = holy_FSHELP_REG;
	  struct holy_squash_dirent di;
	  struct holy_squash_inode ino;

	  err = read_chunk (dir->data, &di, sizeof (di),
			    holy_le_to_cpu64 (dir->data->sb.diroffset)
			    + chunk, off);
	  if (err)
	    return 0;
	  off += sizeof (di);

	  err = read_chunk (dir->data, &ino, sizeof (ino),
			    holy_le_to_cpu64 (dir->data->sb.inodeoffset)
			    + holy_le_to_cpu32 (dh.ino_chunk),
			    holy_cpu_to_le16 (di.ino_offset));
	  if (err)
	    return 0;

	  buf = holy_malloc (holy_le_to_cpu16 (di.namelen) + 2);
	  if (!buf)
	    return 0;
	  err = read_chunk (dir->data, buf,
			    holy_le_to_cpu16 (di.namelen) + 1,
			    holy_le_to_cpu64 (dir->data->sb.diroffset)
			    + chunk, off);
	  if (err)
	    return 0;

	  off += holy_le_to_cpu16 (di.namelen) + 1;
	  buf[holy_le_to_cpu16 (di.namelen) + 1] = 0;
	  if (holy_le_to_cpu16 (di.type) == SQUASH_TYPE_DIR)
	    filetype = holy_FSHELP_DIR;
	  if (holy_le_to_cpu16 (di.type) == SQUASH_TYPE_SYMLINK)
	    filetype = holy_FSHELP_SYMLINK;

	  node = holy_malloc (sizeof (*node)
			      + (dir->stsize + 1) * sizeof (dir->stack[0]));
	  if (! node)
	    return 0;

	  holy_memcpy (node, dir,
		       sizeof (*node) + dir->stsize * sizeof (dir->stack[0]));

	  node->ino = ino;
	  node->stack[node->stsize].ino_chunk = holy_le_to_cpu32 (dh.ino_chunk);
	  node->stack[node->stsize].ino_offset = holy_le_to_cpu16 (di.ino_offset);
	  node->stsize++;
	  r = hook (buf, filetype, node, hook_data);

	  holy_free (buf);
	  if (r)
	    return r;
	}
    }
  return 0;
}

static holy_err_t
make_root_node (struct holy_squash_data *data, struct holy_fshelp_node *root)
{
  holy_memset (root, 0, sizeof (*root));
  root->data = data;
  root->stsize = 1;
  root->stack[0].ino_chunk = holy_le_to_cpu32 (data->sb.root_ino_chunk);
  root->stack[0].ino_offset = holy_cpu_to_le16 (data->sb.root_ino_offset);
 return read_chunk (data, &root->ino, sizeof (root->ino),
		    holy_le_to_cpu64 (data->sb.inodeoffset)
		    + root->stack[0].ino_chunk,
		    root->stack[0].ino_offset);
}

static void
squash_unmount (struct holy_squash_data *data)
{
  if (data->xzdec)
    xz_dec_end (data->xzdec);
  holy_free (data->xzbuf);
  holy_free (data->ino.cumulated_block_sizes);
  holy_free (data->ino.block_sizes);
  holy_free (data);
}


/* Context for holy_squash_dir.  */
struct holy_squash_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_squash_dir.  */
static int
holy_squash_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		      holy_fshelp_node_t node, void *data)
{
  struct holy_squash_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = 1;
  info.mtime = holy_le_to_cpu32 (node->ino.mtime);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_squash_dir (holy_device_t device, const char *path,
		 holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_squash_dir_ctx ctx = { hook, hook_data };
  struct holy_squash_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;
  struct holy_fshelp_node root;
  holy_err_t err;

  data = squash_mount (device->disk);
  if (! data)
    return holy_errno;

  err = make_root_node (data, &root);
  if (err)
    return err;

  holy_fshelp_find_file (path, &root, &fdiro, holy_squash_iterate_dir,
			 holy_squash_read_symlink, holy_FSHELP_DIR);
  if (!holy_errno)
    holy_squash_iterate_dir (fdiro, holy_squash_dir_iter, &ctx);

  squash_unmount (data);

  return holy_errno;
}

static holy_err_t
holy_squash_open (struct holy_file *file, const char *name)
{
  struct holy_squash_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;
  struct holy_fshelp_node root;
  holy_err_t err;

  data = squash_mount (file->device->disk);
  if (! data)
    return holy_errno;

  err = make_root_node (data, &root);
  if (err)
    return err;

  holy_fshelp_find_file (name, &root, &fdiro, holy_squash_iterate_dir,
			 holy_squash_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    {
      squash_unmount (data);
      return holy_errno;
    }

  file->data = data;
  data->ino.ino = fdiro->ino;
  data->ino.block_sizes = NULL;
  data->ino.cumulated_block_sizes = NULL;
  data->ino.ino_chunk = fdiro->stack[fdiro->stsize - 1].ino_chunk;
  data->ino.ino_offset = fdiro->stack[fdiro->stsize - 1].ino_offset;

  switch (fdiro->ino.type)
    {
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_REGULAR):
      file->size = holy_le_to_cpu64 (fdiro->ino.long_file.size);
      break;
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_REGULAR):
      file->size = holy_le_to_cpu32 (fdiro->ino.file.size);
      break;
    default:
      {
	holy_uint16_t type = holy_le_to_cpu16 (fdiro->ino.type);
	holy_free (fdiro);
	squash_unmount (data);
	return holy_error (holy_ERR_BAD_FS, "unexpected ino type 0x%x", type);
      }
    }

  holy_free (fdiro);

  return holy_ERR_NONE;
}

static holy_ssize_t
direct_read (struct holy_squash_data *data,
	     struct holy_squash_cache_inode *ino,
	     holy_off_t off, char *buf, holy_size_t len)
{
  holy_err_t err;
  holy_off_t cumulated_uncompressed_size = 0;
  holy_uint64_t a = 0;
  holy_size_t i;
  holy_size_t origlen = len;

  switch (ino->ino.type)
    {
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_REGULAR):
      a = holy_le_to_cpu64 (ino->ino.long_file.chunk);
      break;
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_REGULAR):
      a = holy_le_to_cpu32 (ino->ino.file.chunk);
      break;
    }

  if (!ino->block_sizes)
    {
      holy_off_t total_size = 0;
      holy_size_t total_blocks;
      holy_size_t block_offset = 0;
      switch (ino->ino.type)
	{
	case holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_REGULAR):
	  total_size = holy_le_to_cpu64 (ino->ino.long_file.size);
	  block_offset = ((char *) &ino->ino.long_file.block_size
			  - (char *) &ino->ino);
	  break;
	case holy_cpu_to_le16_compile_time (SQUASH_TYPE_REGULAR):
	  total_size = holy_le_to_cpu32 (ino->ino.file.size);
	  block_offset = ((char *) &ino->ino.file.block_size
			  - (char *) &ino->ino);
	  break;
	}
      total_blocks = ((total_size + data->blksz - 1) >> data->log2_blksz);
      ino->block_sizes = holy_malloc (total_blocks
				      * sizeof (ino->block_sizes[0]));
      ino->cumulated_block_sizes = holy_malloc (total_blocks
						* sizeof (ino->cumulated_block_sizes[0]));
      if (!ino->block_sizes || !ino->cumulated_block_sizes)
	{
	  holy_free (ino->block_sizes);
	  holy_free (ino->cumulated_block_sizes);
	  ino->block_sizes = 0;
	  ino->cumulated_block_sizes = 0;
	  return -1;
	}
      err = read_chunk (data, ino->block_sizes,
			total_blocks * sizeof (ino->block_sizes[0]),
			holy_le_to_cpu64 (data->sb.inodeoffset)
			+ ino->ino_chunk,
			ino->ino_offset + block_offset);
      if (err)
	{
	  holy_free (ino->block_sizes);
	  holy_free (ino->cumulated_block_sizes);
	  ino->block_sizes = 0;
	  ino->cumulated_block_sizes = 0;
	  return -1;
	}
      ino->cumulated_block_sizes[0] = 0;
      for (i = 1; i < total_blocks; i++)
	ino->cumulated_block_sizes[i] = ino->cumulated_block_sizes[i - 1]
	  + (holy_le_to_cpu32 (ino->block_sizes[i - 1]) & ~SQUASH_BLOCK_FLAGS);
    }

  if (a == 0)
    a = sizeof (struct holy_squash_super);
  i = off >> data->log2_blksz;
  cumulated_uncompressed_size = data->blksz * (holy_disk_addr_t) i;
  while (cumulated_uncompressed_size < off + len)
    {
      holy_size_t boff, curread;
      boff = off - cumulated_uncompressed_size;
      curread = data->blksz - boff;
      if (curread > len)
	curread = len;
      if (!ino->block_sizes[i])
	{
	  /* Sparse block */
	  holy_memset (buf, '\0', curread);
	}
      else if (!(ino->block_sizes[i]
	    & holy_cpu_to_le32_compile_time (SQUASH_BLOCK_UNCOMPRESSED)))
	{
	  char *block;
	  holy_size_t csize;
	  csize = holy_le_to_cpu32 (ino->block_sizes[i]) & ~SQUASH_BLOCK_FLAGS;
	  block = holy_malloc (csize);
	  if (!block)
	    return -1;
	  err = holy_disk_read (data->disk,
				(ino->cumulated_block_sizes[i] + a)
				>> holy_DISK_SECTOR_BITS,
				(ino->cumulated_block_sizes[i] + a)
				& (holy_DISK_SECTOR_SIZE - 1),
				csize, block);
	  if (err)
	    {
	      holy_free (block);
	      return -1;
	    }
	  if (data->decompress (block, csize, boff, buf, curread, data)
	      != (holy_ssize_t) curread)
	    {
	      holy_free (block);
	      if (!holy_errno)
		holy_error (holy_ERR_BAD_FS, "incorrect compressed chunk");
	      return -1;
	    }
	  holy_free (block);
	}
      else
	err = holy_disk_read (data->disk,
			      (ino->cumulated_block_sizes[i] + a + boff)
			      >> holy_DISK_SECTOR_BITS,
			      (ino->cumulated_block_sizes[i] + a + boff)
			      & (holy_DISK_SECTOR_SIZE - 1),
			      curread, buf);
      if (err)
	return -1;
      off += curread;
      len -= curread;
      buf += curread;
      cumulated_uncompressed_size += holy_le_to_cpu32 (data->sb.block_size);
      i++;
    }
  return origlen;
}


static holy_ssize_t
holy_squash_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_squash_data *data = file->data;
  struct holy_squash_cache_inode *ino = &data->ino;
  holy_off_t off = file->offset;
  holy_err_t err;
  holy_uint64_t a, b;
  holy_uint32_t fragment = 0;
  int compressed = 0;
  struct holy_squash_frag_desc frag;
  holy_off_t direct_len;
  holy_uint64_t mask = holy_le_to_cpu32 (data->sb.block_size) - 1;
  holy_size_t orig_len = len;

  switch (ino->ino.type)
    {
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_REGULAR):
      fragment = holy_le_to_cpu32 (ino->ino.long_file.fragment);
      break;
    case holy_cpu_to_le16_compile_time (SQUASH_TYPE_REGULAR):
      fragment = holy_le_to_cpu32 (ino->ino.file.fragment);
      break;
    }

  /* Squash may pack file tail as fragment. So read initial part directly and
     get tail from fragments */
  direct_len = fragment == 0xffffffff ? file->size : file->size & ~mask;
  if (off < direct_len)
    {
      holy_size_t read_len = direct_len - off;
      holy_ssize_t res;

      if (read_len > len)
	read_len = len;
      res = direct_read (data, ino, off, buf, read_len);
      if ((holy_size_t) res != read_len)
	return -1; /* FIXME: is short read possible here? */
      len -= read_len;
      if (!len)
	return read_len;
      buf += read_len;
      off = 0;
    }
  else
    off -= direct_len;
 
  err = read_chunk (data, &frag, sizeof (frag),
		    data->fragments, sizeof (frag) * fragment);
  if (err)
    return -1;
  a = holy_le_to_cpu64 (frag.offset);
  compressed = !(frag.size & holy_cpu_to_le32_compile_time (SQUASH_BLOCK_UNCOMPRESSED));
  if (ino->ino.type == holy_cpu_to_le16_compile_time (SQUASH_TYPE_LONG_REGULAR))
    b = holy_le_to_cpu32 (ino->ino.long_file.offset) + off;
  else
    b = holy_le_to_cpu32 (ino->ino.file.offset) + off;
  
  /* FIXME: cache uncompressed chunks.  */
  if (compressed)
    {
      char *block;
      block = holy_malloc (holy_le_to_cpu32 (frag.size));
      if (!block)
	return -1;
      err = holy_disk_read (data->disk,
			    a >> holy_DISK_SECTOR_BITS,
			    a & (holy_DISK_SECTOR_SIZE - 1),
			    holy_le_to_cpu32 (frag.size), block);
      if (err)
	{
	  holy_free (block);
	  return -1;
	}
      if (data->decompress (block, holy_le_to_cpu32 (frag.size),
			    b, buf, len, data)
	  != (holy_ssize_t) len)
	{
	  holy_free (block);
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_FS, "incorrect compressed chunk");
	  return -1;
	}
      holy_free (block);
    }
  else
    {
      err = holy_disk_read (data->disk, (a + b) >> holy_DISK_SECTOR_BITS,
			  (a + b) & (holy_DISK_SECTOR_SIZE - 1), len, buf);
      if (err)
	return -1;
    }
  return orig_len;
}

static holy_err_t
holy_squash_close (holy_file_t file)
{
  squash_unmount (file->data);
  return holy_ERR_NONE;
}

static holy_err_t
holy_squash_mtime (holy_device_t dev, holy_int32_t *tm)
{
  struct holy_squash_data *data = 0;

  data = squash_mount (dev->disk);
  if (! data)
    return holy_errno;
  *tm = holy_le_to_cpu32 (data->sb.creation_time);
  squash_unmount (data);
  return holy_ERR_NONE;
} 

static struct holy_fs holy_squash_fs =
  {
    .name = "squash4",
    .dir = holy_squash_dir,
    .open = holy_squash_open,
    .read = holy_squash_read,
    .close = holy_squash_close,
    .mtime = holy_squash_mtime,
#ifdef holy_UTIL
    .reserved_first_sector = 0,
    .blocklist_install = 0,
#endif
    .next = 0
  };

holy_MOD_INIT(squash4)
{
  holy_fs_register (&holy_squash_fs);
}

holy_MOD_FINI(squash4)
{
  holy_fs_unregister (&holy_squash_fs);
}

