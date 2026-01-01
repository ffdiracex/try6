/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MULTIBOOT_HEADER
#define holy_MULTIBOOT_HEADER 1

#include <holy/file.h>

#ifdef holy_USE_MULTIBOOT2
#include <multiboot2.h>
/* Same thing as far as our loader is concerned.  */
#define MULTIBOOT_BOOTLOADER_MAGIC	MULTIBOOT2_BOOTLOADER_MAGIC
#define MULTIBOOT_HEADER_MAGIC		MULTIBOOT2_HEADER_MAGIC
#else
#include <multiboot.h>
#endif

#include <holy/types.h>
#include <holy/err.h>

#ifndef holy_USE_MULTIBOOT2
typedef enum
  {
    holy_MULTIBOOT_QUIRKS_NONE = 0,
    holy_MULTIBOOT_QUIRK_BAD_KLUDGE = 1,
    holy_MULTIBOOT_QUIRK_MODULES_AFTER_KERNEL = 2
  } holy_multiboot_quirks_t;
extern holy_multiboot_quirks_t holy_multiboot_quirks;
#endif

extern struct holy_relocator *holy_multiboot_relocator;

void holy_multiboot (int argc, char *argv[]);
void holy_module (int argc, char *argv[]);

void holy_multiboot_set_accepts_video (int val);
holy_err_t holy_multiboot_make_mbi (holy_uint32_t *target);
void holy_multiboot_free_mbi (void);
holy_err_t holy_multiboot_init_mbi (int argc, char *argv[]);
holy_err_t holy_multiboot_add_module (holy_addr_t start, holy_size_t size,
				      int argc, char *argv[]);
void holy_multiboot_set_bootdev (void);
void
holy_multiboot_add_elfsyms (holy_size_t num, holy_size_t entsize,
			    unsigned shndx, void *data);

holy_uint32_t holy_get_multiboot_mmap_count (void);
holy_err_t holy_multiboot_set_video_mode (void);

/* FIXME: support coreboot as well.  */
#if defined (holy_MACHINE_PCBIOS)
#define holy_MACHINE_HAS_VBE 1
#else
#define holy_MACHINE_HAS_VBE 0
#endif

#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_MULTIBOOT) || defined (holy_MACHINE_QEMU)
#define holy_MACHINE_HAS_VGA_TEXT 1
#else
#define holy_MACHINE_HAS_VGA_TEXT 0
#endif

#if defined (holy_MACHINE_EFI) || defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_MULTIBOOT)
#define holy_MACHINE_HAS_ACPI 1
#else
#define holy_MACHINE_HAS_ACPI 0
#endif

#define holy_MULTIBOOT_CONSOLE_EGA_TEXT 1
#define holy_MULTIBOOT_CONSOLE_FRAMEBUFFER 2

holy_err_t
holy_multiboot_set_console (int console_type, int accepted_consoles,
			    int width, int height, int depth,
			    int console_required);
holy_err_t
holy_multiboot_load (holy_file_t file, const char *filename);

struct mbi_load_data
{
  holy_file_t file;
  const char *filename;
  void *buffer;
  unsigned int mbi_ver;
  int relocatable;
  holy_uint32_t min_addr;
  holy_uint32_t max_addr;
  holy_size_t align;
  holy_uint32_t preference;
  holy_uint32_t link_base_addr;
  holy_uint32_t load_base_addr;
  int avoid_efi_boot_services;
};
typedef struct mbi_load_data mbi_load_data_t;

/* Load ELF32 or ELF64.  */
holy_err_t
holy_multiboot_load_elf (mbi_load_data_t *mld);

extern holy_size_t holy_multiboot_pure_size;
extern holy_size_t holy_multiboot_alloc_mbi;
extern holy_uint32_t holy_multiboot_payload_eip;


#endif /* ! holy_MULTIBOOT_HEADER */
