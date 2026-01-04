/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/memory.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/types.h>

holy_err_t
holy_machine_mmap_iterate (holy_memory_hook_t hook, void *hook_data)
{
  holy_ieee1275_phandle_t root;
  holy_ieee1275_phandle_t memory;
  holy_uint32_t available[128];
  holy_ssize_t available_size;
  holy_uint32_t address_cells = 1;
  holy_uint32_t size_cells = 1;
  int i;

  /* Determine the format of each entry in `available'.  */
  holy_ieee1275_finddevice ("/", &root);
  holy_ieee1275_get_integer_property (root, "#address-cells", &address_cells,
				      sizeof address_cells, 0);
  holy_ieee1275_get_integer_property (root, "#size-cells", &size_cells,
				      sizeof size_cells, 0);

  if (size_cells > address_cells)
    address_cells = size_cells;

  /* Load `/memory/available'.  */
  if (holy_ieee1275_finddevice ("/memory", &memory))
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
		       "couldn't find /memory node");
  if (holy_ieee1275_get_integer_property (memory, "available", available,
					  sizeof available, &available_size))
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
		       "couldn't examine /memory/available property");
  if (available_size < 0 || (holy_size_t) available_size > sizeof (available))
    return holy_error (holy_ERR_UNKNOWN_DEVICE,
                       "/memory response buffer exceeded");

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_BROKEN_ADDRESS_CELLS))
    {
      address_cells = 1;
      size_cells = 1;
    }

  /* Decode each entry and call `hook'.  */
  i = 0;
  available_size /= sizeof (holy_uint32_t);
  while (i < available_size)
    {
      holy_uint64_t address;
      holy_uint64_t size;

      address = available[i++];
      if (address_cells == 2)
	address = (address << 32) | available[i++];

      size = available[i++];
      if (size_cells == 2)
	size = (size << 32) | available[i++];

      if (hook (address, size, holy_MEMORY_AVAILABLE, hook_data))
	break;
    }

  return holy_errno;
}
