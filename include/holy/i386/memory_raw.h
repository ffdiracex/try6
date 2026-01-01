/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MEMORY_CPU_RAW_HEADER
#define holy_MEMORY_CPU_RAW_HEADER	1

/* The scratch buffer used in real mode code.  */
#define holy_MEMORY_MACHINE_SCRATCH_ADDR	0x68000
#define holy_MEMORY_MACHINE_SCRATCH_SEG	(holy_MEMORY_MACHINE_SCRATCH_ADDR >> 4)
#define holy_MEMORY_MACHINE_SCRATCH_SIZE	0x9000

/* The real mode stack.  */
#define holy_MEMORY_MACHINE_REAL_STACK	(0x2000 - 0x10)

/* The size of the protect mode stack.  */
#define holy_MEMORY_MACHINE_PROT_STACK_SIZE	0xf000

/* The protected mode stack.  */
#define holy_MEMORY_MACHINE_PROT_STACK	\
	(holy_MEMORY_MACHINE_SCRATCH_ADDR + holy_MEMORY_MACHINE_SCRATCH_SIZE \
	 + holy_MEMORY_MACHINE_PROT_STACK_SIZE - 0x10)

/* The memory area where holy uses its own purpose. This part is not added
   into free memory for dynamic allocations.  */
#define holy_MEMORY_MACHINE_RESERVED_START	\
	holy_MEMORY_MACHINE_SCRATCH_ADDR
#define holy_MEMORY_MACHINE_RESERVED_END	\
	(holy_MEMORY_MACHINE_PROT_STACK + 0x10)

/* The code segment of the protected mode.  */
#define holy_MEMORY_MACHINE_PROT_MODE_CSEG	0x8

/* The data segment of the protected mode.  */
#define holy_MEMORY_MACHINE_PROT_MODE_DSEG	0x10

/* The code segment of the pseudo real mode.  */
#define holy_MEMORY_MACHINE_PSEUDO_REAL_CSEG	0x18

/* The data segment of the pseudo real mode.  */
#define holy_MEMORY_MACHINE_PSEUDO_REAL_DSEG	0x20

#endif
