/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_FILE_HEADER
#define holy_FILE_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/device.h>
#include <holy/fs.h>
#include <holy/disk.h>

typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef unsigned long long int holy_uint64_t;

typedef signed char holy_int8_t;
typedef signed short holy_int16_t;
typedef signed int holy_int32_t;
typedef signed long long int holy_int64_t;

typedef holy_int64_t holy_ssize_t;
typedef holy_uint64_t holy_size_t;

typedef holy_uint64_t holy_off_t;
typedef holy_uint64_t holy_disk_addr_t;

/* READ: We have to "compensate" for the non-working imports */
struct holy_disk;
struct holy_net;

struct holy_device {
    struct holy_disk *disk;
    struct holy_net *net;
}

typedef struct holy_device *holy_device_t;


/* File description.  */
struct holy_file
{
  /* File name.  */
  char *name;

  /* The underlying device.  */
  holy_device_t device;

  /* The underlying filesystem.  */
  holy_fs_t fs;

  /* The current offset.  */
  holy_off_t offset;
  holy_off_t progress_offset;

  /* Progress info. */
  holy_uint64_t last_progress_time;
  holy_off_t last_progress_offset;
  holy_uint64_t estimated_speed;

  /* The file size.  */
  holy_off_t size;

  /* If file is not easily seekable. Should be set by underlying layer.  */
  int not_easily_seekable;

  /* Filesystem-specific data.  */
  void *data;

  /* This is called when a sector is read. Used only for a disk device.  */
  holy_disk_read_hook_t read_hook;

  /* Caller-specific data passed to the read hook.  */
  void *read_hook_data;
};
typedef struct holy_file *holy_file_t;

extern holy_disk_read_hook_t EXPORT_VAR(holy_file_progress_hook);

/* Filters with lower ID are executed first.  */
typedef enum holy_file_filter_id
  {
    holy_FILE_FILTER_PUBKEY,
    holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_XZIO,
    holy_FILE_FILTER_LZOPIO,
    holy_FILE_FILTER_MAX,
    holy_FILE_FILTER_COMPRESSION_FIRST = holy_FILE_FILTER_GZIO,
    holy_FILE_FILTER_COMPRESSION_LAST = holy_FILE_FILTER_LZOPIO,
  } holy_file_filter_id_t;

typedef holy_file_t (*holy_file_filter_t) (holy_file_t in, const char *filename);

extern holy_file_filter_t EXPORT_VAR(holy_file_filters_all)[holy_FILE_FILTER_MAX];
extern holy_file_filter_t EXPORT_VAR(holy_file_filters_enabled)[holy_FILE_FILTER_MAX];

static inline void
holy_file_filter_register (holy_file_filter_id_t id, holy_file_filter_t filter)
{
  holy_file_filters_all[id] = filter;
  holy_file_filters_enabled[id] = filter;
}

static inline void
holy_file_filter_unregister (holy_file_filter_id_t id)
{
  holy_file_filters_all[id] = 0;
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable (holy_file_filter_id_t id)
{
  holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_compression (void)
{
  holy_file_filter_id_t id;

  for (id = holy_FILE_FILTER_COMPRESSION_FIRST;
       id <= holy_FILE_FILTER_COMPRESSION_LAST; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_all (void)
{
  holy_file_filter_id_t id;

  for (id = 0;
       id < holy_FILE_FILTER_MAX; id++)
    holy_file_filters_enabled[id] = 0;
}

static inline void
holy_file_filter_disable_pubkey (void)
{
  holy_file_filters_enabled[holy_FILE_FILTER_PUBKEY] = 0;
}

/* Get a device name from NAME.  */
char *EXPORT_FUNC(holy_file_get_device_name) (const char *name);

holy_file_t EXPORT_FUNC(holy_file_open) (const char *name);
holy_ssize_t EXPORT_FUNC(holy_file_read) (holy_file_t file, void *buf,
					  holy_size_t len);
holy_off_t EXPORT_FUNC(holy_file_seek) (holy_file_t file, holy_off_t offset);
holy_err_t EXPORT_FUNC(holy_file_close) (holy_file_t file);

/* Return value of holy_file_size() in case file size is unknown. */
#define holy_FILE_SIZE_UNKNOWN	 0xffffffffffffffffULL

static inline holy_off_t
holy_file_size (const holy_file_t file)
{
  return file->size;
}

static inline holy_off_t
holy_file_tell (const holy_file_t file)
{
  return file->offset;
}

static inline int
holy_file_seekable (const holy_file_t file)
{
  return !file->not_easily_seekable;
}

holy_file_t
holy_file_offset_open (holy_file_t parent, holy_off_t start,
		       holy_off_t size);
void
holy_file_offset_close (holy_file_t file);

#endif /* ! holy_FILE_HEADER */
