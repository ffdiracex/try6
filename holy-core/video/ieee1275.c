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
#include <holy/ieee1275/ieee1275.h>

holy_MOD_LICENSE ("GPLv2+");

/* Only 8-bit indexed color is supported for now.  */

static unsigned old_width, old_height;
static int restore_needed;
static char *display;
static holy_ieee1275_ihandle_t stdout_ihandle;
static int have_setcolors = 0;

static struct
{
  struct holy_video_mode_info mode_info;
  holy_uint8_t *ptr;
} framebuffer;

static struct holy_video_palette_data serial_colors[] =
  {
    // {R, G, B}
    {0x00, 0x00, 0x00, 0xFF}, // 0 = black
    {0xA8, 0x00, 0x00, 0xFF}, // 1 = red
    {0x00, 0xA8, 0x00, 0xFF}, // 2 = green
    {0xFE, 0xFE, 0x54, 0xFF}, // 3 = yellow
    {0x00, 0x00, 0xA8, 0xFF}, // 4 = blue
    {0xA8, 0x00, 0xA8, 0xFF}, // 5 = magenta
    {0x00, 0xA8, 0xA8, 0xFF}, // 6 = cyan
    {0xFE, 0xFE, 0xFE, 0xFF}  // 7 = white
  };


static holy_err_t
holy_video_ieee1275_set_palette (unsigned int start, unsigned int count,
				 struct holy_video_palette_data *palette_data);

static void
set_video_mode (unsigned width __attribute__ ((unused)),
		unsigned height __attribute__ ((unused)))
{
  /* TODO */
}

static int
find_display_hook (struct holy_ieee1275_devalias *alias)
{
  if (holy_strcmp (alias->type, "display") == 0)
    {
      holy_dprintf ("video", "Found display %s\n", alias->path);
      display = holy_strdup (alias->path);
      return 1;
    }
  return 0;
}

static void
find_display (void)
{
  holy_ieee1275_devices_iterate (find_display_hook);
}

static holy_err_t
holy_video_ieee1275_init (void)
{
  holy_ssize_t actual;

  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  if (! holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CANNOT_SET_COLORS)
      && !holy_ieee1275_get_integer_property (holy_ieee1275_chosen,
					      "stdout", &stdout_ihandle,
					      sizeof (stdout_ihandle), &actual)
      && actual == sizeof (stdout_ihandle))
    have_setcolors = 1;

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_ieee1275_fini (void)
{
  if (restore_needed)
    {
      set_video_mode (old_width, old_height);
      restore_needed = 0;
    }
  return holy_video_fb_fini ();
}

static holy_err_t
holy_video_ieee1275_fill_mode_info (holy_ieee1275_phandle_t dev,
				    struct holy_video_mode_info *out)
{
  holy_uint32_t tmp;

  holy_memset (out, 0, sizeof (*out));

  if (holy_ieee1275_get_integer_property (dev, "width", &tmp,
					  sizeof (tmp), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display width.");
  out->width = tmp;

  if (holy_ieee1275_get_integer_property (dev, "height", &tmp,
					  sizeof (tmp), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display height.");
  out->height = tmp;

  if (holy_ieee1275_get_integer_property (dev, "linebytes", &tmp,
					  sizeof (tmp), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display pitch.");
  out->pitch = tmp;

  if (holy_ieee1275_get_integer_property (dev, "depth", &tmp,
					  sizeof (tmp), 0))
    tmp = 4;

  out->mode_type = holy_VIDEO_MODE_TYPE_RGB;
  out->bpp = tmp;
  out->bytes_per_pixel = (out->bpp + holy_CHAR_BIT - 1) / holy_CHAR_BIT;
  out->number_of_colors = 256;

  switch (tmp)
    {
    case 4:
    case 8:
      out->mode_type = holy_VIDEO_MODE_TYPE_INDEX_COLOR;
      out->bpp = 8;
      if (have_setcolors)
	out->number_of_colors = 1 << tmp;
      else
	out->number_of_colors = 8;
      break;

      /* FIXME: we may need byteswapping for the following. Currently it
	 was seen only on qemu and no byteswap was needed.  */
    case 15:
      out->red_mask_size = 5;
      out->red_field_pos = 10;
      out->green_mask_size = 5;
      out->green_field_pos = 5;
      out->blue_mask_size = 5;
      out->blue_field_pos = 0;
      break;

    case 16:
      out->red_mask_size = 5;
      out->red_field_pos = 11;
      out->green_mask_size = 6;
      out->green_field_pos = 5;
      out->blue_mask_size = 5;
      out->blue_field_pos = 0;
      break;

    case 32:
      out->reserved_mask_size = 8;
      out->reserved_field_pos = 24;
      /* FALLTHROUGH */

    case 24:
      out->red_mask_size = 8;
      out->red_field_pos = 16;
      out->green_mask_size = 8;
      out->green_field_pos = 8;
      out->blue_mask_size = 8;
      out->blue_field_pos = 0;
      break;
    default:
      return holy_error (holy_ERR_IO, "unsupported video depth %d", tmp);
    }

  out->blit_format = holy_video_get_blit_format (out);
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_ieee1275_setup (unsigned int width, unsigned int height,
			   unsigned int mode_type __attribute__ ((unused)),
			   unsigned int mode_mask __attribute__ ((unused)))
{
  holy_uint32_t current_width, current_height, address;
  holy_err_t err;
  holy_ieee1275_phandle_t dev;

  if (!display)
    return holy_error (holy_ERR_IO, "Couldn't find display device.");

  if (holy_ieee1275_finddevice (display, &dev))
    return holy_error (holy_ERR_IO, "Couldn't open display device.");

  if (holy_ieee1275_get_integer_property (dev, "width", &current_width,
					  sizeof (current_width), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display width.");

  if (holy_ieee1275_get_integer_property (dev, "height", &current_height,
					  sizeof (current_width), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display height.");

  if ((width == current_width && height == current_height)
      || (width == 0 && height == 0))
    {
      holy_dprintf ("video", "IEEE1275: keeping current mode %dx%d\n",
		    current_width, current_height);
    }
  else
    {
      holy_dprintf ("video", "IEEE1275: Setting mode %dx%d\n", width, height);
      /* TODO. */
      return holy_error (holy_ERR_IO, "can't set mode %dx%d", width, height);
    }
  
  err = holy_video_ieee1275_fill_mode_info (dev, &framebuffer.mode_info);
  if (err)
    {
      holy_dprintf ("video", "IEEE1275: couldn't fill mode info\n");
      return err;
    }

  if (holy_ieee1275_get_integer_property (dev, "address", (void *) &address,
					  sizeof (address), 0))
    return holy_error (holy_ERR_IO, "Couldn't retrieve display address.");

  /* For some reason sparc64 uses 32-bit pointer too.  */
  framebuffer.ptr = (void *) (holy_addr_t) address;

  holy_dprintf ("video", "IEEE1275: initialising FB @ %p %dx%dx%d\n",
		framebuffer.ptr, framebuffer.mode_info.width,
		framebuffer.mode_info.height, framebuffer.mode_info.bpp);

  err = holy_video_fb_setup (mode_type, mode_mask,
			     &framebuffer.mode_info,
			     framebuffer.ptr, NULL, NULL);
  if (err)
    return err;

  holy_video_ieee1275_set_palette (0, framebuffer.mode_info.number_of_colors,
				   holy_video_fbstd_colors);
    
  return err;
}

static holy_err_t
holy_video_ieee1275_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static holy_err_t
holy_video_ieee1275_set_palette (unsigned int start, unsigned int count,
				 struct holy_video_palette_data *palette_data)
{
  unsigned col;
  struct holy_video_palette_data fb_palette_data[256];
  holy_err_t err;

  if (!(framebuffer.mode_info.mode_type & holy_VIDEO_MODE_TYPE_INDEX_COLOR))
    return holy_video_fb_set_palette (start, count, palette_data);

  if (!have_setcolors)
    return holy_video_fb_set_palette (0, ARRAY_SIZE (serial_colors),
    				      serial_colors);

  if (start >= framebuffer.mode_info.number_of_colors)
    return holy_ERR_NONE;

  if (start + count > framebuffer.mode_info.number_of_colors)
    count = framebuffer.mode_info.number_of_colors - start;

  err = holy_video_fb_set_palette (start, count, palette_data);
  if (err)
    return err;

  /* Set colors.  */
  holy_video_fb_get_palette (0, ARRAY_SIZE (fb_palette_data),
			     fb_palette_data);

  for (col = 0; col < ARRAY_SIZE (fb_palette_data); col++)
    holy_ieee1275_set_color (stdout_ihandle, col, fb_palette_data[col].r,
			     fb_palette_data[col].g,
			     fb_palette_data[col].b);
  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_ieee1275_adapter =
  {
    .name = "IEEE1275 video driver",

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE,
    .id = holy_VIDEO_DRIVER_IEEE1275,

    .init = holy_video_ieee1275_init,
    .fini = holy_video_ieee1275_fini,
    .setup = holy_video_ieee1275_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_ieee1275_get_info_and_fini,
    .set_palette = holy_video_ieee1275_set_palette,
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

holy_MOD_INIT(ieee1275_fb)
{
  find_display ();
  if (display)
    holy_video_register (&holy_video_ieee1275_adapter);
}

holy_MOD_FINI(ieee1275_fb)
{
  if (restore_needed)
    {
      set_video_mode (old_width, old_height);
      restore_needed = 0;
    }
  if (display)
    holy_video_unregister (&holy_video_ieee1275_adapter);
  holy_free (display);
}
