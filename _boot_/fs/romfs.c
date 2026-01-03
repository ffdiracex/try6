/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/disk.h>
#include <holy/fs.h>
#include <holy/fshelp.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_romfs_superblock
{
  char magic[8];
#define holy_ROMFS_MAGIC "-rom1fs-"
  holy_uint32_t total_size;
  holy_uint32_t chksum;
  char label[0];
};

struct holy_romfs_file_header
{
  holy_uint32_t next_file;
  holy_uint32_t spec;
  holy_uint32_t size;
  holy_uint32_t chksum;
  char name[0];
};

struct holy_romfs_data
{
  holy_disk_addr_t first_file;
  holy_disk_t disk;
};

struct holy_fshelp_node
{
  holy_disk_addr_t addr;
  struct holy_romfs_data *data;
  holy_disk_addr_t data_addr;
  /* Not filled for root.  */
  struct holy_romfs_file_header file;
};

#define holy_ROMFS_ALIGN 16
#define holy_ROMFS_TYPE_MASK 7
#define holy_ROMFS_TYPE_HARDLINK 0
#define holy_ROMFS_TYPE_DIRECTORY 1
#define holy_ROMFS_TYPE_REGULAR 2
#define holy_ROMFS_TYPE_SYMLINK 3

static holy_err_t
do_checksum (void *in, holy_size_t insize)
{
  holy_uint32_t *a = in;
  holy_size_t sz = insize / 4;
  holy_uint32_t *b = a + sz;
  holy_uint32_t csum = 0;

  while (a < b)
    csum += holy_be_to_cpu32 (*a++);
  if (csum)
    return holy_error (holy_ERR_BAD_FS, "invalid checksum");
  return holy_ERR_NONE;
}

static struct holy_romfs_data *
holy_romfs_mount (holy_device_t dev)
{
  union {
    struct holy_romfs_superblock sb;
    char d[512];
  } sb;
  holy_err_t err;
  char *ptr;
  holy_disk_addr_t sec = 0;
  struct holy_romfs_data *data;
  if (!dev->disk)
    {
      holy_error (holy_ERR_BAD_FS, "not a disk");
      return NULL;
    }
  err = holy_disk_read (dev->disk, 0, 0, sizeof (sb), &sb);
  if (err == holy_ERR_OUT_OF_RANGE)
    err = holy_errno = holy_ERR_BAD_FS;
  if (err)
    return NULL;
  if (holy_be_to_cpu32 (sb.sb.total_size) < sizeof (sb))
    {
      holy_error (holy_ERR_BAD_FS, "too short filesystem");
      return NULL;
    }
  if (holy_memcmp (sb.sb.magic, holy_ROMFS_MAGIC,
		   sizeof (sb.sb.magic)) != 0)
    {
      holy_error (holy_ERR_BAD_FS, "not romfs");
      return NULL;
    }    
  err = do_checksum (&sb, sizeof (sb) < holy_be_to_cpu32 (sb.sb.total_size) ?
		     sizeof (sb) : holy_be_to_cpu32 (sb.sb.total_size));
  if (err)
    {
      holy_error (holy_ERR_BAD_FS, "checksum incorrect");
      return NULL;
    }
  for (ptr = sb.sb.label; (void *) ptr < (void *) (&sb + 1)
	 && ptr - sb.d < (holy_ssize_t) holy_be_to_cpu32 (sb.sb.total_size); ptr++)
    if (!*ptr)
      break;
  while ((void *) ptr == &sb + 1)
    {
      sec++;
      err = holy_disk_read (dev->disk, sec, 0, sizeof (sb), &sb);
      if (err == holy_ERR_OUT_OF_RANGE)
	err = holy_errno = holy_ERR_BAD_FS;
      if (err)
	return NULL;
      for (ptr = sb.d; (void *) ptr < (void *) (&sb + 1)
	     && (ptr - sb.d + (sec << holy_DISK_SECTOR_BITS)
		 < holy_be_to_cpu32 (sb.sb.total_size));
	   ptr++)
	if (!*ptr)
	  break;
    }
  data = holy_malloc (sizeof (*data));
  if (!data)
    return NULL;
  data->first_file = ALIGN_UP (ptr + 1 - sb.d, holy_ROMFS_ALIGN)
    + (sec << holy_DISK_SECTOR_BITS);
  data->disk = dev->disk;
  return data;
}

static char *
holy_romfs_read_symlink (holy_fshelp_node_t node)
{
  char *ret;
  holy_err_t err;
  ret = holy_malloc (holy_be_to_cpu32 (node->file.size) + 1);
  if (!ret)
    return NULL;
  err = holy_disk_read (node->data->disk,
			(node->data_addr) >> holy_DISK_SECTOR_BITS,
			(node->data_addr) & (holy_DISK_SECTOR_SIZE - 1),
			holy_be_to_cpu32 (node->file.size), ret);
  if (err)
    {
      holy_free (ret);
      return NULL;
    }
  ret[holy_be_to_cpu32 (node->file.size)] = 0;
  return ret;
}

static int
holy_romfs_iterate_dir (holy_fshelp_node_t dir,
			holy_fshelp_iterate_dir_hook_t hook, void *hook_data)
{
  holy_disk_addr_t caddr;
  struct holy_romfs_file_header hdr;
  unsigned nptr;
  unsigned i, j;
  holy_size_t a = 0;
  holy_properly_aligned_t *name = NULL;

  for (caddr = dir->data_addr; caddr;
       caddr = holy_be_to_cpu32 (hdr.next_file) & ~(holy_ROMFS_ALIGN - 1))
    {
      holy_disk_addr_t naddr = caddr + sizeof (hdr);
      holy_uint32_t csum = 0;
      enum holy_fshelp_filetype filetype = holy_FSHELP_UNKNOWN;
      struct holy_fshelp_node *node = NULL;
      holy_err_t err;

      err = holy_disk_read (dir->data->disk, caddr >> holy_DISK_SECTOR_BITS,
			    caddr & (holy_DISK_SECTOR_SIZE - 1),
			    sizeof (hdr), &hdr);
      if (err)
	{
	  holy_free (name);
	  return 1;
	}
      for (nptr = 0; ; nptr++, naddr += 16)
	{
	  if (a <= nptr)
	    {
	      holy_properly_aligned_t *on;
	      a = 2 * (nptr + 1);
	      on = name;
	      name = holy_realloc (name, a * 16);
	      if (!name)
		{
		  holy_free (on);
		  return 1;
		}
	    }
	  COMPILE_TIME_ASSERT (16 % sizeof (name[0]) == 0);
	  err = holy_disk_read (dir->data->disk, naddr >> holy_DISK_SECTOR_BITS,
				naddr & (holy_DISK_SECTOR_SIZE - 1),
				16, name + (16 / sizeof (name[0])) * nptr);
	  if (err)
	    return 1;
	  for (j = 0; j < 16; j++)
	    if (!((char *) name)[16 * nptr + j])
	      break;
	  if (j != 16)
	    break;
	}
      for (i = 0; i < sizeof (hdr) / sizeof (holy_uint32_t); i++)
	csum += holy_be_to_cpu32 (((holy_uint32_t *) &hdr)[i]);
      for (i = 0; i < (nptr + 1) * 4; i++)
	csum += holy_be_to_cpu32 (((holy_uint32_t *) name)[i]);
      if (csum != 0)
	{
	  holy_error (holy_ERR_BAD_FS, "invalid checksum");
	  holy_free (name);
	  return 1;
	}
      node = holy_malloc (sizeof (*node));
      if (!node)
	return 1;
      node->addr = caddr;
      node->data_addr = caddr + (nptr + 1) * 16 + sizeof (hdr);
      node->data = dir->data;
      node->file = hdr;
      switch (holy_be_to_cpu32 (hdr.next_file) & holy_ROMFS_TYPE_MASK)
	{
	case holy_ROMFS_TYPE_REGULAR:
	  filetype = holy_FSHELP_REG;
	  break;
	case holy_ROMFS_TYPE_SYMLINK:
	  filetype = holy_FSHELP_SYMLINK;
	  break;
	case holy_ROMFS_TYPE_DIRECTORY:
	  node->data_addr = holy_be_to_cpu32 (hdr.spec);
	  filetype = holy_FSHELP_DIR;
	  break;
	case holy_ROMFS_TYPE_HARDLINK:
	  {
	    holy_disk_addr_t laddr;
	    node->addr = laddr = holy_be_to_cpu32 (hdr.spec);
	    err = holy_disk_read (dir->data->disk,
				  laddr >> holy_DISK_SECTOR_BITS,
				  laddr & (holy_DISK_SECTOR_SIZE - 1),
				  sizeof (node->file), &node->file);
	    if (err)
	      return 1;
	    if ((holy_be_to_cpu32 (node->file.next_file) & holy_ROMFS_TYPE_MASK)
		== holy_ROMFS_TYPE_REGULAR
		|| (holy_be_to_cpu32 (node->file.next_file)
		    & holy_ROMFS_TYPE_MASK) == holy_ROMFS_TYPE_SYMLINK)
	      {
		laddr += sizeof (hdr);
		while (1)
		  {
		    char buf[16];
		    err = holy_disk_read (dir->data->disk,
					  laddr >> holy_DISK_SECTOR_BITS,
					  laddr & (holy_DISK_SECTOR_SIZE - 1),
					  16, buf);
		    if (err)
		      return 1;
		    for (i = 0; i < 16; i++)
		      if (!buf[i])
			break;
		    if (i != 16)
		      break;
		    laddr += 16;
		  }
		node->data_addr = laddr + 16;
	      }
	    if ((holy_be_to_cpu32 (node->file.next_file)
		 & holy_ROMFS_TYPE_MASK) == holy_ROMFS_TYPE_REGULAR)
	      filetype = holy_FSHELP_REG;
	    if ((holy_be_to_cpu32 (node->file.next_file)
		 & holy_ROMFS_TYPE_MASK) == holy_ROMFS_TYPE_SYMLINK)
	      filetype = holy_FSHELP_SYMLINK;
	    if ((holy_be_to_cpu32 (node->file.next_file) & holy_ROMFS_TYPE_MASK)
		== holy_ROMFS_TYPE_DIRECTORY)
	      {
		node->data_addr = holy_be_to_cpu32 (node->file.spec);
		filetype = holy_FSHELP_DIR;
	      }
	    
	    break;
	  }
	}

      if (hook ((char *) name, filetype, node, hook_data))
	{
	  holy_free (name);
	  return 1;
	}
    }
  holy_free (name);
  return 0;
}

/* Context for holy_romfs_dir.  */
struct holy_romfs_dir_ctx
{
  holy_fs_dir_hook_t hook;
  void *hook_data;
};

/* Helper for holy_romfs_dir.  */
static int
holy_romfs_dir_iter (const char *filename, enum holy_fshelp_filetype filetype,
		     holy_fshelp_node_t node, void *data)
{
  struct holy_romfs_dir_ctx *ctx = data;
  struct holy_dirhook_info info;

  holy_memset (&info, 0, sizeof (info));

  info.dir = ((filetype & holy_FSHELP_TYPE_MASK) == holy_FSHELP_DIR);
  holy_free (node);
  return ctx->hook (filename, &info, ctx->hook_data);
}

static holy_err_t
holy_romfs_dir (holy_device_t device, const char *path,
		holy_fs_dir_hook_t hook, void *hook_data)
{
  struct holy_romfs_dir_ctx ctx = { hook, hook_data };
  struct holy_romfs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0, start;

  data = holy_romfs_mount (device);
  if (! data)
    goto fail;

  start.addr = data->first_file;
  start.data_addr = data->first_file;
  start.data = data;
  holy_fshelp_find_file (path, &start, &fdiro, holy_romfs_iterate_dir,
			 holy_romfs_read_symlink, holy_FSHELP_DIR);
  if (holy_errno)
    goto fail;

  holy_romfs_iterate_dir (fdiro, holy_romfs_dir_iter, &ctx);

 fail:
  holy_free (data);

  return holy_errno;
}

static holy_err_t
holy_romfs_open (struct holy_file *file, const char *name)
{
  struct holy_romfs_data *data = 0;
  struct holy_fshelp_node *fdiro = 0, start;

  data = holy_romfs_mount (file->device);
  if (! data)
    goto fail;

  start.addr = data->first_file;
  start.data_addr = data->first_file;
  start.data = data;

  holy_fshelp_find_file (name, &start, &fdiro, holy_romfs_iterate_dir,
			 holy_romfs_read_symlink, holy_FSHELP_REG);
  if (holy_errno)
    goto fail;

  file->size = holy_be_to_cpu32 (fdiro->file.size);
  file->data = fdiro;
  return holy_ERR_NONE;

 fail:
  holy_free (data);

  return holy_errno;
}

static holy_ssize_t
holy_romfs_read (holy_file_t file, char *buf, holy_size_t len)
{
  struct holy_fshelp_node *data = file->data;

  /* XXX: The file is stored in as a single extent.  */
  data->data->disk->read_hook = file->read_hook;
  data->data->disk->read_hook_data = file->read_hook_data;
  holy_disk_read (data->data->disk,
		  (data->data_addr + file->offset) >> holy_DISK_SECTOR_BITS,
		  (data->data_addr + file->offset) & (holy_DISK_SECTOR_SIZE - 1),
		  len, buf);
  data->data->disk->read_hook = NULL;

  if (holy_errno)
    return -1;

  return len;
}

static holy_err_t
holy_romfs_close (holy_file_t file)
{
  struct holy_fshelp_node *data = file->data;

  holy_free (data->data);
  holy_free (data);

  return holy_ERR_NONE;
}

static holy_err_t
holy_romfs_label (holy_device_t device, char **label)
{
  struct holy_romfs_data *data;
  holy_err_t err;

  *label = NULL;

  data = holy_romfs_mount (device);
  if (!data)
    return holy_errno;
  *label = holy_malloc (data->first_file + 1
			- sizeof (struct holy_romfs_superblock));
  if (!*label)
    {
      holy_free (data);
      return holy_errno;
    }
  err = holy_disk_read (device->disk, 0, sizeof (struct holy_romfs_superblock),
			data->first_file
			- sizeof (struct holy_romfs_superblock),
			*label);
  if (err)
    {
      holy_free (data);
      holy_free (*label);
      *label = NULL;
      return err;
    }
  (*label)[data->first_file - sizeof (struct holy_romfs_superblock)] = 0;
  holy_free (data);
  return holy_ERR_NONE;
}


static struct holy_fs holy_romfs_fs =
  {
    .name = "romfs",
    .dir = holy_romfs_dir,
    .open = holy_romfs_open,
    .read = holy_romfs_read,
    .close = holy_romfs_close,
    .label = holy_romfs_label,
#ifdef holy_UTIL
    .reserved_first_sector = 0,
    .blocklist_install = 0,
#endif
    .next = 0
  };

holy_MOD_INIT(romfs)
{
  holy_fs_register (&holy_romfs_fs);
}

holy_MOD_FINI(romfs)
{
  holy_fs_unregister (&holy_romfs_fs);
}
