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
#include <holy/cache.h>

#define holy_SIS315PRO_PCIID 0x03251039
#define holy_SIS315PRO_TOTAL_MEMORY_SPACE  0x800000
#define holy_SIS315PRO_MMIO_SPACE 0x1000

static struct
{
  struct holy_video_mode_info mode_info;

  holy_uint8_t *ptr;
  volatile holy_uint8_t *direct_ptr;
  int mapped;
  holy_uint32_t base;
  holy_uint32_t mmiobase;
  volatile holy_uint32_t *mmioptr;
  holy_pci_device_t dev;
  holy_port_t io;
} framebuffer;

static holy_uint8_t
read_sis_cmd (holy_uint8_t addr)
{
  holy_outb (addr, framebuffer.io + 0x44);
  return holy_inb (framebuffer.io + 0x45);
}

static void
write_sis_cmd (holy_uint8_t val, holy_uint8_t addr)
{
  holy_outb (addr, framebuffer.io + 0x44);
  holy_outb (val, framebuffer.io + 0x45);
}

#ifndef TEST
static holy_err_t
holy_video_sis315pro_video_init (void)
{
  /* Reset frame buffer.  */
  holy_memset (&framebuffer, 0, sizeof(framebuffer));

  return holy_video_fb_init ();
}

static holy_err_t
holy_video_sis315pro_video_fini (void)
{
  if (framebuffer.mapped)
    {
      holy_pci_device_unmap_range (framebuffer.dev, framebuffer.ptr,
				   holy_SIS315PRO_TOTAL_MEMORY_SPACE);
      holy_pci_device_unmap_range (framebuffer.dev, framebuffer.direct_ptr,
				   holy_SIS315PRO_TOTAL_MEMORY_SPACE);
    }

  return holy_video_fb_fini ();
}
#endif

#include "sis315_init.c"

#ifndef TEST
/* Helper for holy_video_sis315pro_setup.  */
static int
find_card (holy_pci_device_t dev, holy_pci_id_t pciid, void *data)
{
  int *found = data;
  holy_pci_address_t addr;
  holy_uint32_t class;

  addr = holy_pci_make_address (dev, holy_PCI_REG_CLASS);
  class = holy_pci_read (addr);

  if (((class >> 16) & 0xffff) != holy_PCI_CLASS_SUBCLASS_VGA
      || pciid != holy_SIS315PRO_PCIID)
    return 0;
  
  *found = 1;

  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG0);
  framebuffer.base = holy_pci_read (addr) & holy_PCI_ADDR_MEM_MASK;
  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG1);
  framebuffer.mmiobase = holy_pci_read (addr) & holy_PCI_ADDR_MEM_MASK;
  addr = holy_pci_make_address (dev, holy_PCI_REG_ADDRESS_REG2);
  framebuffer.io = (holy_pci_read (addr) & holy_PCI_ADDR_IO_MASK)
    + holy_MACHINE_PCI_IO_BASE;
  framebuffer.dev = dev;

  return 1;
}
#endif

static holy_err_t
holy_video_sis315pro_setup (unsigned int width, unsigned int height,
			    unsigned int mode_type,
			    unsigned int mode_mask __attribute__ ((unused)))
{
  int depth;
  holy_err_t err;
  int found = 0;
  unsigned i;

#ifndef TEST
  /* Decode depth from mode_type.  If it is zero, then autodetect.  */
  depth = (mode_type & holy_VIDEO_MODE_TYPE_DEPTH_MASK)
          >> holy_VIDEO_MODE_TYPE_DEPTH_POS;

  if ((width != 640 && width != 0) || (height != 480 && height != 0)
      || (depth != 8 && depth != 0))
    return holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		       "Only 640x480x8 is supported");

  holy_pci_iterate (find_card, &found);
  if (!found)
    return holy_error (holy_ERR_IO, "Couldn't find graphics card");
#endif
  /* Fill mode info details.  */
  framebuffer.mode_info.width = 640;
  framebuffer.mode_info.height = 480;
  framebuffer.mode_info.mode_type = (holy_VIDEO_MODE_TYPE_INDEX_COLOR
				     | holy_VIDEO_MODE_TYPE_DOUBLE_BUFFERED
				     | holy_VIDEO_MODE_TYPE_UPDATING_SWAP);
  framebuffer.mode_info.bpp = 8;
  framebuffer.mode_info.bytes_per_pixel = 1;
  framebuffer.mode_info.pitch = 640 * 1;
  framebuffer.mode_info.number_of_colors = 16;
  framebuffer.mode_info.red_mask_size = 0;
  framebuffer.mode_info.red_field_pos = 0;
  framebuffer.mode_info.green_mask_size = 0;
  framebuffer.mode_info.green_field_pos = 0;
  framebuffer.mode_info.blue_mask_size = 0;
  framebuffer.mode_info.blue_field_pos = 0;
  framebuffer.mode_info.reserved_mask_size = 0;
  framebuffer.mode_info.reserved_field_pos = 0;
#ifndef TEST
  framebuffer.mode_info.blit_format
    = holy_video_get_blit_format (&framebuffer.mode_info);
#endif

#ifndef TEST
  if (found && (framebuffer.base == 0 || framebuffer.mmiobase == 0))
    {
      holy_pci_address_t addr;
      /* FIXME: choose address dynamically if needed.   */
      framebuffer.base =     0x40000000;
      framebuffer.mmiobase = 0x04000000;
      framebuffer.io = 0xb300;

      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_ADDRESS_REG0);
      holy_pci_write (addr, framebuffer.base | holy_PCI_ADDR_MEM_PREFETCH);

      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_ADDRESS_REG1);
      holy_pci_write (addr, framebuffer.mmiobase);

      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_ADDRESS_REG2);
      holy_pci_write (addr, framebuffer.io | holy_PCI_ADDR_SPACE_IO);

      /* Set latency.  */
      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_CACHELINE);
      holy_pci_write (addr, 0x80004700);

      /* Enable address spaces.  */
      addr = holy_pci_make_address (framebuffer.dev, holy_PCI_REG_COMMAND);
      holy_pci_write (addr, 0x7);

      addr = holy_pci_make_address (framebuffer.dev, 0x30);
      holy_pci_write (addr, 0x04060001);

      framebuffer.io += holy_MACHINE_PCI_IO_BASE;
    }
#endif


  /* We can safely discard volatile attribute.  */
#ifndef TEST
  framebuffer.ptr
    = holy_pci_device_map_range_cached (framebuffer.dev,
					framebuffer.base,
					holy_SIS315PRO_TOTAL_MEMORY_SPACE);
  framebuffer.direct_ptr
    = holy_pci_device_map_range (framebuffer.dev,
				 framebuffer.base,
				 holy_SIS315PRO_TOTAL_MEMORY_SPACE);
  framebuffer.mmioptr = holy_pci_device_map_range (framebuffer.dev,
						   framebuffer.mmiobase,
						   holy_SIS315PRO_MMIO_SPACE);
#endif
  framebuffer.mapped = 1;

#ifndef TEST
  /* Prevent garbage from appearing on the screen.  */
  holy_memset (framebuffer.ptr, 0,
	       framebuffer.mode_info.height * framebuffer.mode_info.pitch);
  holy_arch_sync_dma_caches (framebuffer.ptr,
			     framebuffer.mode_info.height
			     * framebuffer.mode_info.pitch);
#endif

  holy_outb (holy_VGA_IO_MISC_NEGATIVE_VERT_POLARITY
	     | holy_VGA_IO_MISC_NEGATIVE_HORIZ_POLARITY
	     | holy_VGA_IO_MISC_UPPER_64K
	     | holy_VGA_IO_MISC_EXTERNAL_CLOCK_0
	     | holy_VGA_IO_MISC_28MHZ
	     | holy_VGA_IO_MISC_ENABLE_VRAM_ACCESS
	     | holy_VGA_IO_MISC_COLOR,
	     holy_VGA_IO_MISC_WRITE + holy_MACHINE_PCI_IO_BASE);

  holy_vga_sr_write (0x86, 5);
  for (i = 6; i <= 0x27; i++)
    holy_vga_sr_write (0, i);

  for (i = 0x31; i <= 0x3d; i++)
    holy_vga_sr_write (0, i);

  for (i = 0; i < ARRAY_SIZE (sr_dump); i++)
    holy_vga_sr_write (sr_dump[i].val, sr_dump[i].reg);

  for (i = 0x30; i < 0x40; i++)
    holy_vga_cr_write (0, i);

  holy_vga_cr_write (0x77, 0x40);
  holy_vga_cr_write (0x77, 0x41);
  holy_vga_cr_write (0x00, 0x42);
  holy_vga_cr_write (0x5b, 0x43);
  holy_vga_cr_write (0x00, 0x44);
  holy_vga_cr_write (0x23, 0x48);
  holy_vga_cr_write (0xaa, 0x49);
  holy_vga_cr_write (0x02, 0x37);
  holy_vga_cr_write (0x20, 0x5b);
  holy_vga_cr_write (0x00, 0x83);
  holy_vga_cr_write (0x80, 0x63);

  holy_vga_cr_write (0x0c, holy_VGA_CR_VSYNC_END);
  holy_vga_cr_write (0x5f, holy_VGA_CR_HTOTAL);
  holy_vga_cr_write (0x4f, holy_VGA_CR_HORIZ_END);
  holy_vga_cr_write (0x50, holy_VGA_CR_HBLANK_START);
  holy_vga_cr_write (0x82, holy_VGA_CR_HBLANK_END);
  holy_vga_cr_write (0x54, holy_VGA_CR_HORIZ_SYNC_PULSE_START);
  holy_vga_cr_write (0x80, holy_VGA_CR_HORIZ_SYNC_PULSE_END);
  holy_vga_cr_write (0x0b, holy_VGA_CR_VERT_TOTAL);
  holy_vga_cr_write (0x3e, holy_VGA_CR_OVERFLOW);
  holy_vga_cr_write (0x00, holy_VGA_CR_BYTE_PANNING);
  holy_vga_cr_write (0x40, holy_VGA_CR_CELL_HEIGHT);
  holy_vga_cr_write (0x00, holy_VGA_CR_CURSOR_START);
  holy_vga_cr_write (0x00, holy_VGA_CR_CURSOR_END);
  holy_vga_cr_write (0x00, holy_VGA_CR_START_ADDR_HIGH_REGISTER);
  holy_vga_cr_write (0x00, holy_VGA_CR_START_ADDR_LOW_REGISTER);
  holy_vga_cr_write (0x00, holy_VGA_CR_CURSOR_ADDR_HIGH);
  holy_vga_cr_write (0x00, holy_VGA_CR_CURSOR_ADDR_LOW);
  holy_vga_cr_write (0xea, holy_VGA_CR_VSYNC_START);
  holy_vga_cr_write (0x8c, holy_VGA_CR_VSYNC_END);
  holy_vga_cr_write (0xdf, holy_VGA_CR_VDISPLAY_END);
  holy_vga_cr_write (0x28, holy_VGA_CR_PITCH);
  holy_vga_cr_write (0x40, holy_VGA_CR_UNDERLINE_LOCATION);
  holy_vga_cr_write (0xe7, holy_VGA_CR_VERTICAL_BLANK_START);
  holy_vga_cr_write (0x04, holy_VGA_CR_VERTICAL_BLANK_END);
  holy_vga_cr_write (0xa3, holy_VGA_CR_MODE);
  holy_vga_cr_write (0xff, holy_VGA_CR_LINE_COMPARE);

  holy_vga_cr_write (0x0c, holy_VGA_CR_VSYNC_END);
  holy_vga_cr_write (0x5f, holy_VGA_CR_HTOTAL);
  holy_vga_cr_write (0x4f, holy_VGA_CR_HORIZ_END);
  holy_vga_cr_write (0x50, holy_VGA_CR_HBLANK_START);
  holy_vga_cr_write (0x82, holy_VGA_CR_HBLANK_END);
  holy_vga_cr_write (0x55, holy_VGA_CR_HORIZ_SYNC_PULSE_START);
  holy_vga_cr_write (0x81, holy_VGA_CR_HORIZ_SYNC_PULSE_END);
  holy_vga_cr_write (0x0b, holy_VGA_CR_VERT_TOTAL);
  holy_vga_cr_write (0x3e, holy_VGA_CR_OVERFLOW);
  holy_vga_cr_write (0xe9, holy_VGA_CR_VSYNC_START);
  holy_vga_cr_write (0x8b, holy_VGA_CR_VSYNC_END);
  holy_vga_cr_write (0xdf, holy_VGA_CR_VDISPLAY_END);
  holy_vga_cr_write (0xe7, holy_VGA_CR_VERTICAL_BLANK_START);
  holy_vga_cr_write (0x04, holy_VGA_CR_VERTICAL_BLANK_END);
  holy_vga_cr_write (0x40, holy_VGA_CR_CELL_HEIGHT);
  holy_vga_cr_write (0x50, holy_VGA_CR_PITCH);

  holy_vga_cr_write (0x00, 0x19);
  holy_vga_cr_write (0x00, 0x1a);
  holy_vga_cr_write (0x6c, 0x52);
  holy_vga_cr_write (0x2e, 0x34);
  holy_vga_cr_write (0x00, 0x31);


  holy_vga_cr_write (0, holy_VGA_CR_START_ADDR_HIGH_REGISTER);
  holy_vga_cr_write (0, holy_VGA_CR_START_ADDR_LOW_REGISTER);

  for (i = 0; i < 16; i++)
    holy_vga_write_arx (i, i);
  holy_vga_write_arx (1, holy_VGA_ARX_MODE);
  holy_vga_write_arx (0, holy_VGA_ARX_OVERSCAN);
  holy_vga_write_arx (0, holy_VGA_ARX_COLOR_PLANE_ENABLE);
  holy_vga_write_arx (0, holy_VGA_ARX_HORIZONTAL_PANNING);
  holy_vga_write_arx (0, holy_VGA_ARX_COLOR_SELECT);

  holy_outb (0xff, holy_VGA_IO_PIXEL_MASK + holy_MACHINE_PCI_IO_BASE);

  for (i = 0; i < ARRAY_SIZE (gr); i++)
    holy_vga_gr_write (gr[i], i);

  for (i = 0; i < holy_VIDEO_FBSTD_NUMCOLORS; i++)
    holy_vga_palette_write (i, holy_video_fbstd_colors[i].r,
			    holy_video_fbstd_colors[i].g,
			    holy_video_fbstd_colors[i].b);

#if 1
  {
    if (read_sis_cmd (0x5) != 0xa1)
      write_sis_cmd (0x86, 0x5);
    
    write_sis_cmd (read_sis_cmd (0x20) | 0xa1, 0x20);
    write_sis_cmd (read_sis_cmd (0x1e) | 0xda, 0x1e);

#define IND_SIS_CMDQUEUE_SET            0x26
#define IND_SIS_CMDQUEUE_THRESHOLD      0x27

#define COMMAND_QUEUE_THRESHOLD 0x1F
#define SIS_CMD_QUEUE_RESET             0x01

#define SIS_AGP_CMDQUEUE_ENABLE         0x80  /* 315/330/340 series SR26 */
#define SIS_VRAM_CMDQUEUE_ENABLE        0x40
#define SIS_MMIO_CMD_ENABLE             0x20
#define SIS_CMD_QUEUE_SIZE_512k         0x00
#define SIS_CMD_QUEUE_SIZE_1M           0x04
#define SIS_CMD_QUEUE_SIZE_2M           0x08
#define SIS_CMD_QUEUE_SIZE_4M           0x0C
#define SIS_CMD_QUEUE_RESET             0x01
#define SIS_CMD_AUTO_CORR               0x02


    write_sis_cmd (COMMAND_QUEUE_THRESHOLD, IND_SIS_CMDQUEUE_THRESHOLD);
    write_sis_cmd (SIS_CMD_QUEUE_RESET, IND_SIS_CMDQUEUE_SET);
    framebuffer.mmioptr[0x85C4 / 4] = framebuffer.mmioptr[0x85C8 / 4];
    write_sis_cmd (SIS_MMIO_CMD_ENABLE | SIS_CMD_AUTO_CORR, IND_SIS_CMDQUEUE_SET);
    framebuffer.mmioptr[0x85C0 / 4] = (0x1000000 - (512 * 1024));
  }
#endif

#ifndef TEST
  err = holy_video_fb_setup (mode_type, mode_mask,
			     &framebuffer.mode_info,
			     framebuffer.ptr, NULL, NULL);
  if (err)
    return err;

  /* Copy default palette to initialize emulated palette.  */
  err = holy_video_fb_set_palette (0, holy_VIDEO_FBSTD_EXT_NUMCOLORS,
				   holy_video_fbstd_colors);
#endif
  return err;
}

#ifndef TEST

static holy_err_t
holy_video_sis315pro_swap_buffers (void)
{
  holy_size_t s;
  s = (framebuffer.mode_info.height
       * framebuffer.mode_info.pitch
       * framebuffer.mode_info.bytes_per_pixel);
  holy_video_fb_swap_buffers ();
  holy_arch_sync_dma_caches (framebuffer.ptr, s);
  return holy_ERR_NONE;
}

static holy_err_t
holy_video_sis315pro_get_info_and_fini (struct holy_video_mode_info *mode_info,
					void **framebuf)
{
  holy_memcpy (mode_info, &(framebuffer.mode_info), sizeof (*mode_info));
  *framebuf = (void *) framebuffer.direct_ptr;

  holy_video_fb_fini ();

  return holy_ERR_NONE;
}

static struct holy_video_adapter holy_video_sis315pro_adapter =
  {
    .name = "SIS315PRO Video Driver",
    .id = holy_VIDEO_DRIVER_SIS315PRO,

    .prio = holy_VIDEO_ADAPTER_PRIO_NATIVE,

    .init = holy_video_sis315pro_video_init,
    .fini = holy_video_sis315pro_video_fini,
    .setup = holy_video_sis315pro_setup,
    .get_info = holy_video_fb_get_info,
    .get_info_and_fini = holy_video_sis315pro_get_info_and_fini,
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
    .swap_buffers = holy_video_sis315pro_swap_buffers,
    .create_render_target = holy_video_fb_create_render_target,
    .delete_render_target = holy_video_fb_delete_render_target,
    .set_active_render_target = holy_video_fb_set_active_render_target,
    .get_active_render_target = holy_video_fb_get_active_render_target,

    .next = 0
  };

holy_MOD_INIT(video_sis315pro)
{
  holy_video_register (&holy_video_sis315pro_adapter);
}

holy_MOD_FINI(video_sis315pro)
{
  holy_video_unregister (&holy_video_sis315pro_adapter);
}
#else
int
main ()
{
  holy_video_sis315pro_setup (640, 400, 0, 0);
}
#endif
