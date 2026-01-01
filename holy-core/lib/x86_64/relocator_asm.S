#include <holy/symbol.h>
#include <holy/i386/memory.h>

	.p2align	2

VARIABLE(holy_relocator_backward_start)
	/* mov imm32, %rax */
	.byte	0x48
	.byte	0xb8
VARIABLE(holy_relocator_backward_dest)
	.long	0, 0
	movq	%rax, %rdi

	/* mov imm64, %rax */
	.byte	0x48
	.byte	0xb8
VARIABLE(holy_relocator_backward_src)
	.long	0, 0
	movq	%rax, %rsi

	/* mov imm64, %rcx */
	.byte	0x48
	.byte	0xb9
VARIABLE(holy_relocator_backward_chunk_size)
	.long	0, 0

	add	%rcx, %rsi
	add	%rcx, %rdi

	/* Backward movsb is implicitly off-by-one.  compensate that.  */
	sub	$1,	%rsi
	sub	$1,	%rdi

	/* Backward copy.  */
	std

	rep
	movsb
VARIABLE(holy_relocator_backward_end)


VARIABLE(holy_relocator_forward_start)
	/* mov imm64, %rax */
	.byte	0x48
	.byte	0xb8
VARIABLE(holy_relocator_forward_dest)
	.long	0, 0
	movq	%rax, %rdi

	/* mov imm64, %rax */
	.byte	0x48
	.byte	0xb8
VARIABLE(holy_relocator_forward_src)
	.long	0, 0
	movq	%rax, %rsi

	/* mov imm64, %rcx */
	.byte	0x48
	.byte	0xb9
VARIABLE(holy_relocator_forward_chunk_size)
	.long	0, 0

	/* Forward copy.  */
	cld
	rep
	movsb
VARIABLE(holy_relocator_forward_end)
