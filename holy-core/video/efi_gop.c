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
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/edid.h>
#include <holy/efi/graphics_output.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_efi_guid_t graphics_output_guid = holy_EFI_GOP_GUID;
static holy_efi_guid_t active_edid_guid = holy_EFI_EDID_ACTIVE_GUID;
static holy_efi_guid_t discovered_edid_guid = holy_EFI_EDID_DISCOVERED_GUID;
static holy_efi_guid_t efi_var_guid = holy_EFI_GLOBAL_VARIABLE_GUID;
static struct holy_efi_gop *gop;
static unsigned old_mode;
static int restore_needed;
static holy_efi_handle_t gop_handle;

static int
holy_video_gop_iterate (int (*hook) (const struct holy_video_mode_info *info, void *hook_arg), void *hook_arg);

static struct
{
  struct holy_video_mode_info mode_info;
  struct holy_video_render_target *render_target;
  holy_uint8_t *ptr;
  holy_uint8_t *offscreen;
} framebuffer;

static int
check_protocol_hook (const struct holy_video_mode_info *info __attribute__ ((unused)), void *hook_arg)
{
  int *have_usable_mode = hook_arg;
  *have_usable_mode = 1;
  return 1;
}


static int
check_protocol (void)
{
  holy_efi_handle_t *handles;
  holy_efi_uintn_t num_handles, i;
  int have_usable_mode = 0;

  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL,
				    &graphics_output_guid, NULL, &num_handles);
  if (!handles || num_handles == 0)
    return 0;

  for (i = 0; i < num_handles; i++)
    {
      gop_handle = handles[i];
      gop = holy_efi_open_protocol (gop_handle, &graphics_output_guid,
				    holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      holy_video_gop_iterate (check_protocol_hook, &have_usable_mode);
      if (have_usable_mode)
	{
	  holy_free (handles);
	  return 1;
	}
    }

  gop = 0;
  gop_handle = 0;

  return 0;
}

static holy_err_t
holy_video_gop_init (void)
{
  holy_memset (&framebuffer, 0, sizeof(framebuffer));
  return holy_video_fb_init ();
}

static holy_err_t
holy_video_gop_fini (void)
{
  if (restore_needed)
    {
      efi_call_2 (gop->set_mode, gop, old_mode);
      restore_needed = 0;
    }
  holy_free (framebuffer.offscreen);
  framebuffer.offscreen = 0;
  return holy_video_fb_fini ();
}

static int
holy_video_gop_get_bpp (struct holy_efi_gop_mode_info *in)
{
  holy_uint32_t total_mask;
  int i;
  switch (in->pixel_format)
    {
    case holy_EFI_GOT_BGRA8:
    case holy_EFI_GOT_RGBA8:
      return 32;

    case holy_EFI_GOT_BITMASK:
      /* Check overlaps.  */
      if ((in->pixel_bitmask.r & in->pixel_bitmask.g)
	  || (in->pixel_bitmask.r & in->pixel_bitmask.b)
	  || (in->pixel_bitmask.g & in->pixel_bitmask.b)
	  || (in->pixel_bitmask.r & in->pixel_bitmask.a)
	  || (in->pixel_bitmask.g & in->pixel_bitmask.a)
	  || (in->pixel_bitmask.b & in->pixel_bitmask.a))
	return 0;

      total_mask = in->pixel_bitmask.r | in->pixel_bitmask.g
	| in->pixel_bitmask.b | in->pixel_bitmask.a;

      for (i = 31; i >= 0; i--)
	if (total_mask & (1 << i))
	  return i + 1;

      /* Fall through.  */
    default:
      return 0;
    }
}

static void
holy_video_gop_get_bitmask (holy_uint32_t mask, unsigned int *mask_size,
			    unsigned int *field_pos)
{
  int i;
  int last_p;
  for (i = 31; i >= 0; i--)
    if (mask & (1 << i))
      break;
  if (i == -1)
    {
      *mask_size = *field_pos = 0;
      return;
    }
  last_p = i;
  for (; i >= 0; i--)
    if (!(mask & (1 << i)))
      break;
  *field_pos = i + 1;
  *mask_size = last_p - *field_pos + 1;
}

static holy_err_t
holy_video_gop_fill_real_mode_info (unsigned mode,
				    struct holy_efi_gop_mode_info *in,
				    struct holy_video_mode_info *out)
{
  out->mode_number = mode;
  out->number_of_colors = 256;
  out->width = in->width;
  out->height = in->height;
  out->mode_type = holy_VIDEO_MODE_TYPE_RGB;
  out->bpp = holy_video_gop_get_bpp (in);
  out->bytes_per_pixel = out->bpp >> 3;
  if (!out->bpp)
    return holy_error (holy_ERR_IO, "unsupported video mode");
  out->pitch = in->pixels_per_scanline * out->bytes_per_pixel;

  switch (in->pixel_format)
    {
    case holy_EFI_GOT_RGBA8:
      out->red_mask_size = 8;
      out->red_field_pos = 0;
      out->green_mask_size = 8;
      out->green_field_pos = 8;
      out->blue_mask_size = 8;
      out->blue_field_pos = 16;
      out->reserved_mask_size = 8;
      out->reserved_field_pos = 24;
      break;

    case holy_EFI_GOT_BGRA8:
      out->red_mask_size = 8;
      out->red_field_pos = 16;
      out->green_mask_size = 8;
      out->green_field_pos = 8;
      out->blue_mask_size = 8;
      out->blue_field_pos = 0;
      out->reserved_mask_size = 8;
      out->reserved_field_pos = 24;
      break;

    case holy_EFI_GOT_BITMASK:
      holy_video_gop_get_bitmask (in->pixel_bitmask.r, &out->red_mask_size,
				  &out->red_field_pos);
      holy_video_gop_get_bitmask (in->pixel_bitmask.g, &out->green_mask_size,
				  &out->green_field_pos);
      holy_video_gop_get_bitmask (in->pixel_bitmask.b, &out->blue_mask_size,
				  &out->blue_field_pos);
      holy_video_gop_get_bitmask (in->pixel_bitmask.a, &out->reserved_mask_size,
				  &out->reserved_field_pos);
      break;

    default:
      return holy_error (holy_ERR_IO, "unsupported video mode");
    }

  out->blit_format = holy_video_get_blit_format (out);
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_gop_fill_mode_info (unsigned mode,
			       struct holy_efi_gop_mode_info *in,
			       struct holy_video_mode_info *out)
{
  out->mode_number = mode;
  out->number_of_colors = 256;
  out->width = in->width;
  out->height = in->height;
  out->mode_type = holy_VIDEO_MODE_TYPE_RGB;
  out->bytes_per_pixel = sizeof (struct holy_efi_gop_blt_pixel);
  out->bpp = out->bytes_per_pixel << 3;
  out->pitch = in->width * out->bytes_per_pixel;
  out->red_mask_size = 8;
  out->red_field_pos = 16;
  out->green_mask_size = 8;
  out->green_field_pos = 8;
  out->blue_mask_size = 8;
  out->blue_field_pos = 0;
  out->reserved_mask_size = 8;
  out->reserved_field_pos = 24;

  out->blit_format = holy_VIDEO_BLIT_FORMAT_BGRA_8888;
  out->mode_type |= (holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
		     | holy_VIDEO_MODE_TYPE_UPDATING_SWAP);

  return holy_ERR_NONE;
}

static int
holy_video_gop_iterate (int (*hook) (const struct holy_video_mode_info *info, void *hook_arg), void *hook_arg)
{
  unsigned mode;

  for (mode = 0; mode < gop->mode->max_mode; mode++)
    {
      holy_efi_uintn_t size;
      holy_efi_status_t status;
      struct holy_efi_gop_mode_info *info = NULL;
      holy_err_t err;
      struct holy_video_mode_info mode_info;
	 
      status = efi_call_4 (gop->query_mode, gop, mode, &size, &info);

      if (status)
	{
	  info = 0;
	  continue;
	}

      err = holy_video_gop_fill_mode_info (mode, info, &mode_info);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  continue;
	}
      if (hook (&mode_info, hook_arg))
	return 1;
    }
  return 0;
}

static holy_err_t
holy_video_gop_get_edid (struct holy_video_edid_info *edid_info)
{
  struct holy_efi_active_edid *edid;
  holy_size_t copy_size;

  holy_memset (edid_info, 0, sizeof (*edid_info));

  edid = holy_efi_open_protocol (gop_handle, &active_edid_guid,
				 holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!edid || edid->size_of_edid == 0)
    edid = holy_efi_open_protocol (gop_handle, &discovered_edid_guid,
				   holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);

  if (!edid || edid->size_of_edid == 0)
    {
      char edidname[] = "agp-internal-edid";
      holy_size_t datasize;
      holy_uint8_t *data;
      data = holy_efi_get_variable (edidname, &efi_var_guid, &datasize);
      if (data && datasize > 16)
	{
	  copy_size = datasize - 16;
	  if (copy_size > sizeof (*edid_info))
	    copy_size = sizeof (*edid_info);
	  holy_memcpy (edid_info, data + 16, copy_size);
	  holy_free (data);
	  return holy_ERR_NONE;
	}
      return holy_error (holy_ERR_BAD_DEVICE, "EDID information not available");
    }

  copy_size = edid->size_of_edid;
  if (copy_size > sizeof (*edid_info))
    copy_size = sizeof (*edid_info);
  holy_memcpy (edid_info, edid->edid, copy_size);

  return holy_ERR_NONE;
}

static holy_err_t
holy_gop_get_preferred_mode (unsigned int *width, unsigned int *height)
{
  struct holy_video_edid_info edid_info;
  holy_err_t err;

  err = holy_video_gop_get_edid (&edid_info);
  if (err)
    return err;
  err = holy_video_edid_checksum (&edid_info);
  if (err)
    return err;
  err = holy_video_edid_preferred_mode (&edid_info, width, height);
  if (err)
    return err;
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_gop_setup (unsigned int width, unsigned int height,
		      unsigned int mode_type,
		      unsigned int mode_mask __attribute__ ((unused)))
{
  unsigned int depth;
  struct holy_efi_gop_mode_info *info = NULL;
  unsigned best_mode = 0;
  holy_err_t err;
  unsigned bpp;
  int found = 0;
  unsigned long long best_volume = 0;
  unsigned int preferred_width = 0, preferred_height = 0;
  holy_uint8_t *buffer;

  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
    >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if (width == 0 && height == 0)
    {
      err = holy_gop_get_preferred_mode (&preferred_width, &preferred_height);
      if (err || preferred_width >= 4096 || preferred_height >= 4096)
	{
	  preferred_width = 800;
	  preferred_height = 600;
	  holy_errno = holy_ERR_NONE;
	}
    }

  /* Keep current mode if possible.  */
  if (gop->mode->info)
    {
      bpp = holy_video_gop_get_bpp (gop->mode->info);
      if (bpp && ((width == gop->mode->info->width
		   && height == gop->mode->info->height)
		  || (width == 0 && height == 0))
	  && (depth == bpp || depth == 0))
	{
	  holy_dprintf ("video", "GOP: keeping mode %d\n", gop->mode->mode);
	  best_mode = gop->mode->mode;
	  found = 1;
	}
    }
 
  if (!found)
    {
      unsigned mode;
      holy_dprintf ("video", "GOP: %d modes detected\n", gop->mode->max_mode);
      for (mode = 0; mode < gop->mode->max_mode; mode++)
	{
	  holy_efi_uintn_t size;
	  holy_efi_status_t status;
	 
	  status = efi_call_4 (gop->query_mode, gop, mode, &size, &info);
	  if (status)
	    {
	      info = 0;
	      continue;
	    }

	  holy_dprintf ("video", "GOP: mode %d: %dx%d\n", mode, info->width,
			info->height);

	  if (preferred_width && (info->width > preferred_width
				  || info->height > preferred_height))
	    {
	      holy_dprintf ("video", "GOP: mode %d: too large\n", mode);
	      continue;
	    }

	  bpp = holy_video_gop_get_bpp (info);
	  if (!bpp)
	    {
	      holy_dprintf ("video", "GOP: mode %d: incompatible pixel mode\n",
			    mode);
	      continue;
	    }

	  holy_dprintf ("video", "GOP: mode %d: depth %d\n", mode, bpp);

	  if (!(((info->width == width && info->height == height)
		|| (width == 0 && height == 0))
		&& (bpp == depth || depth == 0)))
	    {
	      holy_dprintf ("video", "GOP: mode %d: rejected\n", mode);
	      continue;
	    }

	  if (best_volume < ((unsigned long long) info->width)
	      * ((unsigned long long) info->height)
	      * ((unsigned long long) bpp))
	    {
	      best_volume = ((unsigned long long) info->width)
		* ((unsigned long long) info->height)
		* ((unsigned long long) bpp);
	      best_mode = mode;
	    }
	  found = 1;
	}
    }

  if (!found)
    {
      holy_dprintf ("video", "GOP: no mode found\n");
      return holy_error (holy_ERR_UNKNOWN_DEVICE, "no matching mode found");
    }

  if (best_mode != gop->mode->mode)
    {
      if (!restore_needed)
	{
	  old_mode = gop->mode->mode;
	  restore_needed = 1;
	}
      efi_call_2 (gop->set_mode, gop, best_mode);
    }

  info = gop->mode->info;

  err = holy_video_gop_fill_mode_info (gop->mode->mode, info,
				       &framebuffer.mode_info);
  if (err)
    {
      holy_dprintf ("video", "GOP: couldn't fill mode info\n");
      return err;
    }

  framebuffer.ptr = (void *) (holy_addr_t) gop->mode->fb_base;
  framebuffer.offscreen
    = holy_malloc (framebuffer.mode_info.height
		   * framebuffer.mode_info.width 
		   * sizeof (struct holy_efi_gop_blt_pixel));

  buffer = framebuffer.offscreen;
      
  if (!buffer)
    {
      holy_dprintf ("video", "GOP: couldn't allocate shadow\n");
      holy_errno = 0;
      err = holy_video_gop_fill_mode_info (gop->mode->mode, info,
					   &framebuffer.mode_info);
      buffer = framebuffer.ptr;
    }
    
  holy_dprintf ("video", "GOP: initialising FB @ %p %dx%dx%d\n",
		framebuffer.ptr, framebuffer.mode_info.width,
		framebuffer.mode_info.height, framebuffer.mode_info.bpp);
 
  err = holy_video_fb_create_render_target_from_pointer
    (&framebuffer.render_target, &framebuffer.mode_info, buffer);

  if (err)
    {
      holy_dprintf ("video", "GOP: Couldn't create FB target\n");
      return err;
    }
 
  err = holy_video_fb_set_active_render_target (framebuffer.render_target);
 
  if (err)
    {
      holy_dprintf ("video", "GOP: Couldn't set FB target\n");
      return err;
    }
 
  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				   holy_video_fbstd_colors);

  if (err)
    holy_dprintf ("video", "GOP: Couldn't set palette\n");
  else
    holy_dprintf ("video", "GOP: Success\n");
 
  return err;
}

static holy_err_t
holy_video_gop_swap_buffers (void)
{
  if (framebuffer.offscreen)
    {
      efi_call_10 (gop->blt, gop, framebuffer.offscreen,
		   holy_EFI_BLT_BUFFER_TO_VIDEO, 0, 0, 0, 0,
		   framebuffer.mode_info.width, framebuffer.mode_info.height,
		   framebuffer.mode_info.width * 4);
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_gop_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return holy_video_fb_set_active_render_target (target);
}

static holy_err_t
holy_video_gop_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  holy_err_t err;

  err = holy_video_gop_fill_real_mode_info (gop->mode->mode, gop->mode->info,
					    mode_info);
  if (err)
    {
      holy_dprintf ("video", "GOP: couldn't fill mode info\n");
      return err;
    }

  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  holy_free (framebuffer.offscreen);
  framebuffer.offscreen = 0;

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_gop_adapter =
  {
    .name = "EFI GOP driver",
    .id = holy_VIDEO_DRIVER_EFI_GOP,

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE,

    .init = holy_video_gop_init,
    .fini = holy_video_gop_fini,
    .setup = holy_video_gop_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_gop_get_info_and_fini,
    .get_edid = holy_video_gop_get_edid,
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
    .swap_buffers = holy_video_gop_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_gop_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,
    .iterate = holy_video_gop_iterate,

    .next = 0
  };

holy_MOD_INIT(efi_gop)
{
  if (check_protocol ())
    holy_video_register (&holy_video_gop_adapter);
}

holy_MOD_FINI(efi_gop)
{
  if (restore_needed)
    {
      efi_call_2 (gop->set_mode, gop, old_mode);
      restore_needed = 0;
    }
  if (gop)
    holy_video_unregister (&holy_video_gop_adapter);
}
