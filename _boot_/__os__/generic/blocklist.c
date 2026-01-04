/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/disk.h>
#include <holy/file.h>
#include <holy/partition.h>
#include <holy/util/misc.h>
#include <holy/util/install.h>
#include <holy/emu/hostdisk.h>

#include <string.h>

#define MAX_TRIES	5

void
holy_install_get_blocklist (holy_device_t root_dev,
			    const char *core_path, const char *core_img,
			    size_t core_size,
			    void (*callback) (holy_disk_addr_t sector,
					      unsigned offset,
					      unsigned length,
					      void *data),
			    void *hook_data)
{
  int i;
  char *tmp_img;
  char *core_path_dev;

  core_path_dev = holy_make_system_path_relative_to_its_root (core_path);

  /* Make sure that holy reads the identical image as the OS.  */
  tmp_img = xmalloc (core_size);

  for (i = 0; i < MAX_TRIES; i++)
    {
      holy_file_t file;

      holy_util_info ((i == 0) ? _("attempting to read the core image `%s' from holy")
		      : _("attempting to read the core image `%s' from holy again"),
		      core_path_dev);

      holy_disk_cache_invalidate_all ();

      holy_file_filter_disable_compression ();
      file = holy_file_open (core_path_dev);
      if (file)
	{
	  if (holy_file_size (file) != core_size)
	    holy_util_info ("succeeded in opening the core image but the size is different (%d != %d)",
			    (int) holy_file_size (file), (int) core_size);
	  else if (holy_file_read (file, tmp_img, core_size)
		   != (holy_ssize_t) core_size)
	    holy_util_info ("succeeded in opening the core image but cannot read %d bytes",
			    (int) core_size);
	  else if (memcmp (core_img, tmp_img, core_size) != 0)
	    {
#if 0
	      FILE *dump;
	      FILE *dump2;

	      dump = fopen ("dump.img", "wb");
	      if (dump)
		{
		  fwrite (tmp_img, 1, core_size, dump);
		  fclose (dump);
		}

	      dump2 = fopen ("dump2.img", "wb");
	      if (dump2)
		{
		  fwrite (core_img, 1, core_size, dump2);
		  fclose (dump2);
		}

#endif
	      holy_util_info ("succeeded in opening the core image but the data is different");
	    }
	  else
	    {
	      holy_file_close (file);
	      break;
	    }

	  holy_file_close (file);
	}
      else
	holy_util_info ("couldn't open the core image");

      if (holy_errno)
	holy_util_info ("error message = %s", holy_errmsg);

      holy_errno = holy_ERR_NONE;
      holy_util_biosdisk_flush (root_dev->disk);
      sleep (1);
    }

  if (i == MAX_TRIES)
    holy_util_error (_("cannot read `%s' correctly"), core_path_dev);

  holy_file_t file;
  /* Now read the core image to determine where the sectors are.  */
  holy_file_filter_disable_compression ();
  file = holy_file_open (core_path_dev);
  if (! file)
    holy_util_error ("%s", holy_errmsg);

  file->read_hook = callback;
  file->read_hook_data = hook_data;
  if (holy_file_read (file, tmp_img, core_size) != (holy_ssize_t) core_size)
    holy_util_error ("%s", _("failed to read the sectors of the core image"));

  holy_file_close (file);
  free (tmp_img);

  free (core_path_dev);
}
