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
#include <SDL/SDL.h>

holy_MOD_LICENSE ("GPLv2+");

static SDL_Surface *window = 0;
static struct holy_video_render_target *sdl_render_target;
static struct holy_video_mode_info mode_info;

static holy_err_t
holy_video_sdl_set_palette (unsigned int start, unsigned int count,
                            struct holy_video_palette_data *palette_data);

static holy_err_t
holy_video_sdl_init (void)
{
  window = 0;

  if (SDL_Init (SDL_INIT_VIDEO) < 0)
    return holy_error (holy_ERR_BAD_DEVICE, "Couldn't init SDL: %s",
		       SDL_GetError ());

  holy_memset (&mode_info, 0, sizeof (mode_info));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_sdl_fini (void)
{
  SDL_Quit ();
  window = 0;

  holy_memset (&mode_info, 0, sizeof (mode_info));

  return holy_video_fb_fini ();
}

static inline unsigned int
get_mask_size (holy_uint32_t mask)
{
  unsigned i;
  for (i = 0; mask > 1U << i; i++);
  return i;
}

static holy_err_t
holy_video_sdl_setup (unsigned int width, unsigned int height,
                      unsigned int mode_type, unsigned int mode_mask)
{
  int depth;
  int flags = 0;
  holy_err_t err;

  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if (depth == 0)
    depth = 32;

  if (width == 0 && height == 0)
    {
      width = 800;
      height = 600;
    }

  if ((mode_type & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED)
      || !(mode_mask & holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED))
    flags |= SDL_DOUBLEBUF;

  window = SDL_SetVideoMode (width, height, depth, flags | SDL_HWSURFACE);
  if (! window)
    window = SDL_SetVideoMode (width, height, depth, flags | SDL_SWSURFACE);
  if (! window)
    return holy_error (holy_ERR_BAD_DEVICE, "Couldn't open window: %s",
		       SDL_GetError ());

  holy_memset (&sdl_render_target, 0, sizeof (sdl_render_target));

  mode_info.width = window->w;
  mode_info.height = window->h;
  mode_info.mode_type = 0;
  if (window->flags & SDL_DOUBLEBUF)
    mode_info.mode_type
      |= holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED;
  if (window->format->palette)
    mode_info.mode_type |= holy_VIDEO_MODE_TYPE_INDEX_COLOR;
  else
    mode_info.mode_type |= holy_VIDEO_MODE_TYPE_RGB;

  mode_info.bpp = window->format->BitsPerPixel;
  mode_info.bytes_per_pixel = window->format->BytesPerPixel;
  mode_info.pitch = window->pitch;

  /* In index color mode, number of colors.  In RGB mode this is 256.  */
  if (window->format->palette)
    mode_info.number_of_colors
      = 1 << window->format->BitsPerPixel;
  else
    mode_info.number_of_colors = 256;

  if (! window->format->palette)
    {
      mode_info.red_mask_size
	= get_mask_size (window->format->Rmask >> window->format->Rshift);
      mode_info.red_field_pos = window->format->Rshift;
      mode_info.green_mask_size
	= get_mask_size (window->format->Gmask >> window->format->Gshift);
      mode_info.green_field_pos = window->format->Gshift;
      mode_info.blue_mask_size
	= get_mask_size (window->format->Bmask >> window->format->Bshift);
      mode_info.blue_field_pos = window->format->Bshift;
      mode_info.reserved_mask_size
	= get_mask_size (window->format->Amask >> window->format->Ashift);
      mode_info.reserved_field_pos = window->format->Ashift;
      mode_info.blit_format
	= holy_video_get_blit_format (&mode_info);
    }

  err = holy_video_fb_create_render_target_from_pointer (&sdl_render_target,
							 &mode_info,
							 window->pixels);
  if (err)
    return err;

  /* Copy default palette to initialize emulated palette.  */
  holy_video_sdl_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
			      holy_video_fbstd_colors);

  /* Reset render target to SDL one.  */
  return holy_video_fb_set_active_render_target (sdl_render_target);
}

static holy_err_t
holy_video_sdl_set_palette (unsigned int start, unsigned int count,
                            struct holy_video_palette_data *palette_data)
{
  unsigned i;
  if (window->format->palette)
    {
      SDL_Color *tmp;
      if (start >= mode_info.number_of_colors)
	return holy_ERR_NONE;

      if (start + count > mode_info.number_of_colors)
	count = mode_info.number_of_colors - start;

      tmp = holy_malloc (count * sizeof (tmp[0]));
      for (i = 0; i < count; i++)
	{
	  tmp[i].r = palette_data[i].r;
	  tmp[i].g = palette_data[i].g;
	  tmp[i].b = palette_data[i].b;
	  tmp[i].unused = palette_data[i].a;
	}
      SDL_SetColors (window, tmp, start, count);
      holy_free (tmp);
    }

  return holy_video_fb_set_palette (start, count, palette_data);
}

static holy_err_t
holy_video_sdl_swap_buffers (void)
{
  if (SDL_Flip (window) < 0)
    return holy_error (holy_ERR_BAD_DEVICE, "couldn't swap buffers: %s",
		       SDL_GetError ());
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_sdl_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    return holy_video_fb_set_active_render_target (sdl_render_target);

  return holy_video_fb_set_active_render_target (target);
}

static struct holy_video_adapter holy_video_sdl_adapter =
  {
    .name = "SDL Video Driver",
    .id = holy_VIDEO_DRIVER_SDL,

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .init = holy_video_sdl_init,
    .fini = holy_video_sdl_fini,
    .setup = holy_video_sdl_setup,
    .get_info = holy_video_fb_get_info,
    .set_palette = holy_video_sdl_set_palette,
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
    .swap_buffers = holy_video_sdl_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_sdl_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(sdl)
{
  holy_video_register (&holy_video_sdl_adapter);
}

holy_MOD_FINI(sdl)
{
  holy_video_unregister (&holy_video_sdl_adapter);
}
