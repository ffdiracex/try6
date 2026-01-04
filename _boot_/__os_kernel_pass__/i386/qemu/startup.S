#include <config.h>
#include <holy/symbol.h>

#include <holy/machine/memory.h>
#include <holy/machine/kernel.h>

	.text
	.code32
	.globl _start
_start:
	jmp	codestart

	.org holy_KERNEL_I386_QEMU_CORE_ENTRY_ADDR
VARIABLE(holy_core_entry_addr)
	.long	0

codestart:
	/* Relocate to low memory.  First we figure out our location.
	   We will derive the rom start address from it.  */
	call	1f
1:	popl	%esi

	/* Rom size is a multiple of 64 kiB.  With this we get the
	   value of `holy_core_entry_addr' in %esi.  */
	xorw	%si, %si

	movl    $(_edata - _start), %ecx
	movl	$_start, %edi
	cld
	rep
	movsb
	ljmp	$holy_MEMORY_MACHINE_PROT_MODE_CSEG, $1f
1:

	/* clean out the bss */
	movl	$BSS_START_SYMBOL, %edi

	/* compute the bss length */
	movl	$END_SYMBOL, %ecx
	subl	%edi, %ecx
		
	/* clean out */
	xorl	%eax, %eax
	cld
	rep
	stosb

	/*
	 *  Call the start of main body of C code.
	 */
	call	EXT_C(holy_main)

	/* This should never happen.  */
	cli
1:	
	hlt
	jmp 1b
