/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/cpu/linux.h>
#include <holy/cpu/system.h>
#include <holy/efi/efi.h>
#include <holy/machine/loader.h>

static inline holy_size_t
page_align (holy_size_t size)
{
  return (size + (1 << 12) - 1) & (~((1 << 12) - 1));
}

/* Find the optimal number of pages for the memory map. Is it better to
   move this code to efi/mm.c?  */
static holy_efi_uintn_t
find_mmap_size (void)
{
  static holy_efi_uintn_t mmap_size = 0;

  if (mmap_size != 0)
    return mmap_size;
  
  mmap_size = (1 << 12);
  while (1)
    {
      int ret;
      holy_efi_memory_descriptor_t *mmap;
      holy_efi_uintn_t desc_size;
      
      mmap = holy_malloc (mmap_size);
      if (! mmap)
	return 0;

      ret = holy_efi_get_memory_map (&mmap_size, mmap, 0, &desc_size, 0);
      holy_free (mmap);
      
      if (ret < 0)
	{
	  holy_error (holy_ERR_IO, "cannot get memory map");
	  return 0;
	}
      else if (ret > 0)
	break;

      mmap_size += (1 << 12);
    }

  /* Increase the size a bit for safety, because holy allocates more on
     later, and EFI itself may allocate more.  */
  mmap_size += (1 << 12);

  return page_align (mmap_size);
}

#define NEXT_MEMORY_DESCRIPTOR(desc, size)      \
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))
#define PAGE_SHIFT 12

void *
holy_efi_allocate_loader_memory (holy_uint32_t min_offset, holy_uint32_t size)
{
  holy_efi_uintn_t desc_size;
  holy_efi_memory_descriptor_t *mmap, *mmap_end;
  holy_efi_uintn_t mmap_size, tmp_mmap_size;
  holy_efi_memory_descriptor_t *desc;
  void *mem = NULL;
  holy_addr_t min_start = 0;

  mmap_size = find_mmap_size();
  if (!mmap_size)
    return NULL;

  mmap = holy_malloc(mmap_size);
  if (!mmap)
    return NULL;

  tmp_mmap_size = mmap_size;
  if (holy_efi_get_memory_map (&tmp_mmap_size, mmap, 0, &desc_size, 0) <= 0)
    {
      holy_error (holy_ERR_IO, "cannot get memory map");
      goto fail;
    }

  mmap_end = NEXT_MEMORY_DESCRIPTOR (mmap, tmp_mmap_size);
  /* Find lowest accessible RAM location */
  {
    int found = 0;
    for (desc = mmap ; !found && (desc < mmap_end) ;
	 desc = NEXT_MEMORY_DESCRIPTOR(desc, desc_size))
      {
	switch (desc->type)
	  {
	  case holy_EFI_CONVENTIONAL_MEMORY:
	  case holy_EFI_LOADER_CODE:
	  case holy_EFI_LOADER_DATA:
	    min_start = desc->physical_start + min_offset;
	    found = 1;
	    break;
	  default:
	    break;
	  }
      }
  }

  /* First, find free pages for the real mode code
     and the memory map buffer.  */
  for (desc = mmap ; desc < mmap_end ;
       desc = NEXT_MEMORY_DESCRIPTOR(desc, desc_size))
    {
      holy_uint64_t start, end;

      holy_dprintf("mm", "%s: 0x%08x bytes @ 0x%08x\n",
		   __FUNCTION__,
		   (holy_uint32_t) (desc->num_pages << PAGE_SHIFT),
		   (holy_uint32_t) (desc->physical_start));

      if (desc->type != holy_EFI_CONVENTIONAL_MEMORY)
	continue;

      start = desc->physical_start;
      end = start + (desc->num_pages << PAGE_SHIFT);
      holy_dprintf("mm", "%s: start=0x%016llx, end=0x%016llx\n",
		  __FUNCTION__, start, end);
      start = start < min_start ? min_start : start;
      if (start + size > end)
	continue;
      holy_dprintf("mm", "%s: let's allocate some (0x%x) pages @ 0x%08x...\n",
		  __FUNCTION__, (size >> PAGE_SHIFT), (holy_addr_t) start);
      mem = holy_efi_allocate_pages (start, (size >> PAGE_SHIFT) + 1);
      holy_dprintf("mm", "%s: retval=0x%08x\n",
		   __FUNCTION__, (holy_addr_t) mem);
      if (! mem)
	{
	  holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate memory");
	  goto fail;
	}
      break;
    }

  if (! mem)
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "cannot allocate memory");
      goto fail;
    }

  holy_free (mmap);
  return mem;

 fail:
  holy_free (mmap);
  return NULL;
}

holy_err_t
holy_efi_prepare_platform (void)
{
  holy_efi_uintn_t mmap_size;
  holy_efi_uintn_t map_key;
  holy_efi_uintn_t desc_size;
  holy_efi_uint32_t desc_version;
  holy_efi_memory_descriptor_t *mmap_buf;
  holy_err_t err;

  /*
   * Cloned from IA64
   * Must be done after holy_machine_fini because map_key is used by
   *exit_boot_services.
   */
  mmap_size = find_mmap_size ();
  if (! mmap_size)
    return holy_ERR_OUT_OF_MEMORY;
  mmap_buf = holy_efi_allocate_pages (0, page_align (mmap_size) >> 12);
  if (! mmap_buf)
    return holy_ERR_OUT_OF_MEMORY;

  err = holy_efi_finish_boot_services (&mmap_size, mmap_buf, &map_key,
				       &desc_size, &desc_version);
  if (err != holy_ERR_NONE)
    return err;

  return holy_ERR_NONE;
}
