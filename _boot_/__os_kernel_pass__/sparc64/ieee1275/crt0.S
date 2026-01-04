#include <holy/symbol.h>
#include <holy/machine/kernel.h>
#include <holy/offsets.h>

	.text
	.align	4
	.globl	_start
_start:
	ba	codestart
	 mov  %o4, %o0

	.org holy_KERNEL_MACHINE_TOTAL_MODULE_SIZE

VARIABLE(holy_total_module_size)
	.word	0

codestart:
	/* Copy the modules past the end of the kernel image.
	 * They are currently sitting in the BSS.
	 */
	sethi	%hi(__bss_start + holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN - 1), %o2
	or	%o2, %lo(__bss_start + holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN - 1), %o2
	srl	%o2, holy_KERNEL_SPARC64_IEEE1275_LOG_MOD_ALIGN, %o2
	sll	%o2, holy_KERNEL_SPARC64_IEEE1275_LOG_MOD_ALIGN, %o2
	sethi	%hi(_end + holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN - 1), %o3
	or	%o3, %lo(_end + holy_KERNEL_SPARC64_IEEE1275_MOD_ALIGN - 1), %o3
	srl	%o3, holy_KERNEL_SPARC64_IEEE1275_LOG_MOD_ALIGN, %o3
	sll	%o3, holy_KERNEL_SPARC64_IEEE1275_LOG_MOD_ALIGN, %o3
	sethi	%hi(holy_total_module_size), %o4
	lduw	[%o4 + %lo(holy_total_module_size)], %o4

	add     %o2, %o4, %o2
	add     %o3, %o4, %o3

	/* Save ieee1275 stack for future use by booter.  */
	mov     %o6, %o1
	/* Our future stack.  */
	sethi	%hi(holy_KERNEL_MACHINE_STACK_SIZE), %o5
	or	%o5, %lo(holy_KERNEL_MACHINE_STACK_SIZE), %o5
	add     %o3, %o5, %o6
	and     %o6, ~0xff, %o6
	sub     %o6, 2047, %o6
	
	sub	%o2, 4, %o2
	sub	%o3, 4, %o3
1:	lduw	[%o2], %o5
	stw	%o5, [%o3]
	subcc	%o4, 4, %o4
	sub	%o2, 4, %o2
	bne,pt	%icc, 1b
	 sub	%o3, 4, %o3

	/* Now it's safe to clear out the BSS.  */
	sethi	%hi(__bss_start), %o2
	or	%o2, %lo(__bss_start), %o2
1:	stb	%g0, [%o2]
	add	%o2, 1, %o2
	and	%o2, 7, %o3
	brnz	%o3, 1b
	 nop

	sethi	%hi(_end - 1), %o3
	or	%o3, %lo(_end - 1), %o3
	srl	%o3, 3, %o3
	sll	%o3, 3, %o3
1:	stx	%g0, [%o2]
	add	%o2, 8, %o2
	cmp	%o2, %o3
	blt,pt	%xcc, 1b
	 nop

	sethi	%hi(_end), %o3
	or	%o3, %lo(_end), %o3
1:	stb	%g0, [%o2]
	add	%o2, 1, %o2
	cmp	%o2, %o3
	blt,pt	%xcc, 1b
	 nop

	sethi	%hi(holy_ieee1275_original_stack), %o2
	stx	%o1, [%o2 + %lo(holy_ieee1275_original_stack)]
	sethi	%hi(holy_ieee1275_entry_fn), %o2
	call	holy_main
	 stx	%o0, [%o2 + %lo(holy_ieee1275_entry_fn)]
1:	ba,a	1b
	 nop
