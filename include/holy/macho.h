/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MACHO_H
#define holy_MACHO_H 1
#include <holy/types.h>

/* Multi-architecture header. Always in big-endian. */
struct holy_macho_fat_header
{
  holy_uint32_t magic;
  holy_uint32_t nfat_arch;
} holy_PACKED;

enum
  {
    holy_MACHO_CPUTYPE_IA32 = 0x00000007,
    holy_MACHO_CPUTYPE_AMD64 = 0x01000007
  };

#define holy_MACHO_FAT_MAGIC 0xcafebabe
#define holy_MACHO_FAT_EFI_MAGIC 0x0ef1fab9U

typedef holy_uint32_t holy_macho_cpu_type_t;
typedef holy_uint32_t holy_macho_cpu_subtype_t;

/* Architecture descriptor. Always in big-endian. */
struct holy_macho_fat_arch
{
  holy_macho_cpu_type_t cputype;
  holy_macho_cpu_subtype_t cpusubtype;
  holy_uint32_t offset;
  holy_uint32_t size;
  holy_uint32_t align;
} holy_PACKED;

/* File header for 32-bit. Always in native-endian. */
struct holy_macho_header32
{
#define holy_MACHO_MAGIC32 0xfeedface
  holy_uint32_t magic;
  holy_macho_cpu_type_t cputype;
  holy_macho_cpu_subtype_t cpusubtype;
  holy_uint32_t filetype;
  holy_uint32_t ncmds;
  holy_uint32_t sizeofcmds;
  holy_uint32_t flags;
} holy_PACKED;

/* File header for 64-bit. Always in native-endian. */
struct holy_macho_header64
{
#define holy_MACHO_MAGIC64 0xfeedfacf
  holy_uint32_t magic;
  holy_macho_cpu_type_t cputype;
  holy_macho_cpu_subtype_t cpusubtype;
  holy_uint32_t filetype;
  holy_uint32_t ncmds;
  holy_uint32_t sizeofcmds;
  holy_uint32_t flags;
  holy_uint32_t reserved;
} holy_PACKED;

/* Common header of Mach-O commands. */
struct holy_macho_cmd
{
  holy_uint32_t cmd;
  holy_uint32_t cmdsize;
} holy_PACKED;

typedef holy_uint32_t holy_macho_vmprot_t;

/* 32-bit segment command. */
struct holy_macho_segment32
{
#define holy_MACHO_CMD_SEGMENT32  1
  holy_uint32_t cmd;
  holy_uint32_t cmdsize;
  holy_uint8_t segname[16];
  holy_uint32_t vmaddr;
  holy_uint32_t vmsize;
  holy_uint32_t fileoff;
  holy_uint32_t filesize;
  holy_macho_vmprot_t maxprot;
  holy_macho_vmprot_t initprot;
  holy_uint32_t nsects;
  holy_uint32_t flags;
} holy_PACKED;

/* 64-bit segment command. */
struct holy_macho_segment64
{
#define holy_MACHO_CMD_SEGMENT64  0x19
  holy_uint32_t cmd;
  holy_uint32_t cmdsize;
  holy_uint8_t segname[16];
  holy_uint64_t vmaddr;
  holy_uint64_t vmsize;
  holy_uint64_t fileoff;
  holy_uint64_t filesize;
  holy_macho_vmprot_t maxprot;
  holy_macho_vmprot_t initprot;
  holy_uint32_t nsects;
  holy_uint32_t flags;
} holy_PACKED;

#define holy_MACHO_CMD_THREAD     5

struct holy_macho_lzss_header
{
  char magic[8];
#define holy_MACHO_LZSS_MAGIC "complzss"
  holy_uint32_t unused;
  holy_uint32_t uncompressed_size;
  holy_uint32_t compressed_size;
};

/* Convenience union. What do we need to load to identify the file type. */
union holy_macho_filestart
{
  struct holy_macho_fat_header fat;
  struct holy_macho_header32 thin32;
  struct holy_macho_header64 thin64;
  struct holy_macho_lzss_header lzss;
} holy_PACKED;

struct holy_macho_thread32
{
  holy_uint32_t cmd;
  holy_uint32_t cmdsize;
  holy_uint8_t unknown1[48];
  holy_uint32_t entry_point;
  holy_uint8_t unknown2[20];
} holy_PACKED;

struct holy_macho_thread64
{
  holy_uint32_t cmd;
  holy_uint32_t cmdsize;
  holy_uint8_t unknown1[0x88];
  holy_uint64_t entry_point;
  holy_uint8_t unknown2[0x20];
} holy_PACKED;

#define holy_MACHO_LZSS_OFFSET 0x180

holy_size_t
holy_decompress_lzss (holy_uint8_t *dst, holy_uint8_t *dstend,
		      holy_uint8_t *src, holy_uint8_t *srcend);

#endif
