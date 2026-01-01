/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/cpu/efi/memory.h>

#if defined (__i386__) || defined (__x86_64__)
#include <holy/pci.h>
#endif

#define NEXT_MEMORY_DESCRIPTOR(desc, size)	\
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

#define BYTES_TO_PAGES(bytes)	(((bytes) + 0xfff) >> 12)
#define BYTES_TO_PAGES_DOWN(bytes)	((bytes) >> 12)
#define PAGES_TO_BYTES(pages)	((pages) << 12)

/* The size of a memory map obtained from the firmware. This must be
   a multiplier of 4KB.  */
#define MEMORY_MAP_SIZE	0x3000

/* The minimum and maximum heap size for holy itself.  */
#define MIN_HEAP_SIZE	0x100000
#define MAX_HEAP_SIZE	(1600 * 0x100000)

static void *finish_mmap_buf = 0;
static holy_efi_uintn_t finish_mmap_size = 0;
static holy_efi_uintn_t finish_key = 0;
static holy_efi_uintn_t finish_desc_size;
static holy_efi_uint32_t finish_desc_version;
int holy_efi_is_finished = 0;

/* Allocate pages below a specified address */
void *
holy_efi_allocate_pages_max (holy_efi_physical_address_t max,
			     holy_efi_uintn_t pages)
{
  holy_efi_status_t status;
  holy_efi_boot_services_t *b;
  holy_efi_physical_address_t address = max;

  if (max > 0xffffffff)
    return 0;

  b = holy_efi_system_table->boot_services;
  status = efi_call_4 (b->allocate_pages, holy_EFI_ALLOCATE_MAX_ADDRESS, holy_EFI_LOADER_DATA, pages, &address);

  if (status != holy_EFI_SUCCESS)
    return 0;

  if (address == 0)
    {
      /* Uggh, the address 0 was allocated... This is too annoying,
	 so reallocate another one.  */
      address = max;
      status = efi_call_4 (b->allocate_pages, holy_EFI_ALLOCATE_MAX_ADDRESS, holy_EFI_LOADER_DATA, pages, &address);
      holy_efi_free_pages (0, pages);
      if (status != holy_EFI_SUCCESS)
	return 0;
    }

  return (void *) ((holy_addr_t) address);
}

/* Allocate pages. Return the pointer to the first of allocated pages.  */
void *
holy_efi_allocate_pages (holy_efi_physical_address_t address,
			 holy_efi_uintn_t pages)
{
  holy_efi_allocate_type_t type;
  holy_efi_status_t status;
  holy_efi_boot_services_t *b;

#if 1
  /* Limit the memory access to less than 4GB for 32-bit platforms.  */
  if (address > holy_EFI_MAX_USABLE_ADDRESS)
    return 0;
#endif

#if 1
  if (address == 0)
    {
      type = holy_EFI_ALLOCATE_MAX_ADDRESS;
      address = holy_EFI_MAX_USABLE_ADDRESS;
    }
  else
    type = holy_EFI_ALLOCATE_ADDRESS;
#else
  if (address == 0)
    type = holy_EFI_ALLOCATE_ANY_PAGES;
  else
    type = holy_EFI_ALLOCATE_ADDRESS;
#endif

  b = holy_efi_system_table->boot_services;
  status = efi_call_4 (b->allocate_pages, type, holy_EFI_LOADER_DATA, pages, &address);
  if (status != holy_EFI_SUCCESS)
    return 0;

  if (address == 0)
    {
      /* Uggh, the address 0 was allocated... This is too annoying,
	 so reallocate another one.  */
      address = holy_EFI_MAX_USABLE_ADDRESS;
      status = efi_call_4 (b->allocate_pages, type, holy_EFI_LOADER_DATA, pages, &address);
      holy_efi_free_pages (0, pages);
      if (status != holy_EFI_SUCCESS)
	return 0;
    }

  return (void *) ((holy_addr_t) address);
}

/* Free pages starting from ADDRESS.  */
void
holy_efi_free_pages (holy_efi_physical_address_t address,
		     holy_efi_uintn_t pages)
{
  holy_efi_boot_services_t *b;

  b = holy_efi_system_table->boot_services;
  efi_call_2 (b->free_pages, address, pages);
}

#if defined (__i386__) || defined (__x86_64__)

/* Helper for stop_broadcom.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid,
	   void *data __attribute__ ((unused)))
{
  holy_pci_address_t addr;
  holy_uint8_t cap;
  holy_uint16_t pm_state;

  if ((pciid & 0xffff) != holy_PCI_VENDOR_BROADCOM)
    return 0;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  if (holy_pci_read (addr) >> 24 != holy_PCI_CLASS_NETWORK)
    return 0;
  cap = holy_pci_find_capability (dev, holy_PCI_CAP_POWER_MANAGEMENT);
  if (!cap)
    return 0;
  addr = holy_pci_make_address (dev, cap + 4);
  pm_state = holy_pci_read_word (addr);
  pm_state = pm_state | 0x03;
  holy_pci_write_word (addr, pm_state);
  holy_pci_read_word (addr);
  return 0;
}

static void
stop_broadcom (void)
{
  holy_pci_iterate (find_card, NULL);
}

#endif

holy_err_t
holy_efi_finish_boot_services (holy_efi_uintn_t *outbuf_size, void *outbuf,
			       holy_efi_uintn_t *map_key,
			       holy_efi_uintn_t *efi_desc_size,
			       holy_efi_uint32_t *efi_desc_version)
{
  holy_efi_boot_services_t *b;
  holy_efi_status_t status;

#if defined (__i386__) || defined (__x86_64__)
  const holy_uint16_t apple[] = { 'A', 'p', 'p', 'l', 'e' };
  int is_apple;

  is_apple = (holy_memcmp (holy_efi_system_table->firmware_vendor,
			   apple, sizeof (apple)) == 0);
#endif

  while (1)
    {
      if (holy_efi_get_memory_map (&finish_mmap_size, finish_mmap_buf, &finish_key,
				   &finish_desc_size, &finish_desc_version) < 0)
	return holy_error (holy_ERR_IO, "couldn't retrieve memory map");

      if (outbuf && *outbuf_size < finish_mmap_size)
	return holy_error (holy_ERR_IO, "memory map buffer is too small");

      finish_mmap_buf = holy_malloc (finish_mmap_size);
      if (!finish_mmap_buf)
	return holy_errno;

      if (holy_efi_get_memory_map (&finish_mmap_size, finish_mmap_buf, &finish_key,
				   &finish_desc_size, &finish_desc_version) <= 0)
	{
	  holy_free (finish_mmap_buf);
	  return holy_error (holy_ERR_IO, "couldn't retrieve memory map");
	}

      b = holy_efi_system_table->boot_services;
      status = efi_call_2 (b->exit_boot_services, holy_efi_image_handle,
			   finish_key);
      if (status == holy_EFI_SUCCESS)
	break;

      if (status != holy_EFI_INVALID_PARAMETER)
	{
	  holy_free (finish_mmap_buf);
	  return holy_error (holy_ERR_IO, "couldn't terminate EFI services");
	}

      holy_free (finish_mmap_buf);
      holy_printf ("Trying to terminate EFI services again\n");
    }
  holy_efi_is_finished = 1;
  if (outbuf_size)
    *outbuf_size = finish_mmap_size;
  if (outbuf)
    holy_memcpy (outbuf, finish_mmap_buf, finish_mmap_size);
  if (map_key)
    *map_key = finish_key;
  if (efi_desc_size)
    *efi_desc_size = finish_desc_size;
  if (efi_desc_version)
    *efi_desc_version = finish_desc_version;

#if defined (__i386__) || defined (__x86_64__)
  if (is_apple)
    stop_broadcom ();
#endif

  return holy_ERR_NONE;
}

/* Get the memory map as defined in the EFI spec. Return 1 if successful,
   return 0 if partial, or return -1 if an error occurs.  */
int
holy_efi_get_memory_map (holy_efi_uintn_t *memory_map_size,
			 holy_efi_memory_descriptor_t *memory_map,
			 holy_efi_uintn_t *map_key,
			 holy_efi_uintn_t *descriptor_size,
			 holy_efi_uint32_t *descriptor_version)
{
  holy_efi_status_t status;
  holy_efi_boot_services_t *b;
  holy_efi_uintn_t key;
  holy_efi_uint32_t version;
  holy_efi_uintn_t size;

  if (holy_efi_is_finished)
    {
      int ret = 1;
      if (*memory_map_size < finish_mmap_size)
	{
	  holy_memcpy (memory_map, finish_mmap_buf, *memory_map_size);
	  ret = 0;
	}
      else
	{
	  holy_memcpy (memory_map, finish_mmap_buf, finish_mmap_size);
	  ret = 1;
	}
      *memory_map_size = finish_mmap_size;
      if (map_key)
	*map_key = finish_key;
      if (descriptor_size)
	*descriptor_size = finish_desc_size;
      if (descriptor_version)
	*descriptor_version = finish_desc_version;
      return ret;
    }

  /* Allow some parameters to be missing.  */
  if (! map_key)
    map_key = &key;
  if (! descriptor_version)
    descriptor_version = &version;
  if (! descriptor_size)
    descriptor_size = &size;

  b = holy_efi_system_table->boot_services;
  status = efi_call_5 (b->get_memory_map, memory_map_size, memory_map, map_key,
			      descriptor_size, descriptor_version);
  if (*descriptor_size == 0)
    *descriptor_size = sizeof (holy_efi_memory_descriptor_t);
  if (status == holy_EFI_SUCCESS)
    return 1;
  else if (status == holy_EFI_BUFFER_TOO_SMALL)
    return 0;
  else
    return -1;
}

/* Sort the memory map in place.  */
static void
sort_memory_map (holy_efi_memory_descriptor_t *memory_map,
		 holy_efi_uintn_t desc_size,
		 holy_efi_memory_descriptor_t *memory_map_end)
{
  holy_efi_memory_descriptor_t *d1;
  holy_efi_memory_descriptor_t *d2;

  for (d1 = memory_map;
       d1 < memory_map_end;
       d1 = NEXT_MEMORY_DESCRIPTOR (d1, desc_size))
    {
      holy_efi_memory_descriptor_t *max_desc = d1;

      for (d2 = NEXT_MEMORY_DESCRIPTOR (d1, desc_size);
	   d2 < memory_map_end;
	   d2 = NEXT_MEMORY_DESCRIPTOR (d2, desc_size))
	{
	  if (max_desc->num_pages < d2->num_pages)
	    max_desc = d2;
	}

      if (max_desc != d1)
	{
	  holy_efi_memory_descriptor_t tmp;

	  tmp = *d1;
	  *d1 = *max_desc;
	  *max_desc = tmp;
	}
    }
}

/* Filter the descriptors. holy needs only available memory.  */
static holy_efi_memory_descriptor_t *
filter_memory_map (holy_efi_memory_descriptor_t *memory_map,
		   holy_efi_memory_descriptor_t *filtered_memory_map,
		   holy_efi_uintn_t desc_size,
		   holy_efi_memory_descriptor_t *memory_map_end)
{
  holy_efi_memory_descriptor_t *desc;
  holy_efi_memory_descriptor_t *filtered_desc;

  for (desc = memory_map, filtered_desc = filtered_memory_map;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      if (desc->type == holy_EFI_CONVENTIONAL_MEMORY
#if 1
	  && desc->physical_start <= holy_EFI_MAX_USABLE_ADDRESS
#endif
	  && desc->physical_start + PAGES_TO_BYTES (desc->num_pages) > 0x100000
	  && desc->num_pages != 0)
	{
	  holy_memcpy (filtered_desc, desc, desc_size);

	  /* Avoid less than 1MB, because some loaders seem to be confused.  */
	  if (desc->physical_start < 0x100000)
	    {
	      desc->num_pages -= BYTES_TO_PAGES (0x100000
						 - desc->physical_start);
	      desc->physical_start = 0x100000;
	    }

#if 1
	  if (BYTES_TO_PAGES (filtered_desc->physical_start)
	      + filtered_desc->num_pages
	      > BYTES_TO_PAGES_DOWN (holy_EFI_MAX_USABLE_ADDRESS))
	    filtered_desc->num_pages
	      = (BYTES_TO_PAGES_DOWN (holy_EFI_MAX_USABLE_ADDRESS)
		 - BYTES_TO_PAGES (filtered_desc->physical_start));
#endif

	  if (filtered_desc->num_pages == 0)
	    continue;

	  filtered_desc = NEXT_MEMORY_DESCRIPTOR (filtered_desc, desc_size);
	}
    }

  return filtered_desc;
}

/* Return the total number of pages.  */
static holy_efi_uint64_t
get_total_pages (holy_efi_memory_descriptor_t *memory_map,
		 holy_efi_uintn_t desc_size,
		 holy_efi_memory_descriptor_t *memory_map_end)
{
  holy_efi_memory_descriptor_t *desc;
  holy_efi_uint64_t total = 0;

  for (desc = memory_map;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    total += desc->num_pages;

  return total;
}

/* Add memory regions.  */
static void
add_memory_regions (holy_efi_memory_descriptor_t *memory_map,
		    holy_efi_uintn_t desc_size,
		    holy_efi_memory_descriptor_t *memory_map_end,
		    holy_efi_uint64_t required_pages)
{
  holy_efi_memory_descriptor_t *desc;

  for (desc = memory_map;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      holy_efi_uint64_t pages;
      holy_efi_physical_address_t start;
      void *addr;

      start = desc->physical_start;
      pages = desc->num_pages;
      if (pages > required_pages)
	{
	  start += PAGES_TO_BYTES (pages - required_pages);
	  pages = required_pages;
	}

      addr = holy_efi_allocate_pages (start, pages);
      if (! addr)
	holy_fatal ("cannot allocate conventional memory %p with %u pages",
		    (void *) ((holy_addr_t) start),
		    (unsigned) pages);

      holy_mm_init_region (addr, PAGES_TO_BYTES (pages));

      required_pages -= pages;
      if (required_pages == 0)
	break;
    }

  if (required_pages > 0)
    holy_fatal ("too little memory");
}

#if 0
/* Print the memory map.  */
static void
print_memory_map (holy_efi_memory_descriptor_t *memory_map,
		  holy_efi_uintn_t desc_size,
		  holy_efi_memory_descriptor_t *memory_map_end)
{
  holy_efi_memory_descriptor_t *desc;
  int i;

  for (desc = memory_map, i = 0;
       desc < memory_map_end;
       desc = NEXT_MEMORY_DESCRIPTOR (desc, desc_size), i++)
    {
      holy_printf ("MD: t=%x, p=%llx, v=%llx, n=%llx, a=%llx\n",
		   desc->type, desc->physical_start, desc->virtual_start,
		   desc->num_pages, desc->attribute);
    }
}
#endif

void
holy_efi_mm_init (void)
{
  holy_efi_memory_descriptor_t *memory_map;
  holy_efi_memory_descriptor_t *memory_map_end;
  holy_efi_memory_descriptor_t *filtered_memory_map;
  holy_efi_memory_descriptor_t *filtered_memory_map_end;
  holy_efi_uintn_t map_size;
  holy_efi_uintn_t desc_size;
  holy_efi_uint64_t total_pages;
  holy_efi_uint64_t required_pages;
  int mm_status;

  /* Prepare a memory region to store two memory maps.  */
  memory_map = holy_efi_allocate_pages (0,
					2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE));
  if (! memory_map)
    holy_fatal ("cannot allocate memory");

  /* Obtain descriptors for available memory.  */
  map_size = MEMORY_MAP_SIZE;

  mm_status = holy_efi_get_memory_map (&map_size, memory_map, 0, &desc_size, 0);

  if (mm_status == 0)
    {
      holy_efi_free_pages
	((holy_efi_physical_address_t) ((holy_addr_t) memory_map),
	 2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE));

      /* Freeing/allocating operations may increase memory map size.  */
      map_size += desc_size * 32;

      memory_map = holy_efi_allocate_pages (0, 2 * BYTES_TO_PAGES (map_size));
      if (! memory_map)
	holy_fatal ("cannot allocate memory");

      mm_status = holy_efi_get_memory_map (&map_size, memory_map, 0,
					   &desc_size, 0);
    }

  if (mm_status < 0)
    holy_fatal ("cannot get memory map");

  memory_map_end = NEXT_MEMORY_DESCRIPTOR (memory_map, map_size);

  filtered_memory_map = memory_map_end;

  filtered_memory_map_end = filter_memory_map (memory_map, filtered_memory_map,
					       desc_size, memory_map_end);

  /* By default, request a quarter of the available memory.  */
  total_pages = get_total_pages (filtered_memory_map, desc_size,
				 filtered_memory_map_end);
  required_pages = (total_pages >> 2);
  if (required_pages < BYTES_TO_PAGES (MIN_HEAP_SIZE))
    required_pages = BYTES_TO_PAGES (MIN_HEAP_SIZE);
  else if (required_pages > BYTES_TO_PAGES (MAX_HEAP_SIZE))
    required_pages = BYTES_TO_PAGES (MAX_HEAP_SIZE);

  /* Sort the filtered descriptors, so that holy can allocate pages
     from smaller regions.  */
  sort_memory_map (filtered_memory_map, desc_size, filtered_memory_map_end);

  /* Allocate memory regions for holy's memory management.  */
  add_memory_regions (filtered_memory_map, desc_size,
		      filtered_memory_map_end, required_pages);

#if 0
  /* For debug.  */
  map_size = MEMORY_MAP_SIZE;

  if (holy_efi_get_memory_map (&map_size, memory_map, 0, &desc_size, 0) < 0)
    holy_fatal ("cannot get memory map");

  holy_printf ("printing memory map\n");
  print_memory_map (memory_map, desc_size,
		    NEXT_MEMORY_DESCRIPTOR (memory_map, map_size));
  holy_fatal ("Debug. ");
#endif

  /* Release the memory maps.  */
  holy_efi_free_pages ((holy_addr_t) memory_map,
		       2 * BYTES_TO_PAGES (MEMORY_MAP_SIZE));
}
