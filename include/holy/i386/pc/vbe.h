/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_VBE_MACHINE_HEADER
#define holy_VBE_MACHINE_HEADER	1

#include <holy/video.h>

/* Default video mode to be used.  */
#define holy_VBE_DEFAULT_VIDEO_MODE     0x101

/* VBE status codes.  */
#define holy_VBE_STATUS_OK		0x004f

#define holy_VBE_CAPABILITY_DACWIDTH	(1 << 0)

/* Bits from the holy_VBE "mode_attributes" field in the mode info struct.  */
#define holy_VBE_MODEATTR_SUPPORTED                 (1 << 0)
#define holy_VBE_MODEATTR_RESERVED_1                (1 << 1)
#define holy_VBE_MODEATTR_BIOS_TTY_OUTPUT_SUPPORT   (1 << 2)
#define holy_VBE_MODEATTR_COLOR                     (1 << 3)
#define holy_VBE_MODEATTR_GRAPHICS                  (1 << 4)
#define holy_VBE_MODEATTR_VGA_COMPATIBLE            (1 << 5)
#define holy_VBE_MODEATTR_VGA_WINDOWED_AVAIL        (1 << 6)
#define holy_VBE_MODEATTR_LFB_AVAIL                 (1 << 7)
#define holy_VBE_MODEATTR_DOUBLE_SCAN_AVAIL         (1 << 8)
#define holy_VBE_MODEATTR_INTERLACED_AVAIL          (1 << 9)
#define holy_VBE_MODEATTR_TRIPLE_BUF_AVAIL          (1 << 10)
#define holy_VBE_MODEATTR_STEREO_AVAIL              (1 << 11)
#define holy_VBE_MODEATTR_DUAL_DISPLAY_START        (1 << 12)

/* Values for the holy_VBE memory_model field in the mode info struct.  */
#define holy_VBE_MEMORY_MODEL_TEXT           0x00
#define holy_VBE_MEMORY_MODEL_CGA            0x01
#define holy_VBE_MEMORY_MODEL_HERCULES       0x02
#define holy_VBE_MEMORY_MODEL_PLANAR         0x03
#define holy_VBE_MEMORY_MODEL_PACKED_PIXEL   0x04
#define holy_VBE_MEMORY_MODEL_NONCHAIN4_256  0x05
#define holy_VBE_MEMORY_MODEL_DIRECT_COLOR   0x06
#define holy_VBE_MEMORY_MODEL_YUV            0x07

/* Note:

   Please refer to VESA BIOS Extension 3.0 Specification for more descriptive
   meanings of following structures and how they should be used.

   I have tried to maintain field name compatibility against specification
   while following naming conventions used in holy.  */

typedef holy_uint32_t holy_vbe_farptr_t;
typedef holy_uint32_t holy_vbe_physptr_t;
typedef holy_uint32_t holy_vbe_status_t;

struct holy_vbe_info_block
{
  holy_uint8_t signature[4];
  holy_uint16_t version;

  holy_vbe_farptr_t oem_string_ptr;
  holy_uint32_t capabilities;
  holy_vbe_farptr_t video_mode_ptr;
  holy_uint16_t total_memory;

  holy_uint16_t oem_software_rev;
  holy_vbe_farptr_t oem_vendor_name_ptr;
  holy_vbe_farptr_t oem_product_name_ptr;
  holy_vbe_farptr_t oem_product_rev_ptr;

  holy_uint8_t reserved[222];

  holy_uint8_t oem_data[256];
} holy_PACKED;

struct holy_vbe_mode_info_block
{
  /* Mandatory information for all VBE revisions.  */
  holy_uint16_t mode_attributes;
  holy_uint8_t win_a_attributes;
  holy_uint8_t win_b_attributes;
  holy_uint16_t win_granularity;
  holy_uint16_t win_size;
  holy_uint16_t win_a_segment;
  holy_uint16_t win_b_segment;
  holy_vbe_farptr_t win_func_ptr;
  holy_uint16_t bytes_per_scan_line;

  /* Mandatory information for VBE 1.2 and above.  */
  holy_uint16_t x_resolution;
  holy_uint16_t y_resolution;
  holy_uint8_t x_char_size;
  holy_uint8_t y_char_size;
  holy_uint8_t number_of_planes;
  holy_uint8_t bits_per_pixel;
  holy_uint8_t number_of_banks;
  holy_uint8_t memory_model;
  holy_uint8_t bank_size;
  holy_uint8_t number_of_image_pages;
  holy_uint8_t reserved;

  /* Direct Color fields (required for direct/6 and YUV/7 memory models).  */
  holy_uint8_t red_mask_size;
  holy_uint8_t red_field_position;
  holy_uint8_t green_mask_size;
  holy_uint8_t green_field_position;
  holy_uint8_t blue_mask_size;
  holy_uint8_t blue_field_position;
  holy_uint8_t rsvd_mask_size;
  holy_uint8_t rsvd_field_position;
  holy_uint8_t direct_color_mode_info;

  /* Mandatory information for VBE 2.0 and above.  */
  holy_vbe_physptr_t phys_base_addr;
  holy_uint32_t reserved2;
  holy_uint16_t reserved3;

  /* Mandatory information for VBE 3.0 and above.  */
  holy_uint16_t lin_bytes_per_scan_line;
  holy_uint8_t bnk_number_of_image_pages;
  holy_uint8_t lin_number_of_image_pages;
  holy_uint8_t lin_red_mask_size;
  holy_uint8_t lin_red_field_position;
  holy_uint8_t lin_green_mask_size;
  holy_uint8_t lin_green_field_position;
  holy_uint8_t lin_blue_mask_size;
  holy_uint8_t lin_blue_field_position;
  holy_uint8_t lin_rsvd_mask_size;
  holy_uint8_t lin_rsvd_field_position;
  holy_uint32_t max_pixel_clock;

  /* Reserved field to make structure to be 256 bytes long, VESA BIOS
     Extension 3.0 Specification says to reserve 189 bytes here but
     that doesn't make structure to be 256 bytes.  So additional one is
     added here.  */
  holy_uint8_t reserved4[189 + 1];
} holy_PACKED;

struct holy_vbe_crtc_info_block
{
  holy_uint16_t horizontal_total;
  holy_uint16_t horizontal_sync_start;
  holy_uint16_t horizontal_sync_end;
  holy_uint16_t vertical_total;
  holy_uint16_t vertical_sync_start;
  holy_uint16_t vertical_sync_end;
  holy_uint8_t flags;
  holy_uint32_t pixel_clock;
  holy_uint16_t refresh_rate;
  holy_uint8_t reserved[40];
} holy_PACKED;

struct holy_vbe_palette_data
{
  holy_uint8_t blue;
  holy_uint8_t green;
  holy_uint8_t red;
  holy_uint8_t alignment;
} holy_PACKED;

struct holy_vbe_flat_panel_info
{
  holy_uint16_t horizontal_size;
  holy_uint16_t vertical_size;
  holy_uint16_t panel_type;
  holy_uint8_t red_bpp;
  holy_uint8_t green_bpp;
  holy_uint8_t blue_bpp;
  holy_uint8_t reserved_bpp;
  holy_uint32_t reserved_offscreen_mem_size;
  holy_vbe_farptr_t reserved_offscreen_mem_ptr;

  holy_uint8_t reserved[14];
} holy_PACKED;

/* Prototypes for helper functions.  */
/* Call VESA BIOS 0x4f00 to get VBE Controller Information, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_controller_info (struct holy_vbe_info_block *controller_info);
/* Call VESA BIOS 0x4f01 to get VBE Mode Information, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_mode_info (holy_uint32_t mode,
			     struct holy_vbe_mode_info_block *mode_info);
/* Call VESA BIOS 0x4f03 to return current VBE Mode, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_mode (holy_uint32_t *mode);
/* Call VESA BIOS 0x4f05 to set memory window, return status.  */
holy_vbe_status_t
holy_vbe_bios_set_memory_window (holy_uint32_t window, holy_uint32_t position);
/* Call VESA BIOS 0x4f05 to return memory window, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_memory_window (holy_uint32_t window,
				 holy_uint32_t *position);
/* Call VESA BIOS 0x4f06 to set scanline length (in bytes), return status.  */
holy_vbe_status_t
holy_vbe_bios_set_scanline_length (holy_uint32_t length);
/* Call VESA BIOS 0x4f06 to return scanline length (in bytes), return status.  */
holy_vbe_status_t
holy_vbe_bios_get_scanline_length (holy_uint32_t *length);
/* Call VESA BIOS 0x4f07 to get display start, return status.  */
holy_vbe_status_t
holy_vbe_bios_get_display_start (holy_uint32_t *x,
				 holy_uint32_t *y);

holy_vbe_status_t holy_vbe_bios_getset_dac_palette_width (int set, int *width);

#define holy_vbe_bios_get_dac_palette_width(width)	holy_vbe_bios_getset_dac_palette_width(0, (width))
#define holy_vbe_bios_set_dac_palette_width(width)	holy_vbe_bios_getset_dac_palette_width(1, (width))

holy_err_t holy_vbe_probe (struct holy_vbe_info_block *info_block);
holy_err_t holy_vbe_get_video_mode (holy_uint32_t *mode);
holy_err_t holy_vbe_get_video_mode_info (holy_uint32_t mode,
                                         struct holy_vbe_mode_info_block *mode_info);
holy_vbe_status_t
holy_vbe_bios_get_pm_interface (holy_uint16_t *seg, holy_uint16_t *offset,
				holy_uint16_t *length);


#endif /* ! holy_VBE_MACHINE_HEADER */
