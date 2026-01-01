/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_KERNEL_HEADER
#define holy_KERNEL_HEADER	1

#include <holy/types.h>
#include <holy/symbol.h>

enum
{
  OBJ_TYPE_ELF,
  OBJ_TYPE_MEMDISK,
  OBJ_TYPE_CONFIG,
  OBJ_TYPE_PREFIX,
  OBJ_TYPE_PUBKEY
};

/* The module header.  */
struct holy_module_header
{
  /* The type of object.  */
  holy_uint32_t type;
  /* The size of object (including this header).  */
  holy_uint32_t size;
};

/* "gmim" (holy Module Info Magic).  */
#define holy_MODULE_MAGIC 0x676d696d

struct holy_module_info32
{
  /* Magic number so we know we have modules present.  */
  holy_uint32_t magic;
  /* The offset of the modules.  */
  holy_uint32_t offset;
  /* The size of all modules plus this header.  */
  holy_uint32_t size;
};

struct holy_module_info64
{
  /* Magic number so we know we have modules present.  */
  holy_uint32_t magic;
  holy_uint32_t padding;
  /* The offset of the modules.  */
  holy_uint64_t offset;
  /* The size of all modules plus this header.  */
  holy_uint64_t size;
};

#ifndef holy_UTIL
/* Space isn't reusable on some platforms.  */
/* On Qemu the preload space is readonly.  */
/* On emu there is no preload space.  */
/* On ieee1275 our code assumes that heap is p=v which isn't guaranteed for module space.  */
#if defined (holy_MACHINE_QEMU) || defined (holy_MACHINE_EMU) \
  || defined (holy_MACHINE_EFI) \
  || (defined (holy_MACHINE_IEEE1275) && !defined (__sparc__))
#define holy_KERNEL_PRELOAD_SPACE_REUSABLE 0
#endif

#if defined (holy_MACHINE_PCBIOS) || defined (holy_MACHINE_COREBOOT) \
  || defined (holy_MACHINE_MULTIBOOT) || defined (holy_MACHINE_MIPS_QEMU_MIPS) \
  || defined (holy_MACHINE_MIPS_LOONGSON) || defined (holy_MACHINE_ARC) \
  || (defined (__sparc__) && defined (holy_MACHINE_IEEE1275)) || defined (holy_MACHINE_UBOOT) || defined (holy_MACHINE_XEN)
/* FIXME: stack is between 2 heap regions. Move it.  */
#define holy_KERNEL_PRELOAD_SPACE_REUSABLE 1
#endif

#ifndef holy_KERNEL_PRELOAD_SPACE_REUSABLE
#error "Please check if preload space is reusable on this platform!"
#endif

#if holy_TARGET_SIZEOF_VOID_P == 8
#define holy_module_info holy_module_info64
#else
#define holy_module_info holy_module_info32
#endif

extern holy_addr_t EXPORT_VAR (holy_modbase);

#define FOR_MODULES(var)  for (\
  var = (holy_modbase && ((((struct holy_module_info *) holy_modbase)->magic) == holy_MODULE_MAGIC)) ? (struct holy_module_header *) \
    (holy_modbase + (((struct holy_module_info *) holy_modbase)->offset)) : 0;\
  var && (holy_addr_t) var \
    < (holy_modbase + (((struct holy_module_info *) holy_modbase)->size));    \
  var = (struct holy_module_header *)					\
    (((holy_uint32_t *) var) + ((((struct holy_module_header *) var)->size + sizeof (holy_addr_t) - 1) / sizeof (holy_addr_t)) * (sizeof (holy_addr_t) / sizeof (holy_uint32_t))))

holy_addr_t holy_modules_get_end (void);

#endif

/* The start point of the C code.  */
void holy_main (void) __attribute__ ((noreturn));

/* The machine-specific initialization. This must initialize memory.  */
void holy_machine_init (void);

/* The machine-specific finalization.  */
void EXPORT_FUNC(holy_machine_fini) (int flags);

/* The machine-specific prefix initialization.  */
void
holy_machine_get_bootlocation (char **device, char **path);

/* Register all the exported symbols. This is automatically generated.  */
void holy_register_exported_symbols (void);

extern void (*EXPORT_VAR(holy_net_poll_cards_idle)) (void);

#endif /* ! holy_KERNEL_HEADER */
