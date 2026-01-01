/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHOLOAD_HEADER
#define holy_MACHOLOAD_HEADER	1

#include <holy/err.h>
#include <holy/elf.h>
#include <holy/file.h>
#include <holy/symbol.h>
#include <holy/types.h>

struct holy_macho_file
{
  holy_file_t file;
  holy_ssize_t offset32;
  holy_ssize_t end32;
  int ncmds32;
  holy_size_t cmdsize32;
  holy_uint8_t *cmds32;
  holy_uint8_t *uncompressed32;
  int compressed32;
  holy_size_t compressed_size32;
  holy_size_t uncompressed_size32;
  holy_ssize_t offset64;
  holy_ssize_t end64;
  int ncmds64;
  holy_size_t cmdsize64;
  holy_uint8_t *cmds64;
  holy_uint8_t *uncompressed64;
  int compressed64;
  holy_size_t compressed_size64;
  holy_size_t uncompressed_size64;
};
typedef struct holy_macho_file *holy_macho_t;

holy_macho_t holy_macho_open (const char *, int is_64bit);
holy_macho_t holy_macho_file (holy_file_t file, const char *filename,
			      int is_64bit);
holy_err_t holy_macho_close (holy_macho_t);

holy_err_t holy_macho_size32 (holy_macho_t macho, holy_uint32_t *segments_start,
			      holy_uint32_t *segments_end, int flags,
			      const char *filename);
holy_uint32_t holy_macho_get_entry_point32 (holy_macho_t macho,
					    const char *filename);

holy_err_t holy_macho_size64 (holy_macho_t macho, holy_uint64_t *segments_start,
			      holy_uint64_t *segments_end, int flags,
			      const char *filename);
holy_uint64_t holy_macho_get_entry_point64 (holy_macho_t macho,
					    const char *filename);

/* Ignore BSS segments when loading. */
#define holy_MACHO_NOBSS 0x1
holy_err_t holy_macho_load32 (holy_macho_t macho, const char *filename,
			      char *offset, int flags, int *darwin_version);
holy_err_t holy_macho_load64 (holy_macho_t macho, const char *filename,
			      char *offset, int flags, int *darwin_version);

/* Like filesize and file_read but take only 32-bit part
   for current architecture. */
holy_size_t holy_macho_filesize32 (holy_macho_t macho);
holy_err_t holy_macho_readfile32 (holy_macho_t macho, const char *filename,
				  void *dest);
holy_size_t holy_macho_filesize64 (holy_macho_t macho);
holy_err_t holy_macho_readfile64 (holy_macho_t macho, const char *filename,
				  void *dest);

void holy_macho_parse32 (holy_macho_t macho, const char *filename);
void holy_macho_parse64 (holy_macho_t macho, const char *filename);

#endif /* ! holy_MACHOLOAD_HEADER */
