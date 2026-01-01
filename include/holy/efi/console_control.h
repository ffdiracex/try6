/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_EFI_CONSOLE_CONTROL_HEADER
#define holy_EFI_CONSOLE_CONTROL_HEADER	1

#define holy_EFI_CONSOLE_CONTROL_GUID	\
  { 0xf42f7782, 0x12e, 0x4c12, \
    { 0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21 } \
  }

enum holy_efi_screen_mode
  {
    holy_EFI_SCREEN_TEXT,
    holy_EFI_SCREEN_GRAPHICS,
    holy_EFI_SCREEN_TEXT_MAX_VALUE
  };
typedef enum holy_efi_screen_mode holy_efi_screen_mode_t;

struct holy_efi_console_control_protocol
{
  holy_efi_status_t
  (*get_mode) (struct holy_efi_console_control_protocol *this,
	       holy_efi_screen_mode_t *mode,
	       holy_efi_boolean_t *uga_exists,
	       holy_efi_boolean_t *std_in_locked);

  holy_efi_status_t
  (*set_mode) (struct holy_efi_console_control_protocol *this,
	       holy_efi_screen_mode_t mode);

  holy_efi_status_t
  (*lock_std_in) (struct holy_efi_console_control_protocol *this,
		  holy_efi_char16_t *password);
};
typedef struct holy_efi_console_control_protocol holy_efi_console_control_protocol_t;

#endif /* ! holy_EFI_CONSOLE_CONTROL_HEADER */
