/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/hfsplus.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/deflate.h>
#include <holy/file.h>

holy_MOD_LICENSE ("GPLv2+");

/* big-endian.  */
struct holy_hfsplus_compress_header1
{
  holy_uint32_t header_size;
  holy_uint32_t end_descriptor_offset;
  holy_uint32_t total_compressed_size_including_seek_blocks_and_header2;
  holy_uint32_t value_0x32;
  holy_uint8_t unused[0xf0];
} holy_PACKED;

/* big-endian.  */
struct holy_hfsplus_compress_header2
{
  holy_uint32_t total_compressed_size_including_seek_blocks;
} holy_PACKED;

/* little-endian.  */
struct holy_hfsplus_compress_header3
{
  holy_uint32_t num_chunks;
} holy_PACKED;

/* little-endian.  */
struct holy_hfsplus_compress_block_descriptor
{
  holy_uint32_t offset;
  holy_uint32_t size;
};

struct holy_hfsplus_compress_end_descriptor
{
  holy_uint8_t always_the_same[50];
} holy_PACKED;

struct holy_hfsplus_attr_header
{
  holy_uint8_t unused[3];
  holy_uint8_t type;
  holy_uint32_t unknown[1];
  holy_uint64_t size;
} holy_PACKED;

struct holy_hfsplus_compress_attr
{
  holy_uint32_t magic;
  holy_uint32_t type;
  holy_uint32_t uncompressed_inline_size;
  holy_uint32_t always_0;
} holy_PACKED;

enum
  {
    HFSPLUS_COMPRESSION_INLINE = 3,
    HFSPLUS_COMPRESSION_RESOURCE = 4
  };

static int
holy_hfsplus_cmp_attrkey (struct holy_hfsplus_key *keya,
			  struct holy_hfsplus_key_internal *keyb)
{
  struct holy_hfsplus_attrkey *attrkey_a = &keya->attrkey;
  struct holy_hfsplus_attrkey_internal *attrkey_b = &keyb->attrkey;
  holy_uint32_t aparent = holy_be_to_cpu32 (attrkey_a->cnid);
  holy_size_t len;
  int diff;

  if (aparent > attrkey_b->cnid)
    return 1;
  if (aparent < attrkey_b->cnid)
    return -1;

  len = holy_be_to_cpu16 (attrkey_a->namelen);
  if (len > attrkey_b->namelen)
    len = attrkey_b->namelen;
  /* Since it's big-endian memcmp gives the same result as manually comparing
     uint16_t but may be faster.  */
  diff = holy_memcmp (attrkey_a->name, attrkey_b->name,
		      len * sizeof (attrkey_a->name[0]));
  if (diff == 0)
    diff = holy_be_to_cpu16 (attrkey_a->namelen) - attrkey_b->namelen;
  return diff;
}

#define HFSPLUS_COMPRESS_BLOCK_SIZE 65536

static holy_ssize_t
hfsplus_read_compressed_real (struct holy_hfsplus_file *node,
			      holy_off_t pos, holy_size_t len, char *buf)
{
  char *tmp_buf = 0;
  holy_size_t len0 = len;

  if (node->compressed == 1)
    {
      holy_memcpy (buf, node->cbuf + pos, len);
      if (holy_file_progress_hook && node->file)
	holy_file_progress_hook (0, 0, len, node->file);
      return len;
    }

  while (len)
    {
      holy_uint32_t block = pos / HFSPLUS_COMPRESS_BLOCK_SIZE;
      holy_size_t curlen = HFSPLUS_COMPRESS_BLOCK_SIZE
	- (pos % HFSPLUS_COMPRESS_BLOCK_SIZE);

      if (curlen > len)
	curlen = len;

      if (node->cbuf_block != block)
	{
	  holy_uint32_t sz = holy_le_to_cpu32 (node->compress_index[block].size);
	  holy_size_t ts;
	  if (!tmp_buf)
	    tmp_buf = holy_malloc (HFSPLUS_COMPRESS_BLOCK_SIZE);
	  if (!tmp_buf)
	    return -1;
	  if (holy_hfsplus_read_file (node, 0, 0,
				      holy_le_to_cpu32 (node->compress_index[block].start) + 0x104,
				      sz, tmp_buf)
	      != (holy_ssize_t) sz)
	    {
	      holy_free (tmp_buf);
	      return -1;
	    }
	  ts = HFSPLUS_COMPRESS_BLOCK_SIZE;
	  if (ts > node->size - (pos & ~(HFSPLUS_COMPRESS_BLOCK_SIZE)))
	    ts = node->size - (pos & ~(HFSPLUS_COMPRESS_BLOCK_SIZE));
	  if (holy_zlib_decompress (tmp_buf, sz, 0,
				    node->cbuf, ts) != (holy_ssize_t) ts)
	    {
	      if (!holy_errno)
		holy_error (holy_ERR_BAD_COMPRESSED_DATA,
			    "premature end of compressed");

	      holy_free (tmp_buf);
	      return -1;
	    }
	  node->cbuf_block = block;
	}
      holy_memcpy (buf, node->cbuf + (pos % HFSPLUS_COMPRESS_BLOCK_SIZE),
		   curlen);
      if (holy_file_progress_hook && node->file)
	holy_file_progress_hook (0, 0, curlen, node->file);
      buf += curlen;
      pos += curlen;
      len -= curlen;
    }
  holy_free (tmp_buf);
  return len0;
}

static holy_err_t
hfsplus_open_compressed_real (struct holy_hfsplus_file *node)
{
  holy_err_t err;
  struct holy_hfsplus_btnode *attr_node;
  holy_off_t attr_off;
  struct holy_hfsplus_key_internal key;
  struct holy_hfsplus_attr_header *attr_head;
  struct holy_hfsplus_compress_attr *cmp_head;
#define c holy_cpu_to_be16_compile_time
  const holy_uint16_t compress_attr_name[] =
    {
      c('c'), c('o'), c('m'), c('.'), c('a'), c('p'), c('p'), c('l'), c('e'),
      c('.'), c('d'), c('e'), c('c'), c('m'), c('p'), c('f'), c('s') };
#undef c
  if (node->size)
    return 0;

  key.attrkey.cnid = node->fileid;
  key.attrkey.namelen = sizeof (compress_attr_name) / sizeof (compress_attr_name[0]);
  key.attrkey.name = compress_attr_name;

  err = holy_hfsplus_btree_search (&node->data->attr_tree, &key,
				   holy_hfsplus_cmp_attrkey,
				   &attr_node, &attr_off);
  if (err || !attr_node)
    {
      holy_errno = 0;
      return 0;
    }

  attr_head = (struct holy_hfsplus_attr_header *)
    ((char *) holy_hfsplus_btree_recptr (&node->data->attr_tree,
					 attr_node, attr_off)
     + sizeof (struct holy_hfsplus_attrkey) + sizeof (compress_attr_name));
  if (attr_head->type != 0x10
      || !(attr_head->size & holy_cpu_to_be64_compile_time(~0xfULL)))
    {
      holy_free (attr_node);
      return 0;
    }
  cmp_head = (struct holy_hfsplus_compress_attr *) (attr_head + 1);
  if (cmp_head->magic != holy_cpu_to_be32_compile_time (0x66706d63))
    {
      holy_free (attr_node);
      return 0;
    }
  node->size = holy_le_to_cpu32 (cmp_head->uncompressed_inline_size);

  if (cmp_head->type == holy_cpu_to_le32_compile_time (HFSPLUS_COMPRESSION_RESOURCE))
    {
      holy_uint32_t index_size;
      node->compressed = 2;

      if (holy_hfsplus_read_file (node, 0, 0,
				  0x104, sizeof (index_size),
				  (char *) &index_size)
	  != 4)
	{
	  node->compressed = 0;
	  holy_free (attr_node);
	  holy_errno = 0;
	  return 0;
	}
      node->compress_index_size = holy_le_to_cpu32 (index_size);
      node->compress_index = holy_malloc (node->compress_index_size
					  * sizeof (node->compress_index[0]));
      if (!node->compress_index)
	{
	  node->compressed = 0;
	  holy_free (attr_node);
	  return holy_errno;
	}
      if (holy_hfsplus_read_file (node, 0, 0,
				  0x104 + sizeof (index_size),
				  node->compress_index_size
				  * sizeof (node->compress_index[0]),
				  (char *) node->compress_index)
	  != (holy_ssize_t) (node->compress_index_size
			     * sizeof (node->compress_index[0])))
	{
	  node->compressed = 0;
	  holy_free (attr_node);
	  holy_free (node->compress_index);
	  holy_errno = 0;
	  return 0;
	}

      node->cbuf_block = -1;

      node->cbuf = holy_malloc (HFSPLUS_COMPRESS_BLOCK_SIZE);
      holy_free (attr_node);
      if (!node->cbuf)
	{
	  node->compressed = 0;
	  holy_free (node->compress_index);
	  return holy_errno;
	}
      return 0;
    }
  if (cmp_head->type != HFSPLUS_COMPRESSION_INLINE)
    {
      holy_free (attr_node);
      return 0;
    }

  node->cbuf = holy_malloc (node->size);
  if (!node->cbuf)
    return holy_errno;

  if (holy_zlib_decompress ((char *) (cmp_head + 1),
			    holy_cpu_to_be64 (attr_head->size)
			    - sizeof (*cmp_head), 0,
			    node->cbuf, node->size)
      != (holy_ssize_t) node->size)
    {
      if (!holy_errno)
	holy_error (holy_ERR_BAD_COMPRESSED_DATA,
		    "premature end of compressed");
      return holy_errno;
    }
  node->compressed = 1;
  return 0;
}

holy_MOD_INIT(hfspluscomp)
{
  holy_hfsplus_open_compressed = hfsplus_open_compressed_real;
  holy_hfsplus_read_compressed = hfsplus_read_compressed_real;
}

holy_MOD_FINI(hfspluscomp)
{
  holy_hfsplus_open_compressed = 0;
  holy_hfsplus_read_compressed = 0;
}
