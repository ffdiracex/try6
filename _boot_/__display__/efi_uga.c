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
#include <holy/efi/uga_draw.h>
#include <holy/pci.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_efi_guid_t uga_draw_guid = holy_EFI_UGA_DRAW_GUID;
static struct holy_efi_uga_draw_protocol *uga;
static holy_uint32_t uga_fb;
static holy_uint32_t uga_pitch;

static struct
{
  struct holy_video_mode_info mode_info;
  struct holy_video_render_target *render_target;
  holy_uint8_t *ptr;
} framebuffer;

#define RGB_MASK	0xffffff
#define RGB_MAGIC	0x121314
#define LINE_MIN	800
#define LINE_MAX	4096
#define FBTEST_STEP	(0x10000 >> 2)
#define FBTEST_COUNT	8

static int
find_line_len (holy_uint32_t *fb_base, holy_uint32_t *line_len)
{
  holy_uint32_t *base = (holy_uint32_t *) (holy_addr_t) *fb_base;
  int i;

  for (i = 0; i < FBTEST_COUNT; i++, base += FBTEST_STEP)
    {
      if ((*base & RGB_MASK) == RGB_MAGIC)
	{
	  int j;

	  for (j = LINE_MIN; j <= LINE_MAX; j++)
	    {
	      if ((base[j] & RGB_MASK) == RGB_MAGIC)
		{
		  *fb_base = (holy_uint32_t) (holy_addr_t) base;
		  *line_len = j << 2;

		  return 1;
		}
	    }

	  break;
	}
    }

  return 0;
}

/* Context for find_framebuf.  */
struct find_framebuf_ctx
{
  holy_uint32_t *fb_base;
  holy_uint32_t *line_len;
  int found;
};

/* Helper for find_framebuf.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  struct find_framebuf_ctx *ctx = data;
  holy_pci_address_t addr;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  if (holy_pci_read (addr) >> 24 == 0x3)
    {
      int i;

      holy_dprintf ("fb", "Display controller: %d:%d.%d\nDevice id: %x\n",
		    holy_pci_get_bus (dev), holy_pci_get_device (dev),
		    holy_pci_get_function (dev), pciid);
      addr += 8;
      for (i = 0; i < 6; i++, addr += 4)
	{
	  holy_uint32_t old_bar1, old_bar2, type;
	  holy_uint64_t base64;

	  old_bar1 = holy_pci_read (addr);
	  if ((! old_bar1) || (old_bar1 & holy_PCI_ADDR_SPACE_IO))
	    continue;

	  type = old_bar1 & holy_PCI_ADDR_MEM_TYPE_MASK;
	  if (type == holy_PCI_ADDR_MEM_TYPE_64)
	    {
	      if (i == 5)
		break;

	      old_bar2 = holy_pci_read (addr + 4);
	    }
	  else
	    old_bar2 = 0;

	  base64 = old_bar2;
	  base64 <<= 32;
	  base64 |= (old_bar1 & holy_PCI_ADDR_MEM_MASK);

	  holy_dprintf ("fb", "%s(%d): 0x%llx\n",
			((old_bar1 & holy_PCI_ADDR_MEM_PREFETCH) ?
			"VMEM" : "MMIO"), i,
		       (unsigned long long) base64);

	  if ((old_bar1 & holy_PCI_ADDR_MEM_PREFETCH) && (! ctx->found))
	    {
	      *ctx->fb_base = base64;
	      if (find_line_len (ctx->fb_base, ctx->line_len))
		ctx->found++;
	    }

	  if (type == holy_PCI_ADDR_MEM_TYPE_64)
	    {
	      i++;
	      addr += 4;
	    }
	}
    }

  return ctx->found;
}

static int
find_framebuf (holy_uint32_t *fb_base, holy_uint32_t *line_len)
{
  struct find_framebuf_ctx ctx = {
    .fb_base = fb_base,
    .line_len = line_len,
    .found = 0
  };

  holy_pci_iterate (find_card, &ctx);
  return ctx.found;
}

static int
check_protocol (void)
{
  holy_efi_uga_draw_protocol_t *c;

  c = holy_efi_locate_protocol (&uga_draw_guid, 0);
  if (c)
    {
      holy_uint32_t width, height, depth, rate, pixel;
      int ret;

      if (efi_call_5 (c->get_mode, c, &width, &height, &depth, &rate))
	return 0;

      holy_efi_set_text_mode (0);
      pixel = RGB_MAGIC;
      efi_call_10 (c->blt, c, (struct holy_efi_uga_pixel *) &pixel,
		   holy_EFI_UGA_VIDEO_FILL, 0, 0, 0, 0, 1, height, 0);
      ret = find_framebuf (&uga_fb, &uga_pitch);
      holy_efi_set_text_mode (1);

      if (ret)
	{
	  uga = c;
	  return 1;
	}
    }

  return 0;
}

static holy_err_t
holy_video_uga_init (void)
{
  holy_memset (&framebuffer, 0, sizeof(framebuffer));
  return holy_video_fb_init ();
}

static holy_err_t
holy_video_uga_fini (void)
{
  return holy_video_fb_fini ();
}

static holy_err_t
holy_video_uga_setup (unsigned int width, unsigned int height,
		      unsigned int mode_type,
		      unsigned int mode_mask __attribute__ ((unused)))
{
  unsigned int depth;
  int found = 0;

  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
    >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  {
    holy_uint32_t w;
    holy_uint32_t h;
    holy_uint32_t d;
    holy_uint32_t r;

    if ((! efi_call_5 (uga->get_mode, uga, &w, &h, &d, &r)) &&
	((! width) || (width == w)) &&
	((! height) || (height == h)) &&
	((! depth) || (depth == d)))
      {
	framebuffer.mode_info.width = w;
	framebuffer.mode_info.height = h;
	framebuffer.mode_info.pitch = uga_pitch;
	framebuffer.ptr = (holy_uint8_t *) (holy_addr_t) uga_fb;

	found = 1;
      }
  }

  if (found)
    {
      holy_err_t err;

      framebuffer.mode_info.mode_type = holy_VIDEO_MODE_TYPE_RGB;
      framebuffer.mode_info.bpp = 32;
      framebuffer.mode_info.bytes_per_pixel = 4;
      framebuffer.mode_info.number_of_colors = 256;
      framebuffer.mode_info.red_mask_size = 8;
      framebuffer.mode_info.red_field_pos = 16;
      framebuffer.mode_info.green_mask_size = 8;
      framebuffer.mode_info.green_field_pos = 8;
      framebuffer.mode_info.blue_mask_size = 8;
      framebuffer.mode_info.blue_field_pos = 0;
      framebuffer.mode_info.reserved_mask_size = 8;
      framebuffer.mode_info.reserved_field_pos = 24;

      framebuffer.mode_info.blit_format =
	holy_video_get_blit_format (&framebuffer.mode_info);

      err = holy_video_fb_create_render_target_from_pointer
	(&framebuffer.render_target,
	 &framebuffer.mode_info,
	 framebuffer.ptr);

      if (err)
	return err;

      err = holy_video_fb_set_active_render_target
	(framebuffer.render_target);

      if (err)
	return err;

      err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_NUMCOLORS,
				       holy_video_fbstd_colors);

      return err;
    }

  return holy_error (holy_ERR_UNKNOWN_DEVICE, "no matching mode found");
}

static holy_err_t
holy_video_uga_swap_buffers (void)
{
  /* TODO: Implement buffer swapping.  */
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_uga_set_active_render_target (struct holy_video_render_target *target)
{
  if (target == holy_VIDEO_RENDER_TARGET_DISPLAY)
    target = framebuffer.render_target;

  return holy_video_fb_set_active_render_target (target);
}

static holy_err_t
holy_video_uga_get_info_and_fini (struct holy_video_mode_info *mode_info,
				  void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (char *) framebuffer.ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_uga_adapter =
  {
    .name = "EFI UGA driver",
    .id = holy_VIDEO_DRIVER_EFI_UGA,

    .prio = holy_VIDEO_ADAPTER_PRIO_FIRMWARE_DIRTY,

    .init = holy_video_uga_init,
    .fini = holy_video_uga_fini,
    .setup = holy_video_uga_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_uga_get_info_and_fini,
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
    .swap_buffers = holy_video_uga_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_uga_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,
  };

holy_MOD_INIT(efi_uga)
{
  if (check_protocol ())
    holy_video_register (&holy_video_uga_adapter);
}

holy_MOD_FINI(efi_uga)
{
  if (uga)
    holy_video_unregister (&holy_video_uga_adapter);
}
