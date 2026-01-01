/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/pci.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/ieee1275/ieee1275.h>

volatile void *
holy_pci_device_map_range (holy_pci_device_t dev __attribute__ ((unused)),
			   holy_addr_t base,
			   holy_size_t size)
{
  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_REAL_MODE))
    return (volatile void *) base;
  if (holy_ieee1275_map (base, base, size, 7))
    holy_fatal ("couldn't map 0x%lx", base);
  return (volatile void *) base;
}

void
holy_pci_device_unmap_range (holy_pci_device_t dev __attribute__ ((unused)),
			     volatile void *mem __attribute__ ((unused)),
			     holy_size_t size __attribute__ ((unused)))
{
}
