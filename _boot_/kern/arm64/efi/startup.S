#include <holy/symbol.h>

	.file 	"startup.S"
	.text
FUNCTION(_start)
	/*
	 *  EFI_SYSTEM_TABLE and EFI_HANDLE are passed in x1/x0.
	 */
	ldr	x2, efi_image_handle_val
	str	x0, [x2]
	ldr	x2, efi_system_table_val
	str	x1, [x2]
	ldr	x2, holy_main_val
	br	x2
holy_main_val:
	.quad	EXT_C(holy_main)
efi_system_table_val:
	.quad	EXT_C(holy_efi_system_table)
efi_image_handle_val:
	.quad	EXT_C(holy_efi_image_handle)

