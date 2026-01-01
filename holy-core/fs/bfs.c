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
#include <holy/fshelp.h>

holy_MOD_LICENSE ("GPLv2+");

#ifdef MODE_AFS
#define BTREE_ALIGN 4
#define SUPERBLOCK  2
#else
#define BTREE_ALIGN 8
#define SUPERBLOCK  1
#endif

#define holy_bfs_to_cpu16 holy_le_to_cpu16
#define holy_bfs_to_cpu32 holy_le_to_cpu32
#define holy_bfs_to_cpu64 holy_le_to_cpu64
#define holy_cpu_to_bfs32_compile_time holy_cpu_to_le32_compile_time

#ifdef MODE_AFS
#define holy_bfs_to_cpu_treehead holy_bfs_to_cpu32
#else
#define holy_bfs_to_cpu_treehead holy_bfs_to_cpu16
#endif

#ifdef MODE_AFS
#define SUPER_BLOCK_MAGIC1 0x41465331
#else
#define SUPER_BLOCK_MAGIC1 0x42465331
#endif
#define SUPER_BLOCK_MAGIC2 0xdd121031
#define SUPER_BLOCK_MAGIC3 0x15b6830e
#define POINTER_INVALID 0xffffffffffffffffULL

#define ATTR_TYPE      0160000
#define ATTR_REG       0100000
#define ATTR_DIR       0040000
#define ATTR_LNK       0120000

#define DOUBLE_INDIRECT_SHIFT 2

#define LOG_EXTENT_SIZE 3
struct holy_bfs_extent
{
  holy_uint32_t ag;
  holy_uint16_t start;
  holy_uint16_t len;
} holy_PACKED;

struct holy_bfs_superblock
{
  char label[32];
  holy_uint32_t magic1;
  holy_uint32_t unused1;
  holy_uint32_t bsize;
  holy_uint32_t log2_bsize;
  holy_uint8_t unused[20];
  holy_uint32_t magic2;
  holy_uint32_t unused2;
  holy_uint32_t log2_ag_size;
  holy_uint8_t unused3[32];
  holy_uint32_t magic3;
  struct holy_bfs_extent root_dir;
} holy_PACKED;

struct holy_bfs_inode
{
  holy_uint8_t unused[20];
  holy_uint32_t mode;
  holy_uint32_t flags;
#ifdef MODE_AFS
  holy_uint8_t unused2[12];
#else
  holy_uint8_t unused2[8];
#endif
  holy_uint64_t mtime;
  holy_uint8_t unused3[8];
  struct holy_bfs_extent attr;
  holy_uint8_t unused4[12];

  union
  {
    struct
    {
      struct holy_bfs_extent direct[12];
      holy_uint64_t max_direct_range;
      struct holy_bfs_extent indirect;
      holy_uint64_t max_indirect_range;
      struct holy_bfs_extent double_indirect;
      holy_uint64_t max_double_indirect_range;
      holy_uint64_t size;
      holy_uint32_t pad[4];
    } holy_PACKED;
    char inplace_link[144];
  } holy_PACKED;
  holy_uint8_t small_data[0];
} holy_PACKED;

enum
{
  LONG_SYMLINK = 0x40
};

struct holy_bfs_small_data_element_header
{
  holy_uint32_t type;
  holy_uint16_t name_len;
  holy_uint16_t value_len;
} holy_PACKED;

struct holy_bfs_btree_header
{
  holy_uint32_t magic;
#ifdef MODE_AFS
  holy_uint64_t root;
  holy_uint32_t level;
  holy_uint32_t node_size;
  holy_uint32_t unused;
#else
  holy_uint32_t node_size;
  holy_uint32_t level;
  holy_uint32_t unused;
  holy_uint64_t root;
#endif
  holy_uint32_t unused2[2];
} holy_PACKED;

struct holy_bfs_btree_node
{
  holy_uint64_t unused;
  holy_uint64_t right;
  holy_uint64_t overflow;
#ifdef MODE_AFS
  holy_uint32_t count_keys;
  holy_uint32_t total_key_len;
#else
  holy_uint16_t count_keys;
  holy_uint16_t total_key_len;
#endif
} holy_PACKED;

struct holy_bfs_data
{
  struct holy_bfs_superblock sb;
  struct holy_bfs_inode ino;
};

/* Context for holy_bfs_dir.  */
struct holy_bfs_dir_ctx
{
  holy_device_t device;
  holy_fs_dir_hook_t hook;
  void *hook_data;
  struct holy_bfs_superblock sb;
};

static holy_err_t
read_extent (holy_disk_t disk,
	     const struct holy_bfs_superblock *sb,
	     const struct holy_bfs_extent *in,
	     holy_off_t off, holy_off_t byteoff, void *buf, holy_size_t len)
{
#ifdef MODE_AFS
  return holy_disk_read (disk, ((holy_bfs_to_cpu32 (in->ag)
				 << (holy_bfs_to_cpu32 (sb->log2_ag_size)
				     - holy_DISK_SECTOR_BITS))
				+ ((holy_bfs_to_cpu16 (in->start) + off)
				   << (holy_bfs_to_cpu32 (sb->log2_bsize)
				       - holy_DISK_SECTOR_BITS))),
			 byteoff, len, buf);
#else
  return holy_disk_read (disk, (((holy_bfs_to_cpu32 (in->ag)
				  << holy_bfs_to_cpu32 (sb->log2_ag_size))
				 + holy_bfs_to_cpu16 (in->start) + off)
				<< (holy_bfs_to_cpu32 (sb->log2_bsize)
				    - holy_DISK_SECTOR_BITS)),
			 byteoff, len, buf);
#endif
}

#ifdef MODE_AFS
#define RANGE_SHIFT holy_bfs_to_cpu32 (sb->log2_bsize)
#else
#define RANGE_SHIFT 0
#endif

static holy_err_t
read_bfs_file (holy_disk_t disk,
	       const struct holy_bfs_superblock *sb,
	       const struct holy_bfs_inode *ino,
	       holy_off_t off, void *buf, holy_size_t len,
	       holy_disk_read_hook_t read_hook, void *read_hook_data)
{
  if (len == 0)
    return holy_ERR_NONE;

  if (off + len > holy_bfs_to_cpu64 (ino->size))
    return holy_error (holy_ERR_OUT_OF_RANGE,
		       N_("attempt to read past the end of file"));

  if (off < (holy_bfs_to_cpu64 (ino->max_direct_range) << RANGE_SHIFT))
    {
      unsigned i;
      holy_uint64_t pos = 0;
      for (i = 0; i < ARRAY_SIZE (ino->direct); i++)
	{
	  holy_uint64_t newpos;
	  newpos = pos + (((holy_uint64_t) holy_bfs_to_cpu16 (ino->direct[i].len))
			  << holy_bfs_to_cpu32 (sb->log2_bsize));
	  if (newpos > off)
	    {
	      holy_size_t read_size;
	      holy_err_t err;
	      read_size = newpos - off;
	      if (read_size > len)
		read_size = len;
	      disk->read_hook = read_hook;
	      disk->read_hook_data = read_hook_data;
	      err = read_extent (disk, sb, &ino->direct[i], 0, off - pos,
				 buf, read_size);
	      disk->read_hook = 0;
	      if (err)
		return err;
	      off += read_size;
	      len -= read_size;
	      buf = (char *) buf + read_size;
	      if (len == 0)
		return holy_ERR_NONE;
	    }
	  pos = newpos;
	}
    }

  if (off < (holy_bfs_to_cpu64 (ino->max_direct_range) << RANGE_SHIFT))
    return holy_error (holy_ERR_BAD_FS, "incorrect direct blocks");

  if (off < (holy_bfs_to_cpu64 (ino->max_indirect_range) << RANGE_SHIFT))
    {
      unsigned i;
      struct holy_bfs_extent *entries;
      holy_size_t nentries;
      holy_err_t err;
      holy_uint64_t pos = (holy_bfs_to_cpu64 (ino->max_direct_range)
			   << RANGE_SHIFT);
      nentries = (((holy_size_t) holy_bfs_to_cpu16 (ino->indirect.len))
		  << (holy_bfs_to_cpu32 (sb->log2_bsize) - LOG_EXTENT_SIZE));
      entries = holy_malloc (nentries << LOG_EXTENT_SIZE);
      if (!entries)
	return holy_errno;
      err = read_extent (disk, sb, &ino->indirect, 0, 0,
			 entries, nentries << LOG_EXTENT_SIZE);
      for (i = 0; i < nentries; i++)
	{
	  holy_uint64_t newpos;
	  newpos = pos + (((holy_uint64_t) holy_bfs_to_cpu16 (entries[i].len))
			  << holy_bfs_to_cpu32 (sb->log2_bsize));
	  if (newpos > off)
	    {
	      holy_size_t read_size;
	      read_size = newpos - off;
	      if (read_size > len)
		read_size = len;
	      disk->read_hook = read_hook;
	      disk->read_hook_data = read_hook_data;
	      err = read_extent (disk, sb, &entries[i], 0, off - pos,
				 buf, read_size);
	      disk->read_hook = 0;
	      if (err)
		{
		  holy_free (entries);
		  return err;
		}
	      off += read_size;
	      len -= read_size;
	      buf = (char *) buf + read_size;
	      if (len == 0)
		{
		  holy_free (entries);
		  return holy_ERR_NONE;
		}
	    }
	  pos = newpos;
	}
      holy_free (entries);
    }

  if (off < (holy_bfs_to_cpu64 (ino->max_indirect_range) << RANGE_SHIFT))
    return holy_error (holy_ERR_BAD_FS, "incorrect indirect blocks");

  {
    struct holy_bfs_extent *l1_entries, *l2_entries;
    holy_size_t nl1_entries, nl2_entries;
    holy_off_t last_l1n = ~0ULL;
    holy_err_t err;
    nl1_entries = (((holy_uint64_t) holy_bfs_to_cpu16 (ino->double_indirect.len))
		   << (holy_bfs_to_cpu32 (sb->log2_bsize) - LOG_EXTENT_SIZE));
    l1_entries = holy_malloc (nl1_entries << LOG_EXTENT_SIZE);
    if (!l1_entries)
      return holy_errno;
    nl2_entries = 0;
    l2_entries = holy_malloc (1 << (DOUBLE_INDIRECT_SHIFT
				    + holy_bfs_to_cpu32 (sb->log2_bsize)));
    if (!l2_entries)
      {
	holy_free (l1_entries);
	return holy_errno;
      }
    err = read_extent (disk, sb, &ino->double_indirect, 0, 0,
		       l1_entries, nl1_entries << LOG_EXTENT_SIZE);
    if (err)
      {
	holy_free (l1_entries);
	holy_free (l2_entries);
	return err;
      }

    while (len > 0)
      {
	holy_off_t boff, l2n, l1n;
	holy_size_t read_size;
	holy_off_t double_indirect_offset;
	double_indirect_offset = off
	  - holy_bfs_to_cpu64 (ino->max_indirect_range);
	boff = (double_indirect_offset
		& ((1 << (holy_bfs_to_cpu32 (sb->log2_bsize)
			  + DOUBLE_INDIRECT_SHIFT)) - 1));
	l2n = ((double_indirect_offset >> (holy_bfs_to_cpu32 (sb->log2_bsize)
					   + DOUBLE_INDIRECT_SHIFT))
	       & ((1 << (holy_bfs_to_cpu32 (sb->log2_bsize) - LOG_EXTENT_SIZE
			 + DOUBLE_INDIRECT_SHIFT)) - 1));
	l1n =
	  (double_indirect_offset >>
	   (2 * holy_bfs_to_cpu32 (sb->log2_bsize) - LOG_EXTENT_SIZE +
	    2 * DOUBLE_INDIRECT_SHIFT));
	if (l1n > nl1_entries)
	  {
	    holy_free (l1_entries);
	    holy_free (l2_entries);
	    return holy_error (holy_ERR_BAD_FS,
			       "incorrect double-indirect block");
	  }
	if (l1n != last_l1n)
	  {
	    nl2_entries = (((holy_uint64_t) holy_bfs_to_cpu16 (l1_entries[l1n].len))
			   << (holy_bfs_to_cpu32 (sb->log2_bsize)
			       - LOG_EXTENT_SIZE));
	    if (nl2_entries > (1U << (holy_bfs_to_cpu32 (sb->log2_bsize)
				      - LOG_EXTENT_SIZE
				      + DOUBLE_INDIRECT_SHIFT)))
	      nl2_entries = (1 << (holy_bfs_to_cpu32 (sb->log2_bsize)
				   - LOG_EXTENT_SIZE
				   + DOUBLE_INDIRECT_SHIFT));
	    err = read_extent (disk, sb, &l1_entries[l1n], 0, 0,
			       l2_entries, nl2_entries << LOG_EXTENT_SIZE);
	    if (err)
	      {
		holy_free (l1_entries);
		holy_free (l2_entries);
		return err;
	      }
	    last_l1n = l1n;
	  }
	if (l2n > nl2_entries)
	  {
	    holy_free (l1_entries);
	    holy_free (l2_entries);
	    return holy_error (holy_ERR_BAD_FS,
			       "incorrect double-indirect block");
	  }

	read_size = (1 << (holy_bfs_to_cpu32 (sb->log2_bsize)
			   + DOUBLE_INDIRECT_SHIFT)) - boff;
	if (read_size > len)
	  read_size = len;
	disk->read_hook = read_hook;
	disk->read_hook_data = read_hook_data;
	err = read_extent (disk, sb, &l2_entries[l2n], 0, boff,
			   buf, read_size);
	disk->read_hook = 0;
	if (err)
	  {
	    holy_free (l1_entries);
	    holy_free (l2_entries);
	    return err;
	  }
	off += read_size;
	len -= read_size;
	buf = (char *) buf + read_size;
      }
    return holy_ERR_NONE;
  }
}

static holy_err_t
read_b_node (holy_disk_t disk,
	     const struct holy_bfs_superblock *sb,
	     const struct holy_bfs_inode *ino,
	     holy_uint64_t node_off,
	     struct holy_bfs_btree_node **node,
	     char **key_data, holy_uint16_t **keylen_idx,
	     holy_unaligned_uint64_t **key_values)
{
  void *ret;
  struct holy_bfs_btree_node node_head;
  holy_size_t total_size;
  holy_err_t err;

  *node = NULL;
  *key_data = NULL;
  *keylen_idx = NULL;
  *key_values = NULL;

  err = read_bfs_file (disk, sb, ino, node_off, &node_head, sizeof (node_head),
		       0, 0);
  if (err)
    return err;

  total_size = ALIGN_UP (sizeof (node_head) +
			 holy_bfs_to_cpu_treehead
			 (node_head.total_key_len),
			 BTREE_ALIGN) +
    holy_bfs_to_cpu_treehead (node_head.count_keys) *
    sizeof (holy_uint16_t)
    + holy_bfs_to_cpu_treehead (node_head.count_keys) *
    sizeof (holy_uint64_t);

  ret = holy_malloc (total_size);
  if (!ret)
    return holy_errno;

  err = read_bfs_file (disk, sb, ino, node_off, ret, total_size, 0, 0);
  if (err)
    {
      holy_free (ret);
      return err;
    }

  *node = ret;
  *key_data = (char *) ret + sizeof (node_head);
  *keylen_idx = (holy_uint16_t *) ret
    + ALIGN_UP (sizeof (node_head) +
		holy_bfs_to_cpu_treehead (node_head.total_key_len),
		BTREE_ALIGN) / 2;
  *key_values = (holy_unaligned_uint64_t *)
    (*keylen_idx +
     holy_bfs_to_cpu_treehead (node_head.count_keys));

  return holy_ERR_NONE;
}

static int
iterate_in_b_tree (holy_disk_t disk,
		   const struct holy_bfs_superblock *sb,
		   const struct holy_bfs_inode *ino,
		   int (*hook) (const char *name, holy_uint64_t value,
				struct holy_bfs_dir_ctx *ctx),
		   struct holy_bfs_dir_ctx *ctx)
{
  struct holy_bfs_btree_header head;
  holy_err_t err;
  int level;
  holy_uint64_t node_off;

  err = read_bfs_file (disk, sb, ino, 0, &head, sizeof (head), 0, 0);
  if (err)
    return 0;
  node_off = holy_bfs_to_cpu64 (head.root);

  level = holy_bfs_to_cpu32 (head.level) - 1;
  while (level--)
    {
      struct holy_bfs_btree_node node;
      holy_uint64_t key_value;
      err = read_bfs_file (disk, sb, ino, node_off, &node, sizeof (node),
			   0, 0);
      if (err)
	return 0;
      err = read_bfs_file (disk, sb, ino, node_off
			   + ALIGN_UP (sizeof (node) +
				       holy_bfs_to_cpu_treehead (node.
								 total_key_len),
				       BTREE_ALIGN) +
			   holy_bfs_to_cpu_treehead (node.count_keys) *
			   sizeof (holy_uint16_t), &key_value,
			   sizeof (holy_uint64_t), 0, 0);
      if (err)
	return 0;

      node_off = holy_bfs_to_cpu64 (key_value);
    }

  while (1)
    {
      struct holy_bfs_btree_node *node;
      char *key_data;
      holy_uint16_t *keylen_idx;
      holy_unaligned_uint64_t *key_values;
      unsigned i;
      holy_uint16_t start = 0, end = 0;

      err = read_b_node (disk, sb, ino,
			 node_off,
			 &node,
			 &key_data, 
			 &keylen_idx,
			 &key_values);

      if (err)
	return 0;
      
      for (i = 0; i < holy_bfs_to_cpu_treehead (node->count_keys); i++)
	{
	  char c;
	  start = end;
	  end = holy_bfs_to_cpu16 (keylen_idx[i]);
	  if (holy_bfs_to_cpu_treehead (node->total_key_len) <= end)
	    end = holy_bfs_to_cpu_treehead (node->total_key_len);
	  c = key_data[end];
	  key_data[end] = 0;
	  if (hook (key_data + start, holy_bfs_to_cpu64 (key_values[i].val),
		    ctx))
	    {
	      holy_free (node);
	      return 1;
	    }
	    key_data[end] = c;
	  }
	node_off = holy_bfs_to_cpu64 (node->right);
	holy_free (node);
	if (node_off == POINTER_INVALID)
	  return 0;
    }
}

static int
bfs_strcmp (const char *a, const char *b, holy_size_t alen)
{
  char ac, bc;
  while (*b && alen)
    {
      if (*a != *b)
	break;

      a++;
      b++;
      alen--;
    }

  ac = alen ? *a : 0;
  bc = *b;

#ifdef MODE_AFS
  return (int) (holy_int8_t) ac - (int) (holy_int8_t) bc;
#else
  return (int) (holy_uint8_t) ac - (int) (holy_uint8_t) bc;
#endif
}

static holy_err_t
find_in_b_tree (holy_disk_t disk,
		const struct holy_bfs_superblock *sb,
		const struct holy_bfs_inode *ino, const char *name,
		holy_uint64_t * res)
{
  struct holy_bfs_btree_header head;
  holy_err_t err;
  int level;
  holy_uint64_t node_off;

  err = read_bfs_file (disk, sb, ino, 0, &head, sizeof (head), 0, 0);
  if (err)
    return err;
  node_off = holy_bfs_to_cpu64 (head.root);

  level = holy_bfs_to_cpu32 (head.level) - 1;
  while (1)
    {
      struct holy_bfs_btree_node *node;
      char *key_data;
      holy_uint16_t *keylen_idx;
      holy_unaligned_uint64_t *key_values;
      int lg, j;
      unsigned i;

      err = read_b_node (disk, sb, ino, node_off, &node, &key_data, &keylen_idx, &key_values);
      if (err)
	return err;

      if (node->count_keys == 0)
	{
	  holy_free (node);
	  return holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"),
			     name);
	}

      for (lg = 0; holy_bfs_to_cpu_treehead (node->count_keys) >> lg; lg++);

      i = 0;

      for (j = lg - 1; j >= 0; j--)
	{
	  int cmp;
	  holy_uint16_t start = 0, end = 0;
	  if ((i | (1 << j)) >= holy_bfs_to_cpu_treehead (node->count_keys))
	    continue;
	  start = holy_bfs_to_cpu16 (keylen_idx[(i | (1 << j)) - 1]);
	  end = holy_bfs_to_cpu16 (keylen_idx[(i | (1 << j))]);
	  if (holy_bfs_to_cpu_treehead (node->total_key_len) <= end)
	    end = holy_bfs_to_cpu_treehead (node->total_key_len);
	  cmp = bfs_strcmp (key_data + start, name, end - start);
	  if (cmp == 0 && level == 0)
	    {
	      *res = holy_bfs_to_cpu64 (key_values[i | (1 << j)].val);
	      holy_free (node);
	      return holy_ERR_NONE;
	    }
#ifdef MODE_AFS
	  if (cmp <= 0)
#else
	  if (cmp < 0)
#endif
	    i |= (1 << j);
	}
      if (i == 0)
	{
	  holy_uint16_t end = 0;
	  int cmp;
	  end = holy_bfs_to_cpu16 (keylen_idx[0]);
	  if (holy_bfs_to_cpu_treehead (node->total_key_len) <= end)
	    end = holy_bfs_to_cpu_treehead (node->total_key_len);
	  cmp = bfs_strcmp (key_data, name, end);
	  if (cmp == 0 && level == 0)
	    {
	      *res = holy_bfs_to_cpu64 (key_values[0].val);
	      holy_free (node);
	      return holy_ERR_NONE;
	    }
#ifdef MODE_AFS
	  if (cmp > 0 && level != 0)
#else
	    if (cmp >= 0 && level != 0)
#endif
	      {
		node_off = holy_bfs_to_cpu64 (key_values[0].val);
		level--;
		holy_free (node);
		continue;
	      }
	    else if (level != 0
		     && holy_bfs_to_cpu_treehead (node->count_keys) >= 2)
	      {
		node_off = holy_bfs_to_cpu64 (key_values[1].val);
		level--;
		holy_free (node);
		continue;
	      }	      
	  }
	else if (level != 0
		 && i + 1 < holy_bfs_to_cpu_treehead (node->count_keys))
	  {
	    node_off = holy_bfs_to_cpu64 (key_values[i + 1].val);
	    level--;
	    holy_free (node);
	    continue;
	  }
	if (node->overflow != POINTER_INVALID)
	  {
	    node_off = holy_bfs_to_cpu64 (node->overflow);
	    /* This level-- isn't specified but is needed.  */
	    level--;
	    holy_free (node);
	    continue;
	  }
	holy_free (node);
	return holy_error (holy_ERR_FILE_NOT_FOUND, N_("file `%s' not found"),
			   name);
    }
}

struct holy_fshelp_node
{
  holy_disk_t disk;
  const struct holy_bfs_superblock *sb;
  struct holy_bfs_inode ino;
};

static holy_err_t
lookup_file (holy_fshelp_node_t dir,
	     const char *name,
	     holy_fshelp_node_t *foundnode,
	     enum holy_fshelp_filetype *foundtype)
{
  holy_err_t err;
  struct holy_bfs_inode *new_ino;
  holy_uint64_t res = 0;

  err = find_in_b_tree (dir->disk, dir->sb, &dir->ino, name, &res);
  if (err)
    return err;

  *foundnode = holy_malloc (sizeof (struct holy_fshelp_node));
  if (!*foundnode)
    return holy_errno;

  (*foundnode)->disk = dir->disk;
  (*foundnode)->sb = dir->sb;
  new_ino = &(*foundnode)->ino;

  if (holy_disk_read (dir->disk, res
		      << (holy_bfs_to_cpu32 (dir->sb->log2_bsize)
			  - holy_DISK_SECTOR_BITS), 0,
		      sizeof (*new_ino), (char *) new_ino))
    {
      holy_free (*foundnode);
      return holy_errno;
    }
  switch (holy_bfs_to_cpu32 (new_ino->mode) & ATTR_TYPE)
    {
    default:
    case ATTR_REG:
      *foundtype = holy_FSHELP_REG;
      break;
    case ATTR_DIR:
      *foundtype = holy_FSHELP_DIR;
      break;
    case ATTR_LNK:
      *foundtype = holy_FSHELP_SYMLINK;
      break;
    }
  return holy_ERR_NONE;
}

static char *
read_symlink (holy_fshelp_node_t node)
{
  char *alloc = NULL;
  holy_err_t err;

#ifndef MODE_AFS
  if (!(holy_bfs_to_cpu32 (node->ino.flags) & LONG_SYMLINK))
    {
      alloc = holy_malloc (sizeof (node->ino.inplace_link) + 1);
      if (!alloc)
	{
	  return NULL;
	}
      holy_memcpy (alloc, node->ino.inplace_link,
		   sizeof (node->ino.inplace_link));
      alloc[sizeof (node->ino.inplace_link)] = 0;
    }
  else
#endif
    {
      holy_size_t symsize = holy_bfs_to_cpu64 (node->ino.size);
      alloc = holy_malloc (symsize + 1);
      if (!alloc)
	return NULL;
      err = read_bfs_file (node->disk, node->sb, &node->ino, 0, alloc, symsize, 0, 0);
      if (err)
	{
	  holy_free (alloc);
	  return NULL;
	}
      alloc[symsize] = 0;
    }

  return alloc;
}

static holy_err_t
find_file (const char *path, holy_disk_t disk,
	   const struct holy_bfs_superblock *sb, struct holy_bfs_inode *ino,
	   enum holy_fshelp_filetype exptype)
{
  holy_err_t err;
  struct holy_fshelp_node root = {
    .disk = disk,
    .sb = sb,
  };
  struct holy_fshelp_node *found;

  err = read_extent (disk, sb, &sb->root_dir, 0, 0, &root.ino,
		     sizeof (root.ino));
  if (err)
    return err;
  err = holy_fshelp_find_file_lookup (path, &root, &found, lookup_file, read_symlink, exptype);
  if (!err)
    holy_memcpy (ino, &found->ino, sizeof (*ino));

  if (&root != found)
    holy_free (found);
  return err;
}

static holy_err_t
mount (holy_disk_t disk, struct holy_bfs_superblock *sb)
{
  holy_err_t err;
  err = holy_disk_read (disk, SUPERBLOCK, 0, sizeof (*sb), sb);
  if (err == holy_ERR_OUT_OF_RANGE)
    return holy_error (holy_ERR_BAD_FS,
#ifdef MODE_AFS
		       "not an AFS filesystem"
#else
		       "not a BFS filesystem"
#endif
		       );
  if (err)
    return err;
  if (sb->magic1 != holy_cpu_to_bfs32_compile_time (SUPER_BLOCK_MAGIC1)
      || sb->magic2 != holy_cpu_to_bfs32_compile_time (SUPER_BLOCK_MAGIC2)
      || sb->magic3 != holy_cpu_to_bfs32_compile_time (SUPER_BLOCK_MAGIC3)
      || sb->bsize == 0
      || (holy_bfs_to_cpu32 (sb->bsize)
	  != (1U << holy_bfs_to_cpu32 (sb->log2_bsize)))
      || holy_bfs_to_cpu32 (sb->log2_bsize) < holy_DISK_SECTOR_BITS)
    return holy_error (holy_ERR_BAD_FS,
#ifdef MODE_AFS
		       "not an AFS filesystem"
#else
		       "not a BFS filesystem"
#endif
		       );
  return holy_ERR_NONE;
}

/* Helper for holy_bfs_dir.  */
static int
holy_bfs_dir_iter (const char *name, holy_uint64_t value,
		   struct holy_bfs_dir_ctx *ctx)
{
  holy_err_t err2;
  struct holy_bfs_inode ino;
  struct holy_dirhook_info info;

  err2 = holy_disk_read (ctx->device->disk, value
			 << (holy_bfs_to_cpu32 (ctx->sb.log2_bsize)
			     - holy_DISK_SECTOR_BITS), 0,
			 sizeof (ino), (char *) &ino);
  if (err2)
    {
      holy_print_error ();
      return 0;
    }

  info.mtimeset = 1;
#ifdef MODE_AFS
  info.mtime =
    holy_divmod64 (holy_bfs_to_cpu64 (ino.mtime), 1000000, 0);
#else
  info.mtime = holy_bfs_to_cpu64 (ino.mtime) >> 16;
#endif
  info.dir = ((holy_bfs_to_cpu32 (ino.mode) & ATTR_TYPE) == ATTR_DIR);
  return ctx->hook (name, &info, ctx->hook_data);
}

static holy_err_t
holy_bfs_dir (holy_device_t device, const char *path,
	      holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_bfs_dir_ctx ctx = {
    .device = device,
    .hook = hook,
    .hook_data = hook_data
  };
  holy_err_t err;

  err = mount (device->disk, &ctx.sb);
  if (err)
    return err;

  {
    struct holy_bfs_inode ino;
    err = find_file (path, device->disk, &ctx.sb, &ino, holy_FSHELP_DIR);
    if (err)
      return err;
    iterate_in_b_tree (device->disk, &ctx.sb, &ino, holy_bfs_dir_iter,
		       &ctx);
  }

  return holy_errno;
}

static holy_err_t
holy_bfs_open (struct holy_file *file, const char *name)
{
  struct holy_bfs_superblock sb;
  holy_err_t err;

  err = mount (file->device->disk, &sb);
  if (err)
    return err;

  {
    struct holy_bfs_inode ino;
    struct holy_bfs_data *data;
    err = find_file (name, file->device->disk, &sb, &ino, holy_FSHELP_REG);
    if (err)
      return err;

    data = holy_zalloc (sizeof (struct holy_bfs_data));
    if (!data)
      return holy_errno;
    data->sb = sb;
    holy_memcpy (&data->ino, &ino, sizeof (data->ino));
    file->data = data;
    file->size = holy_bfs_to_cpu64 (ino.size);
  }

  return holy_ERR_NONE;
}

static holy_err_t
holy_bfs_close (holy_file_t file)
{
  holy_free (file->data);

  return holy_ERR_NONE;
}

static holy_ssize_t
holy_bfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  holy_err_t err;
  struct holy_bfs_data *data = file->data;

  err = read_bfs_file (file->device->disk, &data->sb,
		       &data->ino, file->offset, buf, len,
		       file->read_hook, file->read_hook_data);
  if (err)
    return -1;
  return len;
}

static holy_err_t
holy_bfs_label (holy_device_t device, char **label)
{
  struct holy_bfs_superblock sb;
  holy_err_t err;

  *label = 0;

  err = mount (device->disk, &sb);
  if (err)
    return err;

  *label = holy_strndup (sb.label, sizeof (sb.label));
  return holy_ERR_NONE;
}

#ifndef MODE_AFS
static holy_ssize_t
read_bfs_attr (holy_disk_t disk,
	       const struct holy_bfs_superblock *sb,
	       struct holy_bfs_inode *ino,
	       const char *name, void *buf, holy_size_t len)
{
  holy_uint8_t *ptr = (holy_uint8_t *) ino->small_data;
  holy_uint8_t *end = ((holy_uint8_t *) ino + holy_bfs_to_cpu32 (sb->bsize));

  while (ptr + sizeof (struct holy_bfs_small_data_element_header) < end)
    {
      struct holy_bfs_small_data_element_header *el;
      char *el_name;
      holy_uint8_t *data;
      el = (struct holy_bfs_small_data_element_header *) ptr;
      if (el->name_len == 0)
	break;
      el_name = (char *) (el + 1);
      data = (holy_uint8_t *) el_name + holy_bfs_to_cpu16 (el->name_len) + 3;
      ptr = data + holy_bfs_to_cpu16 (el->value_len) + 1;
      if (holy_memcmp (name, el_name, holy_bfs_to_cpu16 (el->name_len)) == 0
	  && name[el->name_len] == 0)
	{
	  holy_size_t copy;
	  copy = len;
	  if (holy_bfs_to_cpu16 (el->value_len) > copy)
	    copy = holy_bfs_to_cpu16 (el->value_len);
	  holy_memcpy (buf, data, copy);
	  return copy;
	}
    }

  if (ino->attr.len != 0)
    {
      holy_size_t read;
      holy_err_t err;
      holy_uint64_t res;

      err = read_extent (disk, sb, &ino->attr, 0, 0, ino,
			 holy_bfs_to_cpu32 (sb->bsize));
      if (err)
	return -1;

      err = find_in_b_tree (disk, sb, ino, name, &res);
      if (err)
	return -1;
      holy_disk_read (disk, res
		      << (holy_bfs_to_cpu32 (sb->log2_bsize)
			  - holy_DISK_SECTOR_BITS), 0,
		      holy_bfs_to_cpu32 (sb->bsize), (char *) ino);
      read = holy_bfs_to_cpu64 (ino->size);
      if (read > len)
	read = len;

      err = read_bfs_file (disk, sb, ino, 0, buf, read, 0, 0);
      if (err)
	return -1;
      return read;
    }
  return -1;
}

static holy_err_t
holy_bfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_bfs_superblock sb;
  holy_err_t err;
  struct holy_bfs_inode *ino;
  holy_uint64_t vid;

  *uuid = 0;

  err = mount (device->disk, &sb);
  if (err)
    return err;

  ino = holy_malloc (holy_bfs_to_cpu32 (sb.bsize));
  if (!ino)
    return holy_errno;

  err = read_extent (device->disk, &sb, &sb.root_dir, 0, 0,
		     ino, holy_bfs_to_cpu32 (sb.bsize));
  if (err)
    {
      holy_free (ino);
      return err;
    }
  if (read_bfs_attr (device->disk, &sb, ino, "be:volume_id",
		     &vid, sizeof (vid)) == sizeof (vid))
    *uuid =
      holy_xasprintf ("%016" PRIxholy_UINT64_T, holy_bfs_to_cpu64 (vid));

  holy_free (ino);

  return holy_ERR_NONE;
}
#endif

static struct holy_fs holy_bfs_fs = {
#ifdef MODE_AFS
  .name = "afs",
#else
  .name = "bfs",
#endif
  .dir = holy_bfs_dir,
  .open = holy_bfs_open,
  .read = holy_bfs_read,
  .close = holy_bfs_close,
  .label = holy_bfs_label,
#ifndef MODE_AFS
  .uuid = holy_bfs_uuid,
#endif
#ifdef holy_UTIL
  .reserved_first_sector = 1,
  .blocklist_install = 1,
#endif
};

#ifdef MODE_AFS
holy_MOD_INIT (afs)
#else
holy_MOD_INIT (bfs)
#endif
{
  COMPILE_TIME_ASSERT (1 << LOG_EXTENT_SIZE ==
		       sizeof (struct holy_bfs_extent));
  holy_fs_register (&holy_bfs_fs);
}

#ifdef MODE_AFS
holy_MOD_FINI (afs)
#else
holy_MOD_FINI (bfs)
#endif
{
  holy_fs_unregister (&holy_bfs_fs);
}
