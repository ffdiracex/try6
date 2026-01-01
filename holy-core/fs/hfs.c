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
#include <holy/hfs.h>
#include <holy/i18n.h>
#include <holy/fshelp.h>

holy_MOD_LICENSE ("GPLv2+");

#define	holy_HFS_SBLOCK		2
#define holy_HFS_EMBED_HFSPLUS_SIG 0x482B

#define holy_HFS_BLKS		(data->blksz >> 9)

#define holy_HFS_NODE_LEAF	0xFF

/* The two supported filesystems a record can have.  */
enum
  {
    holy_HFS_FILETYPE_DIR = 1,
    holy_HFS_FILETYPE_FILE = 2
  };

/* Catalog node ID (CNID).  */
enum holy_hfs_cnid_type
  {
    holy_HFS_CNID_ROOT_PARENT = 1,
    holy_HFS_CNID_ROOT = 2,
    holy_HFS_CNID_EXT = 3,
    holy_HFS_CNID_CAT = 4,
    holy_HFS_CNID_BAD = 5
  };

/* A node descriptor.  This is the header of every node.  */
struct holy_hfs_node
{
  holy_uint32_t next;
  holy_uint32_t prev;
  holy_uint8_t type;
  holy_uint8_t level;
  holy_uint16_t reccnt;
  holy_uint16_t unused;
} holy_PACKED;

/* The head of the B*-Tree.  */
struct holy_hfs_treeheader
{
  holy_uint16_t tree_depth;
  /* The number of the first node.  */
  holy_uint32_t root_node;
  holy_uint32_t leaves;
  holy_uint32_t first_leaf;
  holy_uint32_t last_leaf;
  holy_uint16_t node_size;
  holy_uint16_t key_size;
  holy_uint32_t nodes;
  holy_uint32_t free_nodes;
  holy_uint8_t unused[76];
} holy_PACKED;

/* The state of a mounted HFS filesystem.  */
struct holy_hfs_data
{
  struct holy_hfs_sblock sblock;
  holy_disk_t disk;
  holy_hfs_datarecord_t extents;
  int fileid;
  int size;
  int ext_root;
  int ext_size;
  int cat_root;
  int cat_size;
  int blksz;
  int log2_blksz;
  int rootdir;
};

/* The key as used on disk in a catalog tree.  This is used to lookup
   file/directory nodes by parent directory ID and filename.  */
struct holy_hfs_catalog_key
{
  holy_uint8_t unused;
  holy_uint32_t parent_dir;

  /* Filename length.  */
  holy_uint8_t strlen;

  /* Filename.  */
  holy_uint8_t str[31];
} holy_PACKED;

/* The key as used on disk in a extent overflow tree.  Using this key
   the extents can be looked up using a fileid and logical start block
   as index.  */
struct holy_hfs_extent_key
{
  /* The kind of fork.  This is used to store meta information like
     icons, attributes, etc.  We will only use the datafork, which is
     0.  */
  holy_uint8_t forktype;
  holy_uint32_t fileid;
  holy_uint16_t first_block;
} holy_PACKED;

/* A directory record.  This is used to find out the directory ID.  */
struct holy_hfs_dirrec
{
  /* For a directory, type == 1.  */
  holy_uint8_t type;
  holy_uint8_t unused[5];
  holy_uint32_t dirid;
  holy_uint32_t ctime;
  holy_uint32_t mtime;
} holy_PACKED;

/* Information about a file.  */
struct holy_hfs_filerec
{
  /* For a file, type == 2.  */
  holy_uint8_t type;
  holy_uint8_t unused[19];
  holy_uint32_t fileid;
  holy_uint8_t unused2[2];
  holy_uint32_t size;
  holy_uint8_t unused3[18];
  holy_uint32_t mtime;
  holy_uint8_t unused4[22];

  /* The first 3 extents of the file.  The other extents can be found
     in the extent overflow file.  */
  holy_hfs_datarecord_t extents;
} holy_PACKED;

/* A record descriptor, both key and data, used to pass to call back
   functions.  */
struct holy_hfs_record
{
  void *key;
  holy_size_t keylen;
  void *data;
  holy_size_t datalen;
};

static holy_dl_t my_mod;

static int holy_hfs_find_node (struct holy_hfs_data *, char *,
			       holy_uint32_t, int, char *, holy_size_t);

/* Find block BLOCK of the file FILE in the mounted UFS filesystem
   DATA.  The first 3 extents are described by DAT.  If cache is set,
   using caching to improve non-random reads.  */
static unsigned int
holy_hfs_block (struct holy_hfs_data *data, holy_hfs_datarecord_t dat,
		int file, int block, int cache)
{
  holy_hfs_datarecord_t dr;
  int pos = 0;
  struct holy_hfs_extent_key key;

  int tree = 0;
  static int cache_file = 0;
  static int cache_pos = 0;
  static holy_hfs_datarecord_t cache_dr;

  holy_memcpy (dr, dat, sizeof (dr));

  key.forktype = 0;
  key.fileid = holy_cpu_to_be32 (file);

  if (cache && cache_file == file  && block > cache_pos)
    {
      pos = cache_pos;
      key.first_block = holy_cpu_to_be16 (pos);
      holy_memcpy (dr, cache_dr, sizeof (cache_dr));
    }

  for (;;)
    {
      int i;

      /* Try all 3 extents.  */
      for (i = 0; i < 3; i++)
	{
	  /* Check if the block is stored in this extent.  */
	  if (holy_be_to_cpu16 (dr[i].count) + pos > block)
	    {
	      int first = holy_be_to_cpu16 (dr[i].first_block);

	      /* If the cache is enabled, store the current position
		 in the tree.  */
	      if (tree && cache)
		{
		  cache_file = file;
		  cache_pos = pos;
		  holy_memcpy (cache_dr, dr, sizeof (cache_dr));
		}

	      return (holy_be_to_cpu16 (data->sblock.first_block)
		      + (first + block - pos) * holy_HFS_BLKS);
	    }

	  /* Try the next extent.  */
	  pos += holy_be_to_cpu16 (dr[i].count);
	}

      /* Lookup the block in the extent overflow file.  */
      key.first_block = holy_cpu_to_be16 (pos);
      tree = 1;
      holy_hfs_find_node (data, (char *) &key, data->ext_root,
			  1, (char *) &dr, sizeof (dr));
      if (holy_errno)
	return 0;
    }
}


/* Read LEN bytes from the file described by DATA starting with byte
   POS.  Return the amount of read bytes in READ.  */
static holy_ssize_t
holy_hfs_read_file (struct holy_hfs_data *data,
		    holy_disk_read_hook_t read_hook, void *read_hook_data,
		    holy_uint32_t pos, holy_size_t len, char *buf)
{
  holy_off_t i;
  holy_off_t blockcnt;

  /* Files are at most 2G/4G - 1 bytes on hfs. Avoid 64-bit division.
     Moreover len > 0 as checked in upper layer.  */
  blockcnt = (len + pos - 1) / data->blksz + 1;

  for (i = pos / data->blksz; i < blockcnt; i++)
    {
      holy_disk_addr_t blknr;
      holy_off_t blockoff;
      holy_off_t blockend = data->blksz;

      int skipfirst = 0;

      blockoff = pos % data->blksz;

      blknr = holy_hfs_block (data, data->extents, data->fileid, i, 1);
      if (holy_errno)
	return -1;

      /* Last block.  */
      if (i == blockcnt - 1)
	{
	  blockend = (len + pos) % data->blksz;

	  /* The last portion is exactly EXT2_BLOCK_SIZE (data).  */
	  if (! blockend)
	    blockend = data->blksz;
	}

      /* First block.  */
      if (i == pos / data->blksz)
	{
	  skipfirst = blockoff;
	  blockend -= skipfirst;
	}

      /* If the block number is 0 this block is not stored on disk but
	 is zero filled instead.  */
      if (blknr)
	{
	  data->disk->read_hook = read_hook;
	  data->disk->read_hook_data = read_hook_data;
	  holy_disk_read (data->disk, blknr, skipfirst,
			  blockend, buf);
	  data->disk->read_hook = 0;
	  if (holy_errno)
	    return -1;
	}

      buf += data->blksz - skipfirst;
    }

  return len;
}


/* Mount the filesystem on the disk DISK.  */
static struct holy_hfs_data *
holy_hfs_mount (holy_disk_t disk)
{
  struct holy_hfs_data *data;
  struct holy_hfs_catalog_key key;
  struct holy_hfs_dirrec dir;
  int first_block;

  struct
  {
    struct holy_hfs_node node;
    struct holy_hfs_treeheader head;
  } treehead;

  data = holy_malloc (sizeof (struct holy_hfs_data));
  if (!data)
    return 0;

  /* Read the superblock.  */
  if (holy_disk_read (disk, holy_HFS_SBLOCK, 0,
		      sizeof (struct holy_hfs_sblock), &data->sblock))
    goto fail;

  /* Check if this is a HFS filesystem.  */
  if (holy_be_to_cpu16 (data->sblock.magic) != holy_HFS_MAGIC
      || data->sblock.blksz == 0
      || (data->sblock.blksz & holy_cpu_to_be32_compile_time (0xc00001ff)))
    {
      holy_error (holy_ERR_BAD_FS, "not an HFS filesystem");
      goto fail;
    }

  /* Check if this is an embedded HFS+ filesystem.  */
  if (holy_be_to_cpu16 (data->sblock.embed_sig) == holy_HFS_EMBED_HFSPLUS_SIG)
    {
      holy_error (holy_ERR_BAD_FS, "embedded HFS+ filesystem");
      goto fail;
    }

  data->blksz = holy_be_to_cpu32 (data->sblock.blksz);
  data->disk = disk;

  /* Lookup the root node of the extent overflow tree.  */
  first_block = ((holy_be_to_cpu16 (data->sblock.extent_recs[0].first_block)
		  * holy_HFS_BLKS)
		 + holy_be_to_cpu16 (data->sblock.first_block));

  if (holy_disk_read (data->disk, first_block, 0,
		      sizeof (treehead), &treehead))
    goto fail;
  data->ext_root = holy_be_to_cpu32 (treehead.head.root_node);
  data->ext_size = holy_be_to_cpu16 (treehead.head.node_size);

  /* Lookup the root node of the catalog tree.  */
  first_block = ((holy_be_to_cpu16 (data->sblock.catalog_recs[0].first_block)
		  * holy_HFS_BLKS)
		 + holy_be_to_cpu16 (data->sblock.first_block));
  if (holy_disk_read (data->disk, first_block, 0,
		      sizeof (treehead), &treehead))
    goto fail;
  data->cat_root = holy_be_to_cpu32 (treehead.head.root_node);
  data->cat_size = holy_be_to_cpu16 (treehead.head.node_size);

  if (data->cat_size == 0
      || data->blksz < data->cat_size
      || data->blksz < data->ext_size)
    goto fail;

  /* Lookup the root directory node in the catalog tree using the
     volume name.  */
  key.parent_dir = holy_cpu_to_be32_compile_time (1);
  key.strlen = data->sblock.volname[0];
  holy_strcpy ((char *) key.str, (char *) (data->sblock.volname + 1));

  if (holy_hfs_find_node (data, (char *) &key, data->cat_root,
			  0, (char *) &dir, sizeof (dir)) == 0)
    {
      holy_error (holy_ERR_BAD_FS, "cannot find the HFS root directory");
      goto fail;
    }

  if (holy_errno)
    goto fail;

  data->rootdir = holy_be_to_cpu32 (dir.dirid);

  return data;
 fail:
  holy_free (data);

  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a HFS filesystem");

  return 0;
}

/* Compare the K1 and K2 catalog file keys using HFS character ordering.  */
static int
holy_hfs_cmp_catkeys (const struct holy_hfs_catalog_key *k1,
		      const struct holy_hfs_catalog_key *k2)
{
  /* Taken from hfsutils 3.2.6 and converted to a readable form */
  static const unsigned char hfs_charorder[256] = {
    [0x00] = 0,
    [0x01] = 1,
    [0x02] = 2,
    [0x03] = 3,
    [0x04] = 4,
    [0x05] = 5,
    [0x06] = 6,
    [0x07] = 7,
    [0x08] = 8,
    [0x09] = 9,
    [0x0A] = 10,
    [0x0B] = 11,
    [0x0C] = 12,
    [0x0D] = 13,
    [0x0E] = 14,
    [0x0F] = 15,
    [0x10] = 16,
    [0x11] = 17,
    [0x12] = 18,
    [0x13] = 19,
    [0x14] = 20,
    [0x15] = 21,
    [0x16] = 22,
    [0x17] = 23,
    [0x18] = 24,
    [0x19] = 25,
    [0x1A] = 26,
    [0x1B] = 27,
    [0x1C] = 28,
    [0x1D] = 29,
    [0x1E] = 30,
    [0x1F] = 31,
    [' '] = 32,		[0xCA] = 32,
    ['!'] = 33,
    ['"'] = 34,
    [0xD2] = 35,
    [0xD3] = 36,
    [0xC7] = 37,
    [0xC8] = 38,
    ['#'] = 39,
    ['$'] = 40,
    ['%'] = 41,
    ['&'] = 42,
    ['\''] = 43,
    [0xD4] = 44,
    [0xD5] = 45,
    ['('] = 46,
    [')'] = 47,
    ['*'] = 48,
    ['+'] = 49,
    [','] = 50,
    ['-'] = 51,
    ['.'] = 52,
    ['/'] = 53,
    ['0'] = 54,
    ['1'] = 55,
    ['2'] = 56,
    ['3'] = 57,
    ['4'] = 58,
    ['5'] = 59,
    ['6'] = 60,
    ['7'] = 61,
    ['8'] = 62,
    ['9'] = 63,
    [':'] = 64,
    [';'] = 65,
    ['<'] = 66,
    ['='] = 67,
    ['>'] = 68,
    ['?'] = 69,
    ['@'] = 70,
    ['A'] = 71,		['a'] = 71,
    [0x88] = 72,	[0xCB] = 72,
    [0x80] = 73,	[0x8A] = 73,
    [0x8B] = 74,	[0xCC] = 74,
    [0x81] = 75,	[0x8C] = 75,
    [0xAE] = 76,	[0xBE] = 76,
    ['`'] = 77,
    [0x87] = 78,
    [0x89] = 79,
    [0xBB] = 80,
    ['B'] = 81,		['b'] = 81,
    ['C'] = 82,		['c'] = 82,
    [0x82] = 83,	[0x8D] = 83,
    ['D'] = 84,		['d'] = 84,
    ['E'] = 85,		['e'] = 85,
    [0x83] = 86,	[0x8E] = 86,
    [0x8F] = 87,
    [0x90] = 88,
    [0x91] = 89,
    ['F'] = 90,		['f'] = 90,
    ['G'] = 91,		['g'] = 91,
    ['H'] = 92,		['h'] = 92,
    ['I'] = 93,		['i'] = 93,
    [0x92] = 94,
    [0x93] = 95,
    [0x94] = 96,
    [0x95] = 97,
    ['J'] = 98,		['j'] = 98,
    ['K'] = 99,		['k'] = 99,
    ['L'] = 100,	['l'] = 100,
    ['M'] = 101,	['m'] = 101,
    ['N'] = 102,	['n'] = 102,
    [0x84] = 103,	[0x96] = 103,
    ['O'] = 104,	['o'] = 104,
    [0x85] = 105,	[0x9A] = 105,
    [0x9B] = 106,	[0xCD] = 106,
    [0xAF] = 107,	[0xBF] = 107,
    [0xCE] = 108,	[0xCF] = 108,
    [0x97] = 109,
    [0x98] = 110,
    [0x99] = 111,
    [0xBC] = 112,
    ['P'] = 113,	['p'] = 113,
    ['Q'] = 114,	['q'] = 114,
    ['R'] = 115,	['r'] = 115,
    ['S'] = 116,	['s'] = 116,
    [0xA7] = 117,
    ['T'] = 118,	['t'] = 118,
    ['U'] = 119,	['u'] = 119,
    [0x86] = 120,	[0x9F] = 120,
    [0x9C] = 121,
    [0x9D] = 122,
    [0x9E] = 123,
    ['V'] = 124,	['v'] = 124,
    ['W'] = 125,	['w'] = 125,
    ['X'] = 126,	['x'] = 126,
    ['Y'] = 127,	['y'] = 127,
    [0xD8] = 128,
    ['Z'] = 129,	['z'] = 129,
    ['['] = 130,
    ['\\'] = 131,
    [']'] = 132,
    ['^'] = 133,
    ['_'] = 134,
    ['{'] = 135,
    ['|'] = 136,
    ['}'] = 137,
    ['~'] = 138,
    [0x7F] = 139,
    [0xA0] = 140,
    [0xA1] = 141,
    [0xA2] = 142,
    [0xA3] = 143,
    [0xA4] = 144,
    [0xA5] = 145,
    [0xA6] = 146,
    [0xA8] = 147,
    [0xA9] = 148,
    [0xAA] = 149,
    [0xAB] = 150,
    [0xAC] = 151,
    [0xAD] = 152,
    [0xB0] = 153,
    [0xB1] = 154,
    [0xB2] = 155,
    [0xB3] = 156,
    [0xB4] = 157,
    [0xB5] = 158,
    [0xB6] = 159,
    [0xB7] = 160,
    [0xB8] = 161,
    [0xB9] = 162,
    [0xBA] = 163,
    [0xBD] = 164,
    [0xC0] = 165,
    [0xC1] = 166,
    [0xC2] = 167,
    [0xC3] = 168,
    [0xC4] = 169,
    [0xC5] = 170,
    [0xC6] = 171,
    [0xC9] = 172,
    [0xD0] = 173,
    [0xD1] = 174,
    [0xD6] = 175,
    [0xD7] = 176,
    [0xD9] = 177,
    [0xDA] = 178,
    [0xDB] = 179,
    [0xDC] = 180,
    [0xDD] = 181,
    [0xDE] = 182,
    [0xDF] = 183,
    [0xE0] = 184,
    [0xE1] = 185,
    [0xE2] = 186,
    [0xE3] = 187,
    [0xE4] = 188,
    [0xE5] = 189,
    [0xE6] = 190,
    [0xE7] = 191,
    [0xE8] = 192,
    [0xE9] = 193,
    [0xEA] = 194,
    [0xEB] = 195,
    [0xEC] = 196,
    [0xED] = 197,
    [0xEE] = 198,
    [0xEF] = 199,
    [0xF0] = 200,
    [0xF1] = 201,
    [0xF2] = 202,
    [0xF3] = 203,
    [0xF4] = 204,
    [0xF5] = 205,
    [0xF6] = 206,
    [0xF7] = 207,
    [0xF8] = 208,
    [0xF9] = 209,
    [0xFA] = 210,
    [0xFB] = 211,
    [0xFC] = 212,
    [0xFD] = 213,
    [0xFE] = 214,
    [0xFF] = 215,
  };
  int i;
  int cmp;
  int minlen = (k1->strlen < k2->strlen) ? k1->strlen : k2->strlen;

  cmp = (holy_be_to_cpu32 (k1->parent_dir) - holy_be_to_cpu32 (k2->parent_dir));
  if (cmp != 0)
    return cmp;

  for (i = 0; i < minlen; i++)
    {
      cmp = (hfs_charorder[k1->str[i]] - hfs_charorder[k2->str[i]]);
      if (cmp != 0)
	return cmp;
    }

  /* Shorter strings precede long ones.  */
  return (k1->strlen - k2->strlen);
}


/* Compare the K1 and K2 extent overflow file keys.  */
static int
holy_hfs_cmp_extkeys (const struct holy_hfs_extent_key *k1,
		      const struct holy_hfs_extent_key *k2)
{
  int cmp = k1->forktype - k2->forktype;
  if (cmp == 0)
    cmp = holy_be_to_cpu32 (k1->fileid) - holy_be_to_cpu32 (k2->fileid);
  if (cmp == 0)
    cmp = (holy_be_to_cpu16 (k1->first_block)
	   - holy_be_to_cpu16 (k2->first_block));
  return cmp;
}


/* Iterate the records in the node with index IDX in the mounted HFS
   filesystem DATA.  This node holds data of the type TYPE (0 =
   catalog node, 1 = extent overflow node).  If this is set, continue
   iterating to the next node.  For every records, call NODE_HOOK.  */
static holy_err_t
holy_hfs_iterate_records (struct holy_hfs_data *data, int type, int idx,
			  int this, int (*node_hook) (struct holy_hfs_node *hnd,
						      struct holy_hfs_record *,
						      void *hook_arg),
			  void *hook_arg)
{
  holy_size_t nodesize = type == 0 ? data->cat_size : data->ext_size;

  union node_union
  {
    struct holy_hfs_node node;
    char rawnode[0];
    holy_uint16_t offsets[0];
  } *node;

  if (nodesize < sizeof (struct holy_hfs_node))
    nodesize = sizeof (struct holy_hfs_node);

  node = holy_malloc (nodesize);
  if (!node)
    return holy_errno;

  do
    {
      int i;
      struct holy_hfs_extent *dat;
      int blk;
      holy_uint16_t reccnt;

      dat = (struct holy_hfs_extent *) (type == 0
					? (&data->sblock.catalog_recs)
					: (&data->sblock.extent_recs));

      /* Read the node into memory.  */
      blk = holy_hfs_block (data, dat,
                            (type == 0) ? holy_HFS_CNID_CAT : holy_HFS_CNID_EXT,
			    idx / (data->blksz / nodesize), 0);
      blk += (idx % (data->blksz / nodesize));

      if (holy_errno || holy_disk_read (data->disk, blk, 0,
					nodesize, node))
	{
	  holy_free (node);
	  return holy_errno;
	}

      reccnt = holy_be_to_cpu16 (node->node.reccnt);
      if (reccnt > (nodesize >> 1))
	reccnt = (nodesize >> 1);

      /* Iterate over all records in this node.  */
      for (i = 0; i < reccnt; i++)
	{
	  int pos = (nodesize >> 1) - 1 - i;
 	  struct pointer
	  {
	    holy_uint8_t keylen;
	    holy_uint8_t key;
	  } holy_PACKED *pnt;
	  holy_uint16_t off = holy_be_to_cpu16 (node->offsets[pos]);
	  if (off > nodesize - sizeof(*pnt))
	    continue;
	  pnt = (struct pointer *) (off + node->rawnode);
	  if (nodesize < (holy_size_t) off + pnt->keylen + 1)
	    continue;

	  struct holy_hfs_record rec =
	    {
	      &pnt->key,
	      pnt->keylen,
	      &pnt->key + pnt->keylen +(pnt->keylen + 1) % 2,
	      nodesize - off - pnt->keylen - 1
	    };

	  if (node_hook (&node->node, &rec, hook_arg))
	    {
	      holy_free (node);
	      return 0;
	    }
	}

      idx = holy_be_to_cpu32 (node->node.next);
    } while (idx && this);
  holy_free (node);
  return 0;
}

struct holy_hfs_find_node_node_found_ctx
{
  int found;
  int isleaf;
  int done;
  int type;
  const char *key;
  char *datar;
  holy_size_t datalen;
};

static int
holy_hfs_find_node_node_found (struct holy_hfs_node *hnd, struct holy_hfs_record *rec,
			       void *hook_arg)
{
  struct holy_hfs_find_node_node_found_ctx *ctx = hook_arg;
  int cmp = 1;

  if (ctx->type == 0)
    cmp = holy_hfs_cmp_catkeys (rec->key, (const void *) ctx->key);
  else
    cmp = holy_hfs_cmp_extkeys (rec->key, (const void *) ctx->key);

  /* If the key is smaller or equal to the current node, mark the
     entry.  In case of a non-leaf mode it will be used to lookup
     the rest of the tree.  */
  if (cmp <= 0)
    ctx->found = holy_be_to_cpu32 (holy_get_unaligned32 (rec->data));
  else /* The key can not be found in the tree. */
    return 1;

  /* Check if this node is a leaf node.  */
  if (hnd->type == holy_HFS_NODE_LEAF)
    {
      ctx->isleaf = 1;

      /* Found it!!!!  */
      if (cmp == 0)
	{
	  ctx->done = 1;

	  holy_memcpy (ctx->datar, rec->data,
		       rec->datalen < ctx->datalen ? rec->datalen : ctx->datalen);
	  return 1;
	}
    }

  return 0;
}


/* Lookup a record in the mounted filesystem DATA using the key KEY.
   The index of the node on top of the tree is IDX.  The tree is of
   the type TYPE (0 = catalog node, 1 = extent overflow node).  Return
   the data in DATAR with a maximum length of DATALEN.  */
static int
holy_hfs_find_node (struct holy_hfs_data *data, char *key,
		    holy_uint32_t idx, int type, char *datar, holy_size_t datalen)
{
  struct holy_hfs_find_node_node_found_ctx ctx =
    {
      .found = -1,
      .isleaf = 0,
      .done = 0,
      .type = type,
      .key = key,
      .datar = datar,
      .datalen = datalen
    };

  do
    {
      ctx.found = -1;

      if (holy_hfs_iterate_records (data, type, idx, 0, holy_hfs_find_node_node_found, &ctx))
        return 0;

      if (ctx.found == -1)
        return 0;

      idx = ctx.found;
    } while (! ctx.isleaf);

  return ctx.done;
}

struct holy_hfs_iterate_dir_node_found_ctx
{
  holy_uint32_t dir_be;
  int found;
  int isleaf;
  holy_uint32_t next;
  int (*hook) (struct holy_hfs_record *, void *hook_arg);
  void *hook_arg;
};

static int
holy_hfs_iterate_dir_node_found (struct holy_hfs_node *hnd, struct holy_hfs_record *rec,
				 void *hook_arg)
{
  struct holy_hfs_iterate_dir_node_found_ctx *ctx = hook_arg;
  struct holy_hfs_catalog_key *ckey = rec->key;

  /* The lowest key possible with DIR as root directory.  */
  const struct holy_hfs_catalog_key key = {0, ctx->dir_be, 0, ""};

  if (holy_hfs_cmp_catkeys (rec->key, &key) <= 0)
    ctx->found = holy_be_to_cpu32 (holy_get_unaligned32 (rec->data));

  if (hnd->type == 0xFF && ckey->strlen > 0)
    {
      ctx->isleaf = 1;
      ctx->next = holy_be_to_cpu32 (hnd->next);

      /* An entry was found.  */
      if (ckey->parent_dir == ctx->dir_be)
	return ctx->hook (rec, ctx->hook_arg);
    }

  return 0;
}

static int
holy_hfs_iterate_dir_it_dir (struct holy_hfs_node *hnd __attribute ((unused)),
			     struct holy_hfs_record *rec,
			     void *hook_arg)
{
  struct holy_hfs_catalog_key *ckey = rec->key;
  struct holy_hfs_iterate_dir_node_found_ctx *ctx = hook_arg;
  
  /* Stop when the entries do not match anymore.  */
  if (ckey->parent_dir != ctx->dir_be)
    return 1;

  return ctx->hook (rec, ctx->hook_arg);
}


/* Iterate over the directory with the id DIR.  The tree is searched
   starting with the node ROOT_IDX.  For every entry in this directory
   call HOOK.  */
static holy_err_t
holy_hfs_iterate_dir (struct holy_hfs_data *data, holy_uint32_t root_idx,
		      holy_uint32_t dir, int (*hook) (struct holy_hfs_record *, void *hook_arg),
		      void *hook_arg)
{
  struct holy_hfs_iterate_dir_node_found_ctx ctx =
  {
    .dir_be = holy_cpu_to_be32 (dir),
    .found = -1,
    .isleaf = 0,
    .next = 0,
    .hook = hook,
    .hook_arg = hook_arg
  };

  do
    {
      ctx.found = -1;

      if (holy_hfs_iterate_records (data, 0, root_idx, 0, holy_hfs_iterate_dir_node_found, &ctx))
        return holy_errno;

      if (ctx.found == -1)
        return 0;

      root_idx = ctx.found;
    } while (! ctx.isleaf);

  /* If there was a matching record in this leaf node, continue the
     iteration until the last record was found.  */
  holy_hfs_iterate_records (data, 0, ctx.next, 1, holy_hfs_iterate_dir_it_dir, &ctx);
  return holy_errno;
}

#define MAX_UTF8_PER_MAC_ROMAN 3

static const char macroman[0x80][MAX_UTF8_PER_MAC_ROMAN + 1] =
  {
    /* 80 */ "\xc3\x84",
    /* 81 */ "\xc3\x85",
    /* 82 */ "\xc3\x87",
    /* 83 */ "\xc3\x89",
    /* 84 */ "\xc3\x91",
    /* 85 */ "\xc3\x96",
    /* 86 */ "\xc3\x9c",
    /* 87 */ "\xc3\xa1",
    /* 88 */ "\xc3\xa0",
    /* 89 */ "\xc3\xa2",
    /* 8A */ "\xc3\xa4",
    /* 8B */ "\xc3\xa3",
    /* 8C */ "\xc3\xa5",
    /* 8D */ "\xc3\xa7",
    /* 8E */ "\xc3\xa9",
    /* 8F */ "\xc3\xa8",
    /* 90 */ "\xc3\xaa",
    /* 91 */ "\xc3\xab",
    /* 92 */ "\xc3\xad",
    /* 93 */ "\xc3\xac",
    /* 94 */ "\xc3\xae",
    /* 95 */ "\xc3\xaf",
    /* 96 */ "\xc3\xb1",
    /* 97 */ "\xc3\xb3",
    /* 98 */ "\xc3\xb2",
    /* 99 */ "\xc3\xb4",
    /* 9A */ "\xc3\xb6",
    /* 9B */ "\xc3\xb5",
    /* 9C */ "\xc3\xba",
    /* 9D */ "\xc3\xb9",
    /* 9E */ "\xc3\xbb",
    /* 9F */ "\xc3\xbc",
    /* A0 */ "\xe2\x80\xa0",
    /* A1 */ "\xc2\xb0",
    /* A2 */ "\xc2\xa2",
    /* A3 */ "\xc2\xa3",
    /* A4 */ "\xc2\xa7",
    /* A5 */ "\xe2\x80\xa2",
    /* A6 */ "\xc2\xb6",
    /* A7 */ "\xc3\x9f",
    /* A8 */ "\xc2\xae",
    /* A9 */ "\xc2\xa9",
    /* AA */ "\xe2\x84\xa2",
    /* AB */ "\xc2\xb4",
    /* AC */ "\xc2\xa8",
    /* AD */ "\xe2\x89\xa0",
    /* AE */ "\xc3\x86",
    /* AF */ "\xc3\x98",
    /* B0 */ "\xe2\x88\x9e",
    /* B1 */ "\xc2\xb1",
    /* B2 */ "\xe2\x89\xa4",
    /* B3 */ "\xe2\x89\xa5",
    /* B4 */ "\xc2\xa5",
    /* B5 */ "\xc2\xb5",
    /* B6 */ "\xe2\x88\x82",
    /* B7 */ "\xe2\x88\x91",
    /* B8 */ "\xe2\x88\x8f",
    /* B9 */ "\xcf\x80",
    /* BA */ "\xe2\x88\xab",
    /* BB */ "\xc2\xaa",
    /* BC */ "\xc2\xba",
    /* BD */ "\xce\xa9",
    /* BE */ "\xc3\xa6",
    /* BF */ "\xc3\xb8",
    /* C0 */ "\xc2\xbf",
    /* C1 */ "\xc2\xa1",
    /* C2 */ "\xc2\xac",
    /* C3 */ "\xe2\x88\x9a",
    /* C4 */ "\xc6\x92",
    /* C5 */ "\xe2\x89\x88",
    /* C6 */ "\xe2\x88\x86",
    /* C7 */ "\xc2\xab",
    /* C8 */ "\xc2\xbb",
    /* C9 */ "\xe2\x80\xa6",
    /* CA */ "\xc2\xa0",
    /* CB */ "\xc3\x80",
    /* CC */ "\xc3\x83",
    /* CD */ "\xc3\x95",
    /* CE */ "\xc5\x92",
    /* CF */ "\xc5\x93",
    /* D0 */ "\xe2\x80\x93",
    /* D1 */ "\xe2\x80\x94",
    /* D2 */ "\xe2\x80\x9c",
    /* D3 */ "\xe2\x80\x9d",
    /* D4 */ "\xe2\x80\x98",
    /* D5 */ "\xe2\x80\x99",
    /* D6 */ "\xc3\xb7",
    /* D7 */ "\xe2\x97\x8a",
    /* D8 */ "\xc3\xbf",
    /* D9 */ "\xc5\xb8",
    /* DA */ "\xe2\x81\x84",
    /* DB */ "\xe2\x82\xac",
    /* DC */ "\xe2\x80\xb9",
    /* DD */ "\xe2\x80\xba",
    /* DE */ "\xef\xac\x81",
    /* DF */ "\xef\xac\x82",
    /* E0 */ "\xe2\x80\xa1",
    /* E1 */ "\xc2\xb7",
    /* E2 */ "\xe2\x80\x9a",
    /* E3 */ "\xe2\x80\x9e",
    /* E4 */ "\xe2\x80\xb0",
    /* E5 */ "\xc3\x82",
    /* E6 */ "\xc3\x8a",
    /* E7 */ "\xc3\x81",
    /* E8 */ "\xc3\x8b",
    /* E9 */ "\xc3\x88",
    /* EA */ "\xc3\x8d",
    /* EB */ "\xc3\x8e",
    /* EC */ "\xc3\x8f",
    /* ED */ "\xc3\x8c",
    /* EE */ "\xc3\x93",
    /* EF */ "\xc3\x94",
    /* F0 */ "\xef\xa3\xbf",
    /* F1 */ "\xc3\x92",
    /* F2 */ "\xc3\x9a",
    /* F3 */ "\xc3\x9b",
    /* F4 */ "\xc3\x99",
    /* F5 */ "\xc4\xb1",
    /* F6 */ "\xcb\x86",
    /* F7 */ "\xcb\x9c",
    /* F8 */ "\xc2\xaf",
    /* F9 */ "\xcb\x98",
    /* FA */ "\xcb\x99",
    /* FB */ "\xcb\x9a",
    /* FC */ "\xc2\xb8",
    /* FD */ "\xcb\x9d",
    /* FE */ "\xcb\x9b",
    /* FF */ "\xcb\x87",
  };

static void
macroman_to_utf8 (char *to, const holy_uint8_t *from, holy_size_t len,
		  int translate_slash)
{
  char *optr = to;
  const holy_uint8_t *iptr;

  for (iptr = from; iptr < from + len && *iptr; iptr++)
    {
      /* Translate '/' to ':' as per HFS spec.  */
      if (*iptr == '/' && translate_slash)
	{
	  *optr++ = ':';
	  continue;
	}	
      if (!(*iptr & 0x80))
	{
	  *optr++ = *iptr;
	  continue;
	}
      optr = holy_stpcpy (optr, macroman[*iptr & 0x7f]);
    }
  *optr = 0;
}

static holy_ssize_t
utf8_to_macroman (holy_uint8_t *to, const char *from)
{
  holy_uint8_t *end = to + 31;
  holy_uint8_t *optr = to;
  const char *iptr = from;
  
  while (*iptr && optr < end)
    {
      int i, clen;
      /* Translate ':' to '/' as per HFS spec.  */
      if (*iptr == ':')
	{
	  *optr++ = '/';
	  iptr++;
	  continue;
	}	
      if (!(*iptr & 0x80))
	{
	  *optr++ = *iptr++;
	  continue;
	}
      clen = 2;
      if ((*iptr & 0xf0) == 0xe0)
	clen++;
      for (i = 0; i < 0x80; i++)
	if (holy_memcmp (macroman[i], iptr, clen) == 0)
	  break;
      if (i == 0x80)
	break;
      *optr++ = i | 0x80;
      iptr += clen;
    }
  /* Too long or not encodable.  */
  if (*iptr)
    return -1;
  return optr - to;
}

union holy_hfs_anyrec {
  struct holy_hfs_filerec frec;
  struct holy_hfs_dirrec dir;
};

struct holy_fshelp_node
{
  struct holy_hfs_data *data;
  union holy_hfs_anyrec fdrec;
  holy_uint32_t inode;
};

static holy_err_t
lookup_file (holy_fshelp_node_t dir,
	     const char *name,
	     holy_fshelp_node_t *foundnode,
	     enum holy_fshelp_filetype *foundtype)
{
  struct holy_hfs_catalog_key key;
  holy_ssize_t slen;
  union holy_hfs_anyrec fdrec;

  key.parent_dir = holy_cpu_to_be32 (dir->inode);
  slen = utf8_to_macroman (key.str, name);
  if (slen < 0)
    /* Not found */
    return holy_ERR_NONE;
  key.strlen = slen;

  /* Lookup this node.  */
  if (! holy_hfs_find_node (dir->data, (char *) &key, dir->data->cat_root,
			    0, (char *) &fdrec.frec, sizeof (fdrec.frec)))
    /* Not found */
    return holy_ERR_NONE;

  *foundnode = holy_malloc (sizeof (struct holy_fshelp_node));
  if (!*foundnode)
    return holy_errno;
  
  (*foundnode)->inode = holy_be_to_cpu32 (fdrec.dir.dirid);
  (*foundnode)->fdrec = fdrec;
  (*foundnode)->data = dir->data;
  *foundtype = (fdrec.frec.type == holy_HFS_FILETYPE_DIR) ? holy_FSHELP_DIR : holy_FSHELP_REG;
  return holy_ERR_NONE;
}

/* Find a file or directory with the pathname PATH in the filesystem
   DATA.  Return the file record in RETDATA when it is non-zero.
   Return the directory number in RETINODE when it is non-zero.  */
static holy_err_t
holy_hfs_find_dir (struct holy_hfs_data *data, const char *path,
		   holy_fshelp_node_t *found,
		   enum holy_fshelp_filetype exptype)
{
  struct holy_fshelp_node root = {
    .data = data,
    .inode = data->rootdir,
    .fdrec = {
      .frec = {
	.type = holy_HFS_FILETYPE_DIR
      }
    }
  };
  holy_err_t err;

  err = holy_fshelp_find_file_lookup (path, &root, found, lookup_file, NULL, exptype);

  if (&root == *found)
    {
      *found = holy_malloc (sizeof (root));
      if (!*found)
	return holy_errno;
      holy_memcpy (*found, &root, sizeof (root));
    }
  return err;
}

struct holy_hfs_dir_hook_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

static int
holy_hfs_dir_hook (struct holy_hfs_record *rec, void *hook_arg)
{
  struct holy_hfs_dir_hook_ctx *ctx = hook_arg;
  struct holy_hfs_dirrec *drec = rec->data;
  struct holy_hfs_filerec *frec = rec->data;
  struct holy_hfs_catalog_key *ckey = rec->key;
  char fname[sizeof (ckey->str) * MAX_UTF8_PER_MAC_ROMAN + 1];
  struct holy_dirhook_info info;
  holy_size_t len;

  holy_memset (fname, 0, sizeof (fname));

  holy_memset (&info, 0, sizeof (info));

  len = ckey->strlen;
  if (len > sizeof (ckey->str))
    len = sizeof (ckey->str);
  macroman_to_utf8 (fname, ckey->str, len, 1);

  info.case_insensitive = 1;

  if (drec->type == holy_HFS_FILETYPE_DIR)
    {
      info.dir = 1;
      info.mtimeset = 1;
      info.inodeset = 1;
      info.mtime = holy_be_to_cpu32 (drec->mtime) - 2082844800;
      info.inode = holy_be_to_cpu32 (drec->dirid);
      return ctx->hook (fname, &info, ctx->hook_data);
    }
  if (frec->type == holy_HFS_FILETYPE_FILE)
    {
      info.dir = 0;
      info.mtimeset = 1;
      info.inodeset = 1;
      info.mtime = holy_be_to_cpu32 (frec->mtime) - 2082844800;
      info.inode = holy_be_to_cpu32 (frec->fileid);
      return ctx->hook (fname, &info, ctx->hook_data);
    }

  return 0;
}


static holy_err_t
holy_hfs_dir (holy_device_t device, const char *path, holy_fs_dir_hook_t hook,
	      void *hook_data)
{
  struct holy_hfs_data *data;
  struct holy_hfs_dir_hook_ctx ctx =
    {
      .hook = hook,
      .hook_data = hook_data
    };
  holy_fshelp_node_t found = NULL;
  
  holy_dl_ref (my_mod);

  data = holy_hfs_mount (device->disk);
  if (!data)
    goto fail;

  /* First the directory ID for the directory.  */
  if (holy_hfs_find_dir (data, path, &found, holy_FSHELP_DIR))
    goto fail;

  holy_hfs_iterate_dir (data, data->cat_root, found->inode, holy_hfs_dir_hook, &ctx);

 fail:
  holy_free (found);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_hfs_open (struct holy_file *file, const char *name)
{
  struct holy_hfs_data *data;
  holy_fshelp_node_t found = NULL;
  
  holy_dl_ref (my_mod);

  data = holy_hfs_mount (file->device->disk);

  if (!data)
    {
      holy_dl_unref (my_mod);
      return holy_errno;
    }

  if (holy_hfs_find_dir (data, name, &found, holy_FSHELP_REG))
    {
      holy_free (data);
      holy_free (found);
      holy_dl_unref (my_mod);
      return holy_errno;
    }

  holy_memcpy (data->extents, found->fdrec.frec.extents, sizeof (holy_hfs_datarecord_t));
  file->size = holy_be_to_cpu32 (found->fdrec.frec.size);
  data->size = holy_be_to_cpu32 (found->fdrec.frec.size);
  data->fileid = holy_be_to_cpu32 (found->fdrec.frec.fileid);
  file->offset = 0;

  file->data = data;

  holy_free (found);

  return 0;
}

static holy_ssize_t
holy_hfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_hfs_data *data =
    (struct holy_hfs_data *) file->data;

  return holy_hfs_read_file (data, file->read_hook, file->read_hook_data,
			     file->offset, len, buf);
}


static holy_err_t
holy_hfs_close (holy_file_t file)
{
  holy_free (file->data);

  holy_dl_unref (my_mod);

  return 0;
}


static holy_err_t
holy_hfs_label (holy_device_t device, char **label)
{
  struct holy_hfs_data *data;

  data = holy_hfs_mount (device->disk);

  if (data)
    {
      holy_size_t len = data->sblock.volname[0];
      if (len > sizeof (data->sblock.volname) - 1)
	len = sizeof (data->sblock.volname) - 1;
      *label = holy_malloc (len * MAX_UTF8_PER_MAC_ROMAN + 1);
      if (*label)
	macroman_to_utf8 (*label, data->sblock.volname + 1,
			  len + 1, 0);
    }
  else
    *label = 0;

  holy_free (data);
  return holy_errno;
}

static holy_err_t
holy_hfs_mtime (holy_device_t device, holy_int32_t *tm)
{
  struct holy_hfs_data *data;

  data = holy_hfs_mount (device->disk);

  if (data)
    *tm = holy_be_to_cpu32 (data->sblock.mtime) - 2082844800;
  else
    *tm = 0;

  holy_free (data);
  return holy_errno;
}

static holy_err_t
holy_hfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_hfs_data *data;

  holy_dl_ref (my_mod);

  data = holy_hfs_mount (device->disk);
  if (data && data->sblock.num_serial != 0)
    {
      *uuid = holy_xasprintf ("%016llx",
			     (unsigned long long)
			     holy_be_to_cpu64 (data->sblock.num_serial));
    }
  else
    *uuid = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}



static struct holy_fs holy_hfs_fs =
  {
    .name = "hfs",
    .dir = holy_hfs_dir,
    .open = holy_hfs_open,
    .read = holy_hfs_read,
    .close = holy_hfs_close,
    .label = holy_hfs_label,
    .uuid = holy_hfs_uuid,
    .mtime = holy_hfs_mtime,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(hfs)
{
  holy_fs_register (&holy_hfs_fs);
  my_mod = mod;
}

holy_MOD_FINI(hfs)
{
  holy_fs_unregister (&holy_hfs_fs);
}
