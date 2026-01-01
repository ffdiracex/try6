/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_EDID_HEADER
#define holy_EFI_EDID_HEADER	1

/* Based on UEFI specification.  */

#define holy_EFI_EDID_ACTIVE_GUID \
  { 0xbd8c1056, 0x9f36, 0x44ec, { 0x92, 0xa8, 0xa6, 0x33, 0x7f, 0x81, 0x79, 0x86 }}

#define holy_EFI_EDID_DISCOVERED_GUID \
  {0x1c0c34f6,0xd380,0x41fa, {0xa0,0x49,0x8a,0xd0,0x6c,0x1a,0x66,0xaa}}

#define holy_EFI_EDID_OVERRIDE_GUID \
  {0x48ecb431,0xfb72,0x45c0, {0xa9,0x22,0xf4,0x58,0xfe,0x4,0xb,0xd5}}

struct holy_efi_edid_override;

typedef holy_efi_status_t
(*holy_efi_edid_override_get_edid) (struct holy_efi_edid_override *this,
				    holy_efi_handle_t *childhandle,
				    holy_efi_uint32_t *attributes,
				    holy_efi_uintn_t *edidsize,
				    holy_efi_uint8_t *edid);
struct holy_efi_edid_override {
  holy_efi_edid_override_get_edid get_edid;
};

typedef struct holy_efi_edid_override holy_efi_edid_override_t;
  

struct holy_efi_active_edid
{
  holy_uint32_t size_of_edid;
  holy_uint8_t *edid;
};

#endif
