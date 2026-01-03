/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/pci.h>
#include <holy/dl.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>

holy_pci_address_t
holy_pci_make_address (holy_pci_device_t dev, int reg)
{
  holy_pci_address_t ret;
  ret.dev = dev;
  ret.pos = reg;
  return ret;
}

void
holy_pci_iterate (holy_pci_iteratefunc_t hook, void *hook_data)
{
  struct pci_device_iterator *iter;
  struct pci_slot_match slot;
  struct pci_device *dev;
  slot.domain = PCI_MATCH_ANY;
  slot.bus = PCI_MATCH_ANY;
  slot.dev = PCI_MATCH_ANY;
  slot.func = PCI_MATCH_ANY;
  iter = pci_slot_match_iterator_create (&slot);
  while ((dev = pci_device_next (iter)))
    hook (dev, dev->vendor_id | (dev->device_id << 16), hook_data);
  pci_iterator_destroy (iter);
}

void *
holy_pci_device_map_range (holy_pci_device_t dev, holy_addr_t base,
			   holy_size_t size)
{
  void *addr;
  int err;
  err = pci_device_map_range (dev, base, size, PCI_DEV_MAP_FLAG_WRITABLE, &addr);
  if (err)
    holy_util_error ("mapping 0x%llx failed (error %d)",
		     (unsigned long long) base, err);
  return addr;
}

void
holy_pci_device_unmap_range (holy_pci_device_t dev, void *mem,
			     holy_size_t size)
{
  pci_device_unmap_range (dev, mem, size);
}

holy_MOD_INIT (emupci)
{
  pci_system_init ();
}

holy_MOD_FINI (emupci)
{
  pci_system_cleanup ();
}
