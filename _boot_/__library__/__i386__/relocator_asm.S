#include <holy/symbol.h>
#include <holy/i386/memory.h>

	.p2align	2

VARIABLE(holy_relocator_backward_start)
	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator_backward_dest)
	.long	0
	movl	%eax, %edi

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator_backward_src)
	.long	0
	movl	%eax, %esi

	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE(holy_relocator_backward_chunk_size)
	.long	0
	
	add	%ecx, %esi
	add	%ecx, %edi


	/* Backward movsb is implicitly off-by-one.  compensate that.  */
	sub	$1,	%esi
	sub	$1,	%edi

	/* Backward copy.  */
	std

	rep
	movsb
VARIABLE(holy_relocator_backward_end)


VARIABLE(holy_relocator_forward_start)
	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator_forward_dest)
	.long	0
	movl	%eax, %edi

	/* mov imm32, %rax */
	.byte	0xb8
VARIABLE(holy_relocator_forward_src)
	.long	0
	movl	%eax, %esi

	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE(holy_relocator_forward_chunk_size)
	.long	0

	/* Forward copy.  */
	cld
	rep
	movsb
VARIABLE(holy_relocator_forward_end)
