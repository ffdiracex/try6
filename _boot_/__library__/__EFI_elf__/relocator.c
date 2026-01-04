/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/relocator.h>
#include <holy/relocator_private.h>
#include <holy/memory.h>
#include <holy/efi/efi.h>
#include <holy/efi/api.h>
#include <holy/term.h>

#define NEXT_MEMORY_DESCRIPTOR(desc, size)	\
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

unsigned 
holy_relocator_firmware_get_max_events (void)
{
  holy_efi_uintn_t mmapsize = 0, descriptor_size = 0;
  holy_efi_uint32_t descriptor_version = 0;
  holy_efi_uintn_t key;
  holy_efi_get_memory_map (&mmapsize, NULL, &key, &descriptor_size,
			   &descriptor_version);
  /* Since holy_relocator_firmware_fill_events uses malloc
     we need some reserve. Hence +10.  */
  return 2 * (mmapsize / descriptor_size + 10);
}

unsigned 
holy_relocator_firmware_fill_events (struct holy_relocator_mmap_event *events)
{
  holy_efi_uintn_t mmapsize = 0, desc_size = 0;
  holy_efi_uint32_t descriptor_version = 0;
  holy_efi_memory_descriptor_t *descs = NULL;
  holy_efi_uintn_t key;
  int counter = 0;
  holy_efi_memory_descriptor_t *desc;

  holy_efi_get_memory_map (&mmapsize, NULL, &key, &desc_size,
			   &descriptor_version);
  descs = holy_malloc (mmapsize);
  if (!descs)
    return 0;

  holy_efi_get_memory_map (&mmapsize, descs, &key, &desc_size,
			   &descriptor_version);

  for (desc = descs;
       (char *) desc < ((char *) descs + mmapsize);
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      holy_uint64_t start = desc->physical_start;
      holy_uint64_t end = desc->physical_start + (desc->num_pages << 12);

      /* post-4G addresses are never supported on 32-bit EFI. 
	 Moreover it has been reported that some 64-bit EFI contrary to the
	 spec don't map post-4G pages. So if you enable post-4G allocations,
	 map pages manually or check that they are mapped.
       */
      if (end >= 0x100000000ULL)
	end = 0x100000000ULL;
      if (end <= start)
	continue;
      if (desc->type != holy_EFI_CONVENTIONAL_MEMORY)
	continue;
      events[counter].type = REG_FIRMWARE_START;
      events[counter].pos = start;
      counter++;
      events[counter].type = REG_FIRMWARE_END;
      events[counter].pos = end;
      counter++;      
    }

  return counter;
}

int
holy_relocator_firmware_alloc_region (holy_addr_t start, holy_size_t size)
{
  holy_efi_boot_services_t *b;
  holy_efi_physical_address_t address = start;
  holy_efi_status_t status;

  if (holy_efi_is_finished)
    return 1;
#ifdef DEBUG_RELOCATOR_NOMEM_DPRINTF
  holy_dprintf ("relocator", "EFI alloc: %llx, %llx\n",
		(unsigned long long) start, (unsigned long long) size);
#endif
  b = holy_efi_system_table->boot_services;
  status = efi_call_4 (b->allocate_pages, holy_EFI_ALLOCATE_ADDRESS,
		       holy_EFI_LOADER_DATA, size >> 12, &address);
  return (status == holy_EFI_SUCCESS);
}

void
holy_relocator_firmware_free_region (holy_addr_t start, holy_size_t size)
{
  holy_efi_boot_services_t *b;

  if (holy_efi_is_finished)
    return;

  b = holy_efi_system_table->boot_services;
  efi_call_2 (b->free_pages, start, size >> 12);
}
