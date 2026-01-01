/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_PCI_H
#define	holy_PCI_H	1

#ifndef ASM_FILE
#include "types.h"
#include <holy/symbol.h>
#endif

#define  holy_PCI_ADDR_SPACE_MASK	0x01
#define  holy_PCI_ADDR_SPACE_MEMORY	0x00
#define  holy_PCI_ADDR_SPACE_IO		0x01

#define  holy_PCI_ADDR_MEM_TYPE_MASK	0x06
#define  holy_PCI_ADDR_MEM_TYPE_32	0x00	/* 32 bit address */
#define  holy_PCI_ADDR_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define  holy_PCI_ADDR_MEM_TYPE_64	0x04	/* 64 bit address */
#define  holy_PCI_ADDR_MEM_PREFETCH	0x08	/* prefetchable */

#define  holy_PCI_ADDR_MEM_MASK		~0xf
#define  holy_PCI_ADDR_IO_MASK		~0x03

#define  holy_PCI_REG_PCI_ID       0x00
#define  holy_PCI_REG_VENDOR       0x00
#define  holy_PCI_REG_DEVICE       0x02
#define  holy_PCI_REG_COMMAND      0x04
#define  holy_PCI_REG_STATUS       0x06
#define  holy_PCI_REG_REVISION     0x08
#define  holy_PCI_REG_CLASS        0x08
#define  holy_PCI_REG_CACHELINE    0x0c
#define  holy_PCI_REG_LAT_TIMER    0x0d
#define  holy_PCI_REG_HEADER_TYPE  0x0e
#define  holy_PCI_REG_BIST         0x0f
#define  holy_PCI_REG_ADDRESSES    0x10

/* Beware that 64-bit address takes 2 registers.  */
#define  holy_PCI_REG_ADDRESS_REG0 0x10
#define  holy_PCI_REG_ADDRESS_REG1 0x14
#define  holy_PCI_REG_ADDRESS_REG2 0x18
#define  holy_PCI_REG_ADDRESS_REG3 0x1c
#define  holy_PCI_REG_ADDRESS_REG4 0x20
#define  holy_PCI_REG_ADDRESS_REG5 0x24

#define  holy_PCI_REG_CIS_POINTER  0x28
#define  holy_PCI_REG_SUBVENDOR    0x2c
#define  holy_PCI_REG_SUBSYSTEM    0x2e
#define  holy_PCI_REG_ROM_ADDRESS  0x30
#define  holy_PCI_REG_CAP_POINTER  0x34
#define  holy_PCI_REG_IRQ_LINE     0x3c
#define  holy_PCI_REG_IRQ_PIN      0x3d
#define  holy_PCI_REG_MIN_GNT      0x3e
#define  holy_PCI_REG_MAX_LAT      0x3f

#define  holy_PCI_COMMAND_IO_ENABLED    0x0001
#define  holy_PCI_COMMAND_MEM_ENABLED   0x0002
#define  holy_PCI_COMMAND_BUS_MASTER    0x0004
#define  holy_PCI_COMMAND_PARITY_ERROR  0x0040
#define  holy_PCI_COMMAND_SERR_ENABLE   0x0100

#define  holy_PCI_STATUS_CAPABILITIES      0x0010
#define  holy_PCI_STATUS_66MHZ_CAPABLE     0x0020
#define  holy_PCI_STATUS_FAST_B2B_CAPABLE  0x0080

#define  holy_PCI_STATUS_DEVSEL_TIMING_SHIFT 9
#define  holy_PCI_STATUS_DEVSEL_TIMING_MASK 0x0600
#define  holy_PCI_CLASS_SUBCLASS_VGA  0x0300
#define  holy_PCI_CLASS_SUBCLASS_USB  0x0c03

#ifndef ASM_FILE

enum
  {
    holy_PCI_CLASS_NETWORK = 0x02
  };
enum
  {
    holy_PCI_CAP_POWER_MANAGEMENT = 0x01
  };

enum
  {
    holy_PCI_VENDOR_BROADCOM = 0x14e4
  };


typedef holy_uint32_t holy_pci_id_t;

#ifdef holy_MACHINE_EMU
#include <holy/pciutils.h>
#else
typedef holy_uint32_t holy_pci_address_t;
struct holy_pci_device
{
  int bus;
  int device;
  int function;
};
typedef struct holy_pci_device holy_pci_device_t;
static inline int
holy_pci_get_bus (holy_pci_device_t dev)
{
  return dev.bus;
}

static inline int
holy_pci_get_device (holy_pci_device_t dev)
{
  return dev.device;
}

static inline int
holy_pci_get_function (holy_pci_device_t dev)
{
  return dev.function;
}
#include <holy/cpu/pci.h>
#endif

typedef int (*holy_pci_iteratefunc_t)
     (holy_pci_device_t dev, holy_pci_id_t pciid, void *data);

holy_pci_address_t EXPORT_FUNC(holy_pci_make_address) (holy_pci_device_t dev,
						       int reg);

void EXPORT_FUNC(holy_pci_iterate) (holy_pci_iteratefunc_t hook,
				    void *hook_data);

struct holy_pci_dma_chunk;

struct holy_pci_dma_chunk *EXPORT_FUNC(holy_memalign_dma32) (holy_size_t align,
							     holy_size_t size);
void EXPORT_FUNC(holy_dma_free) (struct holy_pci_dma_chunk *ch);
volatile void *EXPORT_FUNC(holy_dma_get_virt) (struct holy_pci_dma_chunk *ch);
holy_uint32_t EXPORT_FUNC(holy_dma_get_phys) (struct holy_pci_dma_chunk *ch);

static inline void *
holy_dma_phys2virt (holy_uint32_t phys, struct holy_pci_dma_chunk *chunk)
{
  return ((holy_uint8_t *) holy_dma_get_virt (chunk)
	  + (phys - holy_dma_get_phys (chunk)));
}

static inline holy_uint32_t
holy_dma_virt2phys (volatile void *virt, struct holy_pci_dma_chunk *chunk)
{
  return (((holy_uint8_t *) virt - (holy_uint8_t *) holy_dma_get_virt (chunk))
	  + holy_dma_get_phys (chunk));
}

holy_uint8_t
EXPORT_FUNC (holy_pci_find_capability) (holy_pci_device_t dev, holy_uint8_t cap);

#endif

#endif /* holy_PCI_H */
