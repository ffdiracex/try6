#include <holy/symbol.h>
#include <holy/machine/memory.h>
#include <holy/cpu/linux.h>
#include <holy/offsets.h>
#include <multiboot.h>
#include <multiboot2.h>

/*
 * Note: holy is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

	.file	"startup.S"
	.text
	.globl	start, _start
start:
_start:
#ifdef holy_MACHINE_MULTIBOOT
	cmpl	$MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jne 0f
	movl	%ebx, EXT_C(startup_multiboot_info)
0:
#endif

	/* initialize the stack */
	movl $holy_MEMORY_MACHINE_PROT_STACK, %esp

	/* jump to the main body of C code */
	jmp EXT_C(holy_main)

/*
 *  Support for booting holy from a Multiboot boot loader (e.g. holy itself).
 */
	.p2align	2	/* force 4-byte alignment */
multiboot_header:
	/* magic */
	.long	0x1BADB002
	/* flags */
	.long	MULTIBOOT_MEMORY_INFO
	/* checksum */
	.long	-0x1BADB002 - MULTIBOOT_MEMORY_INFO

