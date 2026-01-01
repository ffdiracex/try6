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

#define holy_RADEON_YEELOONG3A_TOTAL_MEMORY_SPACE 1048576

static struct
{
  struct holy_video_mode_info mode_info;
  struct holy_video_render_target *render_target;

  holy_uint8_t *ptr;
  int mapped;
  holy_uint32_t base;
  holy_pci_device_t dev;
} framebuffer;

static holy_err_t
holy_video_radeon_yeeloong3a_video_init (void)
{
  /* Reset frame buffer.  */
  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_radeon_yeeloong3a_video_fini (void)
{
  if (framebuffer.mapped)
    holy_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				 holy_RADEON_YEELOONG3A_TOTAL_MEMORY_SPACE);
  return holy_video_fb_fini ();
}

#ifndef TEST
/* Helper for holy_video_radeon_yeeloong3a_setup.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  int *found = data;
  holy_pci_address_t addr;
  holy_uint32_t class;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  if (((class >> 16) & 0xffff) != holy_PCI_CLASS_SUBCLASS_VGA
      || pciid != 0x96151002)
    return 0;
  
  *found = 1;

  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
  framebuffer.base = holy_pci_read (addr);
  framebuffer.dev = dev;

  return 1;
}
#endif

static holy_err_t
holy_video_radeon_yeeloong3a_setup (unsigned int width, unsigned int height,
			unsigned int mode_type, unsigned int mode_mask __attribute__ ((unused)))
{
  int depth;
  holy_err_t err;
  int found = 0;

#ifndef TEST
  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if ((width != 800 && width != 0) || (height != 600 && height != 0)
      || (depth != 16 && depth != 0))
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "Only 640x480x16 is supported");

  holy_pci_iterate (find_card, &found);
  if (!found)
    return holy_error (holy_ERR_IO, "Couldn't find graphics card");
#endif
  /* Fill mode info details.  */
  framebuffer.mode_info.width = 800;
  framebuffer.mode_info.height = 600;
  framebuffer.mode_info.mode_type = holy_VIDEO_MODE_TYPE_RGB;
  framebuffer.mode_info.bpp = 16;
  framebuffer.mode_info.bytes_per_pixel = 2;
  framebuffer.mode_info.pitch = 800 * 2;
  framebuffer.mode_info.number_of_colors = 256;
  framebuffer.mode_info.red_mask_size = 5;
  framebuffer.mode_info.red_field_pos = 11;
  framebuffer.mode_info.green_mask_size = 6;
  framebuffer.mode_info.green_field_pos = 5;
  framebuffer.mode_info.blue_mask_size = 5;
  framebuffer.mode_info.blue_field_pos = 0;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;
#ifndef TEST
  framebuffer.mode_info.blit_format
    = holy_video_get_blit_format (&framebuffer.mode_info);
#endif

  /* We can safely discard volatile attribute.  */
#ifndef TEST
  framebuffer.ptr = (void *) holy_pci_device_map_range (framebuffer.dev,
							framebuffer.base,
							holy_RADEON_YEELOONG3A_TOTAL_MEMORY_SPACE);
  framebuffer.mapped = 1;
#endif

  /* Prevent garbage from appearing on the screen.  */
  holy_memset (framebuffer.ptr, 0,
	       framebuffer.mode_info.height * framebuffer.mode_info.pitch);

#ifndef TEST
  err = holy_video_fb_create_render_target_from_pointer (&framebuffer
							 .render_target,
							 &framebuffer.mode_info,
							 framebuffer.ptr);

  if (err)
    return err;

  err = holy_video_fb_set_active_render_target (framebuffer.render_target);
  
  if (err)
    return err;

  /* Copy default palette to initialize emulated palette.  */
  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				   holy_video_fbstd_colors);
#endif
  return err;
}

static holy_err_t
holy_video_radeon_yeeloong3a_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_radeon_yeeloong3a_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return holy_video_fb_set_active_render_target (target);
}

static holy_err_t
holy_video_radeon_yeeloong3a_get_info_and_fini (struct holy_video_mode_info *mode_info,
				    void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_radeon_yeeloong3a_adapter =
  {
    .name = "RADEON (Yeeloong3a) Video Driver",
    .id = holy_VIDEO_DRIVER_RADEON_YEELOONG3A,

    .prio = holy_VIDEO_ADAPTER_PRIO_NATIVE,

    .init = holy_video_radeon_yeeloong3a_video_init,
    .fini = holy_video_radeon_yeeloong3a_video_fini,
    .setup = holy_video_radeon_yeeloong3a_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_radeon_yeeloong3a_get_info_and_fini,
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
    .swap_buffers = holy_video_radeon_yeeloong3a_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_radeon_yeeloong3a_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(video_radeon_yeeloong3a)
{
  holy_video_register (&holy_video_radeon_yeeloong3a_adapter);
}

holy_MOD_FINI(video_radeon_yeeloong3a)
{
  holy_video_unregister (&holy_video_radeon_yeeloong3a_adapter);
}
