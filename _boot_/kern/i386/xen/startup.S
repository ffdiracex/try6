#include <config.h>
#include <holy/symbol.h>

        .file   "startup.S"
        .text
        .globl  start, _start
        .code32

start:
_start:
	leal	LOCAL(stack_end), %esp
	movl	%esi, EXT_C(holy_xen_start_page_addr)

	call	EXT_C(holy_main)
	/* Doesn't return.  */

	.bss
	.space (1 << 22)
LOCAL(stack_end):	
