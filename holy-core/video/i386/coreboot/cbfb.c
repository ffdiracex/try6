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
#include <holy/machine/lbio.h>
#include <holy/machine/console.h>

struct holy_linuxbios_table_framebuffer *holy_video_coreboot_fbtable;

static struct
{
  struct holy_video_mode_info mode_info;
  holy_uint8_t *ptr;
} framebuffer;

static holy_err_t
holy_video_cbfb_init (void)
{
  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_cbfb_fill_mode_info (struct holy_video_mode_info *out)
{
  holy_memset (out, 0, sizeof (*out));

  out->width = holy_video_coreboot_fbtable->width;
  out->height = holy_video_coreboot_fbtable->height;
  out->pitch = holy_video_coreboot_fbtable->pitch;

  out->red_field_pos = holy_video_coreboot_fbtable->red_field_pos;
  out->red_mask_size = holy_video_coreboot_fbtable->red_mask_size;
  out->green_field_pos = holy_video_coreboot_fbtable->green_field_pos;
  out->green_mask_size = holy_video_coreboot_fbtable->green_mask_size;
  out->blue_field_pos = holy_video_coreboot_fbtable->blue_field_pos;
  out->blue_mask_size = holy_video_coreboot_fbtable->blue_mask_size;
  out->reserved_field_pos = holy_video_coreboot_fbtable->reserved_field_pos;
  out->reserved_mask_size = holy_video_coreboot_fbtable->reserved_mask_size;

  out->mode_type = holy_VIDEO_MODE_TYPE_RGB;
  out->bpp = holy_video_coreboot_fbtable->bpp;
  out->bytes_per_pixel = (holy_video_coreboot_fbtable->bpp + 7) / 8;
  out->number_of_colors = 256;

  out->blit_format = holy_video_get_blit_format (out);
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_cbfb_setup (unsigned int width, unsigned int height,
			   unsigned int mode_type __attribute__ ((unused)),
			   unsigned int mode_mask __attribute__ ((unused)))
{
  holy_err_t err;

  if (!holy_video_coreboot_fbtable)
    return holy_error (holy_ERR_IO, "Couldn't find display device.");

  if (!((width == holy_video_coreboot_fbtable->width && height == holy_video_coreboot_fbtable->height)
	|| (width == 0 && height == 0)))
    return holy_error (holy_ERR_IO, "can't set mode %dx%d", width, height);

  err = holy_video_cbfb_fill_mode_info (&framebuffer.mode_info);
  if (err)
    {
      holy_dprintf ("video", "CBFB: couldn't fill mode info\n");
      return err;
    }

  framebuffer.ptr = (void *) (holy_addr_t) holy_video_coreboot_fbtable->lfb;

  holy_dprintf ("video", "CBFB: initialising FB @ %p %dx%dx%d\n",
		framebuffer.ptr, framebuffer.mode_info.width,
		framebuffer.mode_info.height, framebuffer.mode_info.bpp);

  err = holy_video_fb_setup (mode_type, mode_mask,
			     &framebuffer.mode_info,
			     framebuffer.ptr, NULL, NULL);
  if (err)
    return err;

  holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
			     holy_video_fbstd_colors);
    
  return err;
}

static holy_err_t
holy_video_cbfb_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_cbfb_adapter =
  {
    .name = "Coreboot video driver",

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE_DIRTY,
    .id = holy_VIDEO_DRIVER_COREBOOT,

    .init = holy_video_cbfb_init,
    .fini = holy_video_fb_fini,
    .setup = holy_video_cbfb_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_cbfb_get_info_and_fini,
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
    .swap_buffers = holy_video_fb_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_fb_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

static int
iterate_linuxbios_table (holy_linuxbios_table_item_t table_item,
			 void *data __attribute__ ((unused)))
{
  if (table_item->tag != holy_LINUXBIOS_MEMBER_FRAMEBUFFER)
    return 0;
  holy_video_coreboot_fbtable = (struct holy_linuxbios_table_framebuffer *) (table_item + 1);
  return 1;
}

void
holy_video_coreboot_fb_early_init (void)
{
  holy_linuxbios_table_iterate (iterate_linuxbios_table, 0);
}

void
holy_video_coreboot_fb_late_init (void)
{
  if (holy_video_coreboot_fbtable)
    holy_video_register (&holy_video_cbfb_adapter);
}

void
holy_video_coreboot_fb_fini (void)
{
  if (holy_video_coreboot_fbtable)
    holy_video_unregister (&holy_video_cbfb_adapter);
}
