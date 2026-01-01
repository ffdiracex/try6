/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_PCIUTILS_H
#define	holy_PCIUTILS_H	1

#include <pciaccess.h>

typedef struct pci_device *holy_pci_device_t;

static inline int
holy_pci_get_bus (holy_pci_device_t dev)
{
  return dev->bus;
}

static inline int
holy_pci_get_device (holy_pci_device_t dev)
{
  return dev->dev;
}

static inline int
holy_pci_get_function (holy_pci_device_t dev)
{
  return dev->func;
}

struct holy_pci_address
{
  holy_pci_device_t dev;
  int pos;
};

typedef struct holy_pci_address holy_pci_address_t;

static inline holy_uint32_t
holy_pci_read (holy_pci_address_t addr)
{
  holy_uint32_t ret;
  pci_device_cfg_read_u32 (addr.dev, &ret, addr.pos);
  return ret;
}

static inline holy_uint16_t
holy_pci_read_word (holy_pci_address_t addr)
{
  holy_uint16_t ret;
  pci_device_cfg_read_u16 (addr.dev, &ret, addr.pos);
  return ret;
}

static inline holy_uint8_t
holy_pci_read_byte (holy_pci_address_t addr)
{
  holy_uint8_t ret;
  pci_device_cfg_read_u8 (addr.dev, &ret, addr.pos);
  return ret;
}

static inline void
holy_pci_write (holy_pci_address_t addr, holy_uint32_t data)
{
  pci_device_cfg_write_u32 (addr.dev, data, addr.pos);
}

static inline void
holy_pci_write_word (holy_pci_address_t addr, holy_uint16_t data)
{
  pci_device_cfg_write_u16 (addr.dev, data, addr.pos);
}

static inline void
holy_pci_write_byte (holy_pci_address_t addr, holy_uint8_t data)
{
  pci_device_cfg_write_u8 (addr.dev, data, addr.pos);
}

void *
holy_pci_device_map_range (holy_pci_device_t dev, holy_addr_t base,
			   holy_size_t size);

void
holy_pci_device_unmap_range (holy_pci_device_t dev, void *mem,
			     holy_size_t size);


#endif /* holy_PCIUTILS_H */
