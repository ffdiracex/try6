/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_HEADER
#define holy_MEMORY_HEADER	1

#include <holy/types.h>
#include <holy/err.h>

typedef enum holy_memory_type
  {
    holy_MEMORY_AVAILABLE = 1,
    holy_MEMORY_RESERVED = 2,
    holy_MEMORY_ACPI = 3,
    holy_MEMORY_NVS = 4,
    holy_MEMORY_BADRAM = 5,
    holy_MEMORY_PERSISTENT = 7,
    holy_MEMORY_PERSISTENT_LEGACY = 12,
    holy_MEMORY_COREBOOT_TABLES = 16,
    holy_MEMORY_CODE = 20,
    /* This one is special: it's used internally but is never reported
       by firmware. Don't use -1 as it's used internally for other purposes. */
    holy_MEMORY_HOLE = -2,
    holy_MEMORY_MAX = 0x10000
  } holy_memory_type_t;

typedef int (*holy_memory_hook_t) (holy_uint64_t,
				   holy_uint64_t,
				   holy_memory_type_t,
				   void *);

holy_err_t holy_mmap_iterate (holy_memory_hook_t hook, void *hook_data);

#ifdef holy_MACHINE_EFI
holy_err_t
holy_efi_mmap_iterate (holy_memory_hook_t hook, void *hook_data,
		       int avoid_efi_boot_services);
#endif

#if !defined (holy_MACHINE_EMU) && !defined (holy_MACHINE_EFI)
holy_err_t EXPORT_FUNC(holy_machine_mmap_iterate) (holy_memory_hook_t hook,
						   void *hook_data);
#else
holy_err_t holy_machine_mmap_iterate (holy_memory_hook_t hook,
				      void *hook_data);
#endif

int holy_mmap_register (holy_uint64_t start, holy_uint64_t size, int type);
holy_err_t holy_mmap_unregister (int handle);

void *holy_mmap_malign_and_register (holy_uint64_t align, holy_uint64_t size,
				     int *handle, int type, int flags);

void holy_mmap_free_and_unregister (int handle);

#ifndef holy_MMAP_REGISTER_BY_FIRMWARE

struct holy_mmap_region
{
  struct holy_mmap_region *next;
  holy_uint64_t start;
  holy_uint64_t end;
  holy_memory_type_t type;
  int handle;
  int priority;
};

extern struct holy_mmap_region *holy_mmap_overlays;
#endif

#endif /* ! holy_MEMORY_HEADER */
