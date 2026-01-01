/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/types.h>
#include <holy/multiboot.h>
#include <holy/err.h>
#include <holy/misc.h>

/* A pointer to the MBI in its initial location.  */
struct multiboot_info *startup_multiboot_info;

/* The MBI has to be copied to our BSS so that it won't be
   overwritten.  This is its final location.  */
static struct multiboot_info kern_multiboot_info;

/* Unfortunately we can't use heap at this point.  But 32 looks like a sane
   limit (used by memtest86).  */
static holy_uint8_t mmap_entries[sizeof (struct multiboot_mmap_entry) * 32];

void
holy_machine_mmap_init (void)
{
  if (! startup_multiboot_info)
    holy_fatal ("Unable to find Multiboot Information (is CONFIG_MULTIBOOT disabled in coreboot?)");

  /* Move MBI to a safe place.  */
  holy_memmove (&kern_multiboot_info, startup_multiboot_info, sizeof (struct multiboot_info));

  if ((kern_multiboot_info.flags & MULTIBOOT_INFO_MEM_MAP) == 0)
    holy_fatal ("Missing Multiboot memory information");

  /* Move the memory map to a safe place.  */
  if (kern_multiboot_info.mmap_length > sizeof (mmap_entries))
    {
      holy_printf ("WARNING: Memory map size exceeds limit (0x%x > 0x%x); it will be truncated\n",
		   kern_multiboot_info.mmap_length, sizeof (mmap_entries));
      kern_multiboot_info.mmap_length = sizeof (mmap_entries);
    }
  holy_memmove (mmap_entries, (void *) kern_multiboot_info.mmap_addr, kern_multiboot_info.mmap_length);
  kern_multiboot_info.mmap_addr = (holy_uint32_t) mmap_entries;
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  struct multiboot_mmap_entry *entry = (void *) kern_multiboot_info.mmap_addr;

  while ((unsigned long) entry < kern_multiboot_info.mmap_addr + kern_multiboot_info.mmap_length)
    {
      if (hook (entry->addr, entry->len, entry->type, hook_data))
	break;

      entry = (void *) ((holy_addr_t) entry + entry->size + sizeof (entry->size));
    }

  return 0;
}
