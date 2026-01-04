#include <config.h>
#include <holy/symbol.h>

        .file   "startup.S"
        .text
        .globl  start, _start
        .code64

start:
_start:
	leaq	LOCAL(stack_end), %rsp
	movq	%rsi, EXT_C(holy_xen_start_page_addr)(%rip)

	andq	$~0xf, %rsp
	call	EXT_C(holy_main)
	/* Doesn't return.  */

	.bss
	.space (1 << 22)
LOCAL(stack_end):	
