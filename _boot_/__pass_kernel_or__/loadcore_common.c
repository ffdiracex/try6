/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efiemu/efiemu.h>
#include <holy/cpu/efiemu.h>

/* Are we in 32 or 64-bit mode?*/
static holy_efiemu_mode_t holy_efiemu_mode = holy_EFIEMU_NOTLOADED;
/* Runtime ELF file */
static holy_ssize_t efiemu_core_size;
static void *efiemu_core = 0;
/* Linked list of segments */
static holy_efiemu_segment_t efiemu_segments = 0;

/* equivalent to sizeof (holy_efi_uintn_t) but taking the mode into account*/
int
holy_efiemu_sizeof_uintn_t (void)
{
  if (holy_efiemu_mode == holy_EFIEMU32)
    return 4;
  if (holy_efiemu_mode == holy_EFIEMU64)
    return 8;
  return 0;
}

/* Check the header and set mode */
static holy_err_t
holy_efiemu_check_header (void *ehdr, holy_size_t size,
			  holy_efiemu_mode_t *mode)
{
  /* Check the magic numbers.  */
  if ((*mode == holy_EFIEMU_NOTLOADED || *mode == holy_EFIEMU32)
      && holy_efiemu_check_header32 (ehdr,size))
    {
      *mode = holy_EFIEMU32;
      return holy_ERR_NONE;
    }
  if ((*mode == holy_EFIEMU_NOTLOADED || *mode == holy_EFIEMU64)
      && holy_efiemu_check_header64 (ehdr,size))
    {
      *mode = holy_EFIEMU64;
      return holy_ERR_NONE;
    }
  return holy_error (holy_ERR_BAD_OS, "invalid ELF magic");
}

/* Unload segments */
static int
holy_efiemu_unload_segs (holy_efiemu_segment_t seg)
{
  holy_efiemu_segment_t segn;
  for (; seg; seg = segn)
    {
      segn = seg->next;
      holy_efiemu_mm_return_request (seg->handle);
      holy_free (seg);
    }
  return 1;
}


holy_err_t
holy_efiemu_loadcore_unload(void)
{
  switch (holy_efiemu_mode)
    {
    case holy_EFIEMU32:
      holy_efiemu_loadcore_unload32 ();
      break;

    case holy_EFIEMU64:
      holy_efiemu_loadcore_unload64 ();
      break;

    default:
      break;
    }

  holy_efiemu_mode = holy_EFIEMU_NOTLOADED;

  holy_free (efiemu_core);
  efiemu_core = 0;

  holy_efiemu_unload_segs (efiemu_segments);
  efiemu_segments = 0;

  holy_efiemu_free_syms ();

  return holy_ERR_NONE;
}

/* Load runtime file and do some initial preparations */
holy_err_t
holy_efiemu_loadcore_init (holy_file_t file,
			   const char *filename)
{
  holy_err_t err;

  efiemu_core_size = holy_file_size (file);
  efiemu_core = 0;
  efiemu_core = holy_malloc (efiemu_core_size);
  if (! efiemu_core)
    return holy_errno;

  if (holy_file_read (file, efiemu_core, efiemu_core_size)
      != (int) efiemu_core_size)
    {
      holy_free (efiemu_core);
      efiemu_core = 0;
      return holy_errno;
    }

  if (holy_efiemu_check_header (efiemu_core, efiemu_core_size,
				&holy_efiemu_mode))
    {
      holy_free (efiemu_core);
      efiemu_core = 0;
      return holy_ERR_BAD_MODULE;
    }

  switch (holy_efiemu_mode)
    {
    case holy_EFIEMU32:
      err = holy_efiemu_loadcore_init32 (efiemu_core, filename,
					 efiemu_core_size,
					 &efiemu_segments);
      if (err)
	{
	  holy_free (efiemu_core);
	  efiemu_core = 0;
	  holy_efiemu_mode = holy_EFIEMU_NOTLOADED;
	  return err;
	}
      break;

    case holy_EFIEMU64:
      err = holy_efiemu_loadcore_init64 (efiemu_core, filename,
					 efiemu_core_size,
					 &efiemu_segments);
      if (err)
	{
	  holy_free (efiemu_core);
	  efiemu_core = 0;
	  holy_efiemu_mode = holy_EFIEMU_NOTLOADED;
	  return err;
	}
      break;

    default:
      return holy_error (holy_ERR_BUG, "unknown EFI runtime");
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_efiemu_loadcore_load (void)
{
  holy_err_t err;
  switch (holy_efiemu_mode)
    {
    case holy_EFIEMU32:
      err = holy_efiemu_loadcore_load32 (efiemu_core, efiemu_core_size,
					 efiemu_segments);
      if (err)
	holy_efiemu_loadcore_unload ();
      return err;
    case holy_EFIEMU64:
      err = holy_efiemu_loadcore_load64 (efiemu_core, efiemu_core_size,
					 efiemu_segments);
      if (err)
	holy_efiemu_loadcore_unload ();
      return err;
    default:
      return holy_error (holy_ERR_BUG, "unknown EFI runtime");
    }
}
