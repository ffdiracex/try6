/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_video_render_target holy_video_fbrender_target

#include <holy/i386/pc/int.h>
#include <holy/machine/console.h>
#include <holy/cpu/io.h>
#include <holy/mm.h>
#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/vga.h>

holy_MOD_LICENSE ("GPLv2+");

#define VGA_WIDTH	640
#define VGA_MEM		((holy_uint8_t *) 0xa0000)
#define PAGE_OFFSET(x)	((x) * (VGA_WIDTH * vga_height / 8))

static unsigned char text_mode;
static unsigned char saved_map_mask;
static int vga_height;

static struct
{
  struct holy_video_mode_info mode_info;
  struct holy_video_render_target *render_target;
  holy_uint8_t *temporary_buffer;
  int front_page;
  int back_page;
} framebuffer;

static unsigned char 
holy_vga_set_mode (unsigned char mode)
{
  struct holy_bios_int_registers regs;
  unsigned char ret;
  /* get current mode */
  regs.eax = 0x0f00;
  regs.ebx = 0;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  ret = regs.eax & 0xff;
  regs.eax = mode;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  holy_bios_interrupt (0x10, &regs);

  return ret;
}

static inline void
wait_vretrace (void)
{
  /* Wait until there is a vertical retrace.  */
  while (! (holy_inb (holy_VGA_IO_INPUT_STATUS1_REGISTER)
	    & holy_VGA_IO_INPUT_STATUS1_VERTR_BIT));
}

/* Get Map Mask Register.  */
static unsigned char
get_map_mask (void)
{
  return holy_vga_sr_read (holy_VGA_SR_MAP_MASK_REGISTER);
}

/* Set Map Mask Register.  */
static void
set_map_mask (unsigned char mask)
{
  holy_vga_sr_write (mask, holy_VGA_SR_MAP_MASK_REGISTER);
}

#if 0
/* Set Read Map Register.  */
static void
set_read_map (unsigned char map)
{
  holy_vga_gr_write (map, holy_VGA_GR_READ_MAP_REGISTER);
}
#endif

/* Set start address.  */
static void
set_start_address (unsigned int start)
{
  holy_vga_cr_write (start & 0xFF, holy_VGA_CR_START_ADDR_LOW_REGISTER);
  holy_vga_cr_write (start >> 8, holy_VGA_CR_START_ADDR_HIGH_REGISTER);
}

static int setup = 0;
static int is_target = 0;

static holy_err_t
holy_video_vga_init (void)
{
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vga_setup (unsigned int width, unsigned int height,
                      holy_video_mode_type_t mode_type,
		      holy_video_mode_type_t mode_mask)
{
  holy_err_t err;

  if ((width && width != VGA_WIDTH) || (height && height != 350 && height != 480))
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "no matching mode found");

  vga_height = height ? : 480;

  framebuffer.temporary_buffer = holy_malloc (vga_height * VGA_WIDTH);
  framebuffer.front_page = 0;
  framebuffer.back_page = 0;
  if (!framebuffer.temporary_buffer)
    return holy_errno;

  saved_map_mask = get_map_mask ();

  text_mode = holy_vga_set_mode (vga_height == 480 ? 0x12 : 0x10);
  setup = 1;
  set_map_mask (0x0f);
  set_start_address (PAGE_OFFSET (framebuffer.front_page));

  framebuffer.mode_info.width = VGA_WIDTH;
  framebuffer.mode_info.height = vga_height;

  framebuffer.mode_info.mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR;

  if (holy_video_check_mode_flag (mode_type, mode_mask,
				  holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED,
				  (VGA_WIDTH * vga_height <= (1 << 18))))
    {
      framebuffer.back_page = 1;
      framebuffer.mode_info.mode_type |= holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
	| holy_VIDEO_MODE_TYPE_UPDATING_SWAP;
    }

  framebuffer.mode_info.bpp = 8;
  framebuffer.mode_info.bytes_per_pixel = 1;
  framebuffer.mode_info.pitch = VGA_WIDTH;
  framebuffer.mode_info.number_of_colors = 16;
  framebuffer.mode_info.red_mask_size = 0;
  framebuffer.mode_info.red_field_pos = 0;
  framebuffer.mode_info.green_mask_size = 0;
  framebuffer.mode_info.green_field_pos = 0;
  framebuffer.mode_info.blue_mask_size = 0;
  framebuffer.mode_info.blue_field_pos = 0;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;

  framebuffer.mode_info.blit_format
    = holy_video_get_blit_format (&framebuffer.mode_info);

  err = holy_video_fb_create_render_target_from_pointer (&framebuffer.render_target,
							 &framebuffer.mode_info,
							 framebuffer.temporary_buffer);

  if (err)
    {
      holy_dprintf ("video", "Couldn't create FB target\n");
      return err;
    }

  is_target = 1;
  err = holy_video_fb_set_active_render_target (framebuffer.render_target);
 
  if (err)
    return err;
 
  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				   holy_video_fbstd_colors);

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vga_fini (void)
{
  if (setup)
    {
      set_map_mask (saved_map_mask);
      holy_vga_set_mode (text_mode);
    }
  setup = 0;
  holy_free (framebuffer.temporary_buffer);
  framebuffer.temporary_buffer = 0;
  return holy_ERR_NONE;
}

static inline void
update_target (void)
{
  int plane;

  if (!is_target)
    return;

  for (plane = 0x01; plane <= 0x08; plane <<= 1)
    {
      holy_uint8_t *ptr;
      volatile holy_uint8_t *ptr2;
      unsigned cbyte = 0;
      int shift = 7;
      set_map_mask (plane);
      for (ptr = framebuffer.temporary_buffer,
	     ptr2 = VGA_MEM + PAGE_OFFSET (framebuffer.back_page);
	   ptr < framebuffer.temporary_buffer + VGA_WIDTH * vga_height; ptr++)
	{
	  cbyte |= (!!(plane & *ptr)) << shift;
	  shift--;
	  if (shift == -1)
	    {
	      *ptr2++ = cbyte;
	      shift = 7;
	      cbyte = 0;
	    }
	}
    }
}

static holy_err_t
holy_video_vga_blit_bitmap (struct holy_video_bitmap *bitmap,
			    enum holy_video_blit_operators oper, int x, int y,
			    int offset_x, int offset_y,
			    unsigned int width, unsigned int height)
{
  holy_err_t ret;
  ret = holy_video_fb_blit_bitmap (bitmap, oper, x, y, offset_x, offset_y,
				   width, height);
  if (!(framebuffer.mode_info.mode_type & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED))
    update_target ();
  return ret;
}

static holy_err_t
holy_video_vga_blit_render_target (struct holy_video_fbrender_target *source,
                                   enum holy_video_blit_operators oper,
                                   int x, int y, int offset_x, int offset_y,
                                   unsigned int width, unsigned int height)
{
  holy_err_t ret;

  ret = holy_video_fb_blit_render_target (source, oper, x, y,
					  offset_x, offset_y, width, height);
  if (!(framebuffer.mode_info.mode_type & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED))
    update_target ();

  return ret;
}

static holy_err_t
holy_video_vga_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    {
      is_target = 1;
      target = framebuffer.render_target;
    }
  else
    is_target = 0;

  return holy_video_fb_set_active_render_target (target);
}

static holy_err_t
holy_video_vga_get_active_render_target (struct holy_video_render_target **target)
{
  holy_err_t err;
  err = holy_video_fb_get_active_render_target (target);
  if (err)
    return err;

  if (*target == framebuffer.render_target)
    *target = holy_VIDEO_RENDER_TARGET_DISPLAY;

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vga_swap_buffers (void)
{
  if (!(framebuffer.mode_info.mode_type & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED))
    return holy_ERR_NONE;

  update_target ();

  if ((VGA_WIDTH * vga_height <= (1 << 18)))
    {
      /* Activate the other page.  */
      framebuffer.front_page = !framebuffer.front_page;
      framebuffer.back_page = !framebuffer.back_page;
      wait_vretrace ();
      set_start_address (PAGE_OFFSET (framebuffer.front_page));
    }

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_vga_set_palette (unsigned int start __attribute__ ((unused)),
			    unsigned int count __attribute__ ((unused)),
                            struct holy_video_palette_data *palette_data __attribute__ ((unused)))
{
  return holy_error (holy_ERR_IO, "can't change palette");
}

static holy_err_t
holy_video_vga_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  set_map_mask (0xf);

  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  mode_info->bpp = 1;
  mode_info->bytes_per_pixel = 0;
  mode_info->pitch = VGA_WIDTH / 8;
  mode_info->number_of_colors = 1;

  mode_info->bg_red = 0;
  mode_info->bg_green = 0;
  mode_info->bg_blue = 0;
  mode_info->bg_alpha = 255;

  mode_info->fg_red = 255;
  mode_info->fg_green = 255;
  mode_info->fg_blue = 255;
  mode_info->fg_alpha = 255;

  *framebuf = VGA_MEM + PAGE_OFFSET (framebuffer.front_page);

  holy_video_fb_fini ();
  holy_free (framebuffer.temporary_buffer);
  framebuffer.temporary_buffer = 0;
  setup = 0;

  return holy_ERR_NONE;
}


static struct holy_video_adapter holy_video_vga_adapter =
  {
    .name = "VGA Video Driver",
    .id = holy_VIDEO_DRIVER_VGA,

    .prio = holy_VIDEO_ADAPTER_PRIO_FALLBACK,

    .init = holy_video_vga_init,
    .fini = holy_video_vga_fini,
    .setup = holy_video_vga_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_vga_get_info_and_fini,
    .set_palette = holy_video_vga_set_palette,
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
    .blit_bitmap = holy_video_vga_blit_bitmap,
    .blit_render_target = holy_video_vga_blit_render_target,
    .scroll = holy_video_fb_scroll,
    .swap_buffers = holy_video_vga_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_vga_set_active_render_target,
    .get_active_render_target = holy_video_vga_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(vga)
{
  holy_video_register (&holy_video_vga_adapter);
}

holy_MOD_FINI(vga)
{
  holy_video_unregister (&holy_video_vga_adapter);
}
