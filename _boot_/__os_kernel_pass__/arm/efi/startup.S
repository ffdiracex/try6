#include <holy/symbol.h>

	.file 	"startup.S"
	.text
	.arm
FUNCTION(_start)
	/*
	 *  EFI_SYSTEM_TABLE and EFI_HANDLE are passed in r1/r0.
	 */
	ldr	ip, =EXT_C(holy_efi_image_handle)
	str	r0, [ip]
	ldr	ip, =EXT_C(holy_efi_system_table)
	str	r1, [ip]
	ldr	ip, =EXT_C(holy_main)
	bx	ip
	END
