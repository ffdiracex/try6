/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define holy_video_render_target holy_video_fbrender_target

#include <holy/video.h>
#include <holy/video_fb.h>
#include <holy/mm.h>
#include <holy/misc.h>

static struct
{
  struct holy_video_mode_info mode_info;
  struct holy_video_render_target *render_target;
  holy_uint8_t *ptr;
} framebuffer;

void (*holy_video_capture_refresh_cb) (void);

static holy_err_t
holy_video_capture_swap_buffers (void)
{
  if (holy_video_capture_refresh_cb)
    holy_video_capture_refresh_cb ();
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_capture_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return holy_video_fb_set_active_render_target (target);
}

static holy_err_t
holy_video_capture_fini (void)
{
  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_capture_adapter =
  {
    .name = "Render capture",

    .prio = 0,
    .id = holy_VIDEO_ADAPTER_CAPTURE,

    .fini = holy_video_capture_fini,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = 0,
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
    .swap_buffers = holy_video_capture_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_capture_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

static struct holy_video_adapter *saved;
static struct holy_video_mode_info saved_mode_info;

holy_err_t
holy_video_capture_start (const struct holy_video_mode_info *mode_info,
			  struct holy_video_palette_data *palette,
			  unsigned int palette_size)
{
  holy_err_t err;
  holy_memset (&framebuffer, 0, sizeof (framebuffer));

  holy_video_fb_init ();

  framebuffer.mode_info = *mode_info;
  framebuffer.mode_info.blit_format = holy_video_get_blit_format (&framebuffer.mode_info);

  framebuffer.ptr = holy_malloc (framebuffer.mode_info.height * framebuffer.mode_info.pitch);
  if (!framebuffer.ptr)
    return holy_errno;
  
  err = holy_video_fb_create_render_target_from_pointer (&framebuffer.render_target,
							 &framebuffer.mode_info,
							 framebuffer.ptr);
  if (err)
    return err;
  err = holy_video_fb_set_active_render_target (framebuffer.render_target);
  if (err)
    return err;
  err = holy_video_fb_set_palette (0, palette_size, palette);
  if (err)
    return err;

  saved = holy_video_adapter_active;
  if (saved)
    {
      holy_video_get_info (&saved_mode_info);
      if (saved->fini)
	saved->fini ();
    }
  holy_video_adapter_active = &holy_video_capture_adapter;

  return holy_ERR_NONE;
}

void *
holy_video_capture_get_framebuffer (void)
{
  return framebuffer.ptr;
}

void
holy_video_capture_end (void)
{
  holy_video_fb_delete_render_target (framebuffer.render_target);
  holy_free (framebuffer.ptr);
  holy_video_fb_fini ();
  holy_video_adapter_active = saved;
  if (saved)
    {
      if (saved->init)
	saved->init ();
      if (saved->setup)
	saved->setup (saved_mode_info.width, saved_mode_info.height, 0, 0);
    }
}
