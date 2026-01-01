/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/types.h>
#include <holy/ieee1275/ieee1275.h>

int (*holy_ieee1275_entry_fn) (void *) holy_IEEE1275_ENTRY_FN_ATTRIBUTE;

holy_ieee1275_phandle_t holy_ieee1275_chosen;
holy_ieee1275_ihandle_t holy_ieee1275_mmu;

static holy_uint32_t holy_ieee1275_flags;



int
holy_ieee1275_test_flag (enum holy_ieee1275_flag flag)
{
  return (holy_ieee1275_flags & (1 << flag));
}

void
holy_ieee1275_set_flag (enum holy_ieee1275_flag flag)
{
  holy_ieee1275_flags |= (1 << flag);
}

static void
holy_ieee1275_find_options (void)
{
  holy_ieee1275_phandle_t root;
  holy_ieee1275_phandle_t options;
  holy_ieee1275_phandle_t openprom;
  holy_ieee1275_phandle_t bootrom;
  int rc;
  holy_uint32_t realmode = 0;
  char tmp[256];
  int is_smartfirmware = 0;
  int is_olpc = 0;
  int is_qemu = 0;
  holy_ssize_t actual;

#ifdef __sparc__
  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_PARTITION_0);
#endif

  holy_ieee1275_finddevice ("/", &root);
  holy_ieee1275_finddevice ("/options", &options);
  holy_ieee1275_finddevice ("/openprom", &openprom);

  rc = holy_ieee1275_get_integer_property (options, "real-mode?", &realmode,
					   sizeof realmode, 0);
  if (((rc >= 0) && realmode) || (holy_ieee1275_mmu == 0))
    holy_ieee1275_set_flag (holy_IEEE1275_FLAG_REAL_MODE);

  rc = holy_ieee1275_get_property (openprom, "CodeGen-copyright",
				   tmp,	sizeof (tmp), 0);
  if (rc >= 0 && !holy_strncmp (tmp, "SmartFirmware(tm)",
				sizeof ("SmartFirmware(tm)") - 1))
    is_smartfirmware = 1;

  rc = holy_ieee1275_get_property (root, "architecture",
				   tmp,	sizeof (tmp), 0);
  if (rc >= 0 && !holy_strcmp (tmp, "OLPC"))
    is_olpc = 1;

  rc = holy_ieee1275_get_property (root, "model",
				   tmp,	sizeof (tmp), 0);
  if (rc >= 0 && (!holy_strcmp (tmp, "Emulated PC")
		  || !holy_strcmp (tmp, "IBM pSeries (emulated by qemu)"))) {
    is_qemu = 1;
  }

  if (rc >= 0 && holy_strncmp (tmp, "IBM", 3) == 0)
    holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_TREE_SCANNING_FOR_DISKS);

  /* Old Macs have no key repeat, newer ones have fully working one.
     The ones inbetween when repeated key generates an escaoe sequence
     only the escape is repeated. With this workaround however a fast
     e.g. down arrow-ESC is perceived as down arrow-down arrow which is
     also annoying but is less so than the original bug of exiting from
     the current window on arrow repeat. To avoid unaffected users suffering
     from this workaround match only exact models known to have this bug.
   */
  if (rc >= 0 && holy_strcmp (tmp, "PowerBook3,3") == 0)
    holy_ieee1275_set_flag (holy_IEEE1275_FLAG_BROKEN_REPEAT);

  rc = holy_ieee1275_get_property (root, "compatible",
				   tmp,	sizeof (tmp), &actual);
  if (rc >= 0)
    {
      char *ptr;
      for (ptr = tmp; ptr - tmp < actual; ptr += holy_strlen (ptr) + 1)
	{
	  if (holy_memcmp (ptr, "MacRISC", sizeof ("MacRISC") - 1) == 0
	      && (ptr[sizeof ("MacRISC") - 1] == 0
		  || holy_isdigit (ptr[sizeof ("MacRISC") - 1])))
	    {
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_BROKEN_ADDRESS_CELLS);
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_OFNET_SUFFIX);
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_VIRT_TO_REAL_BROKEN);
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_CURSORONOFF_ANSI_BROKEN);
	      break;
	    }
	}
    }

  if (is_smartfirmware)
    {
      /* Broken in all versions */
      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_BROKEN_OUTPUT);

      /* There are two incompatible ways of checking the version number.  Try
         both. */
      rc = holy_ieee1275_get_property (openprom, "SmartFirmware-version",
				       tmp, sizeof (tmp), 0);
      if (rc < 0)
	rc = holy_ieee1275_get_property (openprom, "firmware-version",
					 tmp, sizeof (tmp), 0);
      if (rc >= 0)
	{
	  /* It is tempting to implement a version parser to set the flags for
	     e.g. 1.3 and below.  However, there's a special situation here.
	     3rd party updates which fix the partition bugs are common, and for
	     some reason their fixes aren't being merged into trunk.  So for
	     example we know that 1.2 and 1.3 are broken, but there's 1.2.99
	     and 1.3.99 which are known good (and applying this workaround
	     would cause breakage). */
	  if (!holy_strcmp (tmp, "1.0")
	      || !holy_strcmp (tmp, "1.1")
	      || !holy_strcmp (tmp, "1.2")
	      || !holy_strcmp (tmp, "1.3"))
	    {
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_PARTITION_0);
	      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_0_BASED_PARTITIONS);
	    }
	}
    }

  if (is_olpc)
    {
      /* OLPC / XO laptops have three kinds of storage devices:

	 - NAND flash.  These are accessible via OFW callbacks, but:
	   - Follow strange semantics, imposed by hardware constraints.
	   - Its ABI is undocumented, and not stable.
	   They lack "device_type" property, which conveniently makes holy
	   skip them.

	 - USB drives.  Not accessible, because OFW shuts down the controller
	   in order to prevent collisions with applications accessing it
	   directly.  Even worse, attempts to access it will NOT return
	   control to the caller, so we have to avoid probing them.

	 - SD cards.  These work fine.

	 To avoid breakage, we only need to skip USB probing.  However,
	 since detecting SD cards is more reliable, we do that instead.
      */

      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_OFDISK_SDCARD_ONLY);
      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_HAS_CURSORONOFF);
    }

  if (is_qemu)
    {
      /* OpenFirmware hangs on qemu if one requests any memory below 1.5 MiB.  */
      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_PRE1_5M_CLAIM);

      holy_ieee1275_set_flag (holy_IEEE1275_FLAG_HAS_CURSORONOFF);
    }

  if (! holy_ieee1275_finddevice ("/rom/boot-rom", &bootrom)
      || ! holy_ieee1275_finddevice ("/boot-rom", &bootrom))
    {
      rc = holy_ieee1275_get_property (bootrom, "model", tmp, sizeof (tmp), 0);
      if (rc >= 0 && !holy_strncmp (tmp, "PPC Open Hack'Ware",
				    sizeof ("PPC Open Hack'Ware") - 1))
	{
	  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_BROKEN_OUTPUT);
	  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_CANNOT_SET_COLORS);
	  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_CANNOT_INTERPRET);
	  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_FORCE_CLAIM);
	  holy_ieee1275_set_flag (holy_IEEE1275_FLAG_NO_ANSI);
	}
    }
}

void
holy_ieee1275_init (void)
{
  holy_ieee1275_finddevice ("/chosen", &holy_ieee1275_chosen);

  if (holy_ieee1275_get_integer_property (holy_ieee1275_chosen, "mmu", &holy_ieee1275_mmu,
					  sizeof holy_ieee1275_mmu, 0) < 0)
    holy_ieee1275_mmu = 0;

  holy_ieee1275_find_options ();
}
