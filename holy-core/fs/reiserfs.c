/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if 0
# define holy_REISERFS_DEBUG
# define holy_REISERFS_JOURNALING
# define holy_HEXDUMP
#endif

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/fshelp.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

#define MIN(a, b) \
  ({ typeof (a) _a = (a); \
     typeof (b) _b = (b); \
     _a < _b ? _a : _b; })

#define MAX(a, b) \
  ({ typeof (a) _a = (a); \
     typeof (b) _b = (b); \
     _a > _b ? _a : _b; })

#define REISERFS_SUPER_BLOCK_OFFSET 0x10000
#define REISERFS_MAGIC_LEN 12
#define REISERFS_MAGIC_STRING "ReIsEr"
#define REISERFS_MAGIC_DESC_BLOCK "ReIsErLB"
/* If the 3rd bit of an item state is set, then it's visible.  */
#define holy_REISERFS_VISIBLE_MASK ((holy_uint16_t) 0x04)

#define S_IFLNK 0xA000

static holy_dl_t my_mod;

#define assert(boolean) real_assert (boolean, holy_FILE, __LINE__)
static inline void
real_assert (int boolean, const char *file, const int line)
{
  if (! boolean)
    holy_printf ("Assertion failed at %s:%d\n", file, line);
}

enum holy_reiserfs_item_type
  {
    holy_REISERFS_STAT,
    holy_REISERFS_DIRECTORY,
    holy_REISERFS_DIRECT,
    holy_REISERFS_INDIRECT,
    /* Matches both _DIRECT and _INDIRECT when searching.  */
    holy_REISERFS_ANY,
    holy_REISERFS_UNKNOWN
  };

struct holy_reiserfs_superblock
{
  holy_uint32_t block_count;
  holy_uint32_t block_free_count;
  holy_uint32_t root_block;
  holy_uint32_t journal_block;
  holy_uint32_t journal_device;
  holy_uint32_t journal_original_size;
  holy_uint32_t journal_max_transaction_size;
  holy_uint32_t journal_block_count;
  holy_uint32_t journal_max_batch;
  holy_uint32_t journal_max_commit_age;
  holy_uint32_t journal_max_transaction_age;
  holy_uint16_t block_size;
  holy_uint16_t oid_max_size;
  holy_uint16_t oid_current_size;
  holy_uint16_t state;
  holy_uint8_t magic_string[REISERFS_MAGIC_LEN];
  holy_uint32_t function_hash_code;
  holy_uint16_t tree_height;
  holy_uint16_t bitmap_number;
  holy_uint16_t version;
  holy_uint16_t reserved;
  holy_uint32_t inode_generation;
  holy_uint8_t unused[4];
  holy_uint16_t uuid[8];
  char label[16];
} holy_PACKED;

struct holy_reiserfs_journal_header
{
  holy_uint32_t last_flush_uid;
  holy_uint32_t unflushed_offset;
  holy_uint32_t mount_id;
} holy_PACKED;

struct holy_reiserfs_description_block
{
  holy_uint32_t id;
  holy_uint32_t len;
  holy_uint32_t mount_id;
  holy_uint32_t real_blocks[0];
} holy_PACKED;

struct holy_reiserfs_commit_block
{
  holy_uint32_t id;
  holy_uint32_t len;
  holy_uint32_t real_blocks[0];
} holy_PACKED;

struct holy_reiserfs_stat_item_v1
{
  holy_uint16_t mode;
  holy_uint16_t hardlink_count;
  holy_uint16_t uid;
  holy_uint16_t gid;
  holy_uint32_t size;
  holy_uint32_t atime;
  holy_uint32_t mtime;
  holy_uint32_t ctime;
  holy_uint32_t rdev;
  holy_uint32_t first_direct_byte;
} holy_PACKED;

struct holy_reiserfs_stat_item_v2
{
  holy_uint16_t mode;
  holy_uint16_t reserved;
  holy_uint32_t hardlink_count;
  holy_uint64_t size;
  holy_uint32_t uid;
  holy_uint32_t gid;
  holy_uint32_t atime;
  holy_uint32_t mtime;
  holy_uint32_t ctime;
  holy_uint32_t blocks;
  holy_uint32_t first_direct_byte;
} holy_PACKED;

struct holy_reiserfs_key
{
  holy_uint32_t directory_id;
  holy_uint32_t object_id;
  union
  {
    struct
    {
      holy_uint32_t offset;
      holy_uint32_t type;
    } holy_PACKED v1;
    struct
    {
      holy_uint64_t offset_type;
    } holy_PACKED v2;
  } u;
} holy_PACKED;

struct holy_reiserfs_item_header
{
  struct holy_reiserfs_key key;
  union
  {
    holy_uint16_t free_space;
    holy_uint16_t entry_count;
  } holy_PACKED u;
  holy_uint16_t item_size;
  holy_uint16_t item_location;
  holy_uint16_t version;
} holy_PACKED;

struct holy_reiserfs_block_header
{
  holy_uint16_t level;
  holy_uint16_t item_count;
  holy_uint16_t free_space;
  holy_uint16_t reserved;
  struct holy_reiserfs_key block_right_delimiting_key;
} holy_PACKED;

struct holy_reiserfs_disk_child
{
  holy_uint32_t block_number;
  holy_uint16_t size;
  holy_uint16_t reserved;
} holy_PACKED;

struct holy_reiserfs_directory_header
{
  holy_uint32_t offset;
  holy_uint32_t directory_id;
  holy_uint32_t object_id;
  holy_uint16_t location;
  holy_uint16_t state;
} holy_PACKED;

struct holy_fshelp_node
{
  struct holy_reiserfs_data *data;
  holy_uint32_t block_number; /* 0 if node is not found.  */
  holy_uint16_t block_position;
  holy_uint64_t next_offset;
  holy_int32_t mtime;
  holy_off_t size;
  enum holy_reiserfs_item_type type; /* To know how to read the header.  */
  struct holy_reiserfs_item_header header;
};

/* Returned when opening a file.  */
struct holy_reiserfs_data
{
  struct holy_reiserfs_superblock superblock;
  holy_disk_t disk;
};

static holy_ssize_t
holy_reiserfs_read_real (struct holy_fshelp_node *node,
			 holy_off_t off, char *buf, holy_size_t len,
			 holy_disk_read_hook_t read_hook,
			 void *read_hook_data);

/* Internal-only functions. Not to be used outside of this file.  */

/* Return the type of given v2 key.  */
static enum holy_reiserfs_item_type
holy_reiserfs_get_key_v2_type (const struct holy_reiserfs_key *key)
{
  switch (holy_le_to_cpu64 (key->u.v2.offset_type) >> 60)
    {
    case 0:
      return holy_REISERFS_STAT;
    case 15:
      return holy_REISERFS_ANY;
    case 3:
      return holy_REISERFS_DIRECTORY;
    case 2:
      return holy_REISERFS_DIRECT;
    case 1:
      return holy_REISERFS_INDIRECT;
    }
  return holy_REISERFS_UNKNOWN;
}

/* Return the type of given v1 key.  */
static enum holy_reiserfs_item_type
holy_reiserfs_get_key_v1_type (const struct holy_reiserfs_key *key)
{
  switch (holy_le_to_cpu32 (key->u.v1.type))
    {
    case 0:
      return holy_REISERFS_STAT;
    case 555:
      return holy_REISERFS_ANY;
    case 500:
      return holy_REISERFS_DIRECTORY;
    case 0x20000000:
    case 0xFFFFFFFF:
      return holy_REISERFS_DIRECT;
    case 0x10000000:
    case 0xFFFFFFFE:
      return holy_REISERFS_INDIRECT;
    }
  return holy_REISERFS_UNKNOWN;
}

/* Return 1 if the given key is version 1 key, 2 otherwise.  */
static int
holy_reiserfs_get_key_version (const struct holy_reiserfs_key *key)
{
  return holy_reiserfs_get_key_v1_type (key) == holy_REISERFS_UNKNOWN ? 2 : 1;
}

#ifdef holy_HEXDUMP
static void
holy_hexdump (char *buffer, holy_size_t len)
{
  holy_size_t a;
  for (a = 0; a < len; a++)
    {
      if (! (a & 0x0F))
        holy_printf ("\n%08x  ", a);
      holy_printf ("%02x ",
                   ((unsigned int) ((unsigned char *) buffer)[a]) & 0xFF);
    }
  holy_printf ("\n");
}
#endif

#ifdef holy_REISERFS_DEBUG
static holy_uint64_t
holy_reiserfs_get_key_offset (const struct holy_reiserfs_key *key);

static enum holy_reiserfs_item_type
holy_reiserfs_get_key_type (const struct holy_reiserfs_key *key);

static void
holy_reiserfs_print_key (const struct holy_reiserfs_key *key)
{
  unsigned int a;
  char *reiserfs_type_strings[] = {
    "stat     ",
    "directory",
    "direct   ",
    "indirect ",
    "any      ",
    "unknown  "
  };

  for (a = 0; a < sizeof (struct holy_reiserfs_key); a++)
    holy_printf ("%02x ", ((unsigned int) ((unsigned char *) key)[a]) & 0xFF);
  holy_printf ("parent id = 0x%08x, self id = 0x%08x, type = %s, offset = ",
               holy_le_to_cpu32 (key->directory_id),
               holy_le_to_cpu32 (key->object_id),
               reiserfs_type_strings [holy_reiserfs_get_key_type (key)]);
  if (holy_reiserfs_get_key_version (key) == 1)
    holy_printf("%08x", (unsigned int) holy_reiserfs_get_key_offset (key));
  else
    holy_printf("0x%07x%08x",
                (unsigned) (holy_reiserfs_get_key_offset (key) >> 32),
                (unsigned) (holy_reiserfs_get_key_offset (key) & 0xFFFFFFFF));
  holy_printf ("\n");
}
#endif

/* Return the offset of given key.  */
static holy_uint64_t
holy_reiserfs_get_key_offset (const struct holy_reiserfs_key *key)
{
  if (holy_reiserfs_get_key_version (key) == 1)
    return holy_le_to_cpu32 (key->u.v1.offset);
  else
    return holy_le_to_cpu64 (key->u.v2.offset_type) & (~0ULL >> 4);
}

/* Set the offset of given key.  */
static void
holy_reiserfs_set_key_offset (struct holy_reiserfs_key *key,
                              holy_uint64_t value)
{
  if (holy_reiserfs_get_key_version (key) == 1)
    key->u.v1.offset = holy_cpu_to_le32 (value);
  else
    key->u.v2.offset_type \
      = ((key->u.v2.offset_type & holy_cpu_to_le64_compile_time (15ULL << 60))
         | holy_cpu_to_le64 (value & (~0ULL >> 4)));
}

/* Return the type of given key.  */
static enum holy_reiserfs_item_type
holy_reiserfs_get_key_type (const struct holy_reiserfs_key *key)
{
  if (holy_reiserfs_get_key_version (key) == 1)
    return holy_reiserfs_get_key_v1_type (key);
  else
    return holy_reiserfs_get_key_v2_type (key);
}

/* Set the type of given key, with given version number.  */
static void
holy_reiserfs_set_key_type (struct holy_reiserfs_key *key,
                            enum holy_reiserfs_item_type holy_type,
                            int version)
{
  holy_uint32_t type;

  switch (holy_type)
    {
    case holy_REISERFS_STAT:
      type = 0;
      break;
    case holy_REISERFS_ANY:
      type = (version == 1) ? 555 : 15;
      break;
    case holy_REISERFS_DIRECTORY:
      type = (version == 1) ? 500 : 3;
      break;
    case holy_REISERFS_DIRECT:
      type = (version == 1) ? 0xFFFFFFFF : 2;
      break;
    case holy_REISERFS_INDIRECT:
      type = (version == 1) ? 0xFFFFFFFE : 1;
      break;
    default:
      return;
    }

  if (version == 1)
    key->u.v1.type = holy_cpu_to_le32 (type);
  else
    key->u.v2.offset_type
      = ((key->u.v2.offset_type & holy_cpu_to_le64_compile_time (~0ULL >> 4))
         | holy_cpu_to_le64 ((holy_uint64_t) type << 60));

  assert (holy_reiserfs_get_key_type (key) == holy_type);
}

/* -1 if key 1 if lower than key 2.
   0 if key 1 is equal to key 2.
   1 if key 1 is higher than key 2.  */
static int
holy_reiserfs_compare_keys (const struct holy_reiserfs_key *key1,
                            const struct holy_reiserfs_key *key2)
{
  holy_uint64_t offset1, offset2;
  enum holy_reiserfs_item_type type1, type2;
  holy_uint32_t id1, id2;

  if (! key1 || ! key2)
    return -2;

  id1 = holy_le_to_cpu32 (key1->directory_id);
  id2 = holy_le_to_cpu32 (key2->directory_id);
  if (id1 < id2)
    return -1;
  if (id1 > id2)
    return 1;

  id1 = holy_le_to_cpu32 (key1->object_id);
  id2 = holy_le_to_cpu32 (key2->object_id);
  if (id1 < id2)
    return -1;
  if (id1 > id2)
    return 1;

  offset1 = holy_reiserfs_get_key_offset (key1);
  offset2 = holy_reiserfs_get_key_offset (key2);
  if (offset1 < offset2)
    return -1;
  if (offset1 > offset2)
    return 1;

  type1 = holy_reiserfs_get_key_type (key1);
  type2 = holy_reiserfs_get_key_type (key2);
  if ((type1 == holy_REISERFS_ANY
       && (type2 == holy_REISERFS_DIRECT
           || type2 == holy_REISERFS_INDIRECT))
      || (type2 == holy_REISERFS_ANY
          && (type1 == holy_REISERFS_DIRECT
              || type1 == holy_REISERFS_INDIRECT)))
    return 0;
  if (type1 < type2)
    return -1;
  if (type1 > type2)
    return 1;

  return 0;
}

/* Find the item identified by KEY in mounted filesystem DATA, and fill ITEM
   accordingly to what was found.  */
static holy_err_t
holy_reiserfs_get_item (struct holy_reiserfs_data *data,
                        const struct holy_reiserfs_key *key,
                        struct holy_fshelp_node *item, int exact)
{
  holy_uint32_t block_number;
  struct holy_reiserfs_block_header *block_header = 0;
  struct holy_reiserfs_key *block_key = 0;
  holy_uint16_t block_size, item_count, current_level;
  holy_uint16_t i;
  holy_uint16_t previous_level = ~0;
  struct holy_reiserfs_item_header *item_headers = 0;

#if 0
  if (! data)
    {
      holy_error (holy_ERR_BAD_FS, "data is NULL");
      goto fail;
    }

  if (! key)
    {
      holy_error (holy_ERR_BAD_FS, "key is NULL");
      goto fail;
    }

  if (! item)
    {
      holy_error (holy_ERR_BAD_FS, "item is NULL");
      goto fail;
    }
#endif

  block_size = holy_le_to_cpu16 (data->superblock.block_size);
  block_number = holy_le_to_cpu32 (data->superblock.root_block);
#ifdef holy_REISERFS_DEBUG
  holy_printf("Searching for ");
  holy_reiserfs_print_key (key);
#endif
  block_header = holy_malloc (block_size);
  if (! block_header)
    goto fail;

  item->next_offset = 0;
  do
    {
      holy_disk_read (data->disk,
                      block_number * (block_size >> holy_DISK_SECTOR_BITS),
                      (((holy_off_t) block_number * block_size)
                       & (holy_DISK_SECTOR_SIZE - 1)),
                      block_size, block_header);
      if (holy_errno)
        goto fail;
      current_level = holy_le_to_cpu16 (block_header->level);
      holy_dprintf ("reiserfs_tree", " at level %d\n", current_level);
      if (current_level >= previous_level)
        {
          holy_dprintf ("reiserfs_tree", "level loop detected, aborting\n");
          holy_error (holy_ERR_BAD_FS, "level loop");
          goto fail;
        }
      previous_level = current_level;
      item_count = holy_le_to_cpu16 (block_header->item_count);
      holy_dprintf ("reiserfs_tree", " number of contained items : %d\n",
                    item_count);
      if (current_level > 1)
        {
          /* Internal node. Navigate to the child that should contain
             the searched key.  */
          struct holy_reiserfs_key *keys
            = (struct holy_reiserfs_key *) (block_header + 1);
          struct holy_reiserfs_disk_child *children
            = ((struct holy_reiserfs_disk_child *)
               (keys + item_count));

          for (i = 0;
               i < item_count
                 && holy_reiserfs_compare_keys (key, &(keys[i])) >= 0;
               i++)
            {
#ifdef holy_REISERFS_DEBUG
              holy_printf("i %03d/%03d ", i + 1, item_count + 1);
              holy_reiserfs_print_key (&(keys[i]));
#endif
            }
          block_number = holy_le_to_cpu32 (children[i].block_number);
	  if ((i < item_count) && (key->directory_id == keys[i].directory_id)
	       && (key->object_id == keys[i].object_id))
	    item->next_offset = holy_reiserfs_get_key_offset(&(keys[i]));
#ifdef holy_REISERFS_DEBUG
          if (i == item_count
              || holy_reiserfs_compare_keys (key, &(keys[i])) == 0)
            holy_printf(">");
          else
            holy_printf("<");
          if (i < item_count)
            {
              holy_printf (" %03d/%03d ", i + 1, item_count + 1);
              holy_reiserfs_print_key (&(keys[i]));
              if (i + 1 < item_count)
                {
                  holy_printf ("+ %03d/%03d ", i + 2, item_count);
                  holy_reiserfs_print_key (&(keys[i + 1]));
                }
            }
          else
            holy_printf ("Accessing rightmost child at block %d.\n",
                         block_number);
#endif
        }
      else
        {
          /* Leaf node.  Check that the key is actually present.  */
          item_headers
            = (struct holy_reiserfs_item_header *) (block_header + 1);
          for (i = 0;
               i < item_count;
               i++)
	    {
	      int val;
	      val = holy_reiserfs_compare_keys (key, &(item_headers[i].key));
	      if (val == 0)
		{
		  block_key = &(item_headers[i].key);
		  break;
		}
	      if (val < 0 && exact)
		break;
	      if (val < 0)
		{
		  if (i == 0)
		    {
		      holy_error (holy_ERR_READ_ERROR, "unexpected btree node");
		      goto fail;
		    }
		  i--;
		  block_key = &(item_headers[i].key);
		  break;
		}
	    }
	  if (!exact && i == item_count)
	    {
	      if (i == 0)
		{
		  holy_error (holy_ERR_READ_ERROR, "unexpected btree node");
		  goto fail;
		}
	      i--;
	      block_key = &(item_headers[i].key);
	    }
        }
    }
  while (current_level > 1);

  item->data = data;

  if (!block_key)
    {
      item->block_number = 0;
      item->block_position = 0;
      item->type = holy_REISERFS_UNKNOWN;
#ifdef holy_REISERFS_DEBUG
      holy_printf("Not found.\n");
#endif
    }
  else
    {
      item->block_number = block_number;
      item->block_position = i;
      item->type = holy_reiserfs_get_key_type (block_key);
      holy_memcpy (&(item->header), &(item_headers[i]),
                   sizeof (struct holy_reiserfs_item_header));
#ifdef holy_REISERFS_DEBUG
      holy_printf ("F %03d/%03d ", i + 1, item_count);
      holy_reiserfs_print_key (block_key);
#endif
    }

  assert (holy_errno == holy_ERR_NONE);
  holy_free (block_header);
  return holy_ERR_NONE;

 fail:
  assert (holy_errno != holy_ERR_NONE);
  holy_free (block_header);
  assert (holy_errno != holy_ERR_NONE);
  return holy_errno;
}

/* Return the path of the file which is pointed at by symlink NODE.  */
static char *
holy_reiserfs_read_symlink (holy_fshelp_node_t node)
{
  char *symlink_buffer = 0;
  holy_size_t len = node->size;
  holy_ssize_t ret;

  symlink_buffer = holy_malloc (len + 1);
  if (! symlink_buffer)
    return 0;

  ret = holy_reiserfs_read_real (node, 0, symlink_buffer, len, 0, 0);
  if (ret < 0)
    {
      holy_free (symlink_buffer);
      return 0;
    }

  symlink_buffer[ret] = 0;
  return symlink_buffer;
}

/* Fill the mounted filesystem structure and return it.  */
static struct holy_reiserfs_data *
holy_reiserfs_mount (holy_disk_t disk)
{
  struct holy_reiserfs_data *data = 0;
  data = holy_malloc (sizeof (*data));
  if (! data)
    goto fail;
  holy_disk_read (disk, REISERFS_SUPER_BLOCK_OFFSET / holy_DISK_SECTOR_SIZE,
                  0, sizeof (data->superblock), &(data->superblock));
  if (holy_errno)
    goto fail;
  if (holy_memcmp (data->superblock.magic_string,
                   REISERFS_MAGIC_STRING, sizeof (REISERFS_MAGIC_STRING) - 1))
    {
      holy_error (holy_ERR_BAD_FS, "not a ReiserFS filesystem");
      goto fail;
    }
  data->disk = disk;
  return data;

 fail:
  /* Disk is too small to contain a ReiserFS.  */
  if (holy_errno == holy_ERR_OUT_OF_RANGE)
    holy_error (holy_ERR_BAD_FS, "not a ReiserFS filesystem");

  holy_free (data);
  return 0;
}

/* Call HOOK for each file in directory ITEM.  */
static int
holy_reiserfs_iterate_dir (holy_fshelp_node_t item,
                           holy_fshelp_iterate_dir_hook_t hook,
			   void *hook_data)
{
  struct holy_reiserfs_data *data = item->data;
  struct holy_reiserfs_block_header *block_header = 0;
  holy_uint16_t block_size, block_position;
  holy_uint32_t block_number;
  holy_uint64_t next_offset = item->next_offset;
  int ret = 0;

  if (item->type != holy_REISERFS_DIRECTORY)
    {
      holy_error (holy_ERR_BAD_FILE_TYPE, N_("not a directory"));
      goto fail;
    }
  block_size = holy_le_to_cpu16 (data->superblock.block_size);
  block_header = holy_malloc (block_size + 1);
  if (! block_header)
    goto fail;
  block_number = item->block_number;
  block_position = item->block_position;
  holy_dprintf ("reiserfs", "Iterating directory...\n");
  do
    {
      struct holy_reiserfs_directory_header *directory_headers;
      struct holy_fshelp_node directory_item;
      holy_uint16_t entry_count, entry_number;
      struct holy_reiserfs_item_header *item_headers;

      holy_disk_read (data->disk,
                      block_number * (block_size >> holy_DISK_SECTOR_BITS),
                      (((holy_off_t) block_number * block_size)
                       & (holy_DISK_SECTOR_SIZE - 1)),
                      block_size, (char *) block_header);
      if (holy_errno)
        goto fail;

      ((char *) block_header)[block_size] = 0;

#if 0
      if (holy_le_to_cpu16 (block_header->level) != 1)
        {
          holy_error (holy_ERR_BAD_FS,
                      "reiserfs: block %d is not a leaf block",
                      block_number);
          goto fail;
        }
#endif

      item_headers = (struct holy_reiserfs_item_header *) (block_header + 1);
      directory_headers
        = ((struct holy_reiserfs_directory_header *)
           ((char *) block_header
            + holy_le_to_cpu16 (item_headers[block_position].item_location)));
      entry_count
        = holy_le_to_cpu16 (item_headers[block_position].u.entry_count);
      for (entry_number = 0; entry_number < entry_count; entry_number++)
        {
          struct holy_reiserfs_directory_header *directory_header
            = &directory_headers[entry_number];
          holy_uint16_t entry_state
            = holy_le_to_cpu16 (directory_header->state);
	  holy_fshelp_node_t entry_item;
	  struct holy_reiserfs_key entry_key;
	  enum holy_fshelp_filetype entry_type;
	  char *entry_name;
	  char *entry_name_end = 0;
	  char c;
	  
          if (!(entry_state & holy_REISERFS_VISIBLE_MASK))
	    continue;

	  entry_name = (((char *) directory_headers)
			+ holy_le_to_cpu16 (directory_header->location));
	  if (entry_number == 0)
	    {
	      entry_name_end = (char *) block_header
		+ holy_le_to_cpu16 (item_headers[block_position].item_location)
		+ holy_le_to_cpu16 (item_headers[block_position].item_size);
	    }
	  else
	    {
	      entry_name_end = (((char *) directory_headers)
			+ holy_le_to_cpu16 (directory_headers[entry_number - 1].location));
	    }
	  if (entry_name_end < entry_name || entry_name_end > (char *) block_header + block_size)
	    {
	      entry_name_end = (char *) block_header + block_size;
	    }

	  entry_key.directory_id = directory_header->directory_id;
	  entry_key.object_id = directory_header->object_id;
	  entry_key.u.v2.offset_type = 0;
	  holy_reiserfs_set_key_type (&entry_key, holy_REISERFS_DIRECTORY,
				      2);
	  holy_reiserfs_set_key_offset (&entry_key, 1);

	  entry_item = holy_malloc (sizeof (*entry_item));
	  if (! entry_item)
	    goto fail;

	  if (holy_reiserfs_get_item (data, &entry_key, entry_item, 1)
	      != holy_ERR_NONE)
	    {
	      holy_free (entry_item);
	      goto fail;
	    }

	  if (entry_item->type == holy_REISERFS_DIRECTORY)
	    entry_type = holy_FSHELP_DIR;
	  else
	    {
	      holy_uint32_t entry_block_number;
	      /* Order is very important here.
		 First set the offset to 0 using current key version.
		 Then change the key type, which affects key version
		 detection.  */
	      holy_reiserfs_set_key_offset (&entry_key, 0);
	      holy_reiserfs_set_key_type (&entry_key, holy_REISERFS_STAT,
					  2);
	      if (holy_reiserfs_get_item (data, &entry_key, entry_item, 1)
		  != holy_ERR_NONE)
		{
		  holy_free (entry_item);
		  goto fail;
		}

	      if (entry_item->block_number != 0)
		{
		  holy_uint16_t entry_version;
		  entry_version
		    = holy_le_to_cpu16 (entry_item->header.version);
		  entry_block_number = entry_item->block_number;
#if 0
		  holy_dprintf ("reiserfs",
				"version %04x block %08x (%08x) position %08x\n",
				entry_version, entry_block_number,
				((holy_disk_addr_t) entry_block_number * block_size) / holy_DISK_SECTOR_SIZE,
				holy_le_to_cpu16 (entry_item->header.item_location));
#endif
		  if (entry_version == 0) /* Version 1 stat item. */
		    {
		      struct holy_reiserfs_stat_item_v1 entry_v1_stat;
		      holy_disk_read (data->disk,
				      entry_block_number * (block_size >> holy_DISK_SECTOR_BITS),
				      holy_le_to_cpu16 (entry_item->header.item_location),
				      sizeof (entry_v1_stat),
				      (char *) &entry_v1_stat);
		      if (holy_errno)
			goto fail;
#if 0
		      holy_dprintf ("reiserfs",
				    "%04x %04x %04x %04x %08x %08x | %08x %08x %08x %08x\n",
				    holy_le_to_cpu16 (entry_v1_stat.mode),
				    holy_le_to_cpu16 (entry_v1_stat.hardlink_count),
				    holy_le_to_cpu16 (entry_v1_stat.uid),
				    holy_le_to_cpu16 (entry_v1_stat.gid),
				    holy_le_to_cpu32 (entry_v1_stat.size),
				    holy_le_to_cpu32 (entry_v1_stat.atime),
				    holy_le_to_cpu32 (entry_v1_stat.mtime),
				    holy_le_to_cpu32 (entry_v1_stat.ctime),
				    holy_le_to_cpu32 (entry_v1_stat.rdev),
				    holy_le_to_cpu32 (entry_v1_stat.first_direct_byte));
		      holy_dprintf ("reiserfs",
				    "%04x %04x %04x %04x %08x %08x | %08x %08x %08x %08x\n",
				    entry_v1_stat.mode,
				    entry_v1_stat.hardlink_count,
				    entry_v1_stat.uid,
				    entry_v1_stat.gid,
				    entry_v1_stat.size,
				    entry_v1_stat.atime,
				    entry_v1_stat.mtime,
				    entry_v1_stat.ctime,
				    entry_v1_stat.rdev,
				    entry_v1_stat.first_direct_byte);
#endif
		      entry_item->mtime = holy_le_to_cpu32 (entry_v1_stat.mtime);
		      if ((holy_le_to_cpu16 (entry_v1_stat.mode) & S_IFLNK)
			  == S_IFLNK)
			entry_type = holy_FSHELP_SYMLINK;
		      else
			entry_type = holy_FSHELP_REG;
		      entry_item->size = (holy_off_t) holy_le_to_cpu32 (entry_v1_stat.size);
		    }
		  else
		    {
		      struct holy_reiserfs_stat_item_v2 entry_v2_stat;
		      holy_disk_read (data->disk,
				      entry_block_number * (block_size >> holy_DISK_SECTOR_BITS),
				      holy_le_to_cpu16 (entry_item->header.item_location),
				      sizeof (entry_v2_stat),
				      (char *) &entry_v2_stat);
		      if (holy_errno)
			goto fail;
#if 0
		      holy_dprintf ("reiserfs",
				    "%04x %04x %08x %08x%08x | %08x %08x %08x %08x | %08x %08x %08x\n",
				    holy_le_to_cpu16 (entry_v2_stat.mode),
				    holy_le_to_cpu16 (entry_v2_stat.reserved),
				    holy_le_to_cpu32 (entry_v2_stat.hardlink_count),
				    (unsigned int) (holy_le_to_cpu64 (entry_v2_stat.size) >> 32),
				    (unsigned int) (holy_le_to_cpu64 (entry_v2_stat.size) && 0xFFFFFFFF),
				    holy_le_to_cpu32 (entry_v2_stat.uid),
				    holy_le_to_cpu32 (entry_v2_stat.gid),
				    holy_le_to_cpu32 (entry_v2_stat.atime),
				    holy_le_to_cpu32 (entry_v2_stat.mtime),
				    holy_le_to_cpu32 (entry_v2_stat.ctime),
				    holy_le_to_cpu32 (entry_v2_stat.blocks),
				    holy_le_to_cpu32 (entry_v2_stat.first_direct_byte));
		      holy_dprintf ("reiserfs",
				    "%04x %04x %08x %08x%08x | %08x %08x %08x %08x | %08x %08x %08x\n",
				    entry_v2_stat.mode,
				    entry_v2_stat.reserved,
				    entry_v2_stat.hardlink_count,
				    (unsigned int) (entry_v2_stat.size >> 32),
				    (unsigned int) (entry_v2_stat.size && 0xFFFFFFFF),
				    entry_v2_stat.uid,
				    entry_v2_stat.gid,
				    entry_v2_stat.atime,
				    entry_v2_stat.mtime,
				    entry_v2_stat.ctime,
				    entry_v2_stat.blocks,
				    entry_v2_stat.first_direct_byte);
#endif
		      entry_item->mtime = holy_le_to_cpu32 (entry_v2_stat.mtime);
		      entry_item->size = (holy_off_t) holy_le_to_cpu64 (entry_v2_stat.size);
		      if ((holy_le_to_cpu16 (entry_v2_stat.mode) & S_IFLNK)
			  == S_IFLNK)
			entry_type = holy_FSHELP_SYMLINK;
		      else
			entry_type = holy_FSHELP_REG;
		    }
		}
	      else
		{
		  /* Pseudo file ".." never has stat block.  */
		  if (entry_name_end == entry_name + 2 && holy_memcmp (entry_name, "..", 2) != 0)
		    holy_dprintf ("reiserfs",
				  "Warning : %s has no stat block !\n",
				  entry_name);
		  holy_free (entry_item);
		  goto next;
		}
	    }

	  c = *entry_name_end;
	  *entry_name_end = 0;
	  if (hook (entry_name, entry_type, entry_item, hook_data))
	    {
	      *entry_name_end = c;
	      holy_dprintf ("reiserfs", "Found : %s, type=%d\n",
			    entry_name, entry_type);
	      ret = 1;
	      goto found;
	    }
	  *entry_name_end = c;

	next:
	  ;
        }

      if (next_offset == 0)
        break;

      holy_reiserfs_set_key_offset (&(item_headers[block_position].key),
                                    next_offset);
      if (holy_reiserfs_get_item (data, &(item_headers[block_position].key),
                                  &directory_item, 1) != holy_ERR_NONE)
        goto fail;
      block_number = directory_item.block_number;
      block_position = directory_item.block_position;
      next_offset = directory_item.next_offset;
    }
  while (block_number);

 found:
  assert (holy_errno == holy_ERR_NONE);
  holy_free (block_header);
  return ret;
 fail:
  assert (holy_errno != holy_ERR_NONE);
  holy_free (block_header);
  return 0;
}

/****************************************************************************/
/* holy api functions */
/****************************************************************************/

/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_reiserfs_open (struct holy_file *file, const char *name)
{
  struct holy_reiserfs_data *data = 0;
  struct holy_fshelp_node root, *found = 0;
  struct holy_reiserfs_key key;

  holy_dl_ref (my_mod);
  data = holy_reiserfs_mount (file->device->disk);
  if (! data)
    goto fail;
  key.directory_id = holy_cpu_to_le32_compile_time (1);
  key.object_id = holy_cpu_to_le32_compile_time (2);
  key.u.v2.offset_type = 0;
  holy_reiserfs_set_key_type (&key, holy_REISERFS_DIRECTORY, 2);
  holy_reiserfs_set_key_offset (&key, 1);
  if (holy_reiserfs_get_item (data, &key, &root, 1) != holy_ERR_NONE)
    goto fail;
  if (root.block_number == 0)
    {
      holy_error (holy_ERR_BAD_FS, "unable to find root item");
      goto fail; /* Should never happen since checked at mount.  */
    }
  holy_fshelp_find_file (name, &root, &found,
                         holy_reiserfs_iterate_dir,
                         holy_reiserfs_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;
  file->size = found->size;

  holy_dprintf ("reiserfs", "file size : %d (%08x%08x)\n",
                (unsigned int) file->size,
                (unsigned int) (file->size >> 32), (unsigned int) file->size);
  file->offset = 0;
  file->data = found;
  return holy_ERR_NONE;

 fail:
  assert (holy_errno != holy_ERR_NONE);
  if (found != &root)
    holy_free (found);
  holy_free (data);
  holy_dl_unref (my_mod);
  return holy_errno;
}

static holy_ssize_t
holy_reiserfs_read_real (struct holy_fshelp_node *node,
			 holy_off_t off, char *buf, holy_size_t len,
			 holy_disk_read_hook_t read_hook, void *read_hook_data)
{
  unsigned int indirect_block, indirect_block_count;
  struct holy_reiserfs_key key;
  struct holy_reiserfs_data *data = node->data;
  struct holy_fshelp_node found;
  holy_uint16_t block_size = holy_le_to_cpu16 (data->superblock.block_size);
  holy_uint16_t item_size;
  holy_uint32_t *indirect_block_ptr = 0;
  holy_uint64_t current_key_offset = 1;
  holy_off_t initial_position, current_position, final_position, length;
  holy_disk_addr_t block;
  holy_off_t offset;

  key.directory_id = node->header.key.directory_id;
  key.object_id = node->header.key.object_id;
  key.u.v2.offset_type = 0;
  holy_reiserfs_set_key_type (&key, holy_REISERFS_ANY, 2);
  initial_position = off;
  current_position = 0;
  final_position = MIN (len + initial_position, node->size);
  holy_dprintf ("reiserfs",
		"Reading from %lld to %lld (%lld instead of requested %ld)\n",
		(unsigned long long) initial_position,
		(unsigned long long) final_position,
		(unsigned long long) (final_position - initial_position),
		(unsigned long) len);

  holy_reiserfs_set_key_offset (&key, initial_position + 1);

  if (holy_reiserfs_get_item (data, &key, &found, 0) != holy_ERR_NONE)
    goto fail;

  if (found.block_number == 0)
    {
      holy_error (holy_ERR_READ_ERROR, "offset %lld not found",
		  (unsigned long long) initial_position);
      goto fail;
    }

  current_key_offset = holy_reiserfs_get_key_offset (&found.header.key);
  current_position = current_key_offset - 1;

  while (current_position < final_position)
    {
      holy_reiserfs_set_key_offset (&key, current_key_offset);

      if (holy_reiserfs_get_item (data, &key, &found, 1) != holy_ERR_NONE)
        goto fail;
      if (found.block_number == 0)
        goto fail;
      item_size = holy_le_to_cpu16 (found.header.item_size);
      switch (found.type)
        {
        case holy_REISERFS_DIRECT:
          block = ((holy_disk_addr_t) found.block_number) * (block_size  >> holy_DISK_SECTOR_BITS);
          holy_dprintf ("reiserfs_blocktype", "D: %u\n", (unsigned) block);
          if (initial_position < current_position + item_size)
            {
              offset = MAX ((signed) (initial_position - current_position), 0);
              length = (MIN (item_size, final_position - current_position)
                        - offset);
              holy_dprintf ("reiserfs",
                            "Reading direct block %u from %u to %u...\n",
                            (unsigned) block, (unsigned) offset,
                            (unsigned) (offset + length));
              found.data->disk->read_hook = read_hook;
              found.data->disk->read_hook_data = read_hook_data;
              holy_disk_read (found.data->disk,
                              block,
                              offset
                              + holy_le_to_cpu16 (found.header.item_location),
                              length, buf);
              found.data->disk->read_hook = 0;
              if (holy_errno)
                goto fail;
              buf += length;
              current_position += offset + length;
            }
          else
            current_position += item_size;
          break;
        case holy_REISERFS_INDIRECT:
          indirect_block_count = item_size / sizeof (*indirect_block_ptr);
          indirect_block_ptr = holy_malloc (item_size);
          if (! indirect_block_ptr)
            goto fail;
          holy_disk_read (found.data->disk,
                          found.block_number * (block_size >> holy_DISK_SECTOR_BITS),
                          holy_le_to_cpu16 (found.header.item_location),
                          item_size, indirect_block_ptr);
          if (holy_errno)
            goto fail;
          found.data->disk->read_hook = read_hook;
          found.data->disk->read_hook_data = read_hook_data;
          for (indirect_block = 0;
               indirect_block < indirect_block_count
                 && current_position < final_position;
               indirect_block++)
            {
              block = holy_le_to_cpu32 (indirect_block_ptr[indirect_block]) *
                      (block_size >> holy_DISK_SECTOR_BITS);
              holy_dprintf ("reiserfs_blocktype", "I: %u\n", (unsigned) block);
              if (current_position + block_size >= initial_position)
                {
                  offset = MAX ((signed) (initial_position - current_position),
                                0);
                  length = (MIN (block_size, final_position - current_position)
                            - offset);
                  holy_dprintf ("reiserfs",
                                "Reading indirect block %u from %u to %u...\n",
                                (unsigned) block, (unsigned) offset,
                                (unsigned) (offset + length));
#if 0
                  holy_dprintf ("reiserfs",
                                "\nib=%04d/%04d, ip=%d, cp=%d, fp=%d, off=%d, l=%d, tl=%d\n",
                                indirect_block + 1, indirect_block_count,
                                initial_position, current_position,
                                final_position, offset, length, len);
#endif
                  holy_disk_read (found.data->disk, block, offset, length, buf);
                  if (holy_errno)
                    goto fail;
                  buf += length;
                  current_position += offset + length;
                }
              else
                current_position += block_size;
            }
          found.data->disk->read_hook = 0;
          holy_free (indirect_block_ptr);
          indirect_block_ptr = 0;
          break;
        default:
          goto fail;
        }
      current_key_offset = current_position + 1;
    }

  holy_dprintf ("reiserfs",
		"Have successfully read %lld bytes (%ld requested)\n",
		(unsigned long long) (current_position - initial_position),
		(unsigned long) len);
  return current_position - initial_position;

#if 0
  switch (found.type)
    {
      case holy_REISERFS_DIRECT:
        read_length = MIN (len, item_size - file->offset);
        holy_disk_read (found.data->disk,
                        (found.block_number * block_size) / holy_DISK_SECTOR_SIZE,
                        holy_le_to_cpu16 (found.header.item_location) + file->offset,
                        read_length, buf);
        if (holy_errno)
          goto fail;
        break;
      case holy_REISERFS_INDIRECT:
        indirect_block_count = item_size / sizeof (*indirect_block_ptr);
        indirect_block_ptr = holy_malloc (item_size);
        if (!indirect_block_ptr)
          goto fail;
        holy_disk_read (found.data->disk,
                        (found.block_number * block_size) / holy_DISK_SECTOR_SIZE,
                        holy_le_to_cpu16 (found.header.item_location),
                        item_size, (char *) indirect_block_ptr);
        if (holy_errno)
          goto fail;
        len = MIN (len, file->size - file->offset);
        for (indirect_block = file->offset / block_size;
             indirect_block < indirect_block_count && read_length < len;
             indirect_block++)
          {
            read = MIN (block_size, len - read_length);
            holy_disk_read (found.data->disk,
                            (holy_le_to_cpu32 (indirect_block_ptr[indirect_block]) * block_size) / holy_DISK_SECTOR_SIZE,
                            file->offset % block_size, read,
                            ((void *) buf) + read_length);
            if (holy_errno)
              goto fail;
            read_length += read;
          }
        holy_free (indirect_block_ptr);
        break;
      default:
        goto fail;
    }

  return read_length;
#endif

 fail:
  holy_free (indirect_block_ptr);
  return -1;
}

static holy_ssize_t
holy_reiserfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  return holy_reiserfs_read_real (file->data, file->offset, buf, len,
				  file->read_hook, file->read_hook_data);
}

/* Close the file FILE.  */
static holy_err_t
holy_reiserfs_close (holy_file_t file)
{
  struct holy_fshelp_node *node = file->data;
  struct holy_reiserfs_data *data = node->data;

  holy_free (data);
  holy_free (node);
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;
}

/* Context for holy_reiserfs_dir.  */
struct holy_reiserfs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_reiserfs_dir.  */
static int
holy_reiserfs_dir_iter (const char *filename,
			enum holy_fshelp_filetype filetype,
			holy_fshelp_node_t node, void *data)
{
  struct holy_reiserfs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = 1;
  info.mtime = node->mtime;
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

/* Call HOOK with each file under DIR.  */
static holy_err_t
holy_reiserfs_dir (holy_device_t device, const char *path,
                   holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_reiserfs_dir_ctx ctx = { hook, hook_data };
  struct holy_reiserfs_data *data = 0;
  struct holy_fshelp_node root, *found;
  struct holy_reiserfs_key root_key;

  holy_dl_ref (my_mod);
  data = holy_reiserfs_mount (device->disk);
  if (! data)
    goto fail;
  root_key.directory_id = holy_cpu_to_le32_compile_time (1);
  root_key.object_id = holy_cpu_to_le32_compile_time (2);
  root_key.u.v2.offset_type = 0;
  holy_reiserfs_set_key_type (&root_key, holy_REISERFS_DIRECTORY, 2);
  holy_reiserfs_set_key_offset (&root_key, 1);
  if (holy_reiserfs_get_item (data, &root_key, &root, 1) != holy_ERR_NONE)
    goto fail;
  if (root.block_number == 0)
    {
      holy_error(holy_ERR_BAD_FS, "root not found");
      goto fail;
    }
  holy_fshelp_find_file (path, &root, &found, holy_reiserfs_iterate_dir,
                         holy_reiserfs_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;
  holy_reiserfs_iterate_dir (found, holy_reiserfs_dir_iter, &ctx);
  holy_free (data);
  holy_dl_unref (my_mod);
  return holy_ERR_NONE;

 fail:
  holy_free (data);
  holy_dl_unref (my_mod);
  return holy_errno;
}

/* Return the label of the device DEVICE in LABEL.  The label is
   returned in a holy_malloc'ed buffer and should be freed by the
   caller.  */
static holy_err_t
holy_reiserfs_label (holy_device_t device, char **label)
{
  struct holy_reiserfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_reiserfs_mount (disk);
  if (data)
    {
      *label = holy_strndup (data->superblock.label,
			     sizeof (data->superblock.label));
    }
  else
    *label = NULL;

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_reiserfs_uuid (holy_device_t device, char **uuid)
{
  struct holy_reiserfs_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  *uuid = NULL;
  data = holy_reiserfs_mount (disk);
  if (data)
    {
      unsigned i;
      for (i = 0; i < ARRAY_SIZE (data->superblock.uuid); i++)
	if (data->superblock.uuid[i])
	  break;
      if (i < ARRAY_SIZE (data->superblock.uuid))
	*uuid = holy_xasprintf ("%04x%04x-%04x-%04x-%04x-%04x%04x%04x",
				holy_be_to_cpu16 (data->superblock.uuid[0]),
				holy_be_to_cpu16 (data->superblock.uuid[1]),
				holy_be_to_cpu16 (data->superblock.uuid[2]),
				holy_be_to_cpu16 (data->superblock.uuid[3]),
				holy_be_to_cpu16 (data->superblock.uuid[4]),
				holy_be_to_cpu16 (data->superblock.uuid[5]),
				holy_be_to_cpu16 (data->superblock.uuid[6]),
				holy_be_to_cpu16 (data->superblock.uuid[7]));
    }

  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

static struct holy_fs holy_reiserfs_fs =
  {
    .name = "reiserfs",
    .dir = holy_reiserfs_dir,
    .open = holy_reiserfs_open,
    .read = holy_reiserfs_read,
    .close = holy_reiserfs_close,
    .label = holy_reiserfs_label,
    .uuid = holy_reiserfs_uuid,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(reiserfs)
{
  holy_fs_register (&holy_reiserfs_fs);
  my_mod = mod;
}

holy_MOD_FINI(reiserfs)
{
  holy_fs_unregister (&holy_reiserfs_fs);
}
