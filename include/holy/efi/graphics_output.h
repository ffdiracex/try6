/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_GOP_HEADER
#define holy_EFI_GOP_HEADER	1

/* Based on UEFI specification.  */

#define holy_EFI_GOP_GUID \
  { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a }}

typedef enum
  {
    holy_EFI_GOT_RGBA8,
    holy_EFI_GOT_BGRA8,
    holy_EFI_GOT_BITMASK
  }
  holy_efi_gop_pixel_format_t;

typedef enum
  {
    holy_EFI_BLT_VIDEO_FILL,
    holy_EFI_BLT_VIDEO_TO_BLT_BUFFER,
    holy_EFI_BLT_BUFFER_TO_VIDEO,
    holy_EFI_BLT_VIDEO_TO_VIDEO,
    holy_EFI_BLT_OPERATION_MAX
  }
  holy_efi_gop_blt_operation_t;

struct holy_efi_gop_blt_pixel
{
  holy_uint8_t blue;
  holy_uint8_t green;
  holy_uint8_t red;
  holy_uint8_t reserved;
};

struct holy_efi_gop_pixel_bitmask
{
  holy_uint32_t r;
  holy_uint32_t g;
  holy_uint32_t b;
  holy_uint32_t a;
};

struct holy_efi_gop_mode_info
{
  holy_efi_uint32_t version;
  holy_efi_uint32_t width;
  holy_efi_uint32_t height;
  holy_efi_gop_pixel_format_t pixel_format;
  struct holy_efi_gop_pixel_bitmask pixel_bitmask;
  holy_efi_uint32_t pixels_per_scanline;
};

struct holy_efi_gop_mode
{
  holy_efi_uint32_t max_mode;
  holy_efi_uint32_t mode;
  struct holy_efi_gop_mode_info *info;
  holy_efi_uintn_t info_size;
  holy_efi_physical_address_t fb_base;
  holy_efi_uintn_t fb_size;
};

/* Forward declaration.  */
struct holy_efi_gop;

typedef holy_efi_status_t
(*holy_efi_gop_query_mode_t) (struct holy_efi_gop *this,
			      holy_efi_uint32_t mode_number,
			      holy_efi_uintn_t *size_of_info,
			      struct holy_efi_gop_mode_info **info);

typedef holy_efi_status_t
(*holy_efi_gop_set_mode_t) (struct holy_efi_gop *this,
			    holy_efi_uint32_t mode_number);

typedef holy_efi_status_t
(*holy_efi_gop_blt_t) (struct holy_efi_gop *this,
		       void *buffer,
		       holy_efi_uintn_t operation,
		       holy_efi_uintn_t sx,
		       holy_efi_uintn_t sy,
		       holy_efi_uintn_t dx,
		       holy_efi_uintn_t dy,
		       holy_efi_uintn_t width,
		       holy_efi_uintn_t height,
		       holy_efi_uintn_t delta);

struct holy_efi_gop
{
  holy_efi_gop_query_mode_t query_mode;
  holy_efi_gop_set_mode_t set_mode;
  holy_efi_gop_blt_t blt;
  struct holy_efi_gop_mode *mode;
};

#endif
