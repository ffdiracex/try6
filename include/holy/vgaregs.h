/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VGAREGS_HEADER
#define holy_VGAREGS_HEADER	1

#ifdef ASM_FILE
#define holy_VGA_IO_SR_INDEX 0x3c4
#define holy_VGA_IO_SR_DATA 0x3c5
#else

enum
  {
    holy_VGA_IO_CR_BW_INDEX = 0x3b4,
    holy_VGA_IO_CR_BW_DATA = 0x3b5,
    holy_VGA_IO_ARX = 0x3c0,
    holy_VGA_IO_ARX_READ = 0x3c1,
    holy_VGA_IO_MISC_WRITE = 0x3c2,
    holy_VGA_IO_SR_INDEX = 0x3c4,
    holy_VGA_IO_SR_DATA = 0x3c5,
    holy_VGA_IO_PIXEL_MASK = 0x3c6,
    holy_VGA_IO_PALLETTE_READ_INDEX = 0x3c7,
    holy_VGA_IO_PALLETTE_WRITE_INDEX = 0x3c8,
    holy_VGA_IO_PALLETTE_DATA = 0x3c9,
    holy_VGA_IO_GR_INDEX = 0x3ce,
    holy_VGA_IO_GR_DATA = 0x3cf,
    holy_VGA_IO_CR_INDEX = 0x3d4,
    holy_VGA_IO_CR_DATA = 0x3d5,
    holy_VGA_IO_INPUT_STATUS1_REGISTER = 0x3da
  };

#define holy_VGA_IO_INPUT_STATUS1_VERTR_BIT 0x08

enum
  {
    holy_VGA_CR_HTOTAL = 0x00,
    holy_VGA_CR_HORIZ_END = 0x01,
    holy_VGA_CR_HBLANK_START = 0x02,
    holy_VGA_CR_HBLANK_END = 0x03,
    holy_VGA_CR_HORIZ_SYNC_PULSE_START = 0x04,
    holy_VGA_CR_HORIZ_SYNC_PULSE_END = 0x05,
    holy_VGA_CR_VERT_TOTAL = 0x06,
    holy_VGA_CR_OVERFLOW = 0x07,
    holy_VGA_CR_BYTE_PANNING = 0x08,
    holy_VGA_CR_CELL_HEIGHT = 0x09,
    holy_VGA_CR_CURSOR_START = 0x0a,
    holy_VGA_CR_CURSOR_END = 0x0b,
    holy_VGA_CR_START_ADDR_HIGH_REGISTER = 0x0c,
    holy_VGA_CR_START_ADDR_LOW_REGISTER = 0x0d,
    holy_VGA_CR_CURSOR_ADDR_HIGH = 0x0e,
    holy_VGA_CR_CURSOR_ADDR_LOW = 0x0f,
    holy_VGA_CR_VSYNC_START = 0x10,
    holy_VGA_CR_VSYNC_END = 0x11,
    holy_VGA_CR_VDISPLAY_END = 0x12,
    holy_VGA_CR_PITCH = 0x13,
    holy_VGA_CR_UNDERLINE_LOCATION = 0x14,
    holy_VGA_CR_VERTICAL_BLANK_START = 0x15,
    holy_VGA_CR_VERTICAL_BLANK_END = 0x16,
    holy_VGA_CR_MODE = 0x17,
    holy_VGA_CR_LINE_COMPARE = 0x18,
  };

enum
  {
    holy_VGA_CR_BYTE_PANNING_NORMAL = 0
  };

enum
  {
    holy_VGA_CR_UNDERLINE_LOCATION_DWORD_MODE = 0x40
  };

enum
  {
    holy_VGA_IO_MISC_COLOR = 0x01,
    holy_VGA_IO_MISC_ENABLE_VRAM_ACCESS = 0x02,
    holy_VGA_IO_MISC_28MHZ = 0x04,
    holy_VGA_IO_MISC_EXTERNAL_CLOCK_0 = 0x08,
    holy_VGA_IO_MISC_UPPER_64K = 0x20,
    holy_VGA_IO_MISC_NEGATIVE_HORIZ_POLARITY = 0x40,
    holy_VGA_IO_MISC_NEGATIVE_VERT_POLARITY = 0x80,
  };

enum
  {
    holy_VGA_ARX_MODE = 0x10,
    holy_VGA_ARX_OVERSCAN = 0x11,
    holy_VGA_ARX_COLOR_PLANE_ENABLE = 0x12,
    holy_VGA_ARX_HORIZONTAL_PANNING = 0x13,
    holy_VGA_ARX_COLOR_SELECT = 0x14
  };

enum
  {
    holy_VGA_ARX_MODE_TEXT = 0x00,
    holy_VGA_ARX_MODE_GRAPHICS = 0x01,
    holy_VGA_ARX_MODE_ENABLE_256COLOR = 0x40
  };

#define holy_VGA_CR_WIDTH_DIVISOR 8

#define holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END1_SHIFT 7
#define holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END1_MASK 0x02
#define holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END2_SHIFT 3
#define holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END2_MASK 0x40

#define holy_VGA_CR_OVERFLOW_VERT_TOTAL1_SHIFT 8
#define holy_VGA_CR_OVERFLOW_VERT_TOTAL1_MASK 0x01
#define holy_VGA_CR_OVERFLOW_VERT_TOTAL2_SHIFT 4
#define holy_VGA_CR_OVERFLOW_VERT_TOTAL2_MASK 0x20

#define holy_VGA_CR_OVERFLOW_VSYNC_START1_SHIFT 6
#define holy_VGA_CR_OVERFLOW_VSYNC_START1_MASK 0x04
#define holy_VGA_CR_OVERFLOW_VSYNC_START2_SHIFT 2
#define holy_VGA_CR_OVERFLOW_VSYNC_START2_MASK 0x80

#define holy_VGA_CR_OVERFLOW_HEIGHT1_SHIFT 7
#define holy_VGA_CR_OVERFLOW_HEIGHT1_MASK 0x02
#define holy_VGA_CR_OVERFLOW_HEIGHT2_SHIFT 3
#define holy_VGA_CR_OVERFLOW_HEIGHT2_MASK 0xc0
#define holy_VGA_CR_OVERFLOW_LINE_COMPARE_SHIFT 4
#define holy_VGA_CR_OVERFLOW_LINE_COMPARE_MASK 0x10

#define holy_VGA_CR_CELL_HEIGHT_LINE_COMPARE_MASK 0x40
#define holy_VGA_CR_CELL_HEIGHT_LINE_COMPARE_SHIFT 3
#define holy_VGA_CR_CELL_HEIGHT_VERTICAL_BLANK_MASK 0x20
#define holy_VGA_CR_CELL_HEIGHT_VERTICAL_BLANK_SHIFT 4
#define holy_VGA_CR_CELL_HEIGHT_DOUBLE_SCAN 0x80
enum
  {
    holy_VGA_CR_CURSOR_START_DISABLE = (1 << 5)
  };

#define holy_VGA_CR_PITCH_DIVISOR 8

enum
  {
    holy_VGA_CR_MODE_NO_CGA = 0x01,
    holy_VGA_CR_MODE_NO_HERCULES = 0x02,
    holy_VGA_CR_MODE_ADDRESS_WRAP = 0x20,
    holy_VGA_CR_MODE_BYTE_MODE = 0x40,
    holy_VGA_CR_MODE_TIMING_ENABLE = 0x80
  };

enum
  {
    holy_VGA_SR_RESET = 0,
    holy_VGA_SR_CLOCKING_MODE = 1,
    holy_VGA_SR_MAP_MASK_REGISTER = 2,
    holy_VGA_SR_CHAR_MAP_SELECT = 3,
    holy_VGA_SR_MEMORY_MODE = 4,
  };

enum
  {
    holy_VGA_SR_RESET_ASYNC = 1,
    holy_VGA_SR_RESET_SYNC = 2
  };

enum
  {
    holy_VGA_SR_CLOCKING_MODE_8_DOT_CLOCK = 1
  };

enum
  {
    holy_VGA_SR_MEMORY_MODE_NORMAL = 0,
    holy_VGA_SR_MEMORY_MODE_EXTERNAL_VIDEO_MEMORY = 2,
    holy_VGA_SR_MEMORY_MODE_SEQUENTIAL_ADDRESSING = 4,
    holy_VGA_SR_MEMORY_MODE_CHAIN4 = 8,
  };

enum
  {
    holy_VGA_GR_SET_RESET_PLANE = 0,
    holy_VGA_GR_SET_RESET_PLANE_ENABLE = 1,
    holy_VGA_GR_COLOR_COMPARE = 2,
    holy_VGA_GR_DATA_ROTATE = 3,
    holy_VGA_GR_READ_MAP_REGISTER = 4,
    holy_VGA_GR_MODE = 5,
    holy_VGA_GR_GR6 = 6,
    holy_VGA_GR_COLOR_COMPARE_DISABLE = 7,
    holy_VGA_GR_BITMASK = 8,
    holy_VGA_GR_MAX
  };

#define holy_VGA_ALL_PLANES 0xf
#define holy_VGA_NO_PLANES 0x0

enum
  {
    holy_VGA_GR_DATA_ROTATE_NOP = 0
  };

enum
  {
    holy_VGA_TEXT_TEXT_PLANE = 0,
    holy_VGA_TEXT_ATTR_PLANE = 1,
    holy_VGA_TEXT_FONT_PLANE = 2
  };

enum
  {
    holy_VGA_GR_GR6_GRAPHICS_MODE = 1,
    holy_VGA_GR_GR6_MMAP_A0 = (1 << 2),
    holy_VGA_GR_GR6_MMAP_CGA = (3 << 2)
  };

enum
  {
    holy_VGA_GR_MODE_READ_MODE1 = 0x08,
    holy_VGA_GR_MODE_ODD_EVEN = 0x10,
    holy_VGA_GR_MODE_ODD_EVEN_SHIFT = 0x20,
    holy_VGA_GR_MODE_256_COLOR = 0x40
  };

struct holy_video_hw_config
{
  unsigned vertical_total;
  unsigned vertical_blank_start;
  unsigned vertical_blank_end;
  unsigned vertical_sync_start;
  unsigned vertical_sync_end;
  unsigned line_compare;
  unsigned vdisplay_end;
  unsigned pitch;
  unsigned horizontal_total;
  unsigned horizontal_blank_start;
  unsigned horizontal_blank_end;
  unsigned horizontal_sync_pulse_start;
  unsigned horizontal_sync_pulse_end;
  unsigned horizontal_end;
};

static inline void
holy_vga_set_geometry (struct holy_video_hw_config *config,
		       void (*cr_write) (holy_uint8_t val, holy_uint8_t addr))
{
  unsigned vertical_total = config->vertical_total - 2;
  unsigned vertical_blank_start = config->vertical_blank_start - 1;
  unsigned vdisplay_end = config->vdisplay_end - 1;
  holy_uint8_t overflow, cell_height_reg;

  /* Disable CR0-7 write protection.  */
  cr_write (0, holy_VGA_CR_VSYNC_END);

  overflow = ((vertical_total >> holy_VGA_CR_OVERFLOW_VERT_TOTAL1_SHIFT)
	      & holy_VGA_CR_OVERFLOW_VERT_TOTAL1_MASK)
    | ((vertical_total >> holy_VGA_CR_OVERFLOW_VERT_TOTAL2_SHIFT)
       & holy_VGA_CR_OVERFLOW_VERT_TOTAL2_MASK)
    | ((config->vertical_sync_start >> holy_VGA_CR_OVERFLOW_VSYNC_START2_SHIFT)
       & holy_VGA_CR_OVERFLOW_VSYNC_START2_MASK)
    | ((config->vertical_sync_start >> holy_VGA_CR_OVERFLOW_VSYNC_START1_SHIFT)
       & holy_VGA_CR_OVERFLOW_VSYNC_START1_MASK)
    | ((vdisplay_end >> holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END1_SHIFT)
       & holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END1_MASK)
    | ((vdisplay_end >> holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END2_SHIFT)
       & holy_VGA_CR_OVERFLOW_VERT_DISPLAY_ENABLE_END2_MASK)
    | ((config->vertical_sync_start >> holy_VGA_CR_OVERFLOW_VSYNC_START1_SHIFT)
       & holy_VGA_CR_OVERFLOW_VSYNC_START1_MASK)
    | ((config->line_compare >> holy_VGA_CR_OVERFLOW_LINE_COMPARE_SHIFT)
       & holy_VGA_CR_OVERFLOW_LINE_COMPARE_MASK);

  cell_height_reg = ((vertical_blank_start
		      >> holy_VGA_CR_CELL_HEIGHT_VERTICAL_BLANK_SHIFT)
		     & holy_VGA_CR_CELL_HEIGHT_VERTICAL_BLANK_MASK)
    | ((config->line_compare >> holy_VGA_CR_CELL_HEIGHT_LINE_COMPARE_SHIFT)
       & holy_VGA_CR_CELL_HEIGHT_LINE_COMPARE_MASK);

  cr_write (config->horizontal_total - 1, holy_VGA_CR_HTOTAL);
  cr_write (config->horizontal_end - 1, holy_VGA_CR_HORIZ_END);
  cr_write (config->horizontal_blank_start - 1, holy_VGA_CR_HBLANK_START);
  cr_write (config->horizontal_blank_end, holy_VGA_CR_HBLANK_END);
  cr_write (config->horizontal_sync_pulse_start,
	    holy_VGA_CR_HORIZ_SYNC_PULSE_START);
  cr_write (config->horizontal_sync_pulse_end,
	    holy_VGA_CR_HORIZ_SYNC_PULSE_END);
  cr_write (vertical_total & 0xff, holy_VGA_CR_VERT_TOTAL);
  cr_write (overflow, holy_VGA_CR_OVERFLOW);
  cr_write (cell_height_reg, holy_VGA_CR_CELL_HEIGHT);
  cr_write (config->vertical_sync_start & 0xff, holy_VGA_CR_VSYNC_START);
  cr_write (config->vertical_sync_end & 0x0f, holy_VGA_CR_VSYNC_END);
  cr_write (vdisplay_end & 0xff, holy_VGA_CR_VDISPLAY_END);
  cr_write (config->pitch & 0xff, holy_VGA_CR_PITCH);
  cr_write (vertical_blank_start & 0xff, holy_VGA_CR_VERTICAL_BLANK_START);
  cr_write (config->vertical_blank_end & 0xff, holy_VGA_CR_VERTICAL_BLANK_END);
  cr_write (config->line_compare & 0xff, holy_VGA_CR_LINE_COMPARE);
}

#endif

#endif
