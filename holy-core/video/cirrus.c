/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_video_render_target holy_video_fbrender_target

#include <holy/err.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/pci.h>
#include <holy/vga.h>

holy_MOD_LICENSE ("GPLv2+");

static struct
{
  struct holy_video_mode_info mode_info;
  holy_size_t page_size;        /* The size of a page in bytes.  */

  holy_uint8_t *ptr;
  int mapped;
  holy_uint32_t base;
  holy_pci_device_t dev;
} framebuffer;

#define CIRRUS_APERTURE_SIZE 0x1000000

#define CIRRUS_MAX_WIDTH 0x800
#define CIRRUS_MAX_HEIGHT 0x800
#define CIRRUS_MAX_PITCH (0x1ff * holy_VGA_CR_PITCH_DIVISOR)

enum
  {
    CIRRUS_CR_EXTENDED_DISPLAY = 0x1b,
    CIRRUS_CR_EXTENDED_OVERLAY = 0x1d,
    CIRRUS_CR_MAX
  };

#define CIRRUS_CR_EXTENDED_DISPLAY_PITCH_MASK 0x10
#define CIRRUS_CR_EXTENDED_DISPLAY_PITCH_SHIFT 4
#define CIRRUS_CR_EXTENDED_DISPLAY_START_MASK1 0x1
#define CIRRUS_CR_EXTENDED_DISPLAY_START_SHIFT1 16
#define CIRRUS_CR_EXTENDED_DISPLAY_START_MASK2 0xc
#define CIRRUS_CR_EXTENDED_DISPLAY_START_SHIFT2 15

#define CIRRUS_CR_EXTENDED_OVERLAY_DISPLAY_START_MASK 0x80
#define CIRRUS_CR_EXTENDED_OVERLAY_DISPLAY_START_SHIFT 12

enum
  {
    CIRRUS_SR_EXTENDED_MODE = 7,
    CIRRUS_SR_MAX
  };

#define CIRRUS_SR_EXTENDED_MODE_LFB_ENABLE 0xf0
#define CIRRUS_SR_EXTENDED_MODE_ENABLE_EXT 0x01
#define CIRRUS_SR_EXTENDED_MODE_8BPP       0x00
#define CIRRUS_SR_EXTENDED_MODE_16BPP      0x06
#define CIRRUS_SR_EXTENDED_MODE_24BPP      0x04
#define CIRRUS_SR_EXTENDED_MODE_32BPP      0x08

#define CIRRUS_HIDDEN_DAC_ENABLE_EXT 0x80
#define CIRRUS_HIDDEN_DAC_ENABLE_ALL 0x40
#define CIRRUS_HIDDEN_DAC_8BPP 0
#define CIRRUS_HIDDEN_DAC_15BPP (CIRRUS_HIDDEN_DAC_ENABLE_EXT \
				 | CIRRUS_HIDDEN_DAC_ENABLE_ALL | 0)
#define CIRRUS_HIDDEN_DAC_16BPP (CIRRUS_HIDDEN_DAC_ENABLE_EXT \
				 | CIRRUS_HIDDEN_DAC_ENABLE_ALL | 1)
#define CIRRUS_HIDDEN_DAC_888COLOR (CIRRUS_HIDDEN_DAC_ENABLE_EXT \
				    | CIRRUS_HIDDEN_DAC_ENABLE_ALL | 5)

static void
write_hidden_dac (holy_uint8_t data)
{
  holy_inb (holy_VGA_IO_PALLETTE_WRITE_INDEX);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_outb (data, holy_VGA_IO_PIXEL_MASK);
}

static holy_uint8_t
read_hidden_dac (void)
{
  holy_inb (holy_VGA_IO_PALLETTE_WRITE_INDEX);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  holy_inb (holy_VGA_IO_PIXEL_MASK);
  return holy_inb (holy_VGA_IO_PIXEL_MASK);
}

struct saved_state
{
  holy_uint8_t cr[CIRRUS_CR_MAX];
  holy_uint8_t gr[holy_VGA_GR_MAX];
  holy_uint8_t sr[CIRRUS_SR_MAX];
  holy_uint8_t hidden_dac;
  /* We need to preserve VGA font and VGA text. */
  holy_uint8_t vram[32 * 4 * 256];
  holy_uint8_t r[256];
  holy_uint8_t g[256];
  holy_uint8_t b[256];
};

static struct saved_state initial_state;
static int state_saved = 0;

static void
save_state (struct saved_state *st)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (st->cr); i++)
    st->cr[i] = holy_vga_cr_read (i);
  for (i = 0; i < ARRAY_SIZE (st->sr); i++)
    st->sr[i] = holy_vga_sr_read (i);
  for (i = 0; i < ARRAY_SIZE (st->gr); i++)
    st->gr[i] = holy_vga_gr_read (i);
  for (i = 0; i < 256; i++)
    holy_vga_palette_read (i, st->r + i, st->g + i, st->b + i);

  st->hidden_dac = read_hidden_dac ();
  holy_vga_sr_write (holy_VGA_SR_MEMORY_MODE_CHAIN4, holy_VGA_SR_MEMORY_MODE);
  holy_memcpy (st->vram, framebuffer.ptr, sizeof (st->vram));
}

static void
restore_state (struct saved_state *st)
{
  unsigned i;
  holy_vga_sr_write (holy_VGA_SR_MEMORY_MODE_CHAIN4, holy_VGA_SR_MEMORY_MODE);
  holy_memcpy (framebuffer.ptr, st->vram, sizeof (st->vram));
  for (i = 0; i < ARRAY_SIZE (st->cr); i++)
    holy_vga_cr_write (st->cr[i], i);
  for (i = 0; i < ARRAY_SIZE (st->sr); i++)
    holy_vga_sr_write (st->sr[i], i);
  for (i = 0; i < ARRAY_SIZE (st->gr); i++)
    holy_vga_gr_write (st->gr[i], i);
  for (i = 0; i < 256; i++)
    holy_vga_palette_write (i, st->r[i], st->g[i], st->b[i]);

  write_hidden_dac (st->hidden_dac);
}

static holy_err_t
holy_video_cirrus_video_init (void)
{
  /* Reset frame buffer.  */
  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_cirrus_video_fini (void)
{
  if (framebuffer.mapped)
    holy_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				 CIRRUS_APERTURE_SIZE);

  if (state_saved)
    {
      restore_state (&initial_state);
      state_saved = 0;
    }

  return holy_video_fb_fini ();
}

static holy_err_t
doublebuf_pageflipping_set_page (int page)
{
  int start = framebuffer.page_size * page / 4;
  holy_uint8_t cr_ext, cr_overlay;

  holy_vga_cr_write (start & 0xff, holy_VGA_CR_START_ADDR_LOW_REGISTER);
  holy_vga_cr_write ((start & 0xff00) >> 8,
		     holy_VGA_CR_START_ADDR_HIGH_REGISTER);

  cr_ext = holy_vga_cr_read (CIRRUS_CR_EXTENDED_DISPLAY);
  cr_ext &= ~(CIRRUS_CR_EXTENDED_DISPLAY_START_MASK1
	      | CIRRUS_CR_EXTENDED_DISPLAY_START_MASK2);
  cr_ext |= ((start >> CIRRUS_CR_EXTENDED_DISPLAY_START_SHIFT1)
	     & CIRRUS_CR_EXTENDED_DISPLAY_START_MASK1);
  cr_ext |= ((start >> CIRRUS_CR_EXTENDED_DISPLAY_START_SHIFT2)
	     & CIRRUS_CR_EXTENDED_DISPLAY_START_MASK2);
  holy_vga_cr_write (cr_ext, CIRRUS_CR_EXTENDED_DISPLAY);

  cr_overlay = holy_vga_cr_read (CIRRUS_CR_EXTENDED_OVERLAY);
  cr_overlay &= ~(CIRRUS_CR_EXTENDED_OVERLAY_DISPLAY_START_MASK);
  cr_overlay |= ((start >> CIRRUS_CR_EXTENDED_OVERLAY_DISPLAY_START_SHIFT)
		 & CIRRUS_CR_EXTENDED_OVERLAY_DISPLAY_START_MASK);
  holy_vga_cr_write (cr_overlay, CIRRUS_CR_EXTENDED_OVERLAY);

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_cirrus_set_palette (unsigned int start, unsigned int count,
			       struct holy_video_palette_data *palette_data)
{
  if (framebuffer.mode_info.mode_type == holy_VIDEO_MODE_TYPE_INDEX_COLOR)
    {
      unsigned i;
      if (start >= 0x100)
	return holy_ERR_NONE;
      if (start + count >= 0x100)
	count = 0x100 - start;

      for (i = 0; i < count; i++)
	holy_vga_palette_write (start + i, palette_data[i].r, palette_data[i].g,
				palette_data[i].b);
    }

  /* Then set color to emulated palette.  */
  return holy_video_fb_set_palette (start, count, palette_data);
}

/* Helper for holy_video_cirrus_setup.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  int *found = data;
  holy_pci_address_t addr;
  holy_uint32_t class;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  if (((class >> 16) & 0xffff) != 0x0300 || pciid != 0x00b81013)
    return 0;

  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
  framebuffer.base = holy_pci_read (addr) & holy_PCI_ADDR_MEM_MASK;
  if (!framebuffer.base)
    return 0;

  *found = 1;

  /* Enable address spaces.  */
  addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_COMMAND);
  holy_pci_write (addr, 0x7);

  framebuffer.dev = dev;

  return 1;
}

static holy_err_t
holy_video_cirrus_setup (unsigned int width, unsigned int height,
			 holy_video_mode_type_t mode_type,
			 holy_video_mode_type_t mode_mask)
{
  int depth;
  holy_err_t err;
  int found = 0;
  int pitch, bytes_per_pixel;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if (width == 0 || height == 0)
    {
      width = 800;
      height = 600;
    }

  if (width & (holy_VGA_CR_WIDTH_DIVISOR - 1))
    return holy_error (holy_ERR_IO,
		       "screen width must be a multiple of %d",
		       holy_VGA_CR_WIDTH_DIVISOR);

  if (width > CIRRUS_MAX_WIDTH)
    return holy_error (holy_ERR_IO,
		       "screen width must be at most %d", CIRRUS_MAX_WIDTH);

  if (height > CIRRUS_MAX_HEIGHT)
    return holy_error (holy_ERR_IO,
		       "screen height must be at most %d", CIRRUS_MAX_HEIGHT);

  if (depth == 0
      && !holy_video_check_mode_flag (mode_type, mode_mask,
				      holy_VIDEO_MODE_TYPE_INDEX_COLOR, 0))
    depth = 24;
  else if (depth == 0)
    depth = 8;

  if (depth != 32 && depth != 24 && depth != 16 && depth != 15 && depth != 8)
    return holy_error (holy_ERR_IO, "only 32, 24, 16, 15 and 8-bit bpp are"
		       " supported by cirrus video");

  bytes_per_pixel = (depth + holy_CHAR_BIT - 1) / holy_CHAR_BIT;
  pitch = width * bytes_per_pixel;

  if (pitch > CIRRUS_MAX_PITCH)
    return holy_error (holy_ERR_IO,
		       "screen width must be at most %d at bitdepth %d",
		       CIRRUS_MAX_PITCH / bytes_per_pixel, depth);

  framebuffer.page_size = pitch * height;

  if (framebuffer.page_size > CIRRUS_APERTURE_SIZE)
    return holy_error (holy_ERR_IO, "Not enough video memory for this mode");

  holy_pci_iterate (find_card, &found);
  if (!found)
    return holy_error (holy_ERR_IO, "Couldn't find graphics card");

  if (found && framebuffer.base == 0)
    {
      /* FIXME: change framebuffer base */
      return holy_error (holy_ERR_IO, "PCI BAR not set");
    }

  /* We can safely discard volatile attribute.  */
  framebuffer.ptr = (void *) holy_pci_device_map_range (framebuffer.dev,
							framebuffer.base,
							CIRRUS_APERTURE_SIZE);
  framebuffer.mapped = 1;

  if (!state_saved)
    {
      save_state (&initial_state);
      state_saved = 1;
    }

  {
    struct holy_video_hw_config config = {
      .pitch = pitch / holy_VGA_CR_PITCH_DIVISOR,
      .line_compare = 0x3ff,
      .vdisplay_end = height - 1,
      .horizontal_end = width / holy_VGA_CR_WIDTH_DIVISOR
    };
    holy_uint8_t sr_ext = 0, hidden_dac = 0;

    holy_vga_set_geometry (&config, holy_vga_cr_write);
    
    holy_vga_gr_write (holy_VGA_GR_MODE_256_COLOR | holy_VGA_GR_MODE_READ_MODE1,
		       holy_VGA_GR_MODE);
    holy_vga_gr_write (holy_VGA_GR_GR6_GRAPHICS_MODE, holy_VGA_GR_GR6);
    
    holy_vga_sr_write (holy_VGA_SR_MEMORY_MODE_NORMAL, holy_VGA_SR_MEMORY_MODE);

    holy_vga_cr_write ((config.pitch >> CIRRUS_CR_EXTENDED_DISPLAY_PITCH_SHIFT)
	      & CIRRUS_CR_EXTENDED_DISPLAY_PITCH_MASK,
	      CIRRUS_CR_EXTENDED_DISPLAY);

    holy_vga_cr_write (holy_VGA_CR_MODE_TIMING_ENABLE
		       | holy_VGA_CR_MODE_BYTE_MODE
		       | holy_VGA_CR_MODE_NO_HERCULES | holy_VGA_CR_MODE_NO_CGA,
		       holy_VGA_CR_MODE);

    doublebuf_pageflipping_set_page (0);

    sr_ext = CIRRUS_SR_EXTENDED_MODE_LFB_ENABLE
      | CIRRUS_SR_EXTENDED_MODE_ENABLE_EXT;
    switch (depth)
      {
	/* FIXME: support 8-bit grayscale and 8-bit RGB.  */
      case 32:
	hidden_dac = CIRRUS_HIDDEN_DAC_888COLOR;
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_32BPP;
	break;
      case 24:
	hidden_dac = CIRRUS_HIDDEN_DAC_888COLOR;
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_24BPP;
	break;
      case 16:
	hidden_dac = CIRRUS_HIDDEN_DAC_16BPP;
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_16BPP;
	break;
      case 15:
	hidden_dac = CIRRUS_HIDDEN_DAC_15BPP;
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_16BPP;
	break;
      case 8:
	hidden_dac = CIRRUS_HIDDEN_DAC_8BPP;
	sr_ext |= CIRRUS_SR_EXTENDED_MODE_8BPP;
	break;
      }
    holy_vga_sr_write (sr_ext, CIRRUS_SR_EXTENDED_MODE);
    write_hidden_dac (hidden_dac);
  }

  /* Fill mode info details.  */
  framebuffer.mode_info.width = width;
  framebuffer.mode_info.height = height;
  framebuffer.mode_info.mode_type = holy_VIDEO_MODE_TYPE_RGB;
  framebuffer.mode_info.bpp = depth;
  framebuffer.mode_info.bytes_per_pixel = bytes_per_pixel;
  framebuffer.mode_info.pitch = pitch;
  framebuffer.mode_info.number_of_colors = 256;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;

  switch (depth)
    {
    case 8:
      framebuffer.mode_info.mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR;
      framebuffer.mode_info.number_of_colors = 16;
      break;
    case 16:
      framebuffer.mode_info.red_mask_size = 5;
      framebuffer.mode_info.red_field_pos = 11;
      framebuffer.mode_info.green_mask_size = 6;
      framebuffer.mode_info.green_field_pos = 5;
      framebuffer.mode_info.blue_mask_size = 5;
      framebuffer.mode_info.blue_field_pos = 0;
      break;

    case 15:
      framebuffer.mode_info.red_mask_size = 5;
      framebuffer.mode_info.red_field_pos = 10;
      framebuffer.mode_info.green_mask_size = 5;
      framebuffer.mode_info.green_field_pos = 5;
      framebuffer.mode_info.blue_mask_size = 5;
      framebuffer.mode_info.blue_field_pos = 0;
      break;

    case 32:
      framebuffer.mode_info.reserved_mask_size = 8;
      framebuffer.mode_info.reserved_field_pos = 24;
      /* Fallthrough.  */

    case 24:
      framebuffer.mode_info.red_mask_size = 8;
      framebuffer.mode_info.red_field_pos = 16;
      framebuffer.mode_info.green_mask_size = 8;
      framebuffer.mode_info.green_field_pos = 8;
      framebuffer.mode_info.blue_mask_size = 8;
      framebuffer.mode_info.blue_field_pos = 0;
      break;
    }

  framebuffer.mode_info.blit_format = holy_video_get_blit_format (&framebuffer.mode_info);

  if (CIRRUS_APERTURE_SIZE >= 2 * framebuffer.page_size)
    err = holy_video_fb_setup (mode_type, mode_mask,
			       &framebuffer.mode_info,
			       framebuffer.ptr,
			       doublebuf_pageflipping_set_page,
			       framebuffer.ptr + framebuffer.page_size);
  else
    err = holy_video_fb_setup (mode_type, mode_mask,
			       &framebuffer.mode_info,
			       framebuffer.ptr, 0, 0);


  /* Copy default palette to initialize emulated palette.  */
  err = holy_video_cirrus_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				       holy_video_fbstd_colors);
  return err;
}

static struct holy_video_adapter holy_video_cirrus_adapter =
  {
    .name = "Cirrus CLGD 5446 PCI Video Driver",
    .id = holy_VIDEO_DRIVER_CIRRUS,

    .prio = holy_VIDEO_ADAPTER_PRIO_NATIVE,

    .init = holy_video_cirrus_video_init,
    .fini = holy_video_cirrus_video_fini,
    .setup = holy_video_cirrus_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_fb_get_info_and_fini,
    .set_palette = holy_video_cirrus_set_palette,
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
    .swap_buffers = holy_video_fb_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_fb_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(video_cirrus)
{
  holy_video_register (&holy_video_cirrus_adapter);
}

holy_MOD_FINI(video_cirrus)
{
  holy_video_unregister (&holy_video_cirrus_adapter);
}
