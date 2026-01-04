/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef __mips__
#include <holy/pci.h>
#include <holy/mm.h>
#endif
#include <holy/machine/kernel.h>
#include <holy/misc.h>
#include <holy/vga.h>

static struct {holy_uint8_t r, g, b, a; } colors[] =
  {
    // {R, G, B, A}
    {0x00, 0x00, 0x00, 0xFF}, // 0 = black
    {0x00, 0x00, 0xA8, 0xFF}, // 1 = blue
    {0x00, 0xA8, 0x00, 0xFF}, // 2 = green
    {0x00, 0xA8, 0xA8, 0xFF}, // 3 = cyan
    {0xA8, 0x00, 0x00, 0xFF}, // 4 = red
    {0xA8, 0x00, 0xA8, 0xFF}, // 5 = magenta
    {0xA8, 0x54, 0x00, 0xFF}, // 6 = brown
    {0xA8, 0xA8, 0xA8, 0xFF}, // 7 = light gray

    {0x54, 0x54, 0x54, 0xFF}, // 8 = dark gray
    {0x54, 0x54, 0xFE, 0xFF}, // 9 = bright blue
    {0x54, 0xFE, 0x54, 0xFF}, // 10 = bright green
    {0x54, 0xFE, 0xFE, 0xFF}, // 11 = bright cyan
    {0xFE, 0x54, 0x54, 0xFF}, // 12 = bright red
    {0xFE, 0x54, 0xFE, 0xFF}, // 13 = bright magenta
    {0xFE, 0xFE, 0x54, 0xFF}, // 14 = yellow
    {0xFE, 0xFE, 0xFE, 0xFF}  // 15 = white
  };

#include <ascii.h>

#ifdef __mips__
#define VGA_ADDR 0xb00a0000
#else
#define VGA_ADDR 0xa0000
#endif

static void
load_font (void)
{
  unsigned i;

  holy_vga_gr_write (0 << 2, holy_VGA_GR_GR6);

  holy_vga_sr_write (holy_VGA_SR_MEMORY_MODE_NORMAL, holy_VGA_SR_MEMORY_MODE);
  holy_vga_sr_write (1 << holy_VGA_TEXT_FONT_PLANE,
		     holy_VGA_SR_MAP_MASK_REGISTER);

  holy_vga_gr_write (0, holy_VGA_GR_DATA_ROTATE);
  holy_vga_gr_write (0, holy_VGA_GR_MODE);
  holy_vga_gr_write (0xff, holy_VGA_GR_BITMASK);

  for (i = 0; i < 128; i++)
    holy_memcpy ((void *) (VGA_ADDR + 32 * i), ascii_bitmaps + 16 * i, 16);
}

static void
load_palette (void)
{
  unsigned i;
  for (i = 0; i < 16; i++)
    holy_vga_write_arx (i, i);

  for (i = 0; i < ARRAY_SIZE (colors); i++)
    holy_vga_palette_write (i, colors[i].r, colors[i].g, colors[i].b);
}

void
holy_qemu_init_cirrus (void)
{
  holy_outb (holy_VGA_IO_MISC_COLOR,
	     holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_MISC_WRITE);

  load_font ();

  holy_vga_gr_write (holy_VGA_GR_GR6_MMAP_CGA, holy_VGA_GR_GR6);
  holy_vga_gr_write (holy_VGA_GR_MODE_ODD_EVEN, holy_VGA_GR_MODE);

  holy_vga_sr_write (holy_VGA_SR_MEMORY_MODE_NORMAL, holy_VGA_SR_MEMORY_MODE);

  holy_vga_sr_write ((1 << holy_VGA_TEXT_TEXT_PLANE)
		     | (1 << holy_VGA_TEXT_ATTR_PLANE),
		     holy_VGA_SR_MAP_MASK_REGISTER);

  holy_vga_cr_write (15, holy_VGA_CR_CELL_HEIGHT);
  holy_vga_cr_write (79, holy_VGA_CR_HORIZ_END);
  holy_vga_cr_write (40, holy_VGA_CR_PITCH);

  int vert = 25 * 16;
  holy_vga_cr_write (vert & 0xff, holy_VGA_CR_VDISPLAY_END);
  holy_vga_cr_write (((vert >> holy_VGA_CR_OVERFLOW_HEIGHT1_SHIFT)
		      & holy_VGA_CR_OVERFLOW_HEIGHT1_MASK)
		     | ((vert >> holy_VGA_CR_OVERFLOW_HEIGHT2_SHIFT)
			& holy_VGA_CR_OVERFLOW_HEIGHT2_MASK),
		     holy_VGA_CR_OVERFLOW);

  load_palette ();

  holy_vga_write_arx (holy_VGA_ARX_MODE_TEXT, holy_VGA_ARX_MODE);
  holy_vga_write_arx (0, holy_VGA_ARX_COLOR_SELECT);

  holy_vga_sr_write (holy_VGA_SR_CLOCKING_MODE_8_DOT_CLOCK,
		     holy_VGA_SR_CLOCKING_MODE);

  holy_vga_cr_write (14, holy_VGA_CR_CURSOR_START);
  holy_vga_cr_write (15, holy_VGA_CR_CURSOR_END);

  holy_outb (0x20, holy_MACHINE_PCI_IO_BASE + holy_VGA_IO_ARX);
}
