#include <holy/machine/memory.h>

/*
 * Note: These functions defined in this file may be called from C.
 *       Be careful of that you must not modify some registers. Quote
 *       from gcc-2.95.2/gcc/config/i386/i386.h:

   1 for registers not available across function calls.
   These must include the FIXED_REGISTERS and also any
   registers that can be used without being saved.
   The latter must include the registers where values are returned
   and the register where structure-value addresses are passed.
   Aside from that, you can include as many other registers as you like.

  ax,dx,cx,bx,si,di,bp,sp,st,st1,st2,st3,st4,st5,st6,st7,arg
{  1, 1, 1, 0, 0, 0, 0, 1, 1,  1,  1,  1,  1,  1,  1,  1,  1 }
 */

/*
 * Note: holy is compiled with the options -mrtd and -mregparm=3.
 *       So the first three arguments are passed in %eax, %edx, and %ecx,
 *       respectively, and if a function has a fixed number of arguments
 *       and the number if greater than three, the function must return
 *       with "ret $N" where N is ((the number of arguments) - 3) * 4.
 */

/*
 *  This is the area for all of the special variables.
 */

protstack:
	.long	holy_MEMORY_MACHINE_PROT_STACK

	.macro PROT_TO_REAL
	call	prot_to_real
	.endm

	.macro REAL_TO_PROT
	calll	real_to_prot
	.endm

/*
 * This is the Global Descriptor Table
 *
 *  An entry, a "Segment Descriptor", looks like this:
 *
 * 31          24         19   16                 7           0
 * ------------------------------------------------------------
 * |             | |B| |A|       | |   |1|0|E|W|A|            |
 * | BASE 31..24 |G|/|L|V| LIMIT |P|DPL|  TYPE   | BASE 23:16 |  4
 * |             | |D| |L| 19..16| |   |1|1|C|R|A|            |
 * ------------------------------------------------------------
 * |                             |                            |
 * |        BASE 15..0           |       LIMIT 15..0          |  0
 * |                             |                            |
 * ------------------------------------------------------------
 *
 *  Note the ordering of the data items is reversed from the above
 *  description.
 */

	.p2align	5	/* force 4-byte alignment */
gdt:
	.word	0, 0
	.byte	0, 0, 0, 0

	/* -- code segment --
	 * base = 0x00000000, limit = 0xFFFFF (4 KiB Granularity), present
	 * type = 32bit code execute/read, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x9A, 0xCF, 0

	/* -- data segment --
	 * base = 0x00000000, limit 0xFFFFF (4 KiB Granularity), present
	 * type = 32 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0xCF, 0

	/* -- 16 bit real mode CS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit code execute/read only/conforming, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x9E, 0, 0

	/* -- 16 bit real mode DS --
	 * base = 0x00000000, limit 0x0FFFF (1 B Granularity), present
	 * type = 16 bit data read/write, DPL = 0
	 */
	.word	0xFFFF, 0
	.byte	0, 0x92, 0, 0


	.p2align 5
/* this is the GDT descriptor */
gdtdesc:
	.word	0x27			/* limit */
	.long	gdt			/* addr */
LOCAL(realidt):
	.word 0x400
	.long 0
protidt:
	.word 0
	.long 0

/*
 *  These next two routines, "real_to_prot" and "prot_to_real" are structured
 *  in a very specific way.  Be very careful when changing them.
 *
 *  NOTE:  Use of either one messes up %eax and %ebp.
 */

real_to_prot:
	.code16
	cli

	/* load the GDT register */
	xorw	%ax, %ax
	movw	%ax, %ds
#ifdef holy_MACHINE_QEMU
	/*
	qemu is special: gdtdesc is in ROM.
	%cs = 0xf000
	_start + holy_BOOT_MACHINE_SIZE = 0x100000
	So
	_start + holy_BOOT_MACHINE_SIZE - 0x10000 points to the same point
	as %cs.
	gdtdesc - (_start + holy_BOOT_MACHINE_SIZE - 0x10000)
	= gdtdesc - _start - holy_BOOT_MACHINE_SIZE + 0x10000
	but the later can be computed by assembly.
	*/
	lgdtl	%cs:(gdtdesc - _start - holy_BOOT_MACHINE_SIZE + 0x10000)
#else
	lgdtl	gdtdesc
#endif

	/* turn on protected mode */
	movl	%cr0, %eax
	orl	$holy_MEMORY_CPU_CR0_PE_ON, %eax
	movl	%eax, %cr0

	/* jump to relocation, flush prefetch queue, and reload %cs */
	ljmpl	$holy_MEMORY_MACHINE_PROT_MODE_CSEG, $protcseg

	.code32
protcseg:
	/* reload other segment registers */
	movw	$holy_MEMORY_MACHINE_PROT_MODE_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* put the return address in a known safe location */
	movl	(%esp), %eax
	movl	%eax, holy_MEMORY_MACHINE_REAL_STACK

	/* get protected mode stack */
	movl	protstack, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* get return address onto the right stack */
	movl	holy_MEMORY_MACHINE_REAL_STACK, %eax
	movl	%eax, (%esp)

	/* zero %eax */
	xorl	%eax, %eax

	sidt LOCAL(realidt)
	lidt protidt

	/* return on the old (or initialized) stack! */
	ret
	/* prot_to_real assumes that this code is under 64K which is not
	true for qemu.  */
#ifndef holy_MACHINE_QEMU

prot_to_real:
	/* just in case, set GDT */
	lgdt	gdtdesc

	sidt    protidt
	lidt    LOCAL(realidt)

	/* save the protected mode stack */
	movl	%esp, %eax
	movl	%eax, protstack

	/* get the return address */
	movl	(%esp), %eax
	movl	%eax, holy_MEMORY_MACHINE_REAL_STACK

	/* set up new stack */
	movl	$holy_MEMORY_MACHINE_REAL_STACK, %eax
	movl	%eax, %esp
	movl	%eax, %ebp

	/* set up segment limits */
	movw	$holy_MEMORY_MACHINE_PSEUDO_REAL_DSEG, %ax
	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

	/* this might be an extra step */
	/* jump to a 16 bit segment */
	ljmp	$holy_MEMORY_MACHINE_PSEUDO_REAL_CSEG, $tmpcseg

tmpcseg:
	.code16

	/* clear the PE bit of CR0 */
	movl	%cr0, %eax
	andl 	$(~holy_MEMORY_CPU_CR0_PE_ON), %eax
	movl	%eax, %cr0

	/* flush prefetch queue, reload %cs */
	ljmpl	$0, $realcseg

realcseg:
	/* we are in real mode now
	 * set up the real mode segment registers : DS, SS, ES
	 */
	/* zero %eax */
	xorl	%eax, %eax

	movw	%ax, %ds
	movw	%ax, %es
	movw	%ax, %fs
	movw	%ax, %gs
	movw	%ax, %ss

#ifdef holy_MACHINE_PCBIOS
	/* restore interrupts */
	sti
#endif

	/* return on new stack! */
	retl
#endif
	.code32
