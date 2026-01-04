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
#include <holy/lib/crc.h>
#include <holy/deflate.h>
#include <minilzo.h>
#include <holy/i18n.h>
#include <holy/btrfs.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_BTRFS_SIGNATURE "_BHRfS_M"

/* From http://www.oberhumer.com/opensource/lzo/lzofaq.php
 * LZO will expand incompressible data by a little amount. I still haven't
 * computed the exact values, but I suggest using these formulas for
 * a worst-case expansion calculation:
 *
 * output_block_size = input_block_size + (input_block_size / 16) + 64 + 3
 *  */
#define holy_BTRFS_LZO_BLOCK_SIZE 4096
#define holy_BTRFS_LZO_BLOCK_MAX_CSIZE (holy_BTRFS_LZO_BLOCK_SIZE + \
				     (holy_BTRFS_LZO_BLOCK_SIZE / 16) + 64 + 3)

typedef holy_uint8_t holy_btrfs_checksum_t[0x20];
typedef holy_uint16_t holy_btrfs_uuid_t[8];

struct holy_btrfs_device
{
  holy_uint64_t device_id;
  holy_uint64_t size;
  holy_uint8_t dummy[0x62 - 0x10];
} holy_PACKED;

struct holy_btrfs_superblock
{
  holy_btrfs_checksum_t checksum;
  holy_btrfs_uuid_t uuid;
  holy_uint8_t dummy[0x10];
  holy_uint8_t signature[sizeof (holy_BTRFS_SIGNATURE) - 1];
  holy_uint64_t generation;
  holy_uint64_t root_tree;
  holy_uint64_t chunk_tree;
  holy_uint8_t dummy2[0x20];
  holy_uint64_t root_dir_objectid;
  holy_uint8_t dummy3[0x41];
  struct holy_btrfs_device this_device;
  char label[0x100];
  holy_uint8_t dummy4[0x100];
  holy_uint8_t bootstrap_mapping[0x800];
} holy_PACKED;

struct btrfs_header
{
  holy_btrfs_checksum_t checksum;
  holy_btrfs_uuid_t uuid;
  holy_uint8_t dummy[0x30];
  holy_uint32_t nitems;
  holy_uint8_t level;
} holy_PACKED;

struct holy_btrfs_device_desc
{
  holy_device_t dev;
  holy_uint64_t id;
};

struct holy_btrfs_data
{
  struct holy_btrfs_superblock sblock;
  holy_uint64_t tree;
  holy_uint64_t inode;

  struct holy_btrfs_device_desc *devices_attached;
  unsigned n_devices_attached;
  unsigned n_devices_allocated;

  /* Cached extent data.  */
  holy_uint64_t extstart;
  holy_uint64_t extend;
  holy_uint64_t extino;
  holy_uint64_t exttree;
  holy_size_t extsize;
  struct holy_btrfs_extent_data *extent;
};

struct holy_btrfs_chunk_item
{
  holy_uint64_t size;
  holy_uint64_t dummy;
  holy_uint64_t stripe_length;
  holy_uint64_t type;
#define holy_BTRFS_CHUNK_TYPE_BITS_DONTCARE 0x07
#define holy_BTRFS_CHUNK_TYPE_SINGLE        0x00
#define holy_BTRFS_CHUNK_TYPE_RAID0         0x08
#define holy_BTRFS_CHUNK_TYPE_RAID1         0x10
#define holy_BTRFS_CHUNK_TYPE_DUPLICATED    0x20
#define holy_BTRFS_CHUNK_TYPE_RAID10        0x40
  holy_uint8_t dummy2[0xc];
  holy_uint16_t nstripes;
  holy_uint16_t nsubstripes;
} holy_PACKED;

struct holy_btrfs_chunk_stripe
{
  holy_uint64_t device_id;
  holy_uint64_t offset;
  holy_btrfs_uuid_t device_uuid;
} holy_PACKED;

struct holy_btrfs_leaf_node
{
  struct holy_btrfs_key key;
  holy_uint32_t offset;
  holy_uint32_t size;
} holy_PACKED;

struct holy_btrfs_internal_node
{
  struct holy_btrfs_key key;
  holy_uint64_t addr;
  holy_uint64_t dummy;
} holy_PACKED;

struct holy_btrfs_dir_item
{
  struct holy_btrfs_key key;
  holy_uint8_t dummy[8];
  holy_uint16_t m;
  holy_uint16_t n;
#define holy_BTRFS_DIR_ITEM_TYPE_REGULAR 1
#define holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY 2
#define holy_BTRFS_DIR_ITEM_TYPE_SYMLINK 7
  holy_uint8_t type;
  char name[0];
} holy_PACKED;

struct holy_btrfs_leaf_descriptor
{
  unsigned depth;
  unsigned allocated;
  struct
  {
    holy_disk_addr_t addr;
    unsigned iter;
    unsigned maxiter;
    int leaf;
  } *data;
};

struct holy_btrfs_time
{
  holy_int64_t sec;
  holy_uint32_t nanosec;
} holy_PACKED;

struct holy_btrfs_inode
{
  holy_uint8_t dummy1[0x10];
  holy_uint64_t size;
  holy_uint8_t dummy2[0x70];
  struct holy_btrfs_time mtime;
} holy_PACKED;

struct holy_btrfs_extent_data
{
  holy_uint64_t dummy;
  holy_uint64_t size;
  holy_uint8_t compression;
  holy_uint8_t encryption;
  holy_uint16_t encoding;
  holy_uint8_t type;
  union
  {
    char inl[0];
    struct
    {
      holy_uint64_t laddr;
      holy_uint64_t compressed_size;
      holy_uint64_t offset;
      holy_uint64_t filled;
    };
  };
} holy_PACKED;

#define holy_BTRFS_EXTENT_INLINE 0
#define holy_BTRFS_EXTENT_REGULAR 1

#define holy_BTRFS_COMPRESSION_NONE 0
#define holy_BTRFS_COMPRESSION_ZLIB 1
#define holy_BTRFS_COMPRESSION_LZO  2

#define holy_BTRFS_OBJECT_ID_CHUNK 0x100

static holy_disk_addr_t superblock_sectors[] = { 64 * 2, 64 * 1024 * 2,
  256 * 1048576 * 2, 1048576ULL * 1048576ULL * 2
};

static holy_err_t
holy_btrfs_read_logical (struct holy_btrfs_data *data,
			 holy_disk_addr_t addr, void *buf, holy_size_t size,
			 int recursion_depth);

static holy_err_t
read_sblock (holy_disk_t disk, struct holy_btrfs_superblock *sb)
{
  struct holy_btrfs_superblock sblock;
  unsigned i;
  holy_err_t err = holy_ERR_NONE;
  for (i = 0; i < ARRAY_SIZE (superblock_sectors); i++)
    {
      /* Don't try additional superblocks beyond device size.  */
      if (i && (holy_le_to_cpu64 (sblock.this_device.size)
		>> holy_DISK_SECTOR_BITS) <= superblock_sectors[i])
	break;
      err = holy_disk_read (disk, superblock_sectors[i], 0,
			    sizeof (sblock), &sblock);
      if (err == holy_ERR_OUT_OF_RANGE)
	break;

      if (holy_memcmp ((char *) sblock.signature, holy_BTRFS_SIGNATURE,
		       sizeof (holy_BTRFS_SIGNATURE) - 1) != 0)
	break;
      if (i == 0 || holy_le_to_cpu64 (sblock.generation)
	  > holy_le_to_cpu64 (sb->generation))
	holy_memcpy (sb, &sblock, sizeof (sblock));
    }

  if ((err == holy_ERR_OUT_OF_RANGE || !err) && i == 0)
    return holy_error (holy_ERR_BAD_FS, "not a Btrfs filesystem");

  if (err == holy_ERR_OUT_OF_RANGE)
    holy_errno = err = holy_ERR_NONE;

  return err;
}

static int
key_cmp (const struct holy_btrfs_key *a, const struct holy_btrfs_key *b)
{
  if (holy_le_to_cpu64 (a->object_id) < holy_le_to_cpu64 (b->object_id))
    return -1;
  if (holy_le_to_cpu64 (a->object_id) > holy_le_to_cpu64 (b->object_id))
    return +1;

  if (a->type < b->type)
    return -1;
  if (a->type > b->type)
    return +1;

  if (holy_le_to_cpu64 (a->offset) < holy_le_to_cpu64 (b->offset))
    return -1;
  if (holy_le_to_cpu64 (a->offset) > holy_le_to_cpu64 (b->offset))
    return +1;
  return 0;
}

static void
free_iterator (struct holy_btrfs_leaf_descriptor *desc)
{
  holy_free (desc->data);
}

static holy_err_t
save_ref (struct holy_btrfs_leaf_descriptor *desc,
	  holy_disk_addr_t addr, unsigned i, unsigned m, int l)
{
  desc->depth++;
  if (desc->allocated < desc->depth)
    {
      void *newdata;
      desc->allocated *= 2;
      newdata = holy_realloc (desc->data, sizeof (desc->data[0])
			      * desc->allocated);
      if (!newdata)
	return holy_errno;
      desc->data = newdata;
    }
  desc->data[desc->depth - 1].addr = addr;
  desc->data[desc->depth - 1].iter = i;
  desc->data[desc->depth - 1].maxiter = m;
  desc->data[desc->depth - 1].leaf = l;
  return holy_ERR_NONE;
}

static int
next (struct holy_btrfs_data *data,
      struct holy_btrfs_leaf_descriptor *desc,
      holy_disk_addr_t * outaddr, holy_size_t * outsize,
      struct holy_btrfs_key *key_out)
{
  holy_err_t err;
  struct holy_btrfs_leaf_node leaf;

  for (; desc->depth > 0; desc->depth--)
    {
      desc->data[desc->depth - 1].iter++;
      if (desc->data[desc->depth - 1].iter
	  < desc->data[desc->depth - 1].maxiter)
	break;
    }
  if (desc->depth == 0)
    return 0;
  while (!desc->data[desc->depth - 1].leaf)
    {
      struct holy_btrfs_internal_node node;
      struct btrfs_header head;

      err = holy_btrfs_read_logical (data, desc->data[desc->depth - 1].iter
				     * sizeof (node)
				     + sizeof (struct btrfs_header)
				     + desc->data[desc->depth - 1].addr,
				     &node, sizeof (node), 0);
      if (err)
	return -err;

      err = holy_btrfs_read_logical (data, holy_le_to_cpu64 (node.addr),
				     &head, sizeof (head), 0);
      if (err)
	return -err;

      save_ref (desc, holy_le_to_cpu64 (node.addr), 0,
		holy_le_to_cpu32 (head.nitems), !head.level);
    }
  err = holy_btrfs_read_logical (data, desc->data[desc->depth - 1].iter
				 * sizeof (leaf)
				 + sizeof (struct btrfs_header)
				 + desc->data[desc->depth - 1].addr, &leaf,
				 sizeof (leaf), 0);
  if (err)
    return -err;
  *outsize = holy_le_to_cpu32 (leaf.size);
  *outaddr = desc->data[desc->depth - 1].addr + sizeof (struct btrfs_header)
    + holy_le_to_cpu32 (leaf.offset);
  *key_out = leaf.key;
  return 1;
}

static holy_err_t
lower_bound (struct holy_btrfs_data *data,
	     const struct holy_btrfs_key *key_in,
	     struct holy_btrfs_key *key_out,
	     holy_uint64_t root,
	     holy_disk_addr_t *outaddr, holy_size_t *outsize,
	     struct holy_btrfs_leaf_descriptor *desc,
	     int recursion_depth)
{
  holy_disk_addr_t addr = holy_le_to_cpu64 (root);
  int depth = -1;

  if (desc)
    {
      desc->allocated = 16;
      desc->depth = 0;
      desc->data = holy_malloc (sizeof (desc->data[0]) * desc->allocated);
      if (!desc->data)
	return holy_errno;
    }

  /* > 2 would work as well but be robust and allow a bit more just in case.
   */
  if (recursion_depth > 10)
    return holy_error (holy_ERR_BAD_FS, "too deep btrfs virtual nesting");

  holy_dprintf ("btrfs",
		"retrieving %" PRIxholy_UINT64_T
		" %x %" PRIxholy_UINT64_T "\n",
		key_in->object_id, key_in->type, key_in->offset);

  while (1)
    {
      holy_err_t err;
      struct btrfs_header head;

    reiter:
      depth++;
      /* FIXME: preread few nodes into buffer. */
      err = holy_btrfs_read_logical (data, addr, &head, sizeof (head),
				     recursion_depth + 1);
      if (err)
	return err;
      addr += sizeof (head);
      if (head.level)
	{
	  unsigned i;
	  struct holy_btrfs_internal_node node, node_last;
	  int have_last = 0;
	  holy_memset (&node_last, 0, sizeof (node_last));
	  for (i = 0; i < holy_le_to_cpu32 (head.nitems); i++)
	    {
	      err = holy_btrfs_read_logical (data, addr + i * sizeof (node),
					     &node, sizeof (node),
					     recursion_depth + 1);
	      if (err)
		return err;

	      holy_dprintf ("btrfs",
			    "internal node (depth %d) %" PRIxholy_UINT64_T
			    " %x %" PRIxholy_UINT64_T "\n", depth,
			    node.key.object_id, node.key.type,
			    node.key.offset);

	      if (key_cmp (&node.key, key_in) == 0)
		{
		  err = holy_ERR_NONE;
		  if (desc)
		    err = save_ref (desc, addr - sizeof (head), i,
				    holy_le_to_cpu32 (head.nitems), 0);
		  if (err)
		    return err;
		  addr = holy_le_to_cpu64 (node.addr);
		  goto reiter;
		}
	      if (key_cmp (&node.key, key_in) > 0)
		break;
	      node_last = node;
	      have_last = 1;
	    }
	  if (have_last)
	    {
	      err = holy_ERR_NONE;
	      if (desc)
		err = save_ref (desc, addr - sizeof (head), i - 1,
				holy_le_to_cpu32 (head.nitems), 0);
	      if (err)
		return err;
	      addr = holy_le_to_cpu64 (node_last.addr);
	      goto reiter;
	    }
	  *outsize = 0;
	  *outaddr = 0;
	  holy_memset (key_out, 0, sizeof (*key_out));
	  if (desc)
	    return save_ref (desc, addr - sizeof (head), -1,
			     holy_le_to_cpu32 (head.nitems), 0);
	  return holy_ERR_NONE;
	}
      {
	unsigned i;
	struct holy_btrfs_leaf_node leaf, leaf_last;
	int have_last = 0;
	for (i = 0; i < holy_le_to_cpu32 (head.nitems); i++)
	  {
	    err = holy_btrfs_read_logical (data, addr + i * sizeof (leaf),
					   &leaf, sizeof (leaf),
					   recursion_depth + 1);
	    if (err)
	      return err;

	    holy_dprintf ("btrfs",
			  "leaf (depth %d) %" PRIxholy_UINT64_T
			  " %x %" PRIxholy_UINT64_T "\n", depth,
			  leaf.key.object_id, leaf.key.type, leaf.key.offset);

	    if (key_cmp (&leaf.key, key_in) == 0)
	      {
		holy_memcpy (key_out, &leaf.key, sizeof (*key_out));
		*outsize = holy_le_to_cpu32 (leaf.size);
		*outaddr = addr + holy_le_to_cpu32 (leaf.offset);
		if (desc)
		  return save_ref (desc, addr - sizeof (head), i,
				   holy_le_to_cpu32 (head.nitems), 1);
		return holy_ERR_NONE;
	      }

	    if (key_cmp (&leaf.key, key_in) > 0)
	      break;

	    have_last = 1;
	    leaf_last = leaf;
	  }

	if (have_last)
	  {
	    holy_memcpy (key_out, &leaf_last.key, sizeof (*key_out));
	    *outsize = holy_le_to_cpu32 (leaf_last.size);
	    *outaddr = addr + holy_le_to_cpu32 (leaf_last.offset);
	    if (desc)
	      return save_ref (desc, addr - sizeof (head), i - 1,
			       holy_le_to_cpu32 (head.nitems), 1);
	    return holy_ERR_NONE;
	  }
	*outsize = 0;
	*outaddr = 0;
	holy_memset (key_out, 0, sizeof (*key_out));
	if (desc)
	  return save_ref (desc, addr - sizeof (head), -1,
			   holy_le_to_cpu32 (head.nitems), 1);
	return holy_ERR_NONE;
      }
    }
}

/* Context for find_device.  */
struct find_device_ctx
{
  struct holy_btrfs_data *data;
  holy_uint64_t id;
  holy_device_t dev_found;
};

/* Helper for find_device.  */
static int
find_device_iter (const char *name, void *data)
{
  struct find_device_ctx *ctx = data;
  holy_device_t dev;
  holy_err_t err;
  struct holy_btrfs_superblock sb;

  dev = holy_device_open (name);
  if (!dev)
    return 0;
  if (!dev->disk)
    {
      holy_device_close (dev);
      return 0;
    }
  err = read_sblock (dev->disk, &sb);
  if (err == holy_ERR_BAD_FS)
    {
      holy_device_close (dev);
      holy_errno = holy_ERR_NONE;
      return 0;
    }
  if (err)
    {
      holy_device_close (dev);
      holy_print_error ();
      return 0;
    }
  if (holy_memcmp (ctx->data->sblock.uuid, sb.uuid, sizeof (sb.uuid)) != 0
      || sb.this_device.device_id != ctx->id)
    {
      holy_device_close (dev);
      return 0;
    }

  ctx->dev_found = dev;
  return 1;
}

static holy_device_t
find_device (struct holy_btrfs_data *data, holy_uint64_t id, int do_rescan)
{
  struct find_device_ctx ctx = {
    .data = data,
    .id = id,
    .dev_found = NULL
  };
  unsigned i;

  for (i = 0; i < data->n_devices_attached; i++)
    if (id == data->devices_attached[i].id)
      return data->devices_attached[i].dev;
  if (do_rescan)
    holy_device_iterate (find_device_iter, &ctx);
  if (!ctx.dev_found)
    {
      holy_error (holy_ERR_BAD_FS,
		  N_("couldn't find a necessary member device "
		     "of multi-device filesystem"));
      return NULL;
    }
  data->n_devices_attached++;
  if (data->n_devices_attached > data->n_devices_allocated)
    {
      void *tmp;
      data->n_devices_allocated = 2 * data->n_devices_attached + 1;
      data->devices_attached
	= holy_realloc (tmp = data->devices_attached,
			data->n_devices_allocated
			* sizeof (data->devices_attached[0]));
      if (!data->devices_attached)
	{
	  holy_device_close (ctx.dev_found);
	  data->devices_attached = tmp;
	  return NULL;
	}
    }
  data->devices_attached[data->n_devices_attached - 1].id = id;
  data->devices_attached[data->n_devices_attached - 1].dev = ctx.dev_found;
  return ctx.dev_found;
}

static holy_err_t
holy_btrfs_read_logical (struct holy_btrfs_data *data, holy_disk_addr_t addr,
			 void *buf, holy_size_t size, int recursion_depth)
{
  while (size > 0)
    {
      holy_uint8_t *ptr;
      struct holy_btrfs_key *key;
      struct holy_btrfs_chunk_item *chunk;
      holy_uint64_t csize;
      holy_err_t err = 0;
      struct holy_btrfs_key key_out;
      int challoc = 0;
      holy_device_t dev;
      struct holy_btrfs_key key_in;
      holy_size_t chsize;
      holy_disk_addr_t chaddr;

      holy_dprintf ("btrfs", "searching for laddr %" PRIxholy_UINT64_T "\n",
		    addr);
      for (ptr = data->sblock.bootstrap_mapping;
	   ptr < data->sblock.bootstrap_mapping
	   + sizeof (data->sblock.bootstrap_mapping)
	   - sizeof (struct holy_btrfs_key);)
	{
	  key = (struct holy_btrfs_key *) ptr;
	  if (key->type != holy_BTRFS_ITEM_TYPE_CHUNK)
	    break;
	  chunk = (struct holy_btrfs_chunk_item *) (key + 1);
	  holy_dprintf ("btrfs",
			"%" PRIxholy_UINT64_T " %" PRIxholy_UINT64_T " \n",
			holy_le_to_cpu64 (key->offset),
			holy_le_to_cpu64 (chunk->size));
	  if (holy_le_to_cpu64 (key->offset) <= addr
	      && addr < holy_le_to_cpu64 (key->offset)
	      + holy_le_to_cpu64 (chunk->size))
	    goto chunk_found;
	  ptr += sizeof (*key) + sizeof (*chunk)
	    + sizeof (struct holy_btrfs_chunk_stripe)
	    * holy_le_to_cpu16 (chunk->nstripes);
	}

      key_in.object_id = holy_cpu_to_le64_compile_time (holy_BTRFS_OBJECT_ID_CHUNK);
      key_in.type = holy_BTRFS_ITEM_TYPE_CHUNK;
      key_in.offset = holy_cpu_to_le64 (addr);
      err = lower_bound (data, &key_in, &key_out,
			 data->sblock.chunk_tree,
			 &chaddr, &chsize, NULL, recursion_depth);
      if (err)
	return err;
      key = &key_out;
      if (key->type != holy_BTRFS_ITEM_TYPE_CHUNK
	  || !(holy_le_to_cpu64 (key->offset) <= addr))
	return holy_error (holy_ERR_BAD_FS,
			   "couldn't find the chunk descriptor");

      chunk = holy_malloc (chsize);
      if (!chunk)
	return holy_errno;

      challoc = 1;
      err = holy_btrfs_read_logical (data, chaddr, chunk, chsize,
				     recursion_depth);
      if (err)
	{
	  holy_free (chunk);
	  return err;
	}

    chunk_found:
      {
	holy_uint64_t stripen;
	holy_uint64_t stripe_offset;
	holy_uint64_t off = addr - holy_le_to_cpu64 (key->offset);
	holy_uint64_t chunk_stripe_length;
	holy_uint16_t nstripes;
	unsigned redundancy = 1;
	unsigned i, j;

	if (holy_le_to_cpu64 (chunk->size) <= off)
	  {
	    holy_dprintf ("btrfs", "no chunk\n");
	    return holy_error (holy_ERR_BAD_FS,
			       "couldn't find the chunk descriptor");
	  }

	nstripes = holy_le_to_cpu16 (chunk->nstripes) ? : 1;
	chunk_stripe_length = holy_le_to_cpu64 (chunk->stripe_length) ? : 512;
	holy_dprintf ("btrfs", "chunk 0x%" PRIxholy_UINT64_T
		      "+0x%" PRIxholy_UINT64_T
		      " (%d stripes (%d substripes) of %"
		      PRIxholy_UINT64_T ")\n",
		      holy_le_to_cpu64 (key->offset),
		      holy_le_to_cpu64 (chunk->size),
		      nstripes,
		      holy_le_to_cpu16 (chunk->nsubstripes),
		      chunk_stripe_length);

	switch (holy_le_to_cpu64 (chunk->type)
		& ~holy_BTRFS_CHUNK_TYPE_BITS_DONTCARE)
	  {
	  case holy_BTRFS_CHUNK_TYPE_SINGLE:
	    {
	      holy_uint64_t stripe_length;
	      holy_dprintf ("btrfs", "single\n");
	      stripe_length = holy_divmod64 (holy_le_to_cpu64 (chunk->size),
					     nstripes,
					     NULL);
	      if (stripe_length == 0)
		stripe_length = 512;
	      stripen = holy_divmod64 (off, stripe_length, &stripe_offset);
	      csize = (stripen + 1) * stripe_length - off;
	      break;
	    }
	  case holy_BTRFS_CHUNK_TYPE_DUPLICATED:
	  case holy_BTRFS_CHUNK_TYPE_RAID1:
	    {
	      holy_dprintf ("btrfs", "RAID1\n");
	      stripen = 0;
	      stripe_offset = off;
	      csize = holy_le_to_cpu64 (chunk->size) - off;
	      redundancy = 2;
	      break;
	    }
	  case holy_BTRFS_CHUNK_TYPE_RAID0:
	    {
	      holy_uint64_t middle, high;
	      holy_uint64_t low;
	      holy_dprintf ("btrfs", "RAID0\n");
	      middle = holy_divmod64 (off,
				      chunk_stripe_length,
				      &low);

	      high = holy_divmod64 (middle, nstripes,
				    &stripen);
	      stripe_offset =
		low + chunk_stripe_length * high;
	      csize = chunk_stripe_length - low;
	      break;
	    }
	  case holy_BTRFS_CHUNK_TYPE_RAID10:
	    {
	      holy_uint64_t middle, high;
	      holy_uint64_t low;
	      holy_uint16_t nsubstripes;
	      nsubstripes = holy_le_to_cpu16 (chunk->nsubstripes) ? : 1;
	      middle = holy_divmod64 (off,
				      chunk_stripe_length,
				      &low);

	      high = holy_divmod64 (middle,
				    nstripes / nsubstripes ? : 1,
				    &stripen);
	      stripen *= nsubstripes;
	      redundancy = nsubstripes;
	      stripe_offset = low + chunk_stripe_length
		* high;
	      csize = chunk_stripe_length - low;
	      break;
	    }
	  default:
	    holy_dprintf ("btrfs", "unsupported RAID\n");
	    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
			       "unsupported RAID flags %" PRIxholy_UINT64_T,
			       holy_le_to_cpu64 (chunk->type));
	  }
	if (csize == 0)
	  return holy_error (holy_ERR_BUG,
			     "couldn't find the chunk descriptor");
	if (csize > (holy_uint64_t) size)
	  csize = size;

	for (j = 0; j < 2; j++)
	  {
	    for (i = 0; i < redundancy; i++)
	      {
		struct holy_btrfs_chunk_stripe *stripe;
		holy_disk_addr_t paddr;

		stripe = (struct holy_btrfs_chunk_stripe *) (chunk + 1);
		/* Right now the redundancy handling is easy.
		   With RAID5-like it will be more difficult.  */
		stripe += stripen + i;

		paddr = holy_le_to_cpu64 (stripe->offset) + stripe_offset;

		holy_dprintf ("btrfs", "chunk 0x%" PRIxholy_UINT64_T
			      "+0x%" PRIxholy_UINT64_T
			      " (%d stripes (%d substripes) of %"
			      PRIxholy_UINT64_T ") stripe %" PRIxholy_UINT64_T
			      " maps to 0x%" PRIxholy_UINT64_T "\n",
			      holy_le_to_cpu64 (key->offset),
			      holy_le_to_cpu64 (chunk->size),
			      holy_le_to_cpu16 (chunk->nstripes),
			      holy_le_to_cpu16 (chunk->nsubstripes),
			      holy_le_to_cpu64 (chunk->stripe_length),
			      stripen, stripe->offset);
		holy_dprintf ("btrfs", "reading paddr 0x%" PRIxholy_UINT64_T
			      " for laddr 0x%" PRIxholy_UINT64_T "\n", paddr,
			      addr);

		dev = find_device (data, stripe->device_id, j);
		if (!dev)
		  {
		    err = holy_errno;
		    holy_errno = holy_ERR_NONE;
		    continue;
		  }

		err = holy_disk_read (dev->disk, paddr >> holy_DISK_SECTOR_BITS,
				      paddr & (holy_DISK_SECTOR_SIZE - 1),
				      csize, buf);
		if (!err)
		  break;
		holy_errno = holy_ERR_NONE;
	      }
	    if (i != redundancy)
	      break;
	  }
	if (err)
	  return holy_errno = err;
      }
      size -= csize;
      buf = (holy_uint8_t *) buf + csize;
      addr += csize;
      if (challoc)
	holy_free (chunk);
    }
  return holy_ERR_NONE;
}

static struct holy_btrfs_data *
holy_btrfs_mount (holy_device_t dev)
{
  struct holy_btrfs_data *data;
  holy_err_t err;

  if (!dev->disk)
    {
      holy_error (holy_ERR_BAD_FS, "not BtrFS");
      return NULL;
    }

  data = holy_zalloc (sizeof (*data));
  if (!data)
    return NULL;

  err = read_sblock (dev->disk, &data->sblock);
  if (err)
    {
      holy_free (data);
      return NULL;
    }

  data->n_devices_allocated = 16;
  data->devices_attached = holy_malloc (sizeof (data->devices_attached[0])
					* data->n_devices_allocated);
  if (!data->devices_attached)
    {
      holy_free (data);
      return NULL;
    }
  data->n_devices_attached = 1;
  data->devices_attached[0].dev = dev;
  data->devices_attached[0].id = data->sblock.this_device.device_id;

  return data;
}

static void
holy_btrfs_unmount (struct holy_btrfs_data *data)
{
  unsigned i;
  /* The device 0 is closed one layer upper.  */
  for (i = 1; i < data->n_devices_attached; i++)
    holy_device_close (data->devices_attached[i].dev);
  holy_free (data->devices_attached);
  holy_free (data->extent);
  holy_free (data);
}

static holy_err_t
holy_btrfs_read_inode (struct holy_btrfs_data *data,
		       struct holy_btrfs_inode *inode, holy_uint64_t num,
		       holy_uint64_t tree)
{
  struct holy_btrfs_key key_in, key_out;
  holy_disk_addr_t elemaddr;
  holy_size_t elemsize;
  holy_err_t err;

  key_in.object_id = num;
  key_in.type = holy_BTRFS_ITEM_TYPE_INODE_ITEM;
  key_in.offset = 0;

  err = lower_bound (data, &key_in, &key_out, tree, &elemaddr, &elemsize, NULL,
		     0);
  if (err)
    return err;
  if (num != key_out.object_id
      || key_out.type != holy_BTRFS_ITEM_TYPE_INODE_ITEM)
    return holy_error (holy_ERR_BAD_FS, "inode not found");

  return holy_btrfs_read_logical (data, elemaddr, inode, sizeof (*inode), 0);
}

static holy_ssize_t
holy_btrfs_lzo_decompress(char *ibuf, holy_size_t isize, holy_off_t off,
			  char *obuf, holy_size_t osize)
{
  holy_uint32_t total_size, cblock_size;
  holy_size_t ret = 0;
  char *ibuf0 = ibuf;

  total_size = holy_le_to_cpu32 (holy_get_unaligned32 (ibuf));
  ibuf += sizeof (total_size);

  if (isize < total_size)
    return -1;

  /* Jump forward to first block with requested data.  */
  while (off >= holy_BTRFS_LZO_BLOCK_SIZE)
    {
      /* Don't let following uint32_t cross the page boundary.  */
      if (((ibuf - ibuf0) & 0xffc) == 0xffc)
	ibuf = ((ibuf - ibuf0 + 3) & ~3) + ibuf0;

      cblock_size = holy_le_to_cpu32 (holy_get_unaligned32 (ibuf));
      ibuf += sizeof (cblock_size);

      if (cblock_size > holy_BTRFS_LZO_BLOCK_MAX_CSIZE)
	return -1;

      off -= holy_BTRFS_LZO_BLOCK_SIZE;
      ibuf += cblock_size;
    }

  while (osize > 0)
    {
      lzo_uint usize = holy_BTRFS_LZO_BLOCK_SIZE;

      /* Don't let following uint32_t cross the page boundary.  */
      if (((ibuf - ibuf0) & 0xffc) == 0xffc)
	ibuf = ((ibuf - ibuf0 + 3) & ~3) + ibuf0;

      cblock_size = holy_le_to_cpu32 (holy_get_unaligned32 (ibuf));
      ibuf += sizeof (cblock_size);

      if (cblock_size > holy_BTRFS_LZO_BLOCK_MAX_CSIZE)
	return -1;

      /* Block partially filled with requested data.  */
      if (off > 0 || osize < holy_BTRFS_LZO_BLOCK_SIZE)
	{
	  holy_size_t to_copy = holy_BTRFS_LZO_BLOCK_SIZE - off;
	  holy_uint8_t *buf;

	  if (to_copy > osize)
	    to_copy = osize;

	  buf = holy_malloc (holy_BTRFS_LZO_BLOCK_SIZE);
	  if (!buf)
	    return -1;

	  if (lzo1x_decompress_safe ((lzo_bytep)ibuf, cblock_size, buf, &usize,
	      NULL) != LZO_E_OK)
	    {
	      holy_free (buf);
	      return -1;
	    }

	  if (to_copy > usize)
	    to_copy = usize;
	  holy_memcpy(obuf, buf + off, to_copy);

	  osize -= to_copy;
	  ret += to_copy;
	  obuf += to_copy;
	  ibuf += cblock_size;
	  off = 0;

	  holy_free (buf);
	  continue;
	}

      /* Decompress whole block directly to output buffer.  */
      if (lzo1x_decompress_safe ((lzo_bytep)ibuf, cblock_size, (lzo_bytep)obuf,
	  &usize, NULL) != LZO_E_OK)
	return -1;

      osize -= usize;
      ret += usize;
      obuf += usize;
      ibuf += cblock_size;
    }

  return ret;
}

static holy_ssize_t
holy_btrfs_extent_read (struct holy_btrfs_data *data,
			holy_uint64_t ino, holy_uint64_t tree,
			holy_off_t pos0, char *buf, holy_size_t len)
{
  holy_off_t pos = pos0;
  while (len)
    {
      holy_size_t csize;
      holy_err_t err;
      holy_off_t extoff;
      if (!data->extent || data->extstart > pos || data->extino != ino
	  || data->exttree != tree || data->extend <= pos)
	{
	  struct holy_btrfs_key key_in, key_out;
	  holy_disk_addr_t elemaddr;
	  holy_size_t elemsize;

	  holy_free (data->extent);
	  key_in.object_id = ino;
	  key_in.type = holy_BTRFS_ITEM_TYPE_EXTENT_ITEM;
	  key_in.offset = holy_cpu_to_le64 (pos);
	  err = lower_bound (data, &key_in, &key_out, tree,
			     &elemaddr, &elemsize, NULL, 0);
	  if (err)
	    return -1;
	  if (key_out.object_id != ino
	      || key_out.type != holy_BTRFS_ITEM_TYPE_EXTENT_ITEM)
	    {
	      holy_error (holy_ERR_BAD_FS, "extent not found");
	      return -1;
	    }
	  if ((holy_ssize_t) elemsize < ((char *) &data->extent->inl
					 - (char *) data->extent))
	    {
	      holy_error (holy_ERR_BAD_FS, "extent descriptor is too short");
	      return -1;
	    }
	  data->extstart = holy_le_to_cpu64 (key_out.offset);
	  data->extsize = elemsize;
	  data->extent = holy_malloc (elemsize);
	  data->extino = ino;
	  data->exttree = tree;
	  if (!data->extent)
	    return holy_errno;

	  err = holy_btrfs_read_logical (data, elemaddr, data->extent,
					 elemsize, 0);
	  if (err)
	    return err;

	  data->extend = data->extstart + holy_le_to_cpu64 (data->extent->size);
	  if (data->extent->type == holy_BTRFS_EXTENT_REGULAR
	      && (char *) data->extent + elemsize
	      >= (char *) &data->extent->filled + sizeof (data->extent->filled))
	    data->extend =
	      data->extstart + holy_le_to_cpu64 (data->extent->filled);

	  holy_dprintf ("btrfs", "regular extent 0x%" PRIxholy_UINT64_T "+0x%"
			PRIxholy_UINT64_T "\n",
			holy_le_to_cpu64 (key_out.offset),
			holy_le_to_cpu64 (data->extent->size));
	  if (data->extend <= pos)
	    {
	      holy_error (holy_ERR_BAD_FS, "extent not found");
	      return -1;
	    }
	}
      csize = data->extend - pos;
      extoff = pos - data->extstart;
      if (csize > len)
	csize = len;

      if (data->extent->encryption)
	{
	  holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		      "encryption not supported");
	  return -1;
	}

      if (data->extent->compression != holy_BTRFS_COMPRESSION_NONE
	  && data->extent->compression != holy_BTRFS_COMPRESSION_ZLIB
	  && data->extent->compression != holy_BTRFS_COMPRESSION_LZO)
	{
	  holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		      "compression type 0x%x not supported",
		      data->extent->compression);
	  return -1;
	}

      if (data->extent->encoding)
	{
	  holy_error (holy_ERR_NOT_IMPLEMENTED_YET, "encoding not supported");
	  return -1;
	}

      switch (data->extent->type)
	{
	case holy_BTRFS_EXTENT_INLINE:
	  if (data->extent->compression == holy_BTRFS_COMPRESSION_ZLIB)
	    {
	      if (holy_zlib_decompress (data->extent->inl, data->extsize -
					((holy_uint8_t *) data->extent->inl
					 - (holy_uint8_t *) data->extent),
					extoff, buf, csize)
		  != (holy_ssize_t) csize)
		{
		  if (!holy_errno)
		    holy_error (holy_ERR_BAD_COMPRESSED_DATA,
				"premature end of compressed");
		  return -1;
		}
	    }
	  else if (data->extent->compression == holy_BTRFS_COMPRESSION_LZO)
	    {
	      if (holy_btrfs_lzo_decompress(data->extent->inl, data->extsize -
					   ((holy_uint8_t *) data->extent->inl
					    - (holy_uint8_t *) data->extent),
					   extoff, buf, csize)
		  != (holy_ssize_t) csize)
		return -1;
	    }
	  else
	    holy_memcpy (buf, data->extent->inl + extoff, csize);
	  break;
	case holy_BTRFS_EXTENT_REGULAR:
	  if (!data->extent->laddr)
	    {
	      holy_memset (buf, 0, csize);
	      break;
	    }

	  if (data->extent->compression != holy_BTRFS_COMPRESSION_NONE)
	    {
	      char *tmp;
	      holy_uint64_t zsize;
	      holy_ssize_t ret;

	      zsize = holy_le_to_cpu64 (data->extent->compressed_size);
	      tmp = holy_malloc (zsize);
	      if (!tmp)
		return -1;
	      err = holy_btrfs_read_logical (data,
					     holy_le_to_cpu64 (data->extent->laddr),
					     tmp, zsize, 0);
	      if (err)
		{
		  holy_free (tmp);
		  return -1;
		}

	      if (data->extent->compression == holy_BTRFS_COMPRESSION_ZLIB)
		ret = holy_zlib_decompress (tmp, zsize, extoff
				    + holy_le_to_cpu64 (data->extent->offset),
				    buf, csize);
	      else if (data->extent->compression == holy_BTRFS_COMPRESSION_LZO)
		ret = holy_btrfs_lzo_decompress (tmp, zsize, extoff
				    + holy_le_to_cpu64 (data->extent->offset),
				    buf, csize);
	      else
		ret = -1;

	      holy_free (tmp);

	      if (ret != (holy_ssize_t) csize)
		{
		  if (!holy_errno)
		    holy_error (holy_ERR_BAD_COMPRESSED_DATA,
				"premature end of compressed");
		  return -1;
		}

	      break;
	    }
	  err = holy_btrfs_read_logical (data,
					 holy_le_to_cpu64 (data->extent->laddr)
					 + holy_le_to_cpu64 (data->extent->offset)
					 + extoff, buf, csize, 0);
	  if (err)
	    return -1;
	  break;
	default:
	  holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		      "unsupported extent type 0x%x", data->extent->type);
	  return -1;
	}
      buf += csize;
      pos += csize;
      len -= csize;
    }
  return pos - pos0;
}

static holy_err_t
get_root (struct holy_btrfs_data *data, struct holy_btrfs_key *key,
	  holy_uint64_t *tree, holy_uint8_t *type)
{
  holy_err_t err;
  holy_disk_addr_t elemaddr;
  holy_size_t elemsize;
  struct holy_btrfs_key key_out, key_in;
  struct holy_btrfs_root_item ri;

  key_in.object_id = holy_cpu_to_le64_compile_time (holy_BTRFS_ROOT_VOL_OBJECTID);
  key_in.offset = 0;
  key_in.type = holy_BTRFS_ITEM_TYPE_ROOT_ITEM;
  err = lower_bound (data, &key_in, &key_out,
		     data->sblock.root_tree,
		     &elemaddr, &elemsize, NULL, 0);
  if (err)
    return err;
  if (key_in.object_id != key_out.object_id
      || key_in.type != key_out.type
      || key_in.offset != key_out.offset)
    return holy_error (holy_ERR_BAD_FS, "no root");
  err = holy_btrfs_read_logical (data, elemaddr, &ri,
				 sizeof (ri), 0);
  if (err)
    return err;
  key->type = holy_BTRFS_ITEM_TYPE_DIR_ITEM;
  key->offset = 0;
  key->object_id = holy_cpu_to_le64_compile_time (holy_BTRFS_OBJECT_ID_CHUNK);
  *tree = ri.tree;
  *type = holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY;
  return holy_ERR_NONE;
}

static holy_err_t
find_path (struct holy_btrfs_data *data,
	   const char *path, struct holy_btrfs_key *key,
	   holy_uint64_t *tree, holy_uint8_t *type)
{
  const char *slash = path;
  holy_err_t err;
  holy_disk_addr_t elemaddr;
  holy_size_t elemsize;
  holy_size_t allocated = 0;
  struct holy_btrfs_dir_item *direl = NULL;
  struct holy_btrfs_key key_out;
  const char *ctoken;
  holy_size_t ctokenlen;
  char *path_alloc = NULL;
  char *origpath = NULL;
  unsigned symlinks_max = 32;

  err = get_root (data, key, tree, type);
  if (err)
    return err;

  origpath = holy_strdup (path);
  if (!origpath)
    return holy_errno;

  while (1)
    {
      while (path[0] == '/')
	path++;
      if (!path[0])
	break;
      slash = holy_strchr (path, '/');
      if (!slash)
	slash = path + holy_strlen (path);
      ctoken = path;
      ctokenlen = slash - path;

      if (*type != holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
	{
	  holy_free (path_alloc);
	  holy_free (origpath);
	  return holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
	}

      if (ctokenlen == 1 && ctoken[0] == '.')
	{
	  path = slash;
	  continue;
	}
      if (ctokenlen == 2 && ctoken[0] == '.' && ctoken[1] == '.')
	{
	  key->type = holy_BTRFS_ITEM_TYPE_INODE_REF;
	  key->offset = -1;

	  err = lower_bound (data, key, &key_out, *tree, &elemaddr, &elemsize,
			     NULL, 0);
	  if (err)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      holy_free (origpath);
	      return err;
	    }

	  if (key_out.type != key->type
	      || key->object_id != key_out.object_id)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      err = holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);
	      holy_free (origpath);
	      return err;
	    }

	  *type = holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY;
	  key->object_id = key_out.offset;

	  path = slash;

	  continue;
	}

      key->type = holy_BTRFS_ITEM_TYPE_DIR_ITEM;
      key->offset = holy_cpu_to_le64 (~holy_getcrc32c (1, ctoken, ctokenlen));

      err = lower_bound (data, key, &key_out, *tree, &elemaddr, &elemsize,
			 NULL, 0);
      if (err)
	{
	  holy_free (direl);
	  holy_free (path_alloc);
	  holy_free (origpath);
	  return err;
	}
      if (key_cmp (key, &key_out) != 0)
	{
	  holy_free (direl);
	  holy_free (path_alloc);
	  err = holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);
	  holy_free (origpath);
	  return err;
	}

      struct holy_btrfs_dir_item *cdirel;
      if (elemsize > allocated)
	{
	  allocated = 2 * elemsize;
	  holy_free (direl);
	  direl = holy_malloc (allocated + 1);
	  if (!direl)
	    {
	      holy_free (path_alloc);
	      holy_free (origpath);
	      return holy_errno;
	    }
	}

      err = holy_btrfs_read_logical (data, elemaddr, direl, elemsize, 0);
      if (err)
	{
	  holy_free (direl);
	  holy_free (path_alloc);
	  holy_free (origpath);
	  return err;
	}

      for (cdirel = direl;
	   (holy_uint8_t *) cdirel - (holy_uint8_t *) direl
	   < (holy_ssize_t) elemsize;
	   cdirel = (void *) ((holy_uint8_t *) (direl + 1)
			      + holy_le_to_cpu16 (cdirel->n)
			      + holy_le_to_cpu16 (cdirel->m)))
	{
	  if (ctokenlen == holy_le_to_cpu16 (cdirel->n)
	      && holy_memcmp (cdirel->name, ctoken, ctokenlen) == 0)
	    break;
	}
      if ((holy_uint8_t *) cdirel - (holy_uint8_t *) direl
	  >= (holy_ssize_t) elemsize)
	{
	  holy_free (direl);
	  holy_free (path_alloc);
	  err = holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);
	  holy_free (origpath);
	  return err;
	}

      path = slash;
      if (cdirel->type == holy_BTRFS_DIR_ITEM_TYPE_SYMLINK)
	{
	  struct holy_btrfs_inode inode;
	  char *tmp;
	  if (--symlinks_max == 0)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      holy_free (origpath);
	      return holy_error (holy_ERR_SYMLINK_LOOP,
				 N_("too deep nesting of symlinks"));
	    }

	  err = holy_btrfs_read_inode (data, &inode,
				       cdirel->key.object_id, *tree);
	  if (err)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      holy_free (origpath);
	      return err;
	    }
	  tmp = holy_malloc (holy_le_to_cpu64 (inode.size)
			     + holy_strlen (path) + 1);
	  if (!tmp)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      holy_free (origpath);
	      return holy_errno;
	    }

	  if (holy_btrfs_extent_read (data, cdirel->key.object_id,
				      *tree, 0, tmp,
				      holy_le_to_cpu64 (inode.size))
	      != (holy_ssize_t) holy_le_to_cpu64 (inode.size))
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      holy_free (origpath);
	      holy_free (tmp);
	      return holy_errno;
	    }
	  holy_memcpy (tmp + holy_le_to_cpu64 (inode.size), path,
		       holy_strlen (path) + 1);
	  holy_free (path_alloc);
	  path = path_alloc = tmp;
	  if (path[0] == '/')
	    {
	      err = get_root (data, key, tree, type);
	      if (err)
		return err;
	    }
	  continue;
	}
      *type = cdirel->type;

      switch (cdirel->key.type)
	{
	case holy_BTRFS_ITEM_TYPE_ROOT_ITEM:
	  {
	    struct holy_btrfs_root_item ri;
	    err = lower_bound (data, &cdirel->key, &key_out,
			       data->sblock.root_tree,
			       &elemaddr, &elemsize, NULL, 0);
	    if (err)
	      {
		holy_free (direl);
		holy_free (path_alloc);
		holy_free (origpath);
		return err;
	      }
	    if (cdirel->key.object_id != key_out.object_id
		|| cdirel->key.type != key_out.type)
	      {
		holy_free (direl);
		holy_free (path_alloc);
		err = holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);
		holy_free (origpath);
		return err;
	      }
	    err = holy_btrfs_read_logical (data, elemaddr, &ri,
					   sizeof (ri), 0);
	    if (err)
	      {
		holy_free (direl);
		holy_free (path_alloc);
		holy_free (origpath);
		return err;
	      }
	    key->type = holy_BTRFS_ITEM_TYPE_DIR_ITEM;
	    key->offset = 0;
	    key->object_id = holy_cpu_to_le64_compile_time (holy_BTRFS_OBJECT_ID_CHUNK);
	    *tree = ri.tree;
	    break;
	  }
	case holy_BTRFS_ITEM_TYPE_INODE_ITEM:
	  if (*slash && *type == holy_BTRFS_DIR_ITEM_TYPE_REGULAR)
	    {
	      holy_free (direl);
	      holy_free (path_alloc);
	      err = holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"), origpath);
	      holy_free (origpath);
	      return err;
	    }
	  *key = cdirel->key;
	  if (*type == holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
	    key->type = holy_BTRFS_ITEM_TYPE_DIR_ITEM;
	  break;
	default:
	  holy_free (path_alloc);
	  holy_free (origpath);
	  holy_free (direl);
	  return holy_error (holy_ERR_BAD_FS, "unrecognised object type 0x%x",
			     cdirel->key.type);
	}
    }

  holy_free (direl);
  holy_free (origpath);
  holy_free (path_alloc);
  return holy_ERR_NONE;
}

static holy_err_t
holy_btrfs_dir (holy_device_t device, const char *path,
		holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_btrfs_data *data = holy_btrfs_mount (device);
  struct holy_btrfs_key key_in, key_out;
  holy_err_t err;
  holy_disk_addr_t elemaddr;
  holy_size_t elemsize;
  holy_size_t allocated = 0;
  struct holy_btrfs_dir_item *direl = NULL;
  struct holy_btrfs_leaf_descriptor desc;
  int r = 0;
  holy_uint64_t tree;
  holy_uint8_t type;

  if (!data)
    return holy_errno;

  err = find_path (data, path, &key_in, &tree, &type);
  if (err)
    {
      holy_btrfs_unmount (data);
      return err;
    }
  if (type != holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY)
    {
      holy_btrfs_unmount (data);
      return holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
    }

  err = lower_bound (data, &key_in, &key_out, tree,
		     &elemaddr, &elemsize, &desc, 0);
  if (err)
    {
      holy_btrfs_unmount (data);
      return err;
    }
  if (key_out.type != holy_BTRFS_ITEM_TYPE_DIR_ITEM
      || key_out.object_id != key_in.object_id)
    {
      r = next (data, &desc, &elemaddr, &elemsize, &key_out);
      if (r <= 0)
	goto out;
    }
  do
    {
      struct holy_btrfs_dir_item *cdirel;
      if (key_out.type != holy_BTRFS_ITEM_TYPE_DIR_ITEM
	  || key_out.object_id != key_in.object_id)
	{
	  r = 0;
	  break;
	}
      if (elemsize > allocated)
	{
	  allocated = 2 * elemsize;
	  holy_free (direl);
	  direl = holy_malloc (allocated + 1);
	  if (!direl)
	    {
	      r = -holy_errno;
	      break;
	    }
	}

      err = holy_btrfs_read_logical (data, elemaddr, direl, elemsize, 0);
      if (err)
	{
	  r = -err;
	  break;
	}

      for (cdirel = direl;
	   (holy_uint8_t *) cdirel - (holy_uint8_t *) direl
	   < (holy_ssize_t) elemsize;
	   cdirel = (void *) ((holy_uint8_t *) (direl + 1)
			      + holy_le_to_cpu16 (cdirel->n)
			      + holy_le_to_cpu16 (cdirel->m)))
	{
	  char c;
	  struct holy_btrfs_inode inode;
	  struct holy_dirhook_info info;
	  err = holy_btrfs_read_inode (data, &inode, cdirel->key.object_id,
				       tree);
	  holy_memset (&info, 0, sizeof (info));
	  if (err)
	    holy_errno = holy_ERR_NONE;
	  else
	    {
	      info.mtime = holy_le_to_cpu64 (inode.mtime.sec);
	      info.mtimeset = 1;
	    }
	  c = cdirel->name[holy_le_to_cpu16 (cdirel->n)];
	  cdirel->name[holy_le_to_cpu16 (cdirel->n)] = 0;
	  info.dir = (cdirel->type == holy_BTRFS_DIR_ITEM_TYPE_DIRECTORY);
	  if (hook (cdirel->name, &info, hook_data))
	    goto out;
	  cdirel->name[holy_le_to_cpu16 (cdirel->n)] = c;
	}
      r = next (data, &desc, &elemaddr, &elemsize, &key_out);
    }
  while (r > 0);

out:
  holy_free (direl);

  free_iterator (&desc);
  holy_btrfs_unmount (data);

  return -r;
}

static holy_err_t
holy_btrfs_open (struct holy_file *file, const char *name)
{
  struct holy_btrfs_data *data = holy_btrfs_mount (file->device);
  holy_err_t err;
  struct holy_btrfs_inode inode;
  holy_uint8_t type;
  struct holy_btrfs_key key_in;

  if (!data)
    return holy_errno;

  err = find_path (data, name, &key_in, &data->tree, &type);
  if (err)
    {
      holy_btrfs_unmount (data);
      return err;
    }
  if (type != holy_BTRFS_DIR_ITEM_TYPE_REGULAR)
    {
      holy_btrfs_unmount (data);
      return holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a regular file"));
    }

  data->inode = key_in.object_id;
  err = holy_btrfs_read_inode (data, &inode, data->inode, data->tree);
  if (err)
    {
      holy_btrfs_unmount (data);
      return err;
    }

  file->data = data;
  file->size = holy_le_to_cpu64 (inode.size);

  return err;
}

static holy_err_t
holy_btrfs_close (holy_file_t file)
{
  holy_btrfs_unmount (file->data);

  return holy_ERR_NONE;
}

static holy_ssize_t
holy_btrfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_btrfs_data *data = file->data;

  return holy_btrfs_extent_read (data, data->inode,
				 data->tree, file->offset, buf, len);
}

static holy_err_t
holy_btrfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_btrfs_data *data;

  *uuid = NULL;

  data = holy_btrfs_mount (device);
  if (!data)
    return holy_errno;

  *uuid = holy_xasprintf ("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
			  holy_be_to_cpu16 (data->sblock.uuid[0]),
			  holy_be_to_cpu16 (data->sblock.uuid[1]),
			  holy_be_to_cpu16 (data->sblock.uuid[2]),
			  holy_be_to_cpu16 (data->sblock.uuid[3]),
			  holy_be_to_cpu16 (data->sblock.uuid[4]),
			  holy_be_to_cpu16 (data->sblock.uuid[5]),
			  holy_be_to_cpu16 (data->sblock.uuid[6]),
			  holy_be_to_cpu16 (data->sblock.uuid[7]));

  holy_btrfs_unmount (data);

  return holy_errno;
}

static holy_err_t
holy_btrfs_label (holy_device_t device, char **label)
{
  struct holy_btrfs_data *data;

  *label = NULL;

  data = holy_btrfs_mount (device);
  if (!data)
    return holy_errno;

  *label = holy_strndup (data->sblock.label, sizeof (data->sblock.label));

  holy_btrfs_unmount (data);

  return holy_errno;
}

#ifdef holy_UTIL
static holy_err_t
holy_btrfs_embed (holy_device_t device __attribute__ ((unused)),
		  unsigned int *nsectors,
		  unsigned int max_nsectors,
		  holy_embed_type_t embed_type,
		  holy_disk_addr_t **sectors)
{
  unsigned i;

  if (embed_type != holy_EMBED_PCBIOS)
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "BtrFS currently supports only PC-BIOS embedding");

  if (64 * 2 - 1 < *nsectors)
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       N_("your core.img is unusually large.  "
			  "It won't fit in the embedding area"));

  *nsectors = 64 * 2 - 1;
  if (*nsectors > max_nsectors)
    *nsectors = max_nsectors;
  *sectors = holy_malloc (*nsectors * sizeof (**sectors));
  if (!*sectors)
    return holy_errno;
  for (i = 0; i < *nsectors; i++)
    (*sectors)[i] = i + 1;

  return holy_ERR_NONE;
}
#endif

static struct holy_fs holy_btrfs_fs = {
  .name = "btrfs",
  .dir = holy_btrfs_dir,
  .open = holy_btrfs_open,
  .read = holy_btrfs_read,
  .close = holy_btrfs_close,
  .uuid = holy_btrfs_uuid,
  .label = holy_btrfs_label,
#ifdef holy_UTIL
  .embed = holy_btrfs_embed,
  .reserved_first_sector = 1,
  .blocklist_install = 0,
#endif
};

holy_MOD_INIT (btrfs)
{
  holy_fs_register (&holy_btrfs_fs);
}

holy_MOD_FINI (btrfs)
{
  holy_fs_unregister (&holy_btrfs_fs);
}
