/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_BSD_CPU_HEADER
#define holy_BSD_CPU_HEADER	1

#include <holy/types.h>
#include <holy/relocator.h>

#include <holy/i386/freebsd_reboot.h>
#include <holy/i386/netbsd_reboot.h>
#include <holy/i386/openbsd_reboot.h>
#include <holy/i386/freebsd_linker.h>
#include <holy/i386/netbsd_bootinfo.h>
#include <holy/i386/openbsd_bootarg.h>

enum bsd_kernel_types
  {
    KERNEL_TYPE_NONE,
    KERNEL_TYPE_FREEBSD,
    KERNEL_TYPE_OPENBSD,
    KERNEL_TYPE_NETBSD,
  };

#define holy_BSD_TEMP_BUFFER   0x80000

#define FREEBSD_B_DEVMAGIC	OPENBSD_B_DEVMAGIC
#define FREEBSD_B_SLICESHIFT	OPENBSD_B_CTRLSHIFT
#define FREEBSD_B_UNITSHIFT	OPENBSD_B_UNITSHIFT
#define FREEBSD_B_PARTSHIFT	OPENBSD_B_PARTSHIFT
#define FREEBSD_B_TYPESHIFT	OPENBSD_B_TYPESHIFT

#define FREEBSD_MODTYPE_KERNEL		"elf kernel"
#define FREEBSD_MODTYPE_KERNEL64	"elf64 kernel"
#define FREEBSD_MODTYPE_ELF_MODULE	"elf module"
#define FREEBSD_MODTYPE_ELF_MODULE_OBJ	"elf obj module"
#define FREEBSD_MODTYPE_RAW		"raw"

#define FREEBSD_BOOTINFO_VERSION 1

struct holy_freebsd_bootinfo
{
  holy_uint32_t version;
  holy_uint8_t unused1[44];
  holy_uint32_t length;
  holy_uint8_t unused2;
  holy_uint8_t boot_device;
  holy_uint8_t unused3[18];
  holy_uint32_t kern_end;
  holy_uint32_t environment;
  holy_uint32_t tags;
} holy_PACKED;

struct freebsd_tag_header
{
  holy_uint32_t type;
  holy_uint32_t len;
};

holy_err_t holy_freebsd_load_elfmodule32 (struct holy_relocator *relocator,
					  holy_file_t file, int argc,
					  char *argv[], holy_addr_t *kern_end);
holy_err_t holy_freebsd_load_elfmodule_obj64 (struct holy_relocator *relocator,
					      holy_file_t file, int argc,
					      char *argv[],
					      holy_addr_t *kern_end);
holy_err_t holy_freebsd_load_elf_meta32 (struct holy_relocator *relocator,
					 holy_file_t file,
					 const char *filename,
					 holy_addr_t *kern_end);
holy_err_t holy_freebsd_load_elf_meta64 (struct holy_relocator *relocator,
					 holy_file_t file,
					 const char *filename,
					 holy_addr_t *kern_end);

holy_err_t holy_netbsd_load_elf_meta32 (struct holy_relocator *relocator,
					holy_file_t file,
					const char *filename,
					holy_addr_t *kern_end);
holy_err_t holy_netbsd_load_elf_meta64 (struct holy_relocator *relocator,
					holy_file_t file,
					const char *filename,
					holy_addr_t *kern_end);

holy_err_t holy_bsd_add_meta (holy_uint32_t type,
			      const void *data, holy_uint32_t len);
holy_err_t holy_freebsd_add_meta_module (const char *filename, const char *type,
					 int argc, char **argv,
					 holy_addr_t addr, holy_uint32_t size);

struct holy_openbsd_ramdisk_descriptor
{
  holy_size_t max_size;
  holy_uint8_t *target;
  holy_uint32_t *size;
};

holy_err_t holy_openbsd_find_ramdisk32 (holy_file_t file,
					const char *filename,
					holy_addr_t kern_start,
					void *kern_chunk_src,
					struct holy_openbsd_ramdisk_descriptor *desc);
holy_err_t holy_openbsd_find_ramdisk64 (holy_file_t file,
					const char *filename,
					holy_addr_t kern_start,
					void *kern_chunk_src,
					struct holy_openbsd_ramdisk_descriptor *desc);

extern holy_uint8_t holy_bsd64_trampoline_start, holy_bsd64_trampoline_end;
extern holy_uint32_t holy_bsd64_trampoline_selfjump;
extern holy_uint32_t holy_bsd64_trampoline_gdt;

#endif /* ! holy_BSD_CPU_HEADER */
