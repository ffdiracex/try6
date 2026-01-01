/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_fshelp_node holy_hfsplus_file
#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/fshelp.h>
#include <holy/hfs.h>
#include <holy/charset.h>
#include <holy/hfsplus.h>

holy_MOD_LICENSE ("GPLv2+");

/* The type of node.  */
enum holy_hfsplus_btnode_type
  {
    holy_HFSPLUS_BTNODE_TYPE_LEAF = -1,
    holy_HFSPLUS_BTNODE_TYPE_INDEX = 0,
    holy_HFSPLUS_BTNODE_TYPE_HEADER = 1,
    holy_HFSPLUS_BTNODE_TYPE_MAP = 2,
  };

/* The header of a HFS+ B+ Tree.  */
struct holy_hfsplus_btheader
{
  holy_uint16_t depth;
  holy_uint32_t root;
  holy_uint32_t leaf_records;
  holy_uint32_t first_leaf_node;
  holy_uint32_t last_leaf_node;
  holy_uint16_t nodesize;
  holy_uint16_t keysize;
  holy_uint32_t total_nodes;
  holy_uint32_t free_nodes;
  holy_uint16_t reserved1;
  holy_uint32_t clump_size;  /* ignored */
  holy_uint8_t btree_type;
  holy_uint8_t key_compare;
  holy_uint32_t attributes;
} holy_PACKED;

struct holy_hfsplus_catfile
{
  holy_uint16_t type;
  holy_uint16_t flags;
  holy_uint32_t parentid; /* Thread only.  */
  holy_uint32_t fileid;
  holy_uint8_t unused1[4];
  holy_uint32_t mtime;
  holy_uint8_t unused2[22];
  holy_uint16_t mode;
  holy_uint8_t unused3[44];
  struct holy_hfsplus_forkdata data;
  struct holy_hfsplus_forkdata resource;
} holy_PACKED;

/* Filetype information as used in inodes.  */
#define holy_HFSPLUS_FILEMODE_MASK	0170000
#define holy_HFSPLUS_FILEMODE_REG	0100000
#define holy_HFSPLUS_FILEMODE_DIRECTORY	0040000
#define holy_HFSPLUS_FILEMODE_SYMLINK	0120000

/* Some pre-defined file IDs.  */
enum
  {
    holy_HFSPLUS_FILEID_ROOTDIR = 2,
    holy_HFSPLUS_FILEID_OVERFLOW = 3,
    holy_HFSPLUS_FILEID_CATALOG	= 4,
    holy_HFSPLUS_FILEID_ATTR	= 8
  };

enum holy_hfsplus_filetype
  {
    holy_HFSPLUS_FILETYPE_DIR = 1,
    holy_HFSPLUS_FILETYPE_REG = 2,
    holy_HFSPLUS_FILETYPE_DIR_THREAD = 3,
    holy_HFSPLUS_FILETYPE_REG_THREAD = 4
  };

#define holy_HFSPLUSX_BINARYCOMPARE	0xBC
#define holy_HFSPLUSX_CASEFOLDING	0xCF

static holy_dl_t my_mod;



holy_err_t (*holy_hfsplus_open_compressed) (struct holy_fshelp_node *node);
holy_ssize_t (*holy_hfsplus_read_compressed) (struct holy_hfsplus_file *node,
					      holy_off_t pos,
					      holy_size_t len,
					      char *buf);

/* Find the extent that points to FILEBLOCK.  If it is not in one of
   the 8 extents described by EXTENT, return -1.  In that case set
   FILEBLOCK to the next block.  */
static holy_disk_addr_t
holy_hfsplus_find_block (struct holy_hfsplus_extent *extent,
			 holy_disk_addr_t *fileblock)
{
  int i;
  holy_disk_addr_t blksleft = *fileblock;

  /* First lookup the file in the given extents.  */
  for (i = 0; i < 8; i++)
    {
      if (blksleft < holy_be_to_cpu32 (extent[i].count))
	return holy_be_to_cpu32 (extent[i].start) + blksleft;
      blksleft -= holy_be_to_cpu32 (extent[i].count);
    }

  *fileblock = blksleft;
  return 0xffffffffffffffffULL;
}

static int holy_hfsplus_cmp_extkey (struct holy_hfsplus_key *keya,
				    struct holy_hfsplus_key_internal *keyb);

/* Search for the block FILEBLOCK inside the file NODE.  Return the
   blocknumber of this block on disk.  */
static holy_disk_addr_t
holy_hfsplus_read_block (holy_fshelp_node_t node, holy_disk_addr_t fileblock)
{
  struct holy_hfsplus_btnode *nnode = 0;
  holy_disk_addr_t blksleft = fileblock;
  struct holy_hfsplus_extent *extents = node->compressed
    ? &node->resource_extents[0] : &node->extents[0];

  while (1)
    {
      struct holy_hfsplus_extkey *key;
      struct holy_hfsplus_key_internal extoverflow;
      holy_disk_addr_t blk;
      holy_off_t ptr;

      /* Try to find this block in the current set of extents.  */
      blk = holy_hfsplus_find_block (extents, &blksleft);

      /* The previous iteration of this loop allocated memory.  The
	 code above used this memory, it can be freed now.  */
      holy_free (nnode);
      nnode = 0;

      if (blk != 0xffffffffffffffffULL)
	return blk;

      /* For the extent overflow file, extra extents can't be found in
	 the extent overflow file.  If this happens, you found a
	 bug...  */
      if (node->fileid == holy_HFSPLUS_FILEID_OVERFLOW)
	{
	  holy_error (holy_ERR_READ_ERROR,
		      "extra extents found in an extend overflow file");
	  break;
	}

      /* Set up the key to look for in the extent overflow file.  */
      extoverflow.extkey.fileid = node->fileid;
      extoverflow.extkey.type = 0;
      extoverflow.extkey.start = fileblock - blksleft;
      extoverflow.extkey.type = node->compressed ? 0xff : 0;
      if (holy_hfsplus_btree_search (&node->data->extoverflow_tree,
				     &extoverflow,
				     holy_hfsplus_cmp_extkey, &nnode, &ptr)
	  || !nnode)
	{
	  holy_error (holy_ERR_READ_ERROR,
		      "no block found for the file id 0x%x and the block offset 0x%x",
		      node->fileid, fileblock);
	  break;
	}

      /* The extent overflow file has 8 extents right after the key.  */
      key = (struct holy_hfsplus_extkey *)
	holy_hfsplus_btree_recptr (&node->data->extoverflow_tree, nnode, ptr);
      extents = (struct holy_hfsplus_extent *) (key + 1);

      /* The block wasn't found.  Perhaps the next iteration will find
	 it.  The last block we found is stored in BLKSLEFT now.  */
    }

  holy_free (nnode);

  /* Too bad, you lose.  */
  return -1;
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
holy_ssize_t
holy_hfsplus_read_file (holy_fshelp_node_t node,
			holy_disk_read_hook_t read_hook, void *read_hook_data,
			holy_off_t pos, holy_size_t len, char *buf)
{
  return holy_fshelp_read_file (node->data->disk, node,
				read_hook, read_hook_data,
				pos, len, buf, holy_hfsplus_read_block,
				node->size,
				node->data->log2blksize - holy_DISK_SECTOR_BITS,
				node->data->embedded_offset);
}

static struct holy_hfsplus_data *
holy_hfsplus_mount (holy_disk_t disk)
{
  struct holy_hfsplus_data *data;
  struct holy_hfsplus_btheader header;
  struct holy_hfsplus_btnode node;
  holy_uint16_t magic;
  union {
    struct holy_hfs_sblock hfs;
    struct holy_hfsplus_volheader hfsplus;
  } volheader;

  data = holy_malloc (sizeof (*data));
  if (!data)
    return 0;

  data->disk = disk;

  /* Read the bootblock.  */
  holy_disk_read (disk, holy_HFSPLUS_SBLOCK, 0, sizeof (volheader),
		  &volheader);
  if (holy_errno)
    goto fail;

  data->embedded_offset = 0;
  if (holy_be_to_cpu16 (volheader.hfs.magic) == holy_HFS_MAGIC)
    {
      holy_disk_addr_t extent_start;
      holy_disk_addr_t ablk_size;
      holy_disk_addr_t ablk_start;

      /* See if there's an embedded HFS+ filesystem.  */
      if (holy_be_to_cpu16 (volheader.hfs.embed_sig) != holy_HFSPLUS_MAGIC)
	{
	  holy_error (holy_ERR_BAD_FS, "not a HFS+ filesystem");
	  goto fail;
	}

      /* Calculate the offset needed to translate HFS+ sector numbers.  */
      extent_start = holy_be_to_cpu16 (volheader.hfs.embed_extent.first_block);
      ablk_size = holy_be_to_cpu32 (volheader.hfs.blksz);
      ablk_start = holy_be_to_cpu16 (volheader.hfs.first_block);
      data->embedded_offset = (ablk_start
			       + extent_start
			       * (ablk_size >> holy_DISK_SECTOR_BITS));

      holy_disk_read (disk, data->embedded_offset + holy_HFSPLUS_SBLOCK, 0,
		      sizeof (volheader), &volheader);
      if (holy_errno)
	goto fail;
    }

  /* Make sure this is an HFS+ filesystem.  XXX: Do we really support
     HFX?  */
  magic = holy_be_to_cpu16 (volheader.hfsplus.magic);
  if (((magic != holy_HFSPLUS_MAGIC) && (magic != holy_HFSPLUSX_MAGIC))
      || volheader.hfsplus.blksize == 0
      || ((volheader.hfsplus.blksize & (volheader.hfsplus.blksize - 1)) != 0)
      || holy_be_to_cpu32 (volheader.hfsplus.blksize) < holy_DISK_SECTOR_SIZE)
    {
      holy_error (holy_ERR_BAD_FS, "not a HFS+ filesystem");
      goto fail;
    }

  holy_memcpy (&data->volheader, &volheader.hfsplus,
	       sizeof (volheader.hfsplus));

  for (data->log2blksize = 0;
       (1U << data->log2blksize) < holy_be_to_cpu32 (data->volheader.blksize);
       data->log2blksize++);

  /* Make a new node for the catalog tree.  */
  data->catalog_tree.file.data = data;
  data->catalog_tree.file.fileid = holy_HFSPLUS_FILEID_CATALOG;
  data->catalog_tree.file.compressed = 0;
  holy_memcpy (&data->catalog_tree.file.extents,
	       data->volheader.catalog_file.extents,
	       sizeof data->volheader.catalog_file.extents);
  data->catalog_tree.file.size =
    holy_be_to_cpu64 (data->volheader.catalog_file.size);

  data->attr_tree.file.data = data;
  data->attr_tree.file.fileid = holy_HFSPLUS_FILEID_ATTR;
  holy_memcpy (&data->attr_tree.file.extents,
	       data->volheader.attr_file.extents,
	       sizeof data->volheader.attr_file.extents);

  data->attr_tree.file.size =
    holy_be_to_cpu64 (data->volheader.attr_file.size);
  data->attr_tree.file.compressed = 0;

  /* Make a new node for the extent overflow file.  */
  data->extoverflow_tree.file.data = data;
  data->extoverflow_tree.file.fileid = holy_HFSPLUS_FILEID_OVERFLOW;
  data->extoverflow_tree.file.compressed = 0;
  holy_memcpy (&data->extoverflow_tree.file.extents,
	       data->volheader.extents_file.extents,
	       sizeof data->volheader.catalog_file.extents);

  data->extoverflow_tree.file.size =
    holy_be_to_cpu64 (data->volheader.extents_file.size);

  /* Read the essential information about the trees.  */
  if (holy_hfsplus_read_file (&data->catalog_tree.file, 0, 0,
			      sizeof (struct holy_hfsplus_btnode),
			      sizeof (header), (char *) &header) <= 0)
    goto fail;

  data->catalog_tree.root = holy_be_to_cpu32 (header.root);
  data->catalog_tree.nodesize = holy_be_to_cpu16 (header.nodesize);
  data->case_sensitive = ((magic == holy_HFSPLUSX_MAGIC) &&
			  (header.key_compare == holy_HFSPLUSX_BINARYCOMPARE));

  if (data->catalog_tree.nodesize < 2)
    goto fail;

  if (holy_hfsplus_read_file (&data->extoverflow_tree.file, 0, 0,
			      sizeof (struct holy_hfsplus_btnode),
			      sizeof (header), (char *) &header) <= 0)
    goto fail;

  data->extoverflow_tree.root = holy_be_to_cpu32 (header.root);

  if (holy_hfsplus_read_file (&data->extoverflow_tree.file, 0, 0, 0,
			      sizeof (node), (char *) &node) <= 0)
    goto fail;

  data->extoverflow_tree.root = holy_be_to_cpu32 (header.root);
  data->extoverflow_tree.nodesize = holy_be_to_cpu16 (header.nodesize);

  if (data->extoverflow_tree.nodesize < 2)
    goto fail;

  if (holy_hfsplus_read_file (&data->attr_tree.file, 0, 0,
			      sizeof (struct holy_hfsplus_btnode),
			      sizeof (header), (char *) &header) <= 0)
    {
      holy_errno = 0;
      data->attr_tree.root = 0;
      data->attr_tree.nodesize = 0;
    }
  else
    {
      data->attr_tree.root = holy_be_to_cpu32 (header.root);
      data->attr_tree.nodesize = holy_be_to_cpu16 (header.nodesize);
    }

  data->dirroot.data = data;
  data->dirroot.fileid = holy_HFSPLUS_FILEID_ROOTDIR;

  return data;

 fail:

  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a HFS+ filesystem");

  holy_free (data);
  return 0;
}

/* Compare the on disk catalog key KEYA with the catalog key we are
   looking for (KEYB).  */
static int
holy_hfsplus_cmp_catkey (struct holy_hfsplus_key *keya,
			 struct holy_hfsplus_key_internal *keyb)
{
  struct holy_hfsplus_catkey *catkey_a = &keya->catkey;
  struct holy_hfsplus_catkey_internal *catkey_b = &keyb->catkey;
  int diff;
  holy_size_t len;

  /* Safe unsigned comparison */
  holy_uint32_t aparent = holy_be_to_cpu32 (catkey_a->parent);
  if (aparent > catkey_b->parent)
    return 1;
  if (aparent < catkey_b->parent)
    return -1;

  len = holy_be_to_cpu16 (catkey_a->namelen);
  if (len > catkey_b->namelen)
    len = catkey_b->namelen;
  /* Since it's big-endian memcmp gives the same result as manually comparing
     uint16_t but may be faster.  */
  diff = holy_memcmp (catkey_a->name, catkey_b->name,
		      len * sizeof (catkey_a->name[0]));
  if (diff == 0)
    diff = holy_be_to_cpu16 (catkey_a->namelen) - catkey_b->namelen;

  return diff;
}

/* Compare the on disk catalog key KEYA with the catalog key we are
   looking for (KEYB).  */
static int
holy_hfsplus_cmp_catkey_id (struct holy_hfsplus_key *keya,
			 struct holy_hfsplus_key_internal *keyb)
{
  struct holy_hfsplus_catkey *catkey_a = &keya->catkey;
  struct holy_hfsplus_catkey_internal *catkey_b = &keyb->catkey;

  /* Safe unsigned comparison */
  holy_uint32_t aparent = holy_be_to_cpu32 (catkey_a->parent);
  if (aparent > catkey_b->parent)
    return 1;
  if (aparent < catkey_b->parent)
    return -1;

  return 0;
}

/* Compare the on disk extent overflow key KEYA with the extent
   overflow key we are looking for (KEYB).  */
static int
holy_hfsplus_cmp_extkey (struct holy_hfsplus_key *keya,
			 struct holy_hfsplus_key_internal *keyb)
{
  struct holy_hfsplus_extkey *extkey_a = &keya->extkey;
  struct holy_hfsplus_extkey_internal *extkey_b = &keyb->extkey;
  holy_uint32_t akey;

  /* Safe unsigned comparison */
  akey = holy_be_to_cpu32 (extkey_a->fileid);
  if (akey > extkey_b->fileid)
    return 1;
  if (akey < extkey_b->fileid)
    return -1;

  if (extkey_a->type > extkey_b->type)
    return 1;
  if (extkey_a->type < extkey_b->type)
    return -1;

  if (extkey_a->type > extkey_b->type)
    return +1;

  if (extkey_a->type < extkey_b->type)
    return -1;
  
  akey = holy_be_to_cpu32 (extkey_a->start);
  if (akey > extkey_b->start)
    return 1;
  if (akey < extkey_b->start)
    return -1;
  return 0;
}

static char *
holy_hfsplus_read_symlink (holy_fshelp_node_t node)
{
  char *symlink;
  holy_ssize_t numread;

  symlink = holy_malloc (node->size + 1);
  if (!symlink)
    return 0;

  numread = holy_hfsplus_read_file (node, 0, 0, 0, node->size, symlink);
  if (numread != (holy_ssize_t) node->size)
    {
      holy_free (symlink);
      return 0;
    }
  symlink[node->size] = '\0';

  return symlink;
}

static int
holy_hfsplus_btree_iterate_node (struct holy_hfsplus_btree *btree,
				 struct holy_hfsplus_btnode *first_node,
				 holy_disk_addr_t first_rec,
				 int (*hook) (void *record, void *hook_arg),
				 void *hook_arg)
{
  holy_disk_addr_t rec;
  holy_uint64_t saved_node = -1;
  holy_uint64_t node_count = 0;

  for (;;)
    {
      char *cnode = (char *) first_node;

      /* Iterate over all records in this node.  */
      for (rec = first_rec; rec < holy_be_to_cpu16 (first_node->count); rec++)
	{
	  if (hook (holy_hfsplus_btree_recptr (btree, first_node, rec), hook_arg))
	    return 1;
	}

      if (! first_node->next)
	break;

      if (node_count && first_node->next == saved_node)
	{
	  holy_error (holy_ERR_BAD_FS, "HFS+ btree loop");
	  return 0;
	}
      if (!(node_count & (node_count - 1)))
	saved_node = first_node->next;
      node_count++;

      if (holy_hfsplus_read_file (&btree->file, 0, 0,
				  (((holy_disk_addr_t)
				    holy_be_to_cpu32 (first_node->next))
				   * btree->nodesize),
				  btree->nodesize, cnode) <= 0)
	return 1;

      /* Don't skip any record in the next iteration.  */
      first_rec = 0;
    }

  return 0;
}

/* Lookup the node described by KEY in the B+ Tree BTREE.  Compare
   keys using the function COMPARE_KEYS.  When a match is found,
   return the node in MATCHNODE and a pointer to the data in this node
   in KEYOFFSET.  MATCHNODE should be freed by the caller.  */
holy_err_t
holy_hfsplus_btree_search (struct holy_hfsplus_btree *btree,
			   struct holy_hfsplus_key_internal *key,
			   int (*compare_keys) (struct holy_hfsplus_key *keya,
						struct holy_hfsplus_key_internal *keyb),
			   struct holy_hfsplus_btnode **matchnode,
			   holy_off_t *keyoffset)
{
  holy_uint64_t currnode;
  char *node;
  struct holy_hfsplus_btnode *nodedesc;
  holy_disk_addr_t rec;
  holy_uint64_t save_node;
  holy_uint64_t node_count = 0;

  if (!btree->nodesize)
    {
      *matchnode = 0;
      return 0;
    }

  node = holy_malloc (btree->nodesize);
  if (! node)
    return holy_errno;

  currnode = btree->root;
  save_node = currnode - 1;
  while (1)
    {
      int match = 0;

      if (save_node == currnode)
	{
	  holy_free (node);
	  return holy_error (holy_ERR_BAD_FS, "HFS+ btree loop");
	}
      if (!(node_count & (node_count - 1)))
	save_node = currnode;
      node_count++;

      /* Read a node.  */
      if (holy_hfsplus_read_file (&btree->file, 0, 0,
				  (holy_disk_addr_t) currnode
				  * (holy_disk_addr_t) btree->nodesize,
				  btree->nodesize, (char *) node) <= 0)
	{
	  holy_free (node);
	  return holy_error (holy_ERR_BAD_FS, "couldn't read i-node");
	}

      nodedesc = (struct holy_hfsplus_btnode *) node;

      /* Find the record in this tree.  */
      for (rec = 0; rec < holy_be_to_cpu16 (nodedesc->count); rec++)
	{
	  struct holy_hfsplus_key *currkey;
	  currkey = holy_hfsplus_btree_recptr (btree, nodedesc, rec);

	  /* The action that has to be taken depend on the type of
	     record.  */
	  if (nodedesc->type == holy_HFSPLUS_BTNODE_TYPE_LEAF
	      && compare_keys (currkey, key) == 0)
	    {
	      /* An exact match was found!  */

	      *matchnode = nodedesc;
	      *keyoffset = rec;

	      return 0;
	    }
	  else if (nodedesc->type == holy_HFSPLUS_BTNODE_TYPE_INDEX)
	    {
	      void *pointer;

	      /* The place where the key could have been found didn't
		 contain the key.  This means that the previous match
		 is the one that should be followed.  */
	      if (compare_keys (currkey, key) > 0)
		break;

	      /* Mark the last key which is lower or equal to the key
		 that we are looking for.  The last match that is
		 found will be used to locate the child which can
		 contain the record.  */
	      pointer = ((char *) currkey
			 + holy_be_to_cpu16 (currkey->keylen)
			 + 2);
	      currnode = holy_be_to_cpu32 (holy_get_unaligned32 (pointer));
	      match = 1;
	    }
	}

      /* No match is found, no record with this key exists in the
	 tree.  */
      if (! match)
	{
	  *matchnode = 0;
	  holy_free (node);
	  return 0;
	}
    }
}

struct list_nodes_ctx
{
  int ret;
  holy_fshelp_node_t dir;
  holy_fshelp_iterate_dir_hook_t hook;
  void *hook_data;
};

static int
list_nodes (void *record, void *hook_arg)
{
  struct holy_hfsplus_catkey *catkey;
  char *filename;
  int i;
  struct holy_fshelp_node *node;
  struct holy_hfsplus_catfile *fileinfo;
  enum holy_fshelp_filetype type = holy_FSHELP_UNKNOWN;
  struct list_nodes_ctx *ctx = hook_arg;

  catkey = (struct holy_hfsplus_catkey *) record;

  fileinfo =
    (struct holy_hfsplus_catfile *) ((char *) record
				     + holy_be_to_cpu16 (catkey->keylen)
				     + 2 + (holy_be_to_cpu16(catkey->keylen)
					    % 2));

  /* Stop iterating when the last directory entry is found.  */
  if (holy_be_to_cpu32 (catkey->parent) != ctx->dir->fileid)
    return 1;

  /* Determine the type of the node that is found.  */
  switch (fileinfo->type)
    {
    case holy_cpu_to_be16_compile_time (holy_HFSPLUS_FILETYPE_REG):
      {
	int mode = (holy_be_to_cpu16 (fileinfo->mode)
		    & holy_HFSPLUS_FILEMODE_MASK);

	if (mode == holy_HFSPLUS_FILEMODE_REG)
	  type = holy_FSHELP_REG;
	else if (mode == holy_HFSPLUS_FILEMODE_SYMLINK)
	  type = holy_FSHELP_SYMLINK;
	else
	  type = holy_FSHELP_UNKNOWN;
	break;
      }
    case holy_cpu_to_be16_compile_time (holy_HFSPLUS_FILETYPE_DIR):
      type = holy_FSHELP_DIR;
      break;
    case holy_cpu_to_be16_compile_time (holy_HFSPLUS_FILETYPE_DIR_THREAD):
      if (ctx->dir->fileid == 2)
	return 0;
      node = holy_malloc (sizeof (*node));
      if (!node)
	return 1;
      node->data = ctx->dir->data;
      node->mtime = 0;
      node->size = 0;
      node->fileid = holy_be_to_cpu32 (fileinfo->parentid);

      ctx->ret = ctx->hook ("..", holy_FSHELP_DIR, node, ctx->hook_data);
      return ctx->ret;
    }

  if (type == holy_FSHELP_UNKNOWN)
    return 0;

  filename = holy_malloc (holy_be_to_cpu16 (catkey->namelen)
			  * holy_MAX_UTF8_PER_UTF16 + 1);
  if (! filename)
    return 0;

  /* Make sure the byte order of the UTF16 string is correct.  */
  for (i = 0; i < holy_be_to_cpu16 (catkey->namelen); i++)
    {
      catkey->name[i] = holy_be_to_cpu16 (catkey->name[i]);

      if (catkey->name[i] == '/')
	catkey->name[i] = ':';

      /* If the name is obviously invalid, skip this node.  */
      if (catkey->name[i] == 0)
	{
	  holy_free (filename);
	  return 0;
	}
    }

  *holy_utf16_to_utf8 ((holy_uint8_t *) filename, catkey->name,
		       holy_be_to_cpu16 (catkey->namelen)) = '\0';

  /* Restore the byte order to what it was previously.  */
  for (i = 0; i < holy_be_to_cpu16 (catkey->namelen); i++)
    {
      if (catkey->name[i] == ':')
	catkey->name[i] = '/';
      catkey->name[i] = holy_be_to_cpu16 (catkey->name[i]);
    }

  /* hfs+ is case insensitive.  */
  if (! ctx->dir->data->case_sensitive)
    type |= holy_FSHELP_CASE_INSENSITIVE;

  /* A valid node is found; setup the node and call the
     callback function.  */
  node = holy_malloc (sizeof (*node));
  if (!node)
    {
      holy_free (filename);
      return 1;
    }
  node->data = ctx->dir->data;
  node->compressed = 0;
  node->cbuf = 0;
  node->compress_index = 0;

  holy_memcpy (node->extents, fileinfo->data.extents,
	       sizeof (node->extents));
  holy_memcpy (node->resource_extents, fileinfo->resource.extents,
	       sizeof (node->resource_extents));
  node->mtime = holy_be_to_cpu32 (fileinfo->mtime) - 2082844800;
  node->size = holy_be_to_cpu64 (fileinfo->data.size);
  node->resource_size = holy_be_to_cpu64 (fileinfo->resource.size);
  node->fileid = holy_be_to_cpu32 (fileinfo->fileid);

  ctx->ret = ctx->hook (filename, type, node, ctx->hook_data);

  holy_free (filename);

  return ctx->ret;
}

static int
holy_hfsplus_iterate_dir (holy_fshelp_node_t dir,
			  holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  struct list_nodes_ctx ctx =
  {
    .ret = 0,
    .dir = dir,
    .hook = hook,
    .hook_data = hook_data
  };

  struct holy_hfsplus_key_internal intern;
  struct holy_hfsplus_btnode *node = NULL;
  holy_disk_addr_t ptr = 0;

  {
    struct holy_fshelp_node *fsnode;
    fsnode = holy_malloc (sizeof (*fsnode));
    if (!fsnode)
      return 1;
    *fsnode = *dir;
    if (hook (".", holy_FSHELP_DIR, fsnode, hook_data))
      return 1;
  }

  /* Create a key that points to the first entry in the directory.  */
  intern.catkey.parent = dir->fileid;
  intern.catkey.name = 0;
  intern.catkey.namelen = 0;

  /* First lookup the first entry.  */
  if (holy_hfsplus_btree_search (&dir->data->catalog_tree, &intern,
				 holy_hfsplus_cmp_catkey, &node, &ptr)
      || !node)
    return 0;

  /* Iterate over all entries in this directory.  */
  holy_hfsplus_btree_iterate_node (&dir->data->catalog_tree, node, ptr,
				   list_nodes, &ctx);

  holy_free (node);

  return ctx.ret;
}

/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_hfsplus_open (struct holy_file *file, const char *name)
{
  struct holy_hfsplus_data *data;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_hfsplus_mount (file->device->disk);
  if (!data)
    goto fail;

  holy_fshelp_find_file (name, &data->dirroot, &fdiro,
			 holy_hfsplus_iterate_dir,
			 holy_hfsplus_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  if (holy_hfsplus_open_compressed)
    {
      holy_err_t err;
      err = holy_hfsplus_open_compressed (fdiro);
      if (err)
	goto fail;
    }

  file->size = fdiro->size;
  data->opened_file = *fdiro;
  holy_free (fdiro);

  file->data = data;
  file->offset = 0;

  return 0;

 fail:
  if (data && fdiro != &data->dirroot)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_err_t
holy_hfsplus_close (holy_file_t file)
{
  struct holy_hfsplus_data *data =
    (struct holy_hfsplus_data *) file->data;

  holy_free (data->opened_file.cbuf);
  holy_free (data->opened_file.compress_index);

  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}

/* Read LEN bytes data from FILE into BUF.  */
static holy_ssize_t
holy_hfsplus_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_hfsplus_data *data =
    (struct holy_hfsplus_data *) file->data;

  data->opened_file.file = file;

  if (holy_hfsplus_read_compressed && data->opened_file.compressed)
    return holy_hfsplus_read_compressed (&data->opened_file,
					 file->offset, len, buf);

  return holy_hfsplus_read_file (&data->opened_file,
				 file->read_hook, file->read_hook_data,
				 file->offset, len, buf);
}

/* Context for holy_hfsplus_dir.  */
struct holy_hfsplus_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_hfsplus_dir.  */
static int
holy_hfsplus_dir_iter (const char *filename,
		       enum holy_fshelp_filetype filetype,
		       holy_fshelp_node_t node, void *data)
{
  struct holy_hfsplus_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = 1;
  info.mtime = node->mtime;
  info.inodeset = 1;
  info.inode = node->fileid;
  info.case_insensitive = !! (filetype & holy_FSHELP_CASE_INSENSITIVE);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_hfsplus_dir (holy_device_t device, const char *path,
		  holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_hfsplus_dir_ctx ctx = { hook, hook_data };
  struct holy_hfsplus_data *data = 0;
  struct holy_fshelp_node *fdiro = 0;

  holy_dl_ref (my_mod);

  data = holy_hfsplus_mount (device->disk);
  if (!data)
    goto fail;

  /* Find the directory that should be opened.  */
  holy_fshelp_find_file (path, &data->dirroot, &fdiro,
			 holy_hfsplus_iterate_dir,
			 holy_hfsplus_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  /* Iterate over all entries in this directory.  */
  holy_hfsplus_iterate_dir (fdiro, holy_hfsplus_dir_iter, &ctx);

 fail:
  if (data && fdiro != &data->dirroot)
    holy_free (fdiro);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


static holy_err_t
holy_hfsplus_label (holy_device_t device, char **label)
{
  struct holy_hfsplus_data *data;
  holy_disk_t disk = device->disk;
  struct holy_hfsplus_catkey *catkey;
  int i, label_len;
  struct holy_hfsplus_key_internal intern;
  struct holy_hfsplus_btnode *node = NULL;
  holy_disk_addr_t ptr = 0;

  *label = 0;

  data = holy_hfsplus_mount (disk);
  if (!data)
    return holy_errno;

  /* Create a key that points to the label.  */
  intern.catkey.parent = 1;
  intern.catkey.name = 0;
  intern.catkey.namelen = 0;

  /* First lookup the first entry.  */
  if (holy_hfsplus_btree_search (&data->catalog_tree, &intern,
				 holy_hfsplus_cmp_catkey_id, &node, &ptr)
      || !node)
    {
      holy_free (data);
      return 0;
    }

  catkey = (struct holy_hfsplus_catkey *)
    holy_hfsplus_btree_recptr (&data->catalog_tree, node, ptr);

  label_len = holy_be_to_cpu16 (catkey->namelen);
  for (i = 0; i < label_len; i++)
    {
      catkey->name[i] = holy_be_to_cpu16 (catkey->name[i]);

      /* If the name is obviously invalid, skip this node.  */
      if (catkey->name[i] == 0)
	return 0;
    }

  *label = holy_malloc (label_len * holy_MAX_UTF8_PER_UTF16 + 1);
  if (! *label)
    return holy_errno;

  *holy_utf16_to_utf8 ((holy_uint8_t *) (*label), catkey->name,
		       label_len) = '\0';

  holy_free (node);
  holy_free (data);

  return holy_ERR_NONE;
}

/* Get mtime.  */
static holy_err_t
holy_hfsplus_mtime (holy_device_t device, holy_int32_t *tm)
{
  struct holy_hfsplus_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_hfsplus_mount (disk);
  if (!data)
    *tm = 0;
  else
    *tm = holy_be_to_cpu32 (data->volheader.utime) - 2082844800;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;

}

static holy_err_t
holy_hfsplus_uuid (holy_device_t device, char **uuid)
{
  struct holy_hfsplus_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_hfsplus_mount (disk);
  if (data)
    {
      *uuid = holy_xasprintf ("%016llx",
			     (unsigned long long)
			     holy_be_to_cpu64 (data->volheader.num_serial));
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}



static struct holy_fs holy_hfsplus_fs =
  {
    .name = "hfsplus",
    .dir = holy_hfsplus_dir,
    .open = holy_hfsplus_open,
    .read = holy_hfsplus_read,
    .close = holy_hfsplus_close,
    .label = holy_hfsplus_label,
    .mtime = holy_hfsplus_mtime,
    .uuid = holy_hfsplus_uuid,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(hfsplus)
{
  holy_fs_register (&holy_hfsplus_fs);
  my_mod = mod;
}

holy_MOD_FINI(hfsplus)
{
  holy_fs_unregister (&holy_hfsplus_fs);
}
