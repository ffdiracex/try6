/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LOADER_HEADER
#define holy_LOADER_HEADER	1

#include <holy/file.h>
#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/types.h>

/* Check if a loader is loaded.  */
int EXPORT_FUNC (holy_loader_is_loaded) (void);

/* Set loader functions.  */
enum
{
  holy_LOADER_FLAG_NORETURN = 1,
  holy_LOADER_FLAG_PXE_NOT_UNLOAD = 2,
};

void EXPORT_FUNC (holy_loader_set) (holy_err_t (*boot) (void),
				    holy_err_t (*unload) (void),
				    int flags);

/* Unset current loader, if any.  */
void EXPORT_FUNC (holy_loader_unset) (void);

/* Call the boot hook in current loader. This may or may not return,
   depending on the setting by holy_loader_set.  */
holy_err_t holy_loader_boot (void);

/* The space between numbers is intentional for the simplicity of adding new
   values even if external modules use them. */
typedef enum {
  /* A preboot hook which can use everything and turns nothing off. */
  holy_LOADER_PREBOOT_HOOK_PRIO_NORMAL = 400,
  /* A preboot hook which can't use disks and may stop disks. */
  holy_LOADER_PREBOOT_HOOK_PRIO_DISK = 300,
  /* A preboot hook which can't use disks or console and may stop console. */
  holy_LOADER_PREBOOT_HOOK_PRIO_CONSOLE = 200,
  /* A preboot hook which can't use disks or console, can't modify memory map
     and may stop memory services or finalize memory map. */
  holy_LOADER_PREBOOT_HOOK_PRIO_MEMORY = 100,
} holy_loader_preboot_hook_prio_t;

/* Register a preboot hook. */
struct holy_preboot;

struct holy_preboot *EXPORT_FUNC(holy_loader_register_preboot_hook) (holy_err_t (*preboot_func) (int noret),
								     holy_err_t (*preboot_rest_func) (void),
								     holy_loader_preboot_hook_prio_t prio);

/* Unregister given preboot hook. */
void EXPORT_FUNC (holy_loader_unregister_preboot_hook) (struct holy_preboot *hnd);

#ifndef holy_MACHINE_EMU
void holy_boot_init (void);
void holy_boot_fini (void);
#endif

#endif /* ! holy_LOADER_HEADER */
