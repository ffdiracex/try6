#include <holy/machine/boot.h>

	.file	"pxeboot.S"
	.text

	/* Start with the prehistoric environment... */
	.code16

	/* Let's go */
.globl start, _start;
_start:
start:

	/* Use drive number 0x7F for PXE */
        movb	$holy_BOOT_MACHINE_PXE_DL, %dl

	/* Jump to the real world */
	ljmp	$0, $0x8200

	/* This region is a junk. Do you say that this is wasteful?
	   But I like that the memory layout of the body is consistent
	   among different kernels rather than scamping just for 1.5KB. */
	.org  0x8200 - 0x7C00 - 0x200 - 1
	.byte	0
