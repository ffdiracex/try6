/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_video_render_target holy_video_fbrender_target

#if !defined (TEST) && !defined(GENINIT)
#include <holy/err.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/pci.h>
#include <holy/vga.h>
#include <holy/cache.h>
#else
typedef unsigned char holy_uint8_t;
typedef unsigned short holy_uint16_t;
typedef unsigned int holy_uint32_t;
typedef int holy_err_t;
#include <holy/vgaregs.h>
#include <stdio.h>
#define ARRAY_SIZE(array) (sizeof (array) / sizeof (array[0]))
#endif

#include "sm712_init.c"

#pragma GCC diagnostic ignored "-Wcast-align"

#define holy_SM712_TOTAL_MEMORY_SPACE  0x700400
#define holy_SM712_REG_BASE 0x700000
#define holy_SM712_PCIID 0x0712126f

enum
  {
    holy_SM712_SR_TV_CONTROL = 0x65,
    holy_SM712_SR_RAM_LUT = 0x66,
    holy_SM712_SR_CLOCK_CONTROL1 = 0x68,
    holy_SM712_SR_CLOCK_CONTROL2 = 0x69,
    holy_SM712_SR_VCLK_NUM = 0x6c,
    holy_SM712_SR_VCLK_DENOM = 0x6d,
    holy_SM712_SR_VCLK2_NUM = 0x6e,
    holy_SM712_SR_VCLK2_DENOM = 0x6f,
    holy_SM712_SR_POPUP_ICON_LOW = 0x80,
    holy_SM712_SR_POPUP_ICON_HIGH = 0x81,
    holy_SM712_SR_POPUP_ICON_CTRL = 0x82,
    holy_SM712_SR_POPUP_ICON_COLOR1 = 0x84,
    holy_SM712_SR_POPUP_ICON_COLOR2 = 0x85,
    holy_SM712_SR_POPUP_ICON_COLOR3 = 0x86,

    holy_SM712_SR_HW_CURSOR_UPPER_LEFT_X_LOW = 0x88,
    holy_SM712_SR_HW_CURSOR_UPPER_LEFT_X_HIGH = 0x89,
    holy_SM712_SR_HW_CURSOR_UPPER_LEFT_Y_LOW = 0x8a,
    holy_SM712_SR_HW_CURSOR_UPPER_LEFT_Y_HIGH = 0x8b,
    holy_SM712_SR_HW_CURSOR_FG_COLOR = 0x8c,
    holy_SM712_SR_HW_CURSOR_BG_COLOR = 0x8d,

    holy_SM712_SR_POPUP_ICON_X_LOW = 0x90,
    holy_SM712_SR_POPUP_ICON_X_HIGH = 0x91,
    holy_SM712_SR_POPUP_ICON_Y_LOW = 0x92,
    holy_SM712_SR_POPUP_ICON_Y_HIGH = 0x93,
    holy_SM712_SR_PANEL_HW_VIDEO_CONTROL = 0xa0,
    holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_LOW = 0xa1,
    holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_HIGH = 0xa2,
    holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_MASK_LOW = 0xa3,
    holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_MASK_HIGH = 0xa4,
    holy_SM712_SR_PANEL_HW_VIDEO_RED_CONSTANT = 0xa5,
    holy_SM712_SR_PANEL_HW_VIDEO_GREEN_CONSTANT = 0xa6,
    holy_SM712_SR_PANEL_HW_VIDEO_BLUE_CONSTANT = 0xa7,
    holy_SM712_SR_PANEL_HW_VIDEO_TOP_BOUNDARY = 0xa8,
    holy_SM712_SR_PANEL_HW_VIDEO_LEFT_BOUNDARY = 0xa9,
    holy_SM712_SR_PANEL_HW_VIDEO_BOTTOM_BOUNDARY = 0xaa,
    holy_SM712_SR_PANEL_HW_VIDEO_RIGHT_BOUNDARY = 0xab,
    holy_SM712_SR_PANEL_HW_VIDEO_TOP_LEFT_OVERFLOW_BOUNDARY = 0xac,
    holy_SM712_SR_PANEL_HW_VIDEO_BOTTOM_RIGHT_OVERFLOW_BOUNDARY = 0xad,
    holy_SM712_SR_PANEL_HW_VIDEO_VERTICAL_STRETCH_FACTOR = 0xae,
    holy_SM712_SR_PANEL_HW_VIDEO_HORIZONTAL_STRETCH_FACTOR = 0xaf,
  };
enum
  {
    holy_SM712_SR_TV_CRT_SRAM = 0x00,
    holy_SM712_SR_TV_LCD_SRAM = 0x08
  };
enum
  {
    holy_SM712_SR_TV_ALT_CLOCK = 0x00,
    holy_SM712_SR_TV_FREE_RUN_CLOCK = 0x04
  };
enum
  {
    holy_SM712_SR_TV_CLOCK_CKIN_NTSC = 0x00,
    holy_SM712_SR_TV_CLOCK_REFCLK_PAL = 0x04
  };
enum
  {
    holy_SM712_SR_TV_HSYNC = 0x00,
    holy_SM712_SR_TV_COMPOSITE_HSYNC = 0x01
  };
enum
  {
    holy_SM712_SR_RAM_LUT_NORMAL = 0,
    holy_SM712_SR_RAM_LUT_LCD_RAM_OFF = 0x80,
    holy_SM712_SR_RAM_LUT_CRT_RAM_OFF = 0x40,
    holy_SM712_SR_RAM_LUT_LCD_RAM_NO_WRITE = 0x20,
    holy_SM712_SR_RAM_LUT_CRT_RAM_NO_WRITE = 0x10,
    holy_SM712_SR_RAM_LUT_CRT_8BIT = 0x08,
    holy_SM712_SR_RAM_LUT_CRT_GAMMA = 0x04
  };

enum
  {
    holy_SM712_SR_CLOCK_CONTROL1_VCLK_FROM_CCR = 0x40,
    holy_SM712_SR_CLOCK_CONTROL1_8DOT_CLOCK = 0x10,
  };

enum
  {
    holy_SM712_SR_CLOCK_CONTROL2_PROGRAM_VCLOCK = 0x03
  };

#define holy_SM712_SR_POPUP_ICON_HIGH_MASK 0x7
#define holy_SM712_SR_POPUP_ICON_HIGH_HW_CURSOR_EN 0x80
  enum
  {
    holy_SM712_SR_POPUP_ICON_CTRL_DISABLED = 0,
    holy_SM712_SR_POPUP_ICON_CTRL_ZOOM_ENABLED = 0x40,
    holy_SM712_SR_POPUP_ICON_CTRL_ENABLED = 0x80
  };
#define RGB332_BLACK 0
#define RGB332_WHITE 0xff

  enum
  {
    holy_SM712_CR_OVERFLOW_INTERLACE = 0x30,
    holy_SM712_CR_INTERLACE_RETRACE = 0x31,
    holy_SM712_CR_TV_VDISPLAY_START = 0x32,
    holy_SM712_CR_TV_VDISPLAY_END_HIGH = 0x33,
    holy_SM712_CR_TV_VDISPLAY_END_LOW = 0x34,
    holy_SM712_CR_DDA_CONTROL_LOW = 0x35,
    holy_SM712_CR_DDA_CONTROL_HIGH = 0x36,
    holy_SM712_CR_TV_EQUALIZER = 0x38,
    holy_SM712_CR_TV_SERRATION = 0x39,
    holy_SM712_CR_HSYNC_CTRL = 0x3a,
    holy_SM712_CR_DEBUG = 0x3c,
    holy_SM712_CR_SHADOW_VGA_HTOTAL = 0x40,
    holy_SM712_CR_SHADOW_VGA_HBLANK_START = 0x41,
    holy_SM712_CR_SHADOW_VGA_HBLANK_END = 0x42,
    holy_SM712_CR_SHADOW_VGA_HRETRACE_START = 0x43,
    holy_SM712_CR_SHADOW_VGA_HRETRACE_END = 0x44,
    holy_SM712_CR_SHADOW_VGA_VERTICAL_TOTAL = 0x45,
    holy_SM712_CR_SHADOW_VGA_VBLANK_START = 0x46,
    holy_SM712_CR_SHADOW_VGA_VBLANK_END = 0x47,
    holy_SM712_CR_SHADOW_VGA_VRETRACE_START = 0x48,
    holy_SM712_CR_SHADOW_VGA_VRETRACE_END = 0x49,
    holy_SM712_CR_SHADOW_VGA_OVERFLOW = 0x4a,
    holy_SM712_CR_SHADOW_VGA_CELL_HEIGHT = 0x4b,
    holy_SM712_CR_SHADOW_VGA_HDISPLAY_END = 0x4c,
    holy_SM712_CR_SHADOW_VGA_VDISPLAY_END = 0x4d,
    holy_SM712_CR_DDA_LOOKUP_REG3_START = 0x90,
    holy_SM712_CR_DDA_LOOKUP_REG2_START = 0x91,
    holy_SM712_CR_DDA_LOOKUP_REG1_START = 0xa0,
    holy_SM712_CR_VCENTERING_OFFSET = 0xa6,
    holy_SM712_CR_HCENTERING_OFFSET = 0xa7,
  };

#define holy_SM712_CR_DEBUG_NONE 0

#define SM712_DDA_REG3_COMPARE_SHIFT 2
#define SM712_DDA_REG3_COMPARE_MASK  0xfc
#define SM712_DDA_REG3_DDA_SHIFT 8
#define SM712_DDA_REG3_DDA_MASK  0x3
#define SM712_DDA_REG2_DDA_MASK  0xff
#define SM712_DDA_REG2_VCENTER_MASK 0x3f

static struct
{
  holy_uint8_t compare;
  holy_uint16_t dda;
  holy_uint8_t vcentering;
} dda_lookups[] = {
  { 21, 469,  2},
  { 23, 477,  2},
  { 33, 535,  2},
  { 35, 682, 21},
  { 34, 675,  2},
  { 55, 683,  6},
};

static struct
{
#if !defined (TEST) && !defined(GENINIT)
  struct holy_video_mode_info mode_info;
#endif

  volatile holy_uint8_t *ptr;
  holy_uint8_t *cached_ptr;
  int mapped;
  holy_uint32_t base;
#if !defined (TEST) && !defined(GENINIT)
  holy_pci_device_t dev;
#endif
} framebuffer;

#if !defined (TEST) && !defined(GENINIT)
static holy_err_t
holy_video_sm712_video_init (void)
{
  /* Reset frame buffer.  */
  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_sm712_video_fini (void)
{
  if (framebuffer.mapped)
    {
      holy_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				   holy_SM712_TOTAL_MEMORY_SPACE);
      holy_pci_device_unmap_range (framebuffer.dev, framebuffer.cached_ptr,
				   holy_SM712_TOTAL_MEMORY_SPACE);
    }
  return holy_video_fb_fini ();
}
#endif

static inline void
holy_sm712_write_reg (holy_uint8_t val, holy_uint16_t addr)
{
#ifdef TEST
  printf ("  {1, 0x%x, 0x%x},\n", addr, val);
#elif defined (GENINIT)
  printf (" .byte 0x%02x, 0x%02x\n", (addr - 0x3c0), val);
  if ((addr - 0x3c0) & ~0x7f)
    printf ("FAIL\n");
#else
   *(volatile holy_uint8_t *) (framebuffer.ptr + holy_SM712_REG_BASE
			       + addr) = val;
#endif
}

static inline holy_uint8_t
holy_sm712_read_reg (holy_uint16_t addr)
{
#ifdef TEST
  printf ("  {-1, 0x%x, 0x5},\n", addr);
#elif defined (GENINIT)
  if ((addr - 0x3c0) & ~0x7f)
    printf ("FAIL\n");
  printf ("  .byte 0x%04x, 0x00\n", (addr - 0x3c0) | 0x80);
#else
  return *(volatile holy_uint8_t *) (framebuffer.ptr + holy_SM712_REG_BASE
				     + addr);
#endif
}

static inline holy_uint8_t
holy_sm712_sr_read (holy_uint8_t addr)
{
  holy_sm712_write_reg (addr, holy_VGA_IO_SR_INDEX);
  return holy_sm712_read_reg (holy_VGA_IO_SR_DATA);
}

static inline void
holy_sm712_sr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_sm712_write_reg (addr, holy_VGA_IO_SR_INDEX);
  holy_sm712_write_reg (val, holy_VGA_IO_SR_DATA);
}

static inline void
holy_sm712_gr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_sm712_write_reg (addr, holy_VGA_IO_GR_INDEX);
  holy_sm712_write_reg (val, holy_VGA_IO_GR_DATA);
}

static inline void
holy_sm712_cr_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_sm712_write_reg (addr, holy_VGA_IO_CR_INDEX);
  holy_sm712_write_reg (val, holy_VGA_IO_CR_DATA);
}

static inline void
holy_sm712_write_arx (holy_uint8_t val, holy_uint8_t addr)
{
  holy_sm712_read_reg (holy_VGA_IO_INPUT_STATUS1_REGISTER);
  holy_sm712_write_reg (addr, holy_VGA_IO_ARX);
  holy_sm712_read_reg (holy_VGA_IO_ARX_READ);
  holy_sm712_write_reg (val, holy_VGA_IO_ARX);
}

static inline void
holy_sm712_cr_shadow_write (holy_uint8_t val, holy_uint8_t addr)
{
  holy_uint8_t mapping[] =
    {
      [holy_VGA_CR_HTOTAL] = holy_SM712_CR_SHADOW_VGA_HTOTAL,
      [holy_VGA_CR_HORIZ_END] = 0xff,
      [holy_VGA_CR_HBLANK_START] = holy_SM712_CR_SHADOW_VGA_HBLANK_START,
      [holy_VGA_CR_HBLANK_END] = holy_SM712_CR_SHADOW_VGA_HBLANK_END,
      [holy_VGA_CR_HORIZ_SYNC_PULSE_START] = holy_SM712_CR_SHADOW_VGA_HRETRACE_START,
      [holy_VGA_CR_HORIZ_SYNC_PULSE_END] = holy_SM712_CR_SHADOW_VGA_HRETRACE_END,
      [holy_VGA_CR_VERT_TOTAL] = holy_SM712_CR_SHADOW_VGA_VERTICAL_TOTAL,
      [holy_VGA_CR_OVERFLOW] = holy_SM712_CR_SHADOW_VGA_OVERFLOW,
      [holy_VGA_CR_BYTE_PANNING] = 0xff,
      [holy_VGA_CR_CELL_HEIGHT] = holy_SM712_CR_SHADOW_VGA_CELL_HEIGHT,
      [holy_VGA_CR_CURSOR_START] = 0xff,
      [holy_VGA_CR_CURSOR_END] = 0xff,
      [holy_VGA_CR_START_ADDR_HIGH_REGISTER] = 0xff,
      [holy_VGA_CR_START_ADDR_LOW_REGISTER] = 0xff,
      [holy_VGA_CR_CURSOR_ADDR_HIGH] = 0xff,
      [holy_VGA_CR_CURSOR_ADDR_LOW] = 0xff,
      [holy_VGA_CR_VSYNC_START] = holy_SM712_CR_SHADOW_VGA_VRETRACE_START,
      [holy_VGA_CR_VSYNC_END] = holy_SM712_CR_SHADOW_VGA_VRETRACE_END,
      [holy_VGA_CR_VDISPLAY_END] = holy_SM712_CR_SHADOW_VGA_VDISPLAY_END,
      [holy_VGA_CR_PITCH] = holy_SM712_CR_SHADOW_VGA_HDISPLAY_END,
      [holy_VGA_CR_UNDERLINE_LOCATION] = 0xff,

      [holy_VGA_CR_VERTICAL_BLANK_START] = holy_SM712_CR_SHADOW_VGA_VBLANK_START,
      [holy_VGA_CR_VERTICAL_BLANK_END] = holy_SM712_CR_SHADOW_VGA_VBLANK_END,
      [holy_VGA_CR_MODE] = 0xff,
      [holy_VGA_CR_LINE_COMPARE] = 0xff
    };
  if (addr >= ARRAY_SIZE (mapping) || mapping[addr] == 0xff)
    return;
  holy_sm712_cr_write (val, mapping[addr]);
}

static inline void
holy_sm712_write_dda_lookup (int idx, holy_uint8_t compare, holy_uint16_t dda,
			     holy_uint8_t vcentering)
{
  holy_sm712_cr_write (((compare << SM712_DDA_REG3_COMPARE_SHIFT)
			& SM712_DDA_REG3_COMPARE_MASK)
		       | ((dda >> SM712_DDA_REG3_DDA_SHIFT)
			  & SM712_DDA_REG3_DDA_MASK),
		       holy_SM712_CR_DDA_LOOKUP_REG3_START + 2 * idx);
  holy_sm712_cr_write (dda & SM712_DDA_REG2_DDA_MASK,
		       holy_SM712_CR_DDA_LOOKUP_REG2_START + 2 * idx);
  holy_sm712_cr_write (vcentering & SM712_DDA_REG2_VCENTER_MASK,
		       holy_SM712_CR_DDA_LOOKUP_REG1_START + idx);
}

#if !defined (TEST) && !defined(GENINIT)
/* Helper for holy_video_sm712_setup.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  int *found = data;
  holy_pci_address_t addr;
  holy_uint32_t class;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  if (((class >> 16) & 0xffff) != holy_PCI_CLASS_SUBCLASS_VGA
      || pciid != holy_SM712_PCIID)
    return 0;
  
  *found = 1;

  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
  framebuffer.base = holy_pci_read (addr);
  framebuffer.dev = dev;

  return 1;
}
#endif

static holy_err_t
holy_video_sm712_setup (unsigned int width, unsigned int height,
			unsigned int mode_type, unsigned int mode_mask __attribute__ ((unused)))
{
  unsigned i;
#if !defined (TEST) && !defined(GENINIT)
  int depth;
  holy_err_t err;
  int found = 0;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if ((width != 1024 && width != 0) || (height != 600 && height != 0)
      || (depth != 16 && depth != 0))
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "Only 1024x600x16 is supported");

  holy_pci_iterate (find_card, &found);
  if (!found)
    return holy_error (holy_ERR_IO, "Couldn't find graphics card");
  /* Fill mode info details.  */
  framebuffer.mode_info.width = 1024;
  framebuffer.mode_info.height = 600;
  framebuffer.mode_info.mode_type = (holy_VIDEO_MODE_TYPE_RGB
				     | holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
				     | holy_VIDEO_MODE_TYPE_UPDATING_SWAP);
  framebuffer.mode_info.bpp = 16;
  framebuffer.mode_info.bytes_per_pixel = 2;
  framebuffer.mode_info.pitch = 1024 * 2;
  framebuffer.mode_info.number_of_colors = 256;
  framebuffer.mode_info.red_mask_size = 5;
  framebuffer.mode_info.red_field_pos = 11;
  framebuffer.mode_info.green_mask_size = 6;
  framebuffer.mode_info.green_field_pos = 5;
  framebuffer.mode_info.blue_mask_size = 5;
  framebuffer.mode_info.blue_field_pos = 0;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;
  framebuffer.mode_info.blit_format
    = holy_video_get_blit_format (&framebuffer.mode_info);
#endif

#if !defined (TEST) && !defined(GENINIT)
  if (found && framebuffer.base == 0)
    {
      holy_pci_address_t addr;
      /* FIXME: choose address dynamically if needed.   */
      framebuffer.base = 0x04000000;

      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_ADDRESS_REG0);
      holy_pci_write (addr, framebuffer.base);

      /* Set latency.  */
      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_CACHELINE);
      holy_pci_write (addr, 0x8);

      /* Enable address spaces.  */
      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_COMMAND);
      holy_pci_write (addr, 0x7);
    }
#endif

  /* We can safely discard volatile attribute.  */
#if !defined (TEST) && !defined(GENINIT)
  framebuffer.ptr
    = holy_pci_device_map_range (framebuffer.dev,
				 framebuffer.base,
				 holy_SM712_TOTAL_MEMORY_SPACE);
  framebuffer.cached_ptr
    = holy_pci_device_map_range_cached (framebuffer.dev,
					framebuffer.base,
					holy_SM712_TOTAL_MEMORY_SPACE);
#endif
  framebuffer.mapped = 1;

  /* Initialise SM712.  */
#if !defined (TEST) && !defined(GENINIT)
  /* FIXME */
  holy_vga_sr_write (0x11, 0x18);
#endif

#if !defined (TEST) && !defined(GENINIT)
  /* Prevent garbage from appearing on the screen.  */
  holy_memset ((void *) framebuffer.cached_ptr, 0,
	       framebuffer.mode_info.height * framebuffer.mode_info.pitch);
#endif

  /* FIXME */
  holy_sm712_sr_write (0, 0x21);
  holy_sm712_sr_write (0x7a, 0x62);
  holy_sm712_sr_write (0x16, 0x6a);
  holy_sm712_sr_write (0x2, 0x6b);
  holy_sm712_write_reg (0, holy_VGA_IO_PIXEL_MASK);
  holy_sm712_sr_write (holy_VGA_SR_RESET_ASYNC, holy_VGA_SR_RESET);
  holy_sm712_write_reg (holy_VGA_IO_MISC_NEGATIVE_VERT_POLARITY
			| holy_VGA_IO_MISC_NEGATIVE_HORIZ_POLARITY
			| holy_VGA_IO_MISC_UPPER_64K
			| holy_VGA_IO_MISC_EXTERNAL_CLOCK_0
			| holy_VGA_IO_MISC_ENABLE_VRAM_ACCESS
			| holy_VGA_IO_MISC_COLOR, holy_VGA_IO_MISC_WRITE);
  holy_sm712_sr_write (holy_VGA_SR_RESET_ASYNC | holy_VGA_SR_RESET_SYNC,
		       holy_VGA_SR_RESET);
  holy_sm712_sr_write (holy_VGA_SR_CLOCKING_MODE_8_DOT_CLOCK,
		       holy_VGA_SR_CLOCKING_MODE);
  holy_sm712_sr_write (holy_VGA_ALL_PLANES, holy_VGA_SR_MAP_MASK_REGISTER);
  holy_sm712_sr_write (0, holy_VGA_SR_CHAR_MAP_SELECT);
  holy_sm712_sr_write (holy_VGA_SR_MEMORY_MODE_CHAIN4
		       | holy_VGA_SR_MEMORY_MODE_SEQUENTIAL_ADDRESSING
		       | holy_VGA_SR_MEMORY_MODE_EXTERNAL_VIDEO_MEMORY,
		       holy_VGA_SR_MEMORY_MODE);

  for (i = 0; i < ARRAY_SIZE (sm712_sr_seq1); i++)
    holy_sm712_sr_write (sm712_sr_seq1[i], 0x10 + i);

  for (i = 0; i < ARRAY_SIZE (sm712_sr_seq2); i++)
    holy_sm712_sr_write (sm712_sr_seq2[i], 0x30 + i);

  /* Undocumented.  */
  holy_sm712_sr_write (0x1a, 0x63);
  /* Undocumented.  */
  holy_sm712_sr_write (0x1a, 0x64);

  holy_sm712_sr_write (holy_SM712_SR_TV_CRT_SRAM | holy_SM712_SR_TV_ALT_CLOCK
		       | holy_SM712_SR_TV_CLOCK_CKIN_NTSC
		       | holy_SM712_SR_TV_HSYNC,
		       holy_SM712_SR_TV_CONTROL);

  holy_sm712_sr_write (holy_SM712_SR_RAM_LUT_NORMAL, holy_SM712_SR_RAM_LUT);

  /* Undocumented.  */
  holy_sm712_sr_write (0x00, 0x67);

  holy_sm712_sr_write (holy_SM712_SR_CLOCK_CONTROL1_VCLK_FROM_CCR
		       | holy_SM712_SR_CLOCK_CONTROL1_8DOT_CLOCK,
		       holy_SM712_SR_CLOCK_CONTROL1);
  holy_sm712_sr_write (holy_SM712_SR_CLOCK_CONTROL2_PROGRAM_VCLOCK,
		       holy_SM712_SR_CLOCK_CONTROL2);

  holy_sm712_sr_write (82, holy_SM712_SR_VCLK_NUM);
  holy_sm712_sr_write (137, holy_SM712_SR_VCLK_DENOM);

  holy_sm712_sr_write (9, holy_SM712_SR_VCLK2_NUM);
  holy_sm712_sr_write (2, holy_SM712_SR_VCLK2_DENOM);
  /* FIXME */
  holy_sm712_sr_write (0x04, 0x70);
  /* FIXME */
  holy_sm712_sr_write (0x45, 0x71);
  /* Undocumented */
  holy_sm712_sr_write (0x30, 0x72);
  /* Undocumented */
  holy_sm712_sr_write (0x30, 0x73);
  /* Undocumented */
  holy_sm712_sr_write (0x40, 0x74);
  /* Undocumented */
  holy_sm712_sr_write (0x20, 0x75);

  holy_sm712_sr_write (0xff, holy_SM712_SR_POPUP_ICON_LOW);
  holy_sm712_sr_write (holy_SM712_SR_POPUP_ICON_HIGH_MASK,
		       holy_SM712_SR_POPUP_ICON_HIGH);
  holy_sm712_sr_write (holy_SM712_SR_POPUP_ICON_CTRL_DISABLED,
		       holy_SM712_SR_POPUP_ICON_CTRL);
  /* Undocumented */
  holy_sm712_sr_write (0x0, 0x83);

  holy_sm712_sr_write (8, holy_SM712_SR_POPUP_ICON_COLOR1);
  holy_sm712_sr_write (0, holy_SM712_SR_POPUP_ICON_COLOR2);
  holy_sm712_sr_write (0x42, holy_SM712_SR_POPUP_ICON_COLOR3);

  /* Undocumented */
  holy_sm712_sr_write (0x3a, 0x87);

  /* Why theese coordinates?  */
  holy_sm712_sr_write (0x59, holy_SM712_SR_HW_CURSOR_UPPER_LEFT_X_LOW);
  holy_sm712_sr_write (0x02, holy_SM712_SR_HW_CURSOR_UPPER_LEFT_X_HIGH);
  holy_sm712_sr_write (0x44, holy_SM712_SR_HW_CURSOR_UPPER_LEFT_Y_LOW);
  holy_sm712_sr_write (0x02, holy_SM712_SR_HW_CURSOR_UPPER_LEFT_Y_HIGH);

  holy_sm712_sr_write (RGB332_BLACK, holy_SM712_SR_HW_CURSOR_FG_COLOR);
  holy_sm712_sr_write (RGB332_WHITE, holy_SM712_SR_HW_CURSOR_BG_COLOR);

  /* Undocumented */
  holy_sm712_sr_write (0x3a, 0x8e);
  holy_sm712_sr_write (0x3a, 0x8f);

  holy_sm712_sr_write (0, holy_SM712_SR_POPUP_ICON_X_LOW);
  holy_sm712_sr_write (0, holy_SM712_SR_POPUP_ICON_X_HIGH);
  holy_sm712_sr_write (0, holy_SM712_SR_POPUP_ICON_Y_LOW);
  holy_sm712_sr_write (0, holy_SM712_SR_POPUP_ICON_Y_HIGH);

  holy_sm712_sr_write (0, holy_SM712_SR_PANEL_HW_VIDEO_CONTROL);
  holy_sm712_sr_write (0x10, holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_LOW);
  holy_sm712_sr_write (0x08, holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_HIGH);
  holy_sm712_sr_write (0x00, holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_MASK_LOW);
  holy_sm712_sr_write (0x02, holy_SM712_SR_PANEL_HW_VIDEO_COLOR_KEY_MASK_HIGH);
  holy_sm712_sr_write (0xed, holy_SM712_SR_PANEL_HW_VIDEO_RED_CONSTANT);
  holy_sm712_sr_write (0xed, holy_SM712_SR_PANEL_HW_VIDEO_GREEN_CONSTANT);
  holy_sm712_sr_write (0xed, holy_SM712_SR_PANEL_HW_VIDEO_BLUE_CONSTANT);

  holy_sm712_sr_write (0x7b, holy_SM712_SR_PANEL_HW_VIDEO_TOP_BOUNDARY);
  holy_sm712_sr_write (0xfb, holy_SM712_SR_PANEL_HW_VIDEO_LEFT_BOUNDARY);
  holy_sm712_sr_write (0xff, holy_SM712_SR_PANEL_HW_VIDEO_BOTTOM_BOUNDARY);
  holy_sm712_sr_write (0xff, holy_SM712_SR_PANEL_HW_VIDEO_RIGHT_BOUNDARY);
  /* Doesn't match documentation?  */
  holy_sm712_sr_write (0x97, holy_SM712_SR_PANEL_HW_VIDEO_TOP_LEFT_OVERFLOW_BOUNDARY);
  holy_sm712_sr_write (0xef, holy_SM712_SR_PANEL_HW_VIDEO_BOTTOM_RIGHT_OVERFLOW_BOUNDARY);

  holy_sm712_sr_write (0xbf, holy_SM712_SR_PANEL_HW_VIDEO_VERTICAL_STRETCH_FACTOR);
  holy_sm712_sr_write (0xdf, holy_SM712_SR_PANEL_HW_VIDEO_HORIZONTAL_STRETCH_FACTOR);

  holy_sm712_gr_write (holy_VGA_NO_PLANES, holy_VGA_GR_SET_RESET_PLANE);
  holy_sm712_gr_write (holy_VGA_NO_PLANES, holy_VGA_GR_SET_RESET_PLANE_ENABLE);
  holy_sm712_gr_write (holy_VGA_NO_PLANES, holy_VGA_GR_COLOR_COMPARE);
  holy_sm712_gr_write (holy_VGA_GR_DATA_ROTATE_NOP, holy_VGA_GR_DATA_ROTATE);
  holy_sm712_gr_write (holy_VGA_NO_PLANES, holy_VGA_GR_READ_MAP_REGISTER);
  holy_sm712_gr_write (holy_VGA_GR_MODE_256_COLOR, holy_VGA_GR_MODE);
  holy_sm712_gr_write (holy_VGA_GR_GR6_MMAP_A0
		       | holy_VGA_GR_GR6_GRAPHICS_MODE, holy_VGA_GR_GR6);
  holy_sm712_gr_write (holy_VGA_ALL_PLANES, holy_VGA_GR_COLOR_COMPARE_DISABLE);
  holy_sm712_gr_write (0xff, holy_VGA_GR_BITMASK);

  /* Write palette mapping.  */
  for (i = 0; i < 16; i++)
    holy_sm712_write_arx (i, i);

  holy_sm712_write_arx (holy_VGA_ARX_MODE_ENABLE_256COLOR
			| holy_VGA_ARX_MODE_GRAPHICS, holy_VGA_ARX_MODE);
  holy_sm712_write_arx (0, holy_VGA_ARX_OVERSCAN);
  holy_sm712_write_arx (holy_VGA_ALL_PLANES, holy_VGA_ARX_COLOR_PLANE_ENABLE);
  holy_sm712_write_arx (0, holy_VGA_ARX_HORIZONTAL_PANNING);
  holy_sm712_write_arx (0, holy_VGA_ARX_COLOR_SELECT);

  /* FIXME: compute this generically.  */
  {
    struct holy_video_hw_config config =
      {
	.vertical_total = 806,
	.vertical_blank_start = 0x300,
	.vertical_blank_end = 0,
	.vertical_sync_start = 0x303,
	.vertical_sync_end = 0x9,
	.line_compare = 0x3ff,
	.vdisplay_end = 0x300,
	.pitch = 0x80,
	.horizontal_total = 164,
	.horizontal_end = 128,
	.horizontal_blank_start = 128,
	.horizontal_blank_end = 0,
	.horizontal_sync_pulse_start = 133,
	.horizontal_sync_pulse_end = 22
      };
    holy_vga_set_geometry (&config, holy_sm712_cr_write);
    config.horizontal_sync_pulse_start = 134;
    config.horizontal_sync_pulse_end = 21;
    config.vertical_sync_start = 0x301;
    config.vertical_sync_end = 0x0;
    config.line_compare = 0x0ff;
    config.vdisplay_end = 0x258;
    config.pitch = 0x7f;
    holy_vga_set_geometry (&config, holy_sm712_cr_shadow_write);
  }

  holy_sm712_cr_write (holy_VGA_CR_BYTE_PANNING_NORMAL,
		       holy_VGA_CR_BYTE_PANNING);
  holy_sm712_cr_write (0, holy_VGA_CR_CURSOR_START);
  holy_sm712_cr_write (0, holy_VGA_CR_CURSOR_END);
  holy_sm712_cr_write (0, holy_VGA_CR_START_ADDR_HIGH_REGISTER);
  holy_sm712_cr_write (0, holy_VGA_CR_START_ADDR_LOW_REGISTER);
  holy_sm712_cr_write (0, holy_VGA_CR_CURSOR_ADDR_HIGH);
  holy_sm712_cr_write (0, holy_VGA_CR_CURSOR_ADDR_LOW);
  holy_sm712_cr_write (holy_VGA_CR_UNDERLINE_LOCATION_DWORD_MODE,
		       holy_VGA_CR_UNDERLINE_LOCATION);
  holy_sm712_cr_write (holy_VGA_CR_MODE_ADDRESS_WRAP
		       | holy_VGA_CR_MODE_BYTE_MODE
		       | holy_VGA_CR_MODE_TIMING_ENABLE
		       | holy_VGA_CR_MODE_NO_CGA
		       | holy_VGA_CR_MODE_NO_HERCULES,
		       holy_VGA_CR_MODE);

  holy_sm712_cr_write (0, holy_SM712_CR_OVERFLOW_INTERLACE);
  holy_sm712_cr_write (0, holy_SM712_CR_INTERLACE_RETRACE);
  holy_sm712_cr_write (0, holy_SM712_CR_TV_VDISPLAY_START);
  holy_sm712_cr_write (0, holy_SM712_CR_TV_VDISPLAY_END_HIGH);
  holy_sm712_cr_write (0, holy_SM712_CR_TV_VDISPLAY_END_LOW);
  holy_sm712_cr_write (0x80, holy_SM712_CR_DDA_CONTROL_LOW);
  holy_sm712_cr_write (0x02, holy_SM712_CR_DDA_CONTROL_HIGH);

  /* Undocumented */
  holy_sm712_cr_write (0x20, 0x37);

  holy_sm712_cr_write (0, holy_SM712_CR_TV_EQUALIZER);
  holy_sm712_cr_write (0, holy_SM712_CR_TV_SERRATION);
  holy_sm712_cr_write (0, holy_SM712_CR_HSYNC_CTRL);

  /* Undocumented */
  holy_sm712_cr_write (0x40, 0x3b);

  holy_sm712_cr_write (holy_SM712_CR_DEBUG_NONE, holy_SM712_CR_DEBUG);

  /* Undocumented */
  holy_sm712_cr_write (0xff, 0x3d);
  holy_sm712_cr_write (0x46, 0x3e);
  holy_sm712_cr_write (0x91, 0x3f);

  for (i = 0; i < ARRAY_SIZE (dda_lookups); i++)
    holy_sm712_write_dda_lookup (i, dda_lookups[i].compare, dda_lookups[i].dda,
				 dda_lookups[i].vcentering);
  
  /* Undocumented  */
  holy_sm712_cr_write (0, 0x9c);
  holy_sm712_cr_write (0, 0x9d);
  holy_sm712_cr_write (0, 0x9e);
  holy_sm712_cr_write (0, 0x9f);

  holy_sm712_cr_write (0, holy_SM712_CR_VCENTERING_OFFSET);
  holy_sm712_cr_write (0, holy_SM712_CR_HCENTERING_OFFSET);

  holy_sm712_write_reg (holy_VGA_IO_MISC_NEGATIVE_HORIZ_POLARITY
			| holy_VGA_IO_MISC_UPPER_64K
			| holy_VGA_IO_MISC_28MHZ
			| holy_VGA_IO_MISC_ENABLE_VRAM_ACCESS
			| holy_VGA_IO_MISC_COLOR,
			holy_VGA_IO_MISC_WRITE);

#if !defined (TEST) && !defined(GENINIT)
  /* Undocumented? */
  *(volatile holy_uint32_t *) ((char *) framebuffer.ptr + 0x40c00c) = 0;
  *(volatile holy_uint32_t *) ((char *) framebuffer.ptr + 0x40c040) = 0;
  *(volatile holy_uint32_t *) ((char *) framebuffer.ptr + 0x40c000) = 0x20000;
  *(volatile holy_uint32_t *) ((char *) framebuffer.ptr + 0x40c010) = 0x1020100;
#endif

  (void) holy_sm712_sr_read (0x16);

#if !defined (TEST) && !defined(GENINIT)
  err = holy_video_fb_setup (mode_type, mode_mask,
			     &framebuffer.mode_info,
			     framebuffer.cached_ptr, NULL, NULL);
  if (err)
    return err;

  /* Copy default palette to initialize emulated palette.  */
  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				   holy_video_fbstd_colors);
  return err;
#else
  return 0;
#endif
}

#if !defined (TEST) && !defined(GENINIT)

static holy_err_t
holy_video_sm712_swap_buffers (void)
{
  holy_size_t s;
  s = (framebuffer.mode_info.height
       * framebuffer.mode_info.pitch
       * framebuffer.mode_info.bytes_per_pixel);
  holy_video_fb_swap_buffers ();
  holy_arch_sync_dma_caches (framebuffer.cached_ptr, s);
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_sm712_get_info_and_fini (struct holy_video_mode_info *mode_info,
				    void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_sm712_adapter =
  {
    .name = "SM712 Video Driver",
    .id = holy_VIDEO_DRIVER_SM712,

    .prio = holy_VIDEO_ADAPTER_PRIO_NATIVE,

    .init = holy_video_sm712_video_init,
    .fini = holy_video_sm712_video_fini,
    .setup = holy_video_sm712_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_sm712_get_info_and_fini,
    .set_palette = holy_video_fb_set_palette,
    .get_palette = holy_video_fb_get_palette,
    .set_viewport = holy_video_fb_set_viewport,
    .get_viewport = holy_video_fb_get_viewport,
    .set_region = holy_video_fb_set_region,
    .get_region = holy_video_fb_get_region,
    .set_area_status = holy_video_fb_set_area_status,
    .get_area_status = holy_video_fb_get_area_status,
    .map_color = holy_video_fb_map_color,
    .map_rgb = holy_video_fb_map_rgb,
    .map_rgba = holy_video_fb_map_rgba,
    .unmap_color = holy_video_fb_unmap_color,
    .fill_rect = holy_video_fb_fill_rect,
    .blit_bitmap = holy_video_fb_blit_bitmap,
    .blit_render_target = holy_video_fb_blit_render_target,
    .scroll = holy_video_fb_scroll,
    .swap_buffers = holy_video_sm712_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_fb_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(video_sm712)
{
  holy_video_register (&holy_video_sm712_adapter);
}

holy_MOD_FINI(video_sm712)
{
  holy_video_unregister (&holy_video_sm712_adapter);
}
#else
int
main ()
{
  holy_video_sm712_setup (1024, 600, 0, 0);
}
#endif
