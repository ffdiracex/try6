/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_UGA_DRAW_HEADER
#define holy_EFI_UGA_DRAW_HEADER	1

#define holy_EFI_UGA_DRAW_GUID \
  { 0x982c298b, 0xf4fa, 0x41cb, { 0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39 }}

enum holy_efi_uga_blt_operation
{
  holy_EFI_UGA_VIDEO_FILL,
  holy_EFI_UGA_VIDEO_TO_BLT,
  holy_EFI_UGA_BLT_TO_VIDEO,
  holy_EFI_UGA_VIDEO_TO_VIDEO,
  holy_EFI_UGA_BLT_MAX
};

struct holy_efi_uga_pixel
{
  holy_uint8_t Blue;
  holy_uint8_t Green;
  holy_uint8_t Red;
  holy_uint8_t Reserved;
};

struct holy_efi_uga_draw_protocol
{
  holy_efi_status_t
  (*get_mode) (struct holy_efi_uga_draw_protocol *this,
	       holy_uint32_t *width,
	       holy_uint32_t *height,
	       holy_uint32_t *depth,
	       holy_uint32_t *refresh_rate);

  holy_efi_status_t
  (*set_mode) (struct holy_efi_uga_draw_protocol *this,
	       holy_uint32_t width,
	       holy_uint32_t height,
	       holy_uint32_t depth,
	       holy_uint32_t refresh_rate);

  holy_efi_status_t
  (*blt) (struct holy_efi_uga_draw_protocol *this,
	  struct holy_efi_uga_pixel *blt_buffer,
	  enum holy_efi_uga_blt_operation blt_operation,
	  holy_efi_uintn_t src_x,
	  holy_efi_uintn_t src_y,
	  holy_efi_uintn_t dest_x,
	  holy_efi_uintn_t dest_y,
	  holy_efi_uintn_t width,
	  holy_efi_uintn_t height,
	  holy_efi_uintn_t delta);
};
typedef struct holy_efi_uga_draw_protocol holy_efi_uga_draw_protocol_t;

#endif /* ! holy_EFI_UGA_DRAW_HEADER */
