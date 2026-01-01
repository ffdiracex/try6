/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/command.h>

holy_MOD_LICENSE ("GPLv2+");

#define ADD_MEMORY_DESCRIPTOR(desc, size)	\
  ((holy_efi_memory_descriptor_t *) ((char *) (desc) + (size)))

static holy_err_t
holy_cmd_lsefimmap (holy_command_t cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char **args __attribute__ ((unused)))
{
  holy_efi_uintn_t map_size;
  holy_efi_memory_descriptor_t *memory_map;
  holy_efi_memory_descriptor_t *memory_map_end;
  holy_efi_memory_descriptor_t *desc;
  holy_efi_uintn_t desc_size;

  map_size = 0;
  if (holy_efi_get_memory_map (&map_size, NULL, NULL, &desc_size, 0) < 0)
    return 0;

  memory_map = holy_malloc (map_size);
  if (memory_map == NULL)
    return holy_errno;
  if (holy_efi_get_memory_map (&map_size, memory_map, NULL, &desc_size, 0) <= 0)
    goto fail;

  holy_printf
    ("Type      Physical start  - end             #Pages   "
     "     Size Attributes\n");
  memory_map_end = ADD_MEMORY_DESCRIPTOR (memory_map, map_size);
  for (desc = memory_map;
       desc < memory_map_end;
       desc = ADD_MEMORY_DESCRIPTOR (desc, desc_size))
    {
      holy_efi_uint64_t size;
      holy_efi_uint64_t attr;
      static const char types_str[][9] = 
	{
	  "reserved", 
	  "ldr-code",
	  "ldr-data",
	  "BS-code ",
	  "BS-data ",
	  "RT-code ",
	  "RT-data ",
	  "conv-mem",
	  "unusable",
	  "ACPI-rec",
	  "ACPI-nvs",
	  "MMIO    ",
	  "IO-ports",
	  "PAL-code",
	  "persist ",
	};
      if (desc->type < ARRAY_SIZE (types_str))
	holy_printf ("%s ", types_str[desc->type]);
      else
	holy_printf ("Unk %02x   ", desc->type);
      
      holy_printf (" %016" PRIxholy_UINT64_T "-%016" PRIxholy_UINT64_T
		   " %08" PRIxholy_UINT64_T,
		   desc->physical_start,
		   desc->physical_start + (desc->num_pages << 12) - 1,
		   desc->num_pages);

      size = desc->num_pages << 12;	/* 4 KiB page size */
      /*
       * Since size is a multiple of 4 KiB, no need to handle units
       * of just Bytes (which would use a mask of 0x3ff).
       *
       * 14 characters would support the largest possible number of 4 KiB
       * pages that are not a multiple of larger units (e.g., MiB):
       * 17592186044415 (0xffffff_fffff000), but that uses a lot of
       * whitespace for a rare case.  6 characters usually suffices;
       * columns will be off if not, but this is preferable to rounding.
       */
      if (size & 0xfffff)
	holy_printf (" %6" PRIuholy_UINT64_T "KiB", size >> 10);
      else if (size & 0x3fffffff)
	holy_printf (" %6" PRIuholy_UINT64_T "MiB", size >> 20);
      else if (size & 0xffffffffff)
	holy_printf (" %6" PRIuholy_UINT64_T "GiB", size >> 30);
      else if (size & 0x3ffffffffffff)
	holy_printf (" %6" PRIuholy_UINT64_T "TiB", size >> 40);
      else if (size & 0xfffffffffffffff)
	holy_printf (" %6" PRIuholy_UINT64_T "PiB", size >> 50);
      else
	holy_printf (" %6" PRIuholy_UINT64_T "EiB", size >> 60);

      attr = desc->attribute;
      if (attr & holy_EFI_MEMORY_RUNTIME)
	holy_printf (" RT");
      if (attr & holy_EFI_MEMORY_UC)
	holy_printf (" UC");
      if (attr & holy_EFI_MEMORY_WC)
	holy_printf (" WC");
      if (attr & holy_EFI_MEMORY_WT)
	holy_printf (" WT");
      if (attr & holy_EFI_MEMORY_WB)
	holy_printf (" WB");
      if (attr & holy_EFI_MEMORY_UCE)
	holy_printf (" UCE");
      if (attr & holy_EFI_MEMORY_WP)
	holy_printf (" WP");
      if (attr & holy_EFI_MEMORY_RP)
	holy_printf (" RP");
      if (attr & holy_EFI_MEMORY_XP)
	holy_printf (" XP");
      if (attr & holy_EFI_MEMORY_NV)
	holy_printf (" NV");
      if (attr & holy_EFI_MEMORY_MORE_RELIABLE)
	holy_printf (" MR");
      if (attr & holy_EFI_MEMORY_RO)
	holy_printf (" RO");

      holy_printf ("\n");
    }

 fail:
  holy_free (memory_map);
  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(lsefimmap)
{
  cmd = holy_register_command ("lsefimmap", holy_cmd_lsefimmap,
			       "", "Display EFI memory map.");
}

holy_MOD_FINI(lsefimmap)
{
  holy_unregister_command (cmd);
}
