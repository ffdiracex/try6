#include <config.h>
#include <holy/symbol.h>

        .file   "startup.S"
        .text
        .globl  start, _start
        .code64

start:
_start:
	movq	%rcx, EXT_C(holy_efi_image_handle)(%rip)
	movq	%rdx, EXT_C(holy_efi_system_table)(%rip)

	andq	$~0xf, %rsp
	call	EXT_C(holy_main)
	/* Doesn't return.  */
