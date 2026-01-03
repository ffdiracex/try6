#include <config.h>
#include <holy/symbol.h>

        .file   "startup.S"
        .text
        .globl  start, _start
start:
_start:
	/*
	 *  EFI_SYSTEM_TABLE * and EFI_HANDLE are passed on the stack.
	 */
	movl	4(%esp), %eax
	movl	%eax, EXT_C(holy_efi_image_handle)
	movl	8(%esp), %eax
	movl	%eax, EXT_C(holy_efi_system_table)
	call	EXT_C(holy_main)
	ret
