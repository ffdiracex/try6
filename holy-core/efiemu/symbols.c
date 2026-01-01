/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/efiemu/runtime.h>
#include <holy/i18n.h>

static int ptv_written = 0;
static int ptv_alloc = 0;
static int ptv_handle = 0;
static int relocated_handle = 0;
static int ptv_requested = 0;
static struct holy_efiemu_sym *efiemu_syms = 0;

struct holy_efiemu_sym
{
  struct holy_efiemu_sym *next;
  char *name;
  int handle;
  holy_off_t off;
};

void
holy_efiemu_free_syms (void)
{
  struct holy_efiemu_sym *cur, *d;
  for (cur = efiemu_syms; cur;)
    {
      d = cur->next;
      holy_free (cur->name);
      holy_free (cur);
      cur = d;
    }
  efiemu_syms = 0;
  ptv_written = 0;
  ptv_alloc = 0;
  ptv_requested = 0;
  holy_efiemu_mm_return_request (ptv_handle);
  ptv_handle = 0;
  holy_efiemu_mm_return_request (relocated_handle);
  relocated_handle = 0;
}

/* Announce that the module will need NUM allocators */
/* Because of deferred memory allocation all the relocators have to be
   announced during phase 1*/
holy_err_t
holy_efiemu_request_symbols (int num)
{
  if (ptv_alloc)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       "symbols have already been allocated");
  if (num < 0)
    return holy_error (holy_ERR_BUG,
		       "can't request negative symbols");
  ptv_requested += num;
  return holy_ERR_NONE;
}

/* Resolve the symbol name NAME and set HANDLE and OFF accordingly  */
holy_err_t
holy_efiemu_resolve_symbol (const char *name, int *handle, holy_off_t *off)
{
  struct holy_efiemu_sym *cur;
  for (cur = efiemu_syms; cur; cur = cur->next)
    if (!holy_strcmp (name, cur->name))
      {
	*handle = cur->handle;
	*off = cur->off;
	return holy_ERR_NONE;
      }
  holy_dprintf ("efiemu", "%s not found\n", name);
  return holy_error (holy_ERR_BAD_OS, N_("symbol `%s' not found"), name);
}

/* Register symbol named NAME in memory handle HANDLE at offset OFF */
holy_err_t
holy_efiemu_register_symbol (const char *name, int handle, holy_off_t off)
{
  struct holy_efiemu_sym *cur;
  cur = (struct holy_efiemu_sym *) holy_malloc (sizeof (*cur));
  holy_dprintf ("efiemu", "registering symbol '%s'\n", name);
  if (!cur)
    return holy_errno;
  cur->name = holy_strdup (name);
  cur->next = efiemu_syms;
  cur->handle = handle;
  cur->off = off;
  efiemu_syms = cur;

  return 0;
}

/* Go from phase 1 to phase 2. Must be called before similar function in mm.c */
holy_err_t
holy_efiemu_alloc_syms (void)
{
  ptv_alloc = ptv_requested;
  ptv_handle = holy_efiemu_request_memalign
    (1, (ptv_requested + 1) * sizeof (struct holy_efiemu_ptv_rel),
     holy_EFI_RUNTIME_SERVICES_DATA);
  relocated_handle = holy_efiemu_request_memalign
    (1, sizeof (holy_uint8_t), holy_EFI_RUNTIME_SERVICES_DATA);

  holy_efiemu_register_symbol ("efiemu_ptv_relocated", relocated_handle, 0);
  holy_efiemu_register_symbol ("efiemu_ptv_relloc", ptv_handle, 0);
  return holy_errno;
}

holy_err_t
holy_efiemu_write_sym_markers (void)
{
  struct holy_efiemu_ptv_rel *ptv_rels
    = holy_efiemu_mm_obtain_request (ptv_handle);
  holy_uint8_t *relocated = holy_efiemu_mm_obtain_request (relocated_handle);
  holy_memset (ptv_rels, 0, (ptv_requested + 1)
	       * sizeof (struct holy_efiemu_ptv_rel));
  *relocated = 0;
  return holy_ERR_NONE;
}

/* Write value (pointer to memory PLUS_HANDLE)
   - (pointer to memory MINUS_HANDLE) + VALUE to ADDR assuming that the
   size SIZE bytes. If PTV_NEEDED is 1 then announce it to runtime that this
   value needs to be recomputed before going to virtual mode
*/
holy_err_t
holy_efiemu_write_value (void *addr, holy_uint32_t value, int plus_handle,
			 int minus_handle, int ptv_needed, int size)
{
  /* Announce relocator to runtime */
  if (ptv_needed)
    {
      struct holy_efiemu_ptv_rel *ptv_rels
	= holy_efiemu_mm_obtain_request (ptv_handle);

      if (ptv_needed && ptv_written >= ptv_alloc)
	return holy_error (holy_ERR_BUG,
			   "your module didn't declare efiemu "
			   " relocators correctly");

      if (minus_handle)
	ptv_rels[ptv_written].minustype
	  = holy_efiemu_mm_get_type (minus_handle);
      else
	ptv_rels[ptv_written].minustype = 0;

      if (plus_handle)
	ptv_rels[ptv_written].plustype
	  = holy_efiemu_mm_get_type (plus_handle);
      else
	ptv_rels[ptv_written].plustype = 0;

      ptv_rels[ptv_written].addr = (holy_addr_t) addr;
      ptv_rels[ptv_written].size = size;
      ptv_written++;

      /* memset next value to zero to mark the end */
      holy_memset (&ptv_rels[ptv_written], 0, sizeof (ptv_rels[ptv_written]));
    }

  /* Compute the value */
  if (minus_handle)
    value -= (holy_addr_t) holy_efiemu_mm_obtain_request (minus_handle);

  if (plus_handle)
    value += (holy_addr_t) holy_efiemu_mm_obtain_request (plus_handle);

  /* Write the value */
  switch (size)
    {
    case 8:
      *((holy_uint64_t *) addr) = value;
      break;
    case 4:
      *((holy_uint32_t *) addr) = value;
      break;
    case 2:
      *((holy_uint16_t *) addr) = value;
      break;
    case 1:
      *((holy_uint8_t *) addr) = value;
      break;
    default:
      return holy_error (holy_ERR_BUG, "wrong symbol size");
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_set_virtual_address_map (holy_efi_uintn_t memory_map_size,
				     holy_efi_uintn_t descriptor_size,
				     holy_efi_uint32_t descriptor_version
				     __attribute__ ((unused)),
				     holy_efi_memory_descriptor_t *virtual_map)
{
  holy_uint8_t *ptv_relocated;
  struct holy_efiemu_ptv_rel *cur_relloc;
  struct holy_efiemu_ptv_rel *ptv_rels;

  ptv_relocated = holy_efiemu_mm_obtain_request (relocated_handle);
  ptv_rels = holy_efiemu_mm_obtain_request (ptv_handle);

  /* Ensure that we are called only once */
  if (*ptv_relocated)
    return holy_error (holy_ERR_BUG, "EfiEmu is already relocated");
  *ptv_relocated = 1;

  /* Correct addresses using information supplied by holy */
  for (cur_relloc = ptv_rels; cur_relloc->size; cur_relloc++)
    {
      holy_int64_t corr = 0;
      holy_efi_memory_descriptor_t *descptr;

      /* Compute correction */
      for (descptr = virtual_map;
	   (holy_size_t) ((holy_uint8_t *) descptr
			  - (holy_uint8_t *) virtual_map) < memory_map_size;
	   descptr = (holy_efi_memory_descriptor_t *)
	     ((holy_uint8_t *) descptr + descriptor_size))
	{
	  if (descptr->type == cur_relloc->plustype)
	    corr += descptr->virtual_start - descptr->physical_start;
	  if (descptr->type == cur_relloc->minustype)
	    corr -= descptr->virtual_start - descptr->physical_start;
	}

      /* Apply correction */
      switch (cur_relloc->size)
	{
	case 8:
	  *((holy_uint64_t *) (holy_addr_t) cur_relloc->addr) += corr;
	  break;
	case 4:
	  *((holy_uint32_t *) (holy_addr_t) cur_relloc->addr) += corr;
	  break;
	case 2:
	  *((holy_uint16_t *) (holy_addr_t) cur_relloc->addr) += corr;
	  break;
	case 1:
	  *((holy_uint8_t *) (holy_addr_t) cur_relloc->addr) += corr;
	  break;
	}
    }

  /* Recompute crc32 of system table and runtime services */

  if (holy_efiemu_sizeof_uintn_t () == 4)
    return holy_efiemu_crc32 ();
  else
    return holy_efiemu_crc64 ();
}
