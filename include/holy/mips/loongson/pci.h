/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_MACHINE_PCI_H
#define	holy_MACHINE_PCI_H	1

#ifndef ASM_FILE
#include <holy/types.h>
#include <holy/cpu/io.h>
#endif

#define holy_LOONGSON_OHCI_PCIID 0x00351033
#define holy_LOONGSON_EHCI_PCIID 0x00e01033

#define holy_MACHINE_PCI_IO_BASE_2F        0xbfd00000
#define holy_MACHINE_PCI_IO_BASE_3A        0xb8000000
#define holy_MACHINE_PCI_CONFSPACE_2F      0xbfe80000
#define holy_MACHINE_PCI_CONFSPACE_3A      0xba000000
#define holy_MACHINE_PCI_CONFSPACE_3A_EXT  0xbb000000
#define holy_MACHINE_PCI_CONTROLLER_HEADER 0xbfe00000

#define holy_MACHINE_PCI_CONF_CTRL_REG_ADDR_2F 0xbfe00118

#define holy_PCI_NUM_DEVICES_2F 16

#ifndef ASM_FILE

typedef enum { holy_BONITO_2F, holy_BONITO_3A } holy_bonito_type_t;
extern holy_bonito_type_t EXPORT_VAR (holy_bonito_type);

#define holy_PCI_NUM_DEVICES    (holy_bonito_type ? 32 \
				 : holy_PCI_NUM_DEVICES_2F)
#define holy_PCI_NUM_BUS        (holy_bonito_type ? 256 : 1)

#define holy_MACHINE_PCI_IO_BASE	(holy_bonito_type \
					 ? holy_MACHINE_PCI_IO_BASE_3A \
					 : holy_MACHINE_PCI_IO_BASE_2F)

#define holy_MACHINE_PCI_CONF_CTRL_REG_2F    (*(volatile holy_uint32_t *) \
					   holy_MACHINE_PCI_CONF_CTRL_REG_ADDR_2F)
#define holy_MACHINE_PCI_IO_CTRL_REG_2F      (*(volatile holy_uint32_t *) 0xbfe00110)
#define holy_MACHINE_PCI_CONF_CTRL_REG_3A    (*(volatile holy_uint32_t *) \
					      0xbfe00118)

#endif
#define holy_MACHINE_PCI_WIN_MASK_SIZE    6
#define holy_MACHINE_PCI_WIN_MASK         ((1 << holy_MACHINE_PCI_WIN_MASK_SIZE) - 1)

/* We have 3 PCI windows.  */
#define holy_MACHINE_PCI_NUM_WIN          3
/* Each window is 64MiB.  */
#define holy_MACHINE_PCI_WIN_SHIFT        26
#define holy_MACHINE_PCI_WIN_OFFSET_MASK  ((1 << holy_MACHINE_PCI_WIN_SHIFT) - 1)

#define holy_MACHINE_PCI_WIN_SIZE         0x04000000
/* Graphical acceleration takes 1 MiB away.  */
#define holy_MACHINE_PCI_WIN1_SIZE        0x03f00000

#define holy_MACHINE_PCI_WIN1_ADDR        0xb0000000
#define holy_MACHINE_PCI_WIN2_ADDR        0xb4000000
#define holy_MACHINE_PCI_WIN3_ADDR        0xb8000000

#ifndef ASM_FILE
holy_uint32_t
EXPORT_FUNC (holy_pci_read) (holy_pci_address_t addr);

holy_uint16_t
EXPORT_FUNC (holy_pci_read_word) (holy_pci_address_t addr);

holy_uint8_t
EXPORT_FUNC (holy_pci_read_byte) (holy_pci_address_t addr);

void
EXPORT_FUNC (holy_pci_write) (holy_pci_address_t addr, holy_uint32_t data);

void
EXPORT_FUNC (holy_pci_write_word) (holy_pci_address_t addr, holy_uint16_t data);

void
EXPORT_FUNC (holy_pci_write_byte) (holy_pci_address_t addr, holy_uint8_t data);

volatile void *
EXPORT_FUNC (holy_pci_device_map_range) (holy_pci_device_t dev,
					 holy_addr_t base, holy_size_t size);
void *
EXPORT_FUNC (holy_pci_device_map_range_cached) (holy_pci_device_t dev,
						holy_addr_t base,
						holy_size_t size);
void
EXPORT_FUNC (holy_pci_device_unmap_range) (holy_pci_device_t dev,
					   volatile void *mem,
					   holy_size_t size);
#endif

#endif /* holy_MACHINE_PCI_H */
