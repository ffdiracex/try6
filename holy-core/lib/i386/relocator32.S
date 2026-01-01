#define CODE_SEGMENT	0x10

/* The data segment of the protected mode.  */
#define DATA_SEGMENT	0x18

#include "relocator_common.S"

	.p2align	4	/* force 16-byte alignment */

VARIABLE(holy_relocator32_start)
	PREAMBLE

	RELOAD_GDT
	.code32
	/* Update other registers. */
	movl	$DATA_SEGMENT, %eax
	movl	%eax, %ds
	movl	%eax, %es
	movl	%eax, %fs
	movl	%eax, %gs
	movl	%eax, %ss

	DISABLE_PAGING

#ifdef __x86_64__
	/* Disable amd64. */
	movl	$holy_MEMORY_CPU_AMD64_MSR, %ecx
	rdmsr
	andl	$(~holy_MEMORY_CPU_AMD64_MSR_ON), %eax
	wrmsr
#endif

	/* Turn off PAE. */
	movl	%cr4, %eax
	andl	$(~holy_MEMORY_CPU_CR4_PAE_ON), %eax
	movl	%eax, %cr4

	jmp	LOCAL(cont2)
LOCAL(cont2):
	.code32

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator32_esp)
	.long	0

	movl	%eax, %esp

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator32_ebp)
	.long	0

	movl	%eax, %ebp

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator32_esi)
	.long	0

	movl	%eax, %esi

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator32_edi)
	.long	0

	movl	%eax, %edi

	/* mov imm32, %eax */
	.byte	0xb8
VARIABLE(holy_relocator32_eax)
	.long	0

	/* mov imm32, %ebx */
	.byte	0xbb
VARIABLE(holy_relocator32_ebx)
	.long	0

	/* mov imm32, %ecx */
	.byte	0xb9
VARIABLE(holy_relocator32_ecx)
	.long	0

	/* mov imm32, %edx */
	.byte	0xba
VARIABLE(holy_relocator32_edx)
	.long	0

	/* Cleared direction flag is of no problem with any current
	   payload and makes this implementation easier.  */
	cld

	.byte	0xea
VARIABLE(holy_relocator32_eip)
	.long	0
	.word	CODE_SEGMENT

	/* GDT. Copied from loader/i386/linux.c. */
	.p2align	4
LOCAL(gdt):
	/* NULL.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Reserved.  */
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

	/* Code segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x9A, 0xCF, 0x00

	/* Data segment.  */
	.byte 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x92, 0xCF, 0x00
LOCAL(gdt_end):

VARIABLE(holy_relocator32_end)
