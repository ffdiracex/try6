/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_CPU_PCI_H
#define	holy_CPU_PCI_H	1

#include <holy/types.h>
#include <holy/i386/io.h>

#define holy_MACHINE_PCI_IO_BASE          0
#define holy_PCI_ADDR_REG	0xcf8
#define holy_PCI_DATA_REG	0xcfc
#define holy_PCI_NUM_BUS        256
#define holy_PCI_NUM_DEVICES    32

static inline holy_uint32_t
holy_pci_read (holy_pci_address_t addr)
{
  holy_outl (addr, holy_PCI_ADDR_REG);
  return holy_inl (holy_PCI_DATA_REG);
}

static inline holy_uint16_t
holy_pci_read_word (holy_pci_address_t addr)
{
  holy_outl (addr & ~3, holy_PCI_ADDR_REG);
  return holy_inw (holy_PCI_DATA_REG + (addr & 3));
}

static inline holy_uint8_t
holy_pci_read_byte (holy_pci_address_t addr)
{
  holy_outl (addr & ~3, holy_PCI_ADDR_REG);
  return holy_inb (holy_PCI_DATA_REG + (addr & 3));
}

static inline void
holy_pci_write (holy_pci_address_t addr, holy_uint32_t data)
{
  holy_outl (addr, holy_PCI_ADDR_REG);
  holy_outl (data, holy_PCI_DATA_REG);
}

static inline void
holy_pci_write_word (holy_pci_address_t addr, holy_uint16_t data)
{
  holy_outl (addr & ~3, holy_PCI_ADDR_REG);
  holy_outw (data, holy_PCI_DATA_REG + (addr & 3));
}

static inline void
holy_pci_write_byte (holy_pci_address_t addr, holy_uint8_t data)
{
  holy_outl (addr & ~3, holy_PCI_ADDR_REG);
  holy_outb (data, holy_PCI_DATA_REG + (addr & 3));
}

#ifndef holy_MACHINE_IEEE1275

static inline volatile void *
holy_pci_device_map_range (holy_pci_device_t dev __attribute__ ((unused)),
			   holy_addr_t base,
			   holy_size_t size __attribute__ ((unused)))
{
  return (volatile void *) base;
}

static inline void
holy_pci_device_unmap_range (holy_pci_device_t dev __attribute__ ((unused)),
			     volatile void *mem __attribute__ ((unused)),
			     holy_size_t size __attribute__ ((unused)))
{
}

#else

volatile void *
holy_pci_device_map_range (holy_pci_device_t dev,
			   holy_addr_t base,
			   holy_size_t size);

void
holy_pci_device_unmap_range (holy_pci_device_t dev,
			     volatile void *mem,
			     holy_size_t size);

#endif


#endif /* holy_CPU_PCI_H */
