/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/misc.h>
#include <holy/xnu.h>
#include <holy/cpu/xnu.h>
#include <holy/mm.h>
#include <holy/loader.h>
#include <holy/i18n.h>

static void *holy_xnu_hibernate_image;

static holy_err_t
holy_xnu_resume_unload (void)
{
  /* Free loaded image */
  if (holy_xnu_hibernate_image)
    holy_free (holy_xnu_hibernate_image);
  holy_xnu_hibernate_image = 0;
  holy_xnu_unlock ();
  return holy_ERR_NONE;
}

holy_err_t
holy_xnu_resume (char *imagename)
{
  holy_file_t file;
  holy_size_t total_header_size;
  struct holy_xnu_hibernate_header hibhead;
  void *code;
  void *image;
  holy_uint32_t codedest;
  holy_uint32_t codesize;
  holy_addr_t target_image;
  holy_err_t err;

  holy_file_filter_disable_compression ();
  file = holy_file_open (imagename);
  if (! file)
    return 0;

  /* Read the header. */
  if (holy_file_read (file, &hibhead, sizeof (hibhead))
      != sizeof (hibhead))
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return holy_errno;
    }

  /* Check the header. */
  if (hibhead.magic != holy_XNU_HIBERNATE_MAGIC)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_OS,
			 "hibernate header has incorrect magic number");
    }
  if (hibhead.encoffset)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_OS,
			 "encrypted images aren't supported yet");
    }

  if (hibhead.image_size == 0)
    {
      holy_file_close (file);
      return holy_error (holy_ERR_BAD_OS,
			 "hibernate image is empty");
    }

  codedest = hibhead.launchcode_target_page;
  codedest *= holy_XNU_PAGESIZE;
  codesize = hibhead.launchcode_numpages;
  codesize *= holy_XNU_PAGESIZE;

  /* FIXME: check that codedest..codedest+codesize is available. */

  /* Calculate total size before pages to copy. */
  total_header_size = hibhead.extmapsize + sizeof (hibhead);

  /* Unload image if any. */
  if (holy_xnu_hibernate_image)
    holy_free (holy_xnu_hibernate_image);

  /* Try to allocate necessary space.
     FIXME: mm isn't good enough yet to handle huge allocations.
   */
  holy_xnu_relocator = holy_relocator_new ();
  if (!holy_xnu_relocator)
    {
      holy_file_close (file);
      return holy_errno;
    }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_addr (holy_xnu_relocator, &ch, codedest,
					   codesize + holy_XNU_PAGESIZE);
    if (err)
      {
	holy_file_close (file);
	return err;
      }
    code = get_virtual_current_address (ch);
  }

  {
    holy_relocator_chunk_t ch;
    err = holy_relocator_alloc_chunk_align (holy_xnu_relocator, &ch, 0,
					    (0xffffffff - hibhead.image_size) + 1,
					    hibhead.image_size,
					    holy_XNU_PAGESIZE,
					    holy_RELOCATOR_PREFERENCE_NONE, 0);
    if (err)
      {
	holy_file_close (file);
	return err;
      }
    image = get_virtual_current_address (ch);
    target_image = get_physical_target_address (ch);
  }

  /* Read code part. */
  if (holy_file_seek (file, total_header_size) == (holy_off_t) -1
      || holy_file_read (file, code, codesize)
      != (holy_ssize_t) codesize)
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return holy_errno;
    }

  /* Read image. */
  if (holy_file_seek (file, 0) == (holy_off_t) -1
      || holy_file_read (file, image, hibhead.image_size)
      != (holy_ssize_t) hibhead.image_size)
    {
      holy_file_close (file);
      if (!holy_errno)
	holy_error (holy_ERR_READ_ERROR,
		    N_("premature end of file %s"), imagename);
      return holy_errno;
    }
  holy_file_close (file);

  /* Setup variables needed by asm helper. */
  holy_xnu_heap_target_start = codedest;
  holy_xnu_heap_size = target_image + hibhead.image_size - codedest;
  holy_xnu_stack = (codedest + hibhead.stack);
  holy_xnu_entry_point = (codedest + hibhead.entry_point);
  holy_xnu_arg1 = target_image;

  holy_dprintf ("xnu", "entry point 0x%x\n", codedest + hibhead.entry_point);
  holy_dprintf ("xnu", "image at 0x%x\n",
		codedest + codesize + holy_XNU_PAGESIZE);

  /* We're ready now. */
  holy_loader_set (holy_xnu_boot_resume,
		   holy_xnu_resume_unload, 0);

  /* Prevent module from unloading. */
  holy_xnu_lock ();
  return holy_ERR_NONE;
}
