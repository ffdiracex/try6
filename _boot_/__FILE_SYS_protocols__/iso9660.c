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
#include <holy/datetime.h>

holy_MOD_LICENSE ("GPLv2+");

#define holy_ISO9660_FSTYPE_DIR		0040000
#define holy_ISO9660_FSTYPE_REG		0100000
#define holy_ISO9660_FSTYPE_SYMLINK	0120000
#define holy_ISO9660_FSTYPE_MASK	0170000

#define holy_ISO9660_LOG2_BLKSZ		2
#define holy_ISO9660_BLKSZ		2048

#define holy_ISO9660_RR_DOT		2
#define holy_ISO9660_RR_DOTDOT		4

#define holy_ISO9660_VOLDESC_BOOT	0
#define holy_ISO9660_VOLDESC_PRIMARY	1
#define holy_ISO9660_VOLDESC_SUPP	2
#define holy_ISO9660_VOLDESC_PART	3
#define holy_ISO9660_VOLDESC_END	255

/* The head of a volume descriptor.  */
struct holy_iso9660_voldesc
{
  holy_uint8_t type;
  holy_uint8_t magic[5];
  holy_uint8_t version;
} holy_PACKED;

struct holy_iso9660_date2
{
  holy_uint8_t year;
  holy_uint8_t month;
  holy_uint8_t day;
  holy_uint8_t hour;
  holy_uint8_t minute;
  holy_uint8_t second;
  holy_uint8_t offset;
} holy_PACKED;

/* A directory entry.  */
struct holy_iso9660_dir
{
  holy_uint8_t len;
  holy_uint8_t ext_sectors;
  holy_uint32_t first_sector;
  holy_uint32_t first_sector_be;
  holy_uint32_t size;
  holy_uint32_t size_be;
  struct holy_iso9660_date2 mtime;
  holy_uint8_t flags;
  holy_uint8_t unused2[6];
#define MAX_NAMELEN 255
  holy_uint8_t namelen;
} holy_PACKED;

struct holy_iso9660_date
{
  holy_uint8_t year[4];
  holy_uint8_t month[2];
  holy_uint8_t day[2];
  holy_uint8_t hour[2];
  holy_uint8_t minute[2];
  holy_uint8_t second[2];
  holy_uint8_t hundredth[2];
  holy_uint8_t offset;
} holy_PACKED;

/* The primary volume descriptor.  Only little endian is used.  */
struct holy_iso9660_primary_voldesc
{
  struct holy_iso9660_voldesc voldesc;
  holy_uint8_t unused1[33];
  holy_uint8_t volname[32];
  holy_uint8_t unused2[16];
  holy_uint8_t escape[32];
  holy_uint8_t unused3[12];
  holy_uint32_t path_table_size;
  holy_uint8_t unused4[4];
  holy_uint32_t path_table;
  holy_uint8_t unused5[12];
  struct holy_iso9660_dir rootdir;
  holy_uint8_t unused6[624];
  struct holy_iso9660_date created;
  struct holy_iso9660_date modified;
} holy_PACKED;

/* A single entry in the path table.  */
struct holy_iso9660_path
{
  holy_uint8_t len;
  holy_uint8_t sectors;
  holy_uint32_t first_sector;
  holy_uint16_t parentdir;
  holy_uint8_t name[0];
} holy_PACKED;

/* An entry in the System Usage area of the directory entry.  */
struct holy_iso9660_susp_entry
{
  holy_uint8_t sig[2];
  holy_uint8_t len;
  holy_uint8_t version;
  holy_uint8_t data[0];
} holy_PACKED;

/* The CE entry.  This is used to describe the next block where data
   can be found.  */
struct holy_iso9660_susp_ce
{
  struct holy_iso9660_susp_entry entry;
  holy_uint32_t blk;
  holy_uint32_t blk_be;
  holy_uint32_t off;
  holy_uint32_t off_be;
  holy_uint32_t len;
  holy_uint32_t len_be;
} holy_PACKED;

struct holy_iso9660_data
{
  struct holy_iso9660_primary_voldesc voldesc;
  holy_disk_t disk;
  int rockridge;
  int susp_skip;
  int joliet;
  struct holy_fshelp_node *node;
};

struct holy_fshelp_node
{
  struct holy_iso9660_data *data;
  holy_size_t have_dirents, alloc_dirents;
  int have_symlink;
  struct holy_iso9660_dir dirents[8];
  char symlink[0];
};

enum
  {
    FLAG_TYPE_PLAIN = 0,
    FLAG_TYPE_DIR = 2,
    FLAG_TYPE = 3,
    FLAG_MORE_EXTENTS = 0x80
  };

static holy_dl_t my_mod;


static holy_err_t
iso9660_to_unixtime (const struct holy_iso9660_date *i, holy_int32_t *nix)
{
  struct holy_datetime datetime;
  
  if (! i->year[0] && ! i->year[1]
      && ! i->year[2] && ! i->year[3]
      && ! i->month[0] && ! i->month[1]
      && ! i->day[0] && ! i->day[1]
      && ! i->hour[0] && ! i->hour[1]
      && ! i->minute[0] && ! i->minute[1]
      && ! i->second[0] && ! i->second[1]
      && ! i->hundredth[0] && ! i->hundredth[1])
    return holy_error (holy_ERR_BAD_NUMBER, "empty date");
  datetime.year = (i->year[0] - '0') * 1000 + (i->year[1] - '0') * 100
    + (i->year[2] - '0') * 10 + (i->year[3] - '0');
  datetime.month = (i->month[0] - '0') * 10 + (i->month[1] - '0');
  datetime.day = (i->day[0] - '0') * 10 + (i->day[1] - '0');
  datetime.hour = (i->hour[0] - '0') * 10 + (i->hour[1] - '0');
  datetime.minute = (i->minute[0] - '0') * 10 + (i->minute[1] - '0');
  datetime.second = (i->second[0] - '0') * 10 + (i->second[1] - '0');
  
  if (!holy_datetime2unixtime (&datetime, nix))
    return holy_error (holy_ERR_BAD_NUMBER, "incorrect date");
  *nix -= i->offset * 60 * 15;
  return holy_ERR_NONE;
}

static int
iso9660_to_unixtime2 (const struct holy_iso9660_date2 *i, holy_int32_t *nix)
{
  struct holy_datetime datetime;

  datetime.year = i->year + 1900;
  datetime.month = i->month;
  datetime.day = i->day;
  datetime.hour = i->hour;
  datetime.minute = i->minute;
  datetime.second = i->second;
  
  if (!holy_datetime2unixtime (&datetime, nix))
    return 0;
  *nix -= i->offset * 60 * 15;
  return 1;
}

static holy_err_t
read_node (holy_fshelp_node_t node, holy_off_t off, holy_size_t len, char *buf)
{
  holy_size_t i = 0;

  while (len > 0)
    {
      holy_size_t toread;
      holy_err_t err;
      while (i < node->have_dirents
	     && off >= holy_le_to_cpu32 (node->dirents[i].size))
	{
	  off -= holy_le_to_cpu32 (node->dirents[i].size);
	  i++;
	}
      if (i == node->have_dirents)
	return holy_error (holy_ERR_OUT_OF_RANGE, "read out of range");
      toread = holy_le_to_cpu32 (node->dirents[i].size);
      if (toread > len)
	toread = len;
      err = holy_disk_read (node->data->disk,
			    ((holy_disk_addr_t) holy_le_to_cpu32 (node->dirents[i].first_sector)) << holy_ISO9660_LOG2_BLKSZ,
			    off, toread, buf);
      if (err)
	return err;
      len -= toread;
      off += toread;
      buf += toread;
    }
  return holy_ERR_NONE;
}

/* Iterate over the susp entries, starting with block SUA_BLOCK on the
   offset SUA_POS with a size of SUA_SIZE bytes.  Hook is called for
   every entry.  */
static holy_err_t
holy_iso9660_susp_iterate (holy_fshelp_node_t node, holy_off_t off,
			   holy_ssize_t sua_size,
			   holy_err_t (*hook)
			   (struct holy_iso9660_susp_entry *entry, void *hook_arg),
			   void *hook_arg)
{
  char *sua;
  struct holy_iso9660_susp_entry *entry;
  holy_err_t err;

  if (sua_size <= 0)
    return holy_ERR_NONE;

  sua = holy_malloc (sua_size);
  if (!sua)
    return holy_errno;

  /* Load a part of the System Usage Area.  */
  err = read_node (node, off, sua_size, sua);
  if (err)
    return err;

  for (entry = (struct holy_iso9660_susp_entry *) sua; (char *) entry < (char *) sua + sua_size - 1 && entry->len > 0;
       entry = (struct holy_iso9660_susp_entry *)
	 ((char *) entry + entry->len))
    {
      /* The last entry.  */
      if (holy_strncmp ((char *) entry->sig, "ST", 2) == 0)
	break;

      /* Additional entries are stored elsewhere.  */
      if (holy_strncmp ((char *) entry->sig, "CE", 2) == 0)
	{
	  struct holy_iso9660_susp_ce *ce;
	  holy_disk_addr_t ce_block;

	  ce = (struct holy_iso9660_susp_ce *) entry;
	  sua_size = holy_le_to_cpu32 (ce->len);
	  off = holy_le_to_cpu32 (ce->off);
	  ce_block = holy_le_to_cpu32 (ce->blk) << holy_ISO9660_LOG2_BLKSZ;

	  holy_free (sua);
	  sua = holy_malloc (sua_size);
	  if (!sua)
	    return holy_errno;

	  /* Load a part of the System Usage Area.  */
	  err = holy_disk_read (node->data->disk, ce_block, off,
				sua_size, sua);
	  if (err)
	    return err;

	  entry = (struct holy_iso9660_susp_entry *) sua;
	}

      if (hook (entry, hook_arg))
	{
	  holy_free (sua);
	  return 0;
	}
    }

  holy_free (sua);
  return 0;
}

static char *
holy_iso9660_convert_string (holy_uint8_t *us, int len)
{
  char *p;
  int i;
  holy_uint16_t t[MAX_NAMELEN / 2 + 1];

  p = holy_malloc (len * holy_MAX_UTF8_PER_UTF16 + 1);
  if (! p)
    return NULL;

  for (i=0; i<len; i++)
    t[i] = holy_be_to_cpu16 (holy_get_unaligned16 (us + 2 * i));

  *holy_utf16_to_utf8 ((holy_uint8_t *) p, t, len) = '\0';

  return p;
}

static holy_err_t
susp_iterate_set_rockridge (struct holy_iso9660_susp_entry *susp_entry,
			    void *_data)
{
  struct holy_iso9660_data *data = _data;
  /* The "ER" entry is used to detect extensions.  The
     `IEEE_P1285' extension means Rock ridge.  */
  if (holy_strncmp ((char *) susp_entry->sig, "ER", 2) == 0)
    {
      data->rockridge = 1;
      return 1;
    }
  return 0;
}

static holy_err_t
set_rockridge (struct holy_iso9660_data *data)
{
  int sua_pos;
  int sua_size;
  char *sua;
  struct holy_iso9660_dir rootdir;
  struct holy_iso9660_susp_entry *entry;

  data->rockridge = 0;

  /* Read the system use area and test it to see if SUSP is
     supported.  */
  if (holy_disk_read (data->disk,
		      (holy_le_to_cpu32 (data->voldesc.rootdir.first_sector)
		       << holy_ISO9660_LOG2_BLKSZ), 0,
		      sizeof (rootdir), (char *) &rootdir))
    return holy_error (holy_ERR_BAD_FS, "not a ISO9660 filesystem");

  sua_pos = (sizeof (rootdir) + rootdir.namelen
	     + (rootdir.namelen % 2) - 1);
  sua_size = rootdir.len - sua_pos;

  if (!sua_size)
    return holy_ERR_NONE;

  sua = holy_malloc (sua_size);
  if (! sua)
    return holy_errno;

  if (holy_disk_read (data->disk,
		      (holy_le_to_cpu32 (data->voldesc.rootdir.first_sector)
		       << holy_ISO9660_LOG2_BLKSZ), sua_pos,
		      sua_size, sua))
    {
      holy_free (sua);
      return holy_error (holy_ERR_BAD_FS, "not a ISO9660 filesystem");
    }

  entry = (struct holy_iso9660_susp_entry *) sua;

  /* Test if the SUSP protocol is used on this filesystem.  */
  if (holy_strncmp ((char *) entry->sig, "SP", 2) == 0)
    {
      struct holy_fshelp_node rootnode;

      rootnode.data = data;
      rootnode.alloc_dirents = ARRAY_SIZE (rootnode.dirents);
      rootnode.have_dirents = 1;
      rootnode.have_symlink = 0;
      rootnode.dirents[0] = data->voldesc.rootdir;

      /* The 2nd data byte stored how many bytes are skipped every time
	 to get to the SUA (System Usage Area).  */
      data->susp_skip = entry->data[2];
      entry = (struct holy_iso9660_susp_entry *) ((char *) entry + entry->len);

      /* Iterate over the entries in the SUA area to detect
	 extensions.  */
      if (holy_iso9660_susp_iterate (&rootnode,
				     sua_pos, sua_size, susp_iterate_set_rockridge,
				     data))
	{
	  holy_free (sua);
	  return holy_errno;
	}
    }
  holy_free (sua);
  return holy_ERR_NONE;
}

static struct holy_iso9660_data *
holy_iso9660_mount (holy_disk_t disk)
{
  struct holy_iso9660_data *data = 0;
  struct holy_iso9660_primary_voldesc voldesc;
  int block;

  data = holy_zalloc (sizeof (struct holy_iso9660_data));
  if (! data)
    return 0;

  data->disk = disk;

  block = 16;
  do
    {
      int copy_voldesc = 0;

      /* Read the superblock.  */
      if (holy_disk_read (disk, block << holy_ISO9660_LOG2_BLKSZ, 0,
			  sizeof (struct holy_iso9660_primary_voldesc),
			  (char *) &voldesc))
        {
          holy_error (holy_ERR_BAD_FS, "not a ISO9660 filesystem");
          goto fail;
        }

      if (holy_strncmp ((char *) voldesc.voldesc.magic, "CD001", 5) != 0)
        {
          holy_error (holy_ERR_BAD_FS, "not a ISO9660 filesystem");
          goto fail;
        }

      if (voldesc.voldesc.type == holy_ISO9660_VOLDESC_PRIMARY)
	copy_voldesc = 1;
      else if (!data->rockridge
	       && (voldesc.voldesc.type == holy_ISO9660_VOLDESC_SUPP)
	       && (voldesc.escape[0] == 0x25) && (voldesc.escape[1] == 0x2f)
	       &&
               ((voldesc.escape[2] == 0x40) ||	/* UCS-2 Level 1.  */
                (voldesc.escape[2] == 0x43) ||  /* UCS-2 Level 2.  */
                (voldesc.escape[2] == 0x45)))	/* UCS-2 Level 3.  */
        {
          copy_voldesc = 1;
          data->joliet = 1;
        }

      if (copy_voldesc)
	{
	  holy_memcpy((char *) &data->voldesc, (char *) &voldesc,
		      sizeof (struct holy_iso9660_primary_voldesc));
	  if (set_rockridge (data))
	    goto fail;
	}

      block++;
    } while (voldesc.voldesc.type != holy_ISO9660_VOLDESC_END);

  return data;

 fail:
  holy_free (data);
  return 0;
}


static char *
holy_iso9660_read_symlink (holy_fshelp_node_t node)
{
  return node->have_symlink 
    ? holy_strdup (node->symlink
		   + (node->have_dirents) * sizeof (node->dirents[0])
		   - sizeof (node->dirents)) : holy_strdup ("");
}

static holy_off_t
get_node_size (holy_fshelp_node_t node)
{
  holy_off_t ret = 0;
  holy_size_t i;

  for (i = 0; i < node->have_dirents; i++)
    ret += holy_le_to_cpu32 (node->dirents[i].size);
  return ret;
}

struct iterate_dir_ctx
{
  char *filename;
  int filename_alloc;
  enum holy_fshelp_filetype type;
  char *symlink;
  int was_continue;
};

  /* Extend the symlink.  */
static void
add_part (struct iterate_dir_ctx *ctx,
	  const char *part,
	  int len2)
{
  int size = ctx->symlink ? holy_strlen (ctx->symlink) : 0;

  ctx->symlink = holy_realloc (ctx->symlink, size + len2 + 1);
  if (! ctx->symlink)
    return;

  holy_memcpy (ctx->symlink + size, part, len2);
  ctx->symlink[size + len2] = 0;  
}

static holy_err_t
susp_iterate_dir (struct holy_iso9660_susp_entry *entry,
		  void *_ctx)
{
  struct iterate_dir_ctx *ctx = _ctx;

  /* The filename in the rock ridge entry.  */
  if (holy_strncmp ("NM", (char *) entry->sig, 2) == 0)
    {
      /* The flags are stored at the data position 0, here the
	 filename type is stored.  */
      /* FIXME: Fix this slightly improper cast.  */
      if (entry->data[0] & holy_ISO9660_RR_DOT)
	ctx->filename = (char *) ".";
      else if (entry->data[0] & holy_ISO9660_RR_DOTDOT)
	ctx->filename = (char *) "..";
      else if (entry->len >= 5)
	{
	  holy_size_t off = 0, csize = 1;
	  char *old;
	  csize = entry->len - 5;
	  old = ctx->filename;
	  if (ctx->filename_alloc)
	    {
	      off = holy_strlen (ctx->filename);
	      ctx->filename = holy_realloc (ctx->filename, csize + off + 1);
	    }
	  else
	    {
	      off = 0;
	      ctx->filename = holy_zalloc (csize + 1);
	    }
	  if (!ctx->filename)
	    {
	      ctx->filename = old;
	      return holy_errno;
	    }
	  ctx->filename_alloc = 1;
	  holy_memcpy (ctx->filename + off, (char *) &entry->data[1], csize);
	  ctx->filename[off + csize] = '\0';
	}
    }
  /* The mode information (st_mode).  */
  else if (holy_strncmp ((char *) entry->sig, "PX", 2) == 0)
    {
      /* At position 0 of the PX record the st_mode information is
	 stored (little-endian).  */
      holy_uint32_t mode = ((entry->data[0] + (entry->data[1] << 8))
			    & holy_ISO9660_FSTYPE_MASK);

      switch (mode)
	{
	case holy_ISO9660_FSTYPE_DIR:
	  ctx->type = holy_FSHELP_DIR;
	  break;
	case holy_ISO9660_FSTYPE_REG:
	  ctx->type = holy_FSHELP_REG;
	  break;
	case holy_ISO9660_FSTYPE_SYMLINK:
	  ctx->type = holy_FSHELP_SYMLINK;
	  break;
	default:
	  ctx->type = holy_FSHELP_UNKNOWN;
	}
    }
  else if (holy_strncmp ("SL", (char *) entry->sig, 2) == 0)
    {
      unsigned int pos = 1;

      /* The symlink is not stored as a POSIX symlink, translate it.  */
      while (pos + sizeof (*entry) < entry->len)
	{
	  /* The current position is the `Component Flag'.  */
	  switch (entry->data[pos] & 30)
	    {
	    case 0:
	      {
		/* The data on pos + 2 is the actual data, pos + 1
		   is the length.  Both are part of the `Component
		   Record'.  */
		if (ctx->symlink && !ctx->was_continue)
		  add_part (ctx, "/", 1);
		add_part (ctx, (char *) &entry->data[pos + 2],
			  entry->data[pos + 1]);
		ctx->was_continue = (entry->data[pos] & 1);
		break;
	      }

	    case 2:
	      add_part (ctx, "./", 2);
	      break;

	    case 4:
	      add_part (ctx, "../", 3);
	      break;

	    case 8:
	      add_part (ctx, "/", 1);
	      break;
	    }
	  /* In pos + 1 the length of the `Component Record' is
	     stored.  */
	  pos += entry->data[pos + 1] + 2;
	}

      /* Check if `holy_realloc' failed.  */
      if (holy_errno)
	return holy_errno;
    }

  return 0;
}

static int
holy_iso9660_iterate_dir (holy_fshelp_node_t dir,
			  holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  struct holy_iso9660_dir dirent;
  holy_off_t offset = 0;
  holy_off_t len;
  struct iterate_dir_ctx ctx;

  len = get_node_size (dir);

  for (; offset < len; offset += dirent.len)
    {
      ctx.symlink = 0;
      ctx.was_continue = 0;

      if (read_node (dir, offset, sizeof (dirent), (char *) &dirent))
	return 0;

      /* The end of the block, skip to the next one.  */
      if (!dirent.len)
	{
	  offset = (offset / holy_ISO9660_BLKSZ + 1) * holy_ISO9660_BLKSZ;
	  continue;
	}

      {
	char name[MAX_NAMELEN + 1];
	int nameoffset = offset + sizeof (dirent);
	struct holy_fshelp_node *node;
	int sua_off = (sizeof (dirent) + dirent.namelen + 1
		       - (dirent.namelen % 2));
	int sua_size = dirent.len - sua_off;

	sua_off += offset + dir->data->susp_skip;

	ctx.filename = 0;
	ctx.filename_alloc = 0;
	ctx.type = holy_FSHELP_UNKNOWN;

	if (dir->data->rockridge
	    && holy_iso9660_susp_iterate (dir, sua_off, sua_size,
					  susp_iterate_dir, &ctx))
	  return 0;

	/* Read the name.  */
	if (read_node (dir, nameoffset, dirent.namelen, (char *) name))
	  return 0;

	node = holy_malloc (sizeof (struct holy_fshelp_node));
	if (!node)
	  return 0;

	node->alloc_dirents = ARRAY_SIZE (node->dirents);
	node->have_dirents = 1;

	/* Setup a new node.  */
	node->data = dir->data;
	node->have_symlink = 0;

	/* If the filetype was not stored using rockridge, use
	   whatever is stored in the iso9660 filesystem.  */
	if (ctx.type == holy_FSHELP_UNKNOWN)
	  {
	    if ((dirent.flags & FLAG_TYPE) == FLAG_TYPE_DIR)
	      ctx.type = holy_FSHELP_DIR;
	    else
	      ctx.type = holy_FSHELP_REG;
	  }

	/* . and .. */
	if (!ctx.filename && dirent.namelen == 1 && name[0] == 0)
	  ctx.filename = (char *) ".";

	if (!ctx.filename && dirent.namelen == 1 && name[0] == 1)
	  ctx.filename = (char *) "..";

	/* The filename was not stored in a rock ridge entry.  Read it
	   from the iso9660 filesystem.  */
	if (!dir->data->joliet && !ctx.filename)
	  {
	    char *ptr;
	    name[dirent.namelen] = '\0';
	    ctx.filename = holy_strrchr (name, ';');
	    if (ctx.filename)
	      *ctx.filename = '\0';
	    /* ISO9660 names are not case-preserving.  */
	    ctx.type |= holy_FSHELP_CASE_INSENSITIVE;
	    for (ptr = name; *ptr; ptr++)
	      *ptr = holy_tolower (*ptr);
	    if (ptr != name && *(ptr - 1) == '.')
	      *(ptr - 1) = 0;
	    ctx.filename = name;
	  }

        if (dir->data->joliet && !ctx.filename)
          {
            char *semicolon;

            ctx.filename = holy_iso9660_convert_string
                  ((holy_uint8_t *) name, dirent.namelen >> 1);

	    semicolon = holy_strrchr (ctx.filename, ';');
	    if (semicolon)
	      *semicolon = '\0';

            ctx.filename_alloc = 1;
          }

	node->dirents[0] = dirent;
	while (dirent.flags & FLAG_MORE_EXTENTS)
	  {
	    offset += dirent.len;
	    if (read_node (dir, offset, sizeof (dirent), (char *) &dirent))
	      {
		if (ctx.filename_alloc)
		  holy_free (ctx.filename);
		holy_free (node);
		return 0;
	      }
	    if (node->have_dirents >= node->alloc_dirents)
	      {
		struct holy_fshelp_node *new_node;
		node->alloc_dirents *= 2;
		new_node = holy_realloc (node,
					 sizeof (struct holy_fshelp_node)
					 + ((node->alloc_dirents
					     - ARRAY_SIZE (node->dirents))
					    * sizeof (node->dirents[0])));
		if (!new_node)
		  {
		    if (ctx.filename_alloc)
		      holy_free (ctx.filename);
		    holy_free (node);
		    return 0;
		  }
		node = new_node;
	      }
	    node->dirents[node->have_dirents++] = dirent;
	  }
	if (ctx.symlink)
	  {
	    if ((node->alloc_dirents - node->have_dirents)
		* sizeof (node->dirents[0]) < holy_strlen (ctx.symlink) + 1)
	      {
		struct holy_fshelp_node *new_node;
		new_node = holy_realloc (node,
					 sizeof (struct holy_fshelp_node)
					 + ((node->alloc_dirents
					     - ARRAY_SIZE (node->dirents))
					    * sizeof (node->dirents[0]))
					 + holy_strlen (ctx.symlink) + 1);
		if (!new_node)
		  {
		    if (ctx.filename_alloc)
		      holy_free (ctx.filename);
		    holy_free (node);
		    return 0;
		  }
		node = new_node;
	      }
	    node->have_symlink = 1;
	    holy_strcpy (node->symlink
			 + node->have_dirents * sizeof (node->dirents[0])
			 - sizeof (node->dirents), ctx.symlink);
	    holy_free (ctx.symlink);
	    ctx.symlink = 0;
	    ctx.was_continue = 0;
	  }
	if (hook (ctx.filename, ctx.type, node, hook_data))
	  {
	    if (ctx.filename_alloc)
	      holy_free (ctx.filename);
	    return 1;
	  }
	if (ctx.filename_alloc)
	  holy_free (ctx.filename);
      }
    }

  return 0;
}



/* Context for holy_iso9660_dir.  */
struct holy_iso9660_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_iso9660_dir.  */
static int
holy_iso9660_dir_iter (const char *filename,
		       enum holy_fshelp_filetype filetype,
		       holy_fshelp_node_t node, void *data)
{
  struct holy_iso9660_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));
  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  info.mtimeset = !!iso9660_to_unixtime2 (&node->dirents[0].mtime, &info.mtime);

  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_iso9660_dir (holy_device_t device, const char *path,
		  holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_iso9660_dir_ctx ctx = { hook, hook_data };
  struct holy_iso9660_data *data = 0;
  struct holy_fshelp_node rootnode;
  struct holy_fshelp_node *foundnode;

  holy_dl_ref (my_mod);

  data = holy_iso9660_mount (device->disk);
  if (! data)
    goto fail;

  rootnode.data = data;
  rootnode.alloc_dirents = 0;
  rootnode.have_dirents = 1;
  rootnode.have_symlink = 0;
  rootnode.dirents[0] = data->voldesc.rootdir;

  /* Use the fshelp function to traverse the path.  */
  if (holy_fshelp_find_file (path, &rootnode,
			     &foundnode,
			     holy_iso9660_iterate_dir,
			     holy_iso9660_read_symlink,
			     holy_FSHELP_DIR))
    goto fail;

  /* List the files in the directory.  */
  holy_iso9660_iterate_dir (foundnode, holy_iso9660_dir_iter, &ctx);

  if (foundnode != &rootnode)
    holy_free (foundnode);

 fail:
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_errno;
}


/* Open a file named NAME and initialize FILE.  */
static holy_err_t
holy_iso9660_open (struct holy_file *file, const char *name)
{
  struct holy_iso9660_data *data;
  struct holy_fshelp_node rootnode;
  struct holy_fshelp_node *foundnode;

  holy_dl_ref (my_mod);

  data = holy_iso9660_mount (file->device->disk);
  if (!data)
    goto fail;

  rootnode.data = data;
  rootnode.alloc_dirents = 0;
  rootnode.have_dirents = 1;
  rootnode.have_symlink = 0;
  rootnode.dirents[0] = data->voldesc.rootdir;

  /* Use the fshelp function to traverse the path.  */
  if (holy_fshelp_find_file (name, &rootnode,
			     &foundnode,
			     holy_iso9660_iterate_dir,
			     holy_iso9660_read_symlink,
			     holy_FSHELP_REG))
    goto fail;

  data->node = foundnode;
  file->data = data;
  file->size = get_node_size (foundnode);
  file->offset = 0;

  return 0;

 fail:
  holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}


static holy_ssize_t
holy_iso9660_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_iso9660_data *data =
    (struct holy_iso9660_data *) file->data;
  holy_err_t err;

  /* XXX: The file is stored in as a single extent.  */
  data->disk->read_hook = file->read_hook;
  data->disk->read_hook_data = file->read_hook_data;
  err = read_node (data->node, file->offset, len, buf);
  data->disk->read_hook = NULL;

  if (err || holy_errno)
    return -1;

  return len;
}


static holy_err_t
holy_iso9660_close (holy_file_t file)
{
  struct holy_iso9660_data *data =
    (struct holy_iso9660_data *) file->data;
  holy_free (data->node);
  holy_free (data);

  holy_dl_unref (my_mod);

  return holy_ERR_NONE;
}


static holy_err_t
holy_iso9660_label (holy_device_t device, char **label)
{
  struct holy_iso9660_data *data;
  data = holy_iso9660_mount (device->disk);

  if (data)
    {
      if (data->joliet)
        *label = holy_iso9660_convert_string (data->voldesc.volname, 16);
      else
        *label = holy_strndup ((char *) data->voldesc.volname, 32);
      if (*label)
	{
	  char *ptr;
	  for (ptr = *label; *ptr;ptr++);
	  ptr--;
	  while (ptr >= *label && *ptr == ' ')
	    *ptr-- = 0;
	}

      holy_free (data);
    }
  else
    *label = 0;

  return holy_errno;
}


static holy_err_t
holy_iso9660_uuid (holy_device_t device, char **uuid)
{
  struct holy_iso9660_data *data;
  holy_disk_t disk = device->disk;

  holy_dl_ref (my_mod);

  data = holy_iso9660_mount (disk);
  if (data)
    {
      if (! data->voldesc.modified.year[0] && ! data->voldesc.modified.year[1]
	  && ! data->voldesc.modified.year[2] && ! data->voldesc.modified.year[3]
	  && ! data->voldesc.modified.month[0] && ! data->voldesc.modified.month[1]
	  && ! data->voldesc.modified.day[0] && ! data->voldesc.modified.day[1]
	  && ! data->voldesc.modified.hour[0] && ! data->voldesc.modified.hour[1]
	  && ! data->voldesc.modified.minute[0] && ! data->voldesc.modified.minute[1]
	  && ! data->voldesc.modified.second[0] && ! data->voldesc.modified.second[1]
	  && ! data->voldesc.modified.hundredth[0] && ! data->voldesc.modified.hundredth[1])
	{
	  holy_error (holy_ERR_BAD_NUMBER, "no creation date in filesystem to generate UUID");
	  *uuid = NULL;
	}
      else
	{
	  *uuid = holy_xasprintf ("%c%c%c%c-%c%c-%c%c-%c%c-%c%c-%c%c-%c%c",
				 data->voldesc.modified.year[0],
				 data->voldesc.modified.year[1],
				 data->voldesc.modified.year[2],
				 data->voldesc.modified.year[3],
				 data->voldesc.modified.month[0],
				 data->voldesc.modified.month[1],
				 data->voldesc.modified.day[0],
				 data->voldesc.modified.day[1],
				 data->voldesc.modified.hour[0],
				 data->voldesc.modified.hour[1],
				 data->voldesc.modified.minute[0],
				 data->voldesc.modified.minute[1],
				 data->voldesc.modified.second[0],
				 data->voldesc.modified.second[1],
				 data->voldesc.modified.hundredth[0],
				 data->voldesc.modified.hundredth[1]);
	}
    }
  else
    *uuid = NULL;

	holy_dl_unref (my_mod);

  holy_free (data);

  return holy_errno;
}

/* Get writing time of filesystem. */
static holy_err_t
holy_iso9660_mtime (holy_device_t device, holy_int32_t *timebuf)
{
  struct holy_iso9660_data *data;
  holy_disk_t disk = device->disk;
  holy_err_t err;

  holy_dl_ref (my_mod);

  data = holy_iso9660_mount (disk);
  if (!data)
    {
      holy_dl_unref (my_mod);
      return holy_errno;
    }
  err = iso9660_to_unixtime (&data->voldesc.modified, timebuf);

  holy_dl_unref (my_mod);

  holy_free (data);

  return err;
}




static struct holy_fs holy_iso9660_fs =
  {
    .name = "iso9660",
    .dir = holy_iso9660_dir,
    .open = holy_iso9660_open,
    .read = holy_iso9660_read,
    .close = holy_iso9660_close,
    .label = holy_iso9660_label,
    .uuid = holy_iso9660_uuid,
    .mtime = holy_iso9660_mtime,
#ifdef holy_UTIL
    .reserved_first_sector = 1,
    .blocklist_install = 1,
#endif
    .next = 0
  };

holy_MOD_INIT(iso9660)
{
  holy_fs_register (&holy_iso9660_fs);
  my_mod = mod;
}

holy_MOD_FINI(iso9660)
{
  holy_fs_unregister (&holy_iso9660_fs);
}
