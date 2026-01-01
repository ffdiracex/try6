/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/machine/memory.h>
#include <holy/memory.h>
#include <holy/err.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/mm.h>
#include <holy/misc.h>

#define NEXT_MEMORY_DESCRIPTOR(desc, size)      \
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

holy_err_t
holy_efi_mmap_iterate (holy_memory_hook_t hook, void *hook_data,
		       int avoid_efi_boot_services)
{
  holy_efi_uintn_t mmap_size = 0;
  holy_efi_memory_descriptor_t *map_buf = 0;
  holy_efi_uintn_t map_key = 0;
  holy_efi_uintn_t desc_size = 0;
  holy_efi_uint32_t desc_version = 0;
  holy_efi_memory_descriptor_t *desc;

  if (holy_efi_get_memory_map (&mmap_size, map_buf,
			       &map_key, &desc_size,
			       &desc_version) < 0)
    return holy_errno;

  map_buf = holy_malloc (mmap_size);
  if (! map_buf)
    return holy_errno;

  if (holy_efi_get_memory_map (&mmap_size, map_buf,
			       &map_key, &desc_size,
			       &desc_version) <= 0)
    {
      holy_free (map_buf);
      return holy_errno;
    }

  for (desc = map_buf;
       desc < NEXT_MEMORY_DESCRIPTOR (map_buf, mmap_size);
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      holy_dprintf ("mmap", "EFI memory region 0x%llx-0x%llx: %d\n",
		    (unsigned long long) desc->physical_start,
		    (unsigned long long) desc->physical_start
		    + desc->num_pages * 4096, desc->type);
      switch (desc->type)
	{
	case holy_EFI_BOOT_SERVICES_CODE:
	  if (!avoid_efi_boot_services)
	    {
	      hook (desc->physical_start, desc->num_pages * 4096,
		    holy_MEMORY_AVAILABLE, hook_data);
	      break;
	    }
	  /* FALLTHROUGH */
	case holy_EFI_RUNTIME_SERVICES_CODE:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_CODE, hook_data);
	  break;

	case holy_EFI_UNUSABLE_MEMORY:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_BADRAM, hook_data);
	  break;

	case holy_EFI_BOOT_SERVICES_DATA:
	  if (!avoid_efi_boot_services)
	    {
	      hook (desc->physical_start, desc->num_pages * 4096,
		    holy_MEMORY_AVAILABLE, hook_data);
	      break;
	    }
	  /* FALLTHROUGH */
	case holy_EFI_RESERVED_MEMORY_TYPE:
	case holy_EFI_RUNTIME_SERVICES_DATA:
	case holy_EFI_MEMORY_MAPPED_IO:
	case holy_EFI_MEMORY_MAPPED_IO_PORT_SPACE:
	case holy_EFI_PAL_CODE:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_RESERVED, hook_data);
	  break;

	case holy_EFI_LOADER_CODE:
	case holy_EFI_LOADER_DATA:
	case holy_EFI_CONVENTIONAL_MEMORY:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_AVAILABLE, hook_data);
	  break;

	case holy_EFI_ACPI_RECLAIM_MEMORY:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_ACPI, hook_data);
	  break;

	case holy_EFI_ACPI_MEMORY_NVS:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_NVS, hook_data);
	  break;

	case holy_EFI_PERSISTENT_MEMORY:
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_PERSISTENT, hook_data);
	break;

	default:
	  holy_printf ("Unknown memory type %d, considering reserved\n",
		       desc->type);
	  hook (desc->physical_start, desc->num_pages * 4096,
		holy_MEMORY_RESERVED, hook_data);
	  break;
	}
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  return holy_efi_mmap_iterate (hook, hook_data, 0);
}

static inline holy_efi_memory_type_t
make_efi_memtype (int type)
{
  switch (type)
    {
    case holy_MEMORY_CODE:
      return holy_EFI_RUNTIME_SERVICES_CODE;

      /* No way to remove a chunk of memory from EFI mmap.
	 So mark it as unusable. */
    case holy_MEMORY_HOLE:
    /*
     * AllocatePages() does not support holy_EFI_PERSISTENT_MEMORY,
     * so no translation for holy_MEMORY_PERSISTENT or
     * holy_MEMORY_PERSISTENT_LEGACY.
     */
    case holy_MEMORY_PERSISTENT:
    case holy_MEMORY_PERSISTENT_LEGACY:
    case holy_MEMORY_RESERVED:
      return holy_EFI_UNUSABLE_MEMORY;

    case holy_MEMORY_AVAILABLE:
      return holy_EFI_CONVENTIONAL_MEMORY;

    case holy_MEMORY_ACPI:
      return holy_EFI_ACPI_RECLAIM_MEMORY;

    case holy_MEMORY_NVS:
      return holy_EFI_ACPI_MEMORY_NVS;
    }

  return holy_EFI_UNUSABLE_MEMORY;
}

struct overlay
{
  struct overlay *next;
  holy_efi_physical_address_t address;
  holy_efi_uintn_t pages;
  int handle;
};

static struct overlay *overlays = 0;
static int curhandle = 1;

int
holy_mmap_register (holy_uint64_t start, holy_uint64_t size, int type)
{
  holy_uint64_t end = start + size;
  holy_efi_physical_address_t address;
  holy_efi_boot_services_t *b;
  holy_efi_uintn_t pages;
  holy_efi_status_t status;
  struct overlay *curover;

  curover = (struct overlay *) holy_malloc (sizeof (struct overlay));
  if (! curover)
    return 0;

  b = holy_efi_system_table->boot_services;
  address = start & (~0xfffULL);
  pages = (end - address + 0xfff) >> 12;
  status = efi_call_2 (b->free_pages, address, pages);
  if (status != holy_EFI_SUCCESS && status != holy_EFI_NOT_FOUND)
    {
      holy_free (curover);
      return 0;
    }
  status = efi_call_4 (b->allocate_pages, holy_EFI_ALLOCATE_ADDRESS,
		       make_efi_memtype (type), pages, &address);
  if (status != holy_EFI_SUCCESS)
    {
      holy_free (curover);
      return 0;
    }
  curover->next = overlays;
  curover->handle = curhandle++;
  curover->address = address;
  curover->pages = pages;
  overlays = curover;

  return curover->handle;
}

holy_err_t
holy_mmap_unregister (int handle)
{
  struct overlay *curover, *prevover;
  holy_efi_boot_services_t *b;

  b = holy_efi_system_table->boot_services;


  for (curover = overlays, prevover = 0; curover;
       prevover = curover, curover = curover->next)
    {
      if (curover->handle == handle)
	{
	  efi_call_2 (b->free_pages, curover->address, curover->pages);
	  if (prevover != 0)
	    prevover->next = curover->next;
	  else
	    overlays = curover->next;
	  holy_free (curover);
	  return holy_ERR_NONE;
	}
    }
  return holy_error (holy_ERR_BUG, "handle %d not found", handle);
}

/* Result is always page-aligned. */
void *
holy_mmap_malign_and_register (holy_uint64_t align __attribute__ ((unused)),
			       holy_uint64_t size,
			       int *handle, int type,
			       int flags __attribute__ ((unused)))
{
  holy_efi_physical_address_t address;
  holy_efi_boot_services_t *b;
  holy_efi_uintn_t pages;
  holy_efi_status_t status;
  struct overlay *curover;
  holy_efi_allocate_type_t atype;

  curover = (struct overlay *) holy_malloc (sizeof (struct overlay));
  if (! curover)
    return 0;

  b = holy_efi_system_table->boot_services;

  address = 0xffffffff;

#if holy_TARGET_SIZEOF_VOID_P < 8
  /* Limit the memory access to less than 4GB for 32-bit platforms.  */
  atype = holy_EFI_ALLOCATE_MAX_ADDRESS;
#else
  atype = holy_EFI_ALLOCATE_ANY_PAGES;
#endif

  pages = (size + 0xfff) >> 12;
  status = efi_call_4 (b->allocate_pages, atype,
		       make_efi_memtype (type), pages, &address);
  if (status != holy_EFI_SUCCESS)
    {
      holy_free (curover);
      return 0;
    }

  if (address == 0)
    {
      /* Uggh, the address 0 was allocated... This is too annoying,
	 so reallocate another one.  */
      address = 0xffffffff;
      status = efi_call_4 (b->allocate_pages, atype,
			   make_efi_memtype (type), pages, &address);
      holy_efi_free_pages (0, pages);
      if (status != holy_EFI_SUCCESS)
	return 0;
    }

  curover->next = overlays;
  curover->handle = curhandle++;
  curover->address = address;
  curover->pages = pages;
  overlays = curover;
  *handle = curover->handle;

  return (void *) (holy_addr_t) curover->address;
}

void
holy_mmap_free_and_unregister (int handle)
{
  holy_mmap_unregister (handle);
}
