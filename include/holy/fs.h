/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FS_HEADER
#define holy_FS_HEADER	1

#include <holy/device.h>
#include <holy/symbol.h>
#include <holy/types.h>

#include <holy/list.h>
/* For embedding types.  */
#ifdef holy_UTIL
#include <holy/partition.h>
#endif

/* Forward declaration is required, because of mutual reference.  */
struct holy_file;

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long int holy_uint64_t;
typedef holy_uint64_t holy_size_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long int holy_int64_t;
typedef holy_int64_t holy_ssize_t;



struct holy_dirhook_info
{
  unsigned dir:1;
  unsigned mtimeset:1;
  unsigned case_insensitive:1;
  unsigned inodeset:1;
  holy_int32_t mtime;
  holy_uint64_t inode;
};

typedef int (*holy_fs_dir_hook_t) (const char *filename,
				   const struct holy_dirhook_info *info,
				   void *data);

/* Filesystem descriptor.  */
struct holy_fs
{
  /* The next filesystem.  */
  struct holy_fs *next;
  struct holy_fs **prev;

  /* My name.  */
  const char *name;

  /* Call HOOK with each file under DIR.  */
  holy_err_t (*dir) (holy_device_t device, const char *path,
		     holy_fs_dir_hook_t hook, void *hook_data);

  /* Open a file named NAME and initialize FILE.  */
  holy_err_t (*open) (struct holy_file *file, const char *name);

  /* Read LEN bytes data from FILE into BUF.  */
  holy_ssize_t (*read) (struct holy_file *file, char *buf, holy_size_t len);

  /* Close the file FILE.  */
  holy_err_t (*close) (struct holy_file *file);

  /* Return the label of the device DEVICE in LABEL.  The label is
     returned in a holy_malloc'ed buffer and should be freed by the
     caller.  */
  holy_err_t (*label) (holy_device_t device, char **label);

  /* Return the uuid of the device DEVICE in UUID.  The uuid is
     returned in a holy_malloc'ed buffer and should be freed by the
     caller.  */
  holy_err_t (*uuid) (holy_device_t device, char **uuid);

  /* Get writing time of filesystem. */
  holy_err_t (*mtime) (holy_device_t device, holy_int32_t *timebuf);

#ifdef holy_UTIL
  /* Determine sectors available for embedding.  */
  holy_err_t (*embed) (holy_device_t device, unsigned int *nsectors,
		       unsigned int max_nsectors,
		       holy_embed_type_t embed_type,
		       holy_disk_addr_t **sectors);

  /* Whether this filesystem reserves first sector for DOS-style boot.  */
  int reserved_first_sector;

  /* Whether blocklist installs have a chance to work.  */
  int blocklist_install;
#endif
};
typedef struct holy_fs *holy_fs_t;

/* This is special, because block lists are not files in usual sense.  */
extern struct holy_fs holy_fs_blocklist;

/* This hook is used to automatically load filesystem modules.
   If this hook loads a module, return non-zero. Otherwise return zero.
   The newly loaded filesystem is assumed to be inserted into the head of
   the linked list holy_FS_LIST through the function holy_fs_register.  */
typedef int (*holy_fs_autoload_hook_t) (void);
extern holy_fs_autoload_hook_t EXPORT_VAR(holy_fs_autoload_hook);
extern holy_fs_t EXPORT_VAR (holy_fs_list);

#ifndef holy_LST_GENERATOR
static inline void
holy_fs_register (holy_fs_t fs)
{
  holy_list_push (holy_AS_LIST_P (&holy_fs_list), holy_AS_LIST (fs));
}
#endif

static inline void
holy_fs_unregister (holy_fs_t fs)
{
  holy_list_remove (holy_AS_LIST (fs));
}

#define FOR_FILESYSTEMS(var) FOR_LIST_ELEMENTS((var), (holy_fs_list))

holy_fs_t EXPORT_FUNC(holy_fs_probe) (holy_device_t device);

#endif /* ! holy_FS_HEADER */
