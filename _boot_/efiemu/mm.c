/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/normal.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/memory.h>

struct holy_efiemu_memrequest
{
  struct holy_efiemu_memrequest *next;
  holy_efi_memory_type_t type;
  holy_size_t size;
  holy_size_t align_overhead;
  int handle;
  void *val;
};
/* Linked list of requested memory. */
static struct holy_efiemu_memrequest *memrequests = 0;
/* Memory map. */
static holy_efi_memory_descriptor_t *efiemu_mmap = 0;
/* Pointer to allocated memory */
static void *resident_memory = 0;
/* Size of requested memory per type */
static holy_size_t requested_memory[holy_EFI_MAX_MEMORY_TYPE];
/* How many slots is allocated for memory_map and how many are already used */
static int mmap_reserved_size = 0, mmap_num = 0;

/* Add a memory region to map*/
static holy_err_t
holy_efiemu_add_to_mmap (holy_uint64_t start, holy_uint64_t size,
			 holy_efi_memory_type_t type)
{
  holy_uint64_t page_start, npages;

  /* Extend map if necessary*/
  if (mmap_num >= mmap_reserved_size)
    {
      void *old;
      mmap_reserved_size = 2 * (mmap_reserved_size + 1);
      old = efiemu_mmap;
      efiemu_mmap = (holy_efi_memory_descriptor_t *)
	holy_realloc (efiemu_mmap, mmap_reserved_size
		      * sizeof (holy_efi_memory_descriptor_t));
      if (!efiemu_mmap)
	{
	  holy_free (old);
	  return holy_errno;
	}
    }

  /* Fill slot*/
  page_start = start - (start % holy_EFIEMU_PAGESIZE);
  npages = (size + (start % holy_EFIEMU_PAGESIZE) + holy_EFIEMU_PAGESIZE - 1)
    / holy_EFIEMU_PAGESIZE;
  efiemu_mmap[mmap_num].physical_start = page_start;
  efiemu_mmap[mmap_num].virtual_start = page_start;
  efiemu_mmap[mmap_num].num_pages = npages;
  efiemu_mmap[mmap_num].type = type;
  mmap_num++;

  return holy_ERR_NONE;
}

/* Request a resident memory of type TYPE of size SIZE aligned at ALIGN
   ALIGN must be a divisor of page size (if it's a divisor of 4096
   it should be ok on all platforms)
 */
int
holy_efiemu_request_memalign (holy_size_t align, holy_size_t size,
			      holy_efi_memory_type_t type)
{
  holy_size_t align_overhead;
  struct holy_efiemu_memrequest *ret, *cur, *prev;
  /* Check that the request is correct */
  if (type <= holy_EFI_LOADER_CODE || type == holy_EFI_PERSISTENT_MEMORY ||
	type >= holy_EFI_MAX_MEMORY_TYPE)
    return -2;

  /* Add new size to requested size */
  align_overhead = align - (requested_memory[type]%align);
  if (align_overhead == align)
    align_overhead = 0;
  requested_memory[type] += align_overhead + size;

  /* Remember the request */
  ret = holy_zalloc (sizeof (*ret));
  if (!ret)
    return -1;
  ret->type = type;
  ret->size = size;
  ret->align_overhead = align_overhead;
  prev = 0;

  /* Add request to the end of the chain.
     It should be at the end because otherwise alignment isn't guaranteed */
  for (cur = memrequests; cur; prev = cur, cur = cur->next);
  if (prev)
    {
      ret->handle = prev->handle + 1;
      prev->next = ret;
    }
  else
    {
      ret->handle = 1; /* Avoid 0 handle*/
      memrequests = ret;
    }
  return ret->handle;
}

/* Really allocate the memory */
static holy_err_t
efiemu_alloc_requests (void)
{
  holy_size_t align_overhead = 0;
  holy_uint8_t *curptr, *typestart;
  struct holy_efiemu_memrequest *cur;
  holy_size_t total_alloc = 0;
  unsigned i;
  /* Order of memory regions */
  holy_efi_memory_type_t reqorder[] =
    {
      /* First come regions usable by OS*/
      holy_EFI_LOADER_CODE,
      holy_EFI_LOADER_DATA,
      holy_EFI_BOOT_SERVICES_CODE,
      holy_EFI_BOOT_SERVICES_DATA,
      holy_EFI_CONVENTIONAL_MEMORY,
      holy_EFI_ACPI_RECLAIM_MEMORY,

      /* Then memory used by runtime */
      /* This way all our regions are in a single block */
      holy_EFI_RUNTIME_SERVICES_CODE,
      holy_EFI_RUNTIME_SERVICES_DATA,
      holy_EFI_ACPI_MEMORY_NVS,

      /* And then unavailable memory types. This is more for a completeness.
	 You should double think before allocating memory of any of these types
       */
      holy_EFI_UNUSABLE_MEMORY,
      holy_EFI_MEMORY_MAPPED_IO,
      holy_EFI_MEMORY_MAPPED_IO_PORT_SPACE,
      holy_EFI_PAL_CODE

      /*
       * These are not allocatable:
       * holy_EFI_RESERVED_MEMORY_TYPE
       * holy_EFI_PERSISTENT_MEMORY
       * >= holy_EFI_MAX_MEMORY_TYPE
       */
    };

  /* Compute total memory needed */
  for (i = 0; i < sizeof (reqorder) / sizeof (reqorder[0]); i++)
    {
      align_overhead = holy_EFIEMU_PAGESIZE
	- (requested_memory[reqorder[i]] % holy_EFIEMU_PAGESIZE);
      if (align_overhead == holy_EFIEMU_PAGESIZE)
	align_overhead = 0;
      total_alloc += requested_memory[reqorder[i]] + align_overhead;
    }

  /* Allocate the whole memory in one block */
  resident_memory = holy_memalign (holy_EFIEMU_PAGESIZE, total_alloc);
  if (!resident_memory)
    return holy_errno;

  /* Split the memory into blocks by type */
  curptr = resident_memory;
  for (i = 0; i < sizeof (reqorder) / sizeof (reqorder[0]); i++)
    {
      if (!requested_memory[reqorder[i]])
	continue;
      typestart = curptr;

      /* Write pointers to requests */
      for (cur = memrequests; cur; cur = cur->next)
	if (cur->type == reqorder[i])
	  {
	    curptr = ((holy_uint8_t *)curptr) + cur->align_overhead;
	    cur->val = curptr;
	    curptr = ((holy_uint8_t *)curptr) + cur->size;
	  }

      /* Ensure that the regions are page-aligned */
      align_overhead = holy_EFIEMU_PAGESIZE
	- (requested_memory[reqorder[i]] % holy_EFIEMU_PAGESIZE);
      if (align_overhead == holy_EFIEMU_PAGESIZE)
	align_overhead = 0;
      curptr = ((holy_uint8_t *) curptr) + align_overhead;

      /* Add the region to memory map */
      holy_efiemu_add_to_mmap ((holy_addr_t) typestart,
			       curptr - typestart, reqorder[i]);
    }

  return holy_ERR_NONE;
}

/* Get a pointer to requested memory from handle */
void *
holy_efiemu_mm_obtain_request (int handle)
{
  struct holy_efiemu_memrequest *cur;
  for (cur = memrequests; cur; cur = cur->next)
    if (cur->handle == handle)
      return cur->val;
  return 0;
}

/* Get type of requested memory by handle */
holy_efi_memory_type_t
holy_efiemu_mm_get_type (int handle)
{
  struct holy_efiemu_memrequest *cur;
  for (cur = memrequests; cur; cur = cur->next)
    if (cur->handle == handle)
      return cur->type;
  return 0;
}

/* Free a request */
void
holy_efiemu_mm_return_request (int handle)
{
  struct holy_efiemu_memrequest *cur, *prev;

  /* Remove head if necessary */
  while (memrequests && memrequests->handle == handle)
    {
      cur = memrequests->next;
      holy_free (memrequests);
      memrequests = cur;
    }
  if (!memrequests)
    return;

  /* Remove request from a middle of chain*/
  for (prev = memrequests, cur = prev->next; cur;)
    if (cur->handle == handle)
      {
	prev->next = cur->next;
	holy_free (cur);
	cur = prev->next;
      }
    else
      {
	prev = cur;
	cur = prev->next;
      }
}

/* Helper for holy_efiemu_mmap_init.  */
static int
bounds_hook (holy_uint64_t addr __attribute__ ((unused)),
	     holy_uint64_t size __attribute__ ((unused)),
	     holy_memory_type_t type __attribute__ ((unused)),
	     void *data __attribute__ ((unused)))
{
  mmap_reserved_size++;
  return 0;
}

/* Reserve space for memory map */
static holy_err_t
holy_efiemu_mmap_init (void)
{
  // the place for memory used by efiemu itself
  mmap_reserved_size = holy_EFI_MAX_MEMORY_TYPE + 1;

#ifndef holy_MACHINE_EMU
  holy_machine_mmap_iterate (bounds_hook, NULL);
#endif

  return holy_ERR_NONE;
}

/* This is a drop-in replacement of holy_efi_get_memory_map */
/* Get the memory map as defined in the EFI spec. Return 1 if successful,
   return 0 if partial, or return -1 if an error occurs.  */
int
holy_efiemu_get_memory_map (holy_efi_uintn_t *memory_map_size,
			    holy_efi_memory_descriptor_t *memory_map,
			    holy_efi_uintn_t *map_key,
			    holy_efi_uintn_t *descriptor_size,
			    holy_efi_uint32_t *descriptor_version)
{
  if (!efiemu_mmap)
    {
      holy_error (holy_ERR_INVALID_COMMAND,
		  "you need to first launch efiemu_prepare");
      return -1;
    }

  if (*memory_map_size < mmap_num * sizeof (holy_efi_memory_descriptor_t))
    {
      *memory_map_size = mmap_num * sizeof (holy_efi_memory_descriptor_t);
      return 0;
    }

  *memory_map_size = mmap_num * sizeof (holy_efi_memory_descriptor_t);
  holy_memcpy (memory_map, efiemu_mmap, *memory_map_size);
  if (descriptor_size)
    *descriptor_size = sizeof (holy_efi_memory_descriptor_t);
  if (descriptor_version)
    *descriptor_version = 1;
  if (map_key)
    *map_key = 0;

  return 1;
}

holy_err_t
holy_efiemu_finish_boot_services (holy_efi_uintn_t *memory_map_size,
				  holy_efi_memory_descriptor_t *memory_map,
				  holy_efi_uintn_t *map_key,
				  holy_efi_uintn_t *descriptor_size,
				  holy_efi_uint32_t *descriptor_version)
{
  int val = holy_efiemu_get_memory_map (memory_map_size,
					memory_map, map_key,
					descriptor_size,
					descriptor_version);
  if (val == 1)
    return holy_ERR_NONE;
  if (val == -1)
    return holy_errno;
  return holy_error (holy_ERR_IO, "memory map buffer is too small");
}


/* Free everything */
holy_err_t
holy_efiemu_mm_unload (void)
{
  struct holy_efiemu_memrequest *cur, *d;
  for (cur = memrequests; cur;)
    {
      d = cur->next;
      holy_free (cur);
      cur = d;
    }
  memrequests = 0;
  holy_memset (&requested_memory, 0, sizeof (requested_memory));
  holy_free (resident_memory);
  resident_memory = 0;
  holy_free (efiemu_mmap);
  efiemu_mmap = 0;
  mmap_reserved_size = mmap_num = 0;
  return holy_ERR_NONE;
}

/* This function should be called before doing any requests */
holy_err_t
holy_efiemu_mm_init (void)
{
  holy_err_t err;

  err = holy_efiemu_mm_unload ();
  if (err)
    return err;

  holy_efiemu_mmap_init ();

  return holy_ERR_NONE;
}

/* Helper for holy_efiemu_mmap_fill.  */
static int
fill_hook (holy_uint64_t addr, holy_uint64_t size, holy_memory_type_t type,
	   void *data __attribute__ ((unused)))
  {
    switch (type)
      {
      case holy_MEMORY_AVAILABLE:
	return holy_efiemu_add_to_mmap (addr, size,
					holy_EFI_CONVENTIONAL_MEMORY);

      case holy_MEMORY_ACPI:
	return holy_efiemu_add_to_mmap (addr, size,
					holy_EFI_ACPI_RECLAIM_MEMORY);

      case holy_MEMORY_NVS:
	return holy_efiemu_add_to_mmap (addr, size,
					holy_EFI_ACPI_MEMORY_NVS);

      case holy_MEMORY_PERSISTENT:
      case holy_MEMORY_PERSISTENT_LEGACY:
	return holy_efiemu_add_to_mmap (addr, size,
					holy_EFI_PERSISTENT_MEMORY);
      default:
	holy_dprintf ("efiemu",
		      "Unknown memory type %d. Assuming unusable\n", type);
	/* FALLTHROUGH */
      case holy_MEMORY_RESERVED:
	return holy_efiemu_add_to_mmap (addr, size,
					holy_EFI_UNUSABLE_MEMORY);
      }
  }

/* Copy host memory map */
static holy_err_t
holy_efiemu_mmap_fill (void)
{
#ifndef holy_MACHINE_EMU
  holy_machine_mmap_iterate (fill_hook, NULL);
#endif

  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  unsigned i;

  for (i = 0; i < (unsigned) mmap_num; i++)
    switch (efiemu_mmap[i].type)
      {
      case holy_EFI_RUNTIME_SERVICES_CODE:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_CODE, hook_data);
	break;

      case holy_EFI_UNUSABLE_MEMORY:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_BADRAM, hook_data);
	break;

      case holy_EFI_RESERVED_MEMORY_TYPE:
      case holy_EFI_RUNTIME_SERVICES_DATA:
      case holy_EFI_MEMORY_MAPPED_IO:
      case holy_EFI_MEMORY_MAPPED_IO_PORT_SPACE:
      case holy_EFI_PAL_CODE:
      default:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_RESERVED, hook_data);
	break;

      case holy_EFI_LOADER_CODE:
      case holy_EFI_LOADER_DATA:
      case holy_EFI_BOOT_SERVICES_CODE:
      case holy_EFI_BOOT_SERVICES_DATA:
      case holy_EFI_CONVENTIONAL_MEMORY:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_AVAILABLE, hook_data);
	break;

      case holy_EFI_ACPI_RECLAIM_MEMORY:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_ACPI, hook_data);
	break;

      case holy_EFI_ACPI_MEMORY_NVS:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_NVS, hook_data);
	break;

      case holy_EFI_PERSISTENT_MEMORY:
	hook (efiemu_mmap[i].physical_start, efiemu_mmap[i].num_pages * 4096,
	      holy_MEMORY_PERSISTENT, hook_data);
	break;

      }

  return 0;
}


/* This function resolves overlapping regions and sorts the memory map
   It uses scanline (sweeping) algorithm
 */
static holy_err_t
holy_efiemu_mmap_sort_and_uniq (void)
{
  /* If same page is used by multiple types it's resolved
     according to priority
     0 - free memory
     1 - memory immediately usable after ExitBootServices
     2 - memory usable after loading ACPI tables
     3 - efiemu memory
     4 - unusable memory
  */
  int priority[holy_EFI_MAX_MEMORY_TYPE] =
    {
      [holy_EFI_RESERVED_MEMORY_TYPE] = 4,
      [holy_EFI_LOADER_CODE] = 1,
      [holy_EFI_LOADER_DATA] = 1,
      [holy_EFI_BOOT_SERVICES_CODE] = 1,
      [holy_EFI_BOOT_SERVICES_DATA] = 1,
      [holy_EFI_RUNTIME_SERVICES_CODE] = 3,
      [holy_EFI_RUNTIME_SERVICES_DATA] = 3,
      [holy_EFI_CONVENTIONAL_MEMORY] = 0,
      [holy_EFI_UNUSABLE_MEMORY] = 4,
      [holy_EFI_ACPI_RECLAIM_MEMORY] = 2,
      [holy_EFI_ACPI_MEMORY_NVS] = 3,
      [holy_EFI_MEMORY_MAPPED_IO] = 4,
      [holy_EFI_MEMORY_MAPPED_IO_PORT_SPACE] = 4,
      [holy_EFI_PAL_CODE] = 4,
      [holy_EFI_PERSISTENT_MEMORY] = 4
    };

  int i, j, k, done;

  /* Scanline events */
  struct holy_efiemu_mmap_scan
  {
    /* At which memory address*/
    holy_uint64_t pos;
    /* 0 = region starts, 1 = region ends */
    int type;
    /* Which type of memory region */
    holy_efi_memory_type_t memtype;
  };
  struct holy_efiemu_mmap_scan *scanline_events;
  struct holy_efiemu_mmap_scan t;

  /* Previous scanline event */
  holy_uint64_t lastaddr;
  int lasttype;
  /* Current scanline event */
  int curtype;
  /* how many regions of given type overlap at current location */
  int present[holy_EFI_MAX_MEMORY_TYPE];
  /* Here is stored the resulting memory map*/
  holy_efi_memory_descriptor_t *result;

  /* Initialize variables*/
  holy_memset (present, 0, sizeof (int) * holy_EFI_MAX_MEMORY_TYPE);
  scanline_events = (struct holy_efiemu_mmap_scan *)
    holy_malloc (sizeof (struct holy_efiemu_mmap_scan) * 2 * mmap_num);

  /* Number of chunks can't increase more than by factor of 2 */
  result = (holy_efi_memory_descriptor_t *)
    holy_malloc (sizeof (holy_efi_memory_descriptor_t) * 2 * mmap_num);
  if (!result || !scanline_events)
    {
      holy_free (result);
      holy_free (scanline_events);
      return holy_errno;
    }

  /* Register scanline events */
  for (i = 0; i < mmap_num; i++)
    {
      scanline_events[2 * i].pos = efiemu_mmap[i].physical_start;
      scanline_events[2 * i].type = 0;
      scanline_events[2 * i].memtype = efiemu_mmap[i].type;
      scanline_events[2 * i + 1].pos = efiemu_mmap[i].physical_start
	+ efiemu_mmap[i].num_pages * holy_EFIEMU_PAGESIZE;
      scanline_events[2 * i + 1].type = 1;
      scanline_events[2 * i + 1].memtype = efiemu_mmap[i].type;
    }

  /* Primitive bubble sort. It has complexity O(n^2) but since we're
     unlikely to have more than 100 chunks it's probably one of the
     fastest for one purpose */
  done = 1;
  while (done)
    {
      done = 0;
      for (i = 0; i < 2 * mmap_num - 1; i++)
	if (scanline_events[i + 1].pos < scanline_events[i].pos)
	  {
	    t = scanline_events[i + 1];
	    scanline_events[i + 1] = scanline_events[i];
	    scanline_events[i] = t;
	    done = 1;
	  }
    }

  /* Pointer in resulting memory map */
  j = 0;
  lastaddr = scanline_events[0].pos;
  lasttype = scanline_events[0].memtype;
  for (i = 0; i < 2 * mmap_num; i++)
    {
      /* Process event */
      if (scanline_events[i].type)
	present[scanline_events[i].memtype]--;
      else
	present[scanline_events[i].memtype]++;

      /* Determine current region type */
      curtype = -1;
      for (k = 0; k < holy_EFI_MAX_MEMORY_TYPE; k++)
	if (present[k] && (curtype == -1 || priority[k] > priority[curtype]))
	  curtype = k;

      /* Add memory region to resulting map if necessary */
      if ((curtype == -1 || curtype != lasttype)
	  && lastaddr != scanline_events[i].pos
	  && lasttype != -1)
	{
	  result[j].virtual_start = result[j].physical_start = lastaddr;
	  result[j].num_pages = (scanline_events[i].pos - lastaddr)
	    / holy_EFIEMU_PAGESIZE;
	  result[j].type = lasttype;

	  /* We set runtime attribute on pages we need to be mapped */
	  result[j].attribute
	    = (lasttype == holy_EFI_RUNTIME_SERVICES_CODE
		   || lasttype == holy_EFI_RUNTIME_SERVICES_DATA)
	    ? holy_EFI_MEMORY_RUNTIME : 0;
	  holy_dprintf ("efiemu",
			"mmap entry: type %d start 0x%llx 0x%llx pages\n",
			result[j].type,
			result[j].physical_start, result[j].num_pages);
	  j++;
	}

      /* Update last values if necessary */
      if (curtype == -1 || curtype != lasttype)
	{
	  lasttype = curtype;
	  lastaddr = scanline_events[i].pos;
	}
    }

  holy_free (scanline_events);

  /* Shrink resulting memory map to really used size and replace efiemu_mmap
     by new value */
  holy_free (efiemu_mmap);
  efiemu_mmap = holy_realloc (result, j * sizeof (*result));
  return holy_ERR_NONE;
}

/* This function is called to switch from first to second phase */
holy_err_t
holy_efiemu_mm_do_alloc (void)
{
  holy_err_t err;

  /* Preallocate mmap */
  efiemu_mmap = (holy_efi_memory_descriptor_t *)
    holy_malloc (mmap_reserved_size * sizeof (holy_efi_memory_descriptor_t));
  if (!efiemu_mmap)
    {
      holy_efiemu_unload ();
      return holy_errno;
    }

  err = efiemu_alloc_requests ();
  if (err)
    return err;
  err = holy_efiemu_mmap_fill ();
  if (err)
    return err;
  return holy_efiemu_mmap_sort_and_uniq ();
}
