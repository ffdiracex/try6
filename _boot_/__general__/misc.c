/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/disk.h>
#include <holy/fs.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/datetime.h>
#include <holy/term.h>
#include <holy/i18n.h>
#include <holy/partition.h>

static const char *holy_human_sizes[3][6] =
  {
    /* This algorithm in reality would work only up to (2^64) / 100 B = 81 PiB.
       Put here all possible suffixes it can produce so no array bounds check
       is needed.
     */
    /* TRANSLATORS: that's the list of binary unit prefixes.  */
    { N_("B"),   N_("KiB"),   N_("MiB"),   N_("GiB"),   N_("TiB"),   N_("PiB")},
    /* TRANSLATORS: that's the list of binary unit prefixes.  */
    {    "",     N_("K"),     N_("M"),     N_("G"),     N_("T"),     N_("P") },
    /* TRANSLATORS: that's the list of binary unit prefixes.  */
    { N_("B/s"), N_("KiB/s"), N_("MiB/s"), N_("GiB/s"), N_("TiB/s"), N_("PiB/s"),  },    
  };

const char *
holy_get_human_size (holy_uint64_t size, enum holy_human_size_type type)
{
  holy_uint64_t fsize;
  unsigned units = 0;
  static char buf[30];
  const char *umsg;

  if (type != holy_HUMAN_SIZE_SPEED)
    fsize = size * 100ULL;
  else
    fsize = size;

  /* Since 2^64 / 1024^5  < 102400, this can give at most 5 iterations.
     So units <=5, so impossible to go past the end of array.
   */
  while (fsize >= 102400)
    {
      fsize = (fsize + 512) / 1024;
      units++;
    }

  umsg = _(holy_human_sizes[type][units]);

  if (units || type == holy_HUMAN_SIZE_SPEED)
    {
      holy_uint64_t whole, fraction;

      whole = holy_divmod64 (fsize, 100, &fraction);
      holy_snprintf (buf, sizeof (buf),
		     "%" PRIuholy_UINT64_T
		     ".%02" PRIuholy_UINT64_T "%s", whole, fraction,
		     umsg);
    }
  else
    holy_snprintf (buf, sizeof (buf), "%llu%s", (unsigned long long) size,
		   umsg);
  return buf;
}

/* Print the information on the device NAME.  */
holy_err_t
holy_normal_print_device_info (const char *name)
{
  holy_device_t dev;
  char *p;

  p = holy_strchr (name, ',');
  if (p)
    {
      holy_xputs ("\t");
      holy_printf_ (N_("Partition %s:"), name);
      holy_xputs (" ");
    }
  else
    {
      holy_printf_ (N_("Device %s:"), name);
      holy_xputs (" ");
    }

  dev = holy_device_open (name);
  if (! dev)
    holy_printf ("%s", _("Filesystem cannot be accessed"));
  else if (dev->disk)
    {
      holy_fs_t fs;

      fs = holy_fs_probe (dev);
      /* Ignore all errors.  */
      holy_errno = 0;

      if (fs)
	{
	  const char *fsname = fs->name;
	  if (holy_strcmp (fsname, "ext2") == 0)
	    fsname = "ext*";
	  holy_printf_ (N_("Filesystem type %s"), fsname);
	  if (fs->label)
	    {
	      char *label;
	      (fs->label) (dev, &label);
	      if (holy_errno == holy_ERR_NONE)
		{
		  if (label && holy_strlen (label))
		    {
		      holy_xputs (" ");
		      holy_printf_ (N_("- Label `%s'"), label);
		    }
		  holy_free (label);
		}
	      holy_errno = holy_ERR_NONE;
	    }
	  if (fs->mtime)
	    {
	      holy_int32_t tm;
	      struct holy_datetime datetime;
	      (fs->mtime) (dev, &tm);
	      if (holy_errno == holy_ERR_NONE)
		{
		  holy_unixtime2datetime (tm, &datetime);
		  holy_xputs (" ");
		  /* TRANSLATORS: Arguments are year, month, day, hour, minute,
		     second, day of the week (translated).  */
		  holy_printf_ (N_("- Last modification time %d-%02d-%02d "
			       "%02d:%02d:%02d %s"),
			       datetime.year, datetime.month, datetime.day,
			       datetime.hour, datetime.minute, datetime.second,
			       holy_get_weekday_name (&datetime));

		}
	      holy_errno = holy_ERR_NONE;
	    }
	  if (fs->uuid)
	    {
	      char *uuid;
	      (fs->uuid) (dev, &uuid);
	      if (holy_errno == holy_ERR_NONE)
		{
		  if (uuid && holy_strlen (uuid))
		    holy_printf (", UUID %s", uuid);
		  holy_free (uuid);
		}
	      holy_errno = holy_ERR_NONE;
	    }
	}
      else
	holy_printf ("%s", _("No known filesystem detected"));

      if (dev->disk->partition)
	holy_printf (_(" - Partition start at %llu%sKiB"),
		     (unsigned long long) (holy_partition_get_start (dev->disk->partition) >> 1),
		     (holy_partition_get_start (dev->disk->partition) & 1) ? ".5" : "" );
      else
	holy_printf_ (N_(" - Sector size %uB"), 1 << dev->disk->log_sector_size);
      if (holy_disk_get_size (dev->disk) == holy_DISK_SIZE_UNKNOWN)
	holy_puts_ (N_(" - Total size unknown"));
      else
	holy_printf (_(" - Total size %llu%sKiB"),
		     (unsigned long long) (holy_disk_get_size (dev->disk) >> 1),
		     /* TRANSLATORS: Replace dot with appropriate decimal separator for
			your language.  */
		     (holy_disk_get_size (dev->disk) & 1) ? _(".5") : "");
    }

  if (dev)
    holy_device_close (dev);

  holy_xputs ("\n");
  return holy_errno;
}
