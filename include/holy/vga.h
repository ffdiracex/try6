/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VGA_HEADER
#define holy_VGA_HEADER	1

#ifndef holy_MACHINE_MIPS_QEMU_MIPS
#include "pci.h"
#else
#include <holy/cpu/io.h>
#define holy_MACHINE_PCI_IO_BASE  0xb4000000
#endif

#include <holy/vgaregs.h>

static inline void
holy_vga_gr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_GR_INDEX);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_GR_DATA);
}

static inline holy_uint8_t
holy_vga_gr_read (holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_GR_INDEX);
  return holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_GR_DATA);
}

static inline void
holy_vga_cr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_INDEX);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_DATA);
}

static inline holy_uint8_t
holy_vga_cr_read (holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_INDEX);
  return holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_DATA);
}

static inline void
holy_vga_cr_bw_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_BW_INDEX);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_BW_DATA);
}

static inline holy_uint8_t
holy_vga_cr_bw_read (holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_BW_INDEX);
  return holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_CR_BW_DATA);
}

static inline void
holy_vga_sr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_SR_INDEX);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_SR_DATA);
}

static inline holy_uint8_t
holy_vga_sr_read (holy_uint8_t addr)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_SR_INDEX);
  return holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_SR_DATA);
}

static inline void
holy_vga_palette_read (holy_uint8_t addr, holy_uint8_t *r, holy_uint8_t *g,
		       holy_uint8_t *b)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_READ_INDEX);
  *r = holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
  *g = holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
  *b = holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
}

static inline void
holy_vga_palette_write (holy_uint8_t addr, holy_uint8_t r, holy_uint8_t g,
			holy_uint8_t b)
{
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_WRITE_INDEX);
  holy_outb (r, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
  holy_outb (g, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
  holy_outb (b, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_PALLETTE_DATA);
}

static inline void
holy_vga_write_arx (holy_uint8_t val, holy_uint8_t addr)
{
  holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_INPUT_STATUS1_REGISTER);
  holy_inb (holy_MACHINE_PCI_IO_BASE + 0x3ba);

  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX);
  holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX_READ);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX);
}

static inline holy_uint8_t
holy_vga_read_arx (holy_uint8_t addr)
{
  holy_uint8_t val;
  holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_INPUT_STATUS1_REGISTER);
  holy_outb (addr, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX);
  val = holy_inb (holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX_READ);
  holy_outb (val, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX);
  return val;
}

#endif
