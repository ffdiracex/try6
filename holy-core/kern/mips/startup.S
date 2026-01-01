#include <holy/symbol.h>
#include <holy/offsets.h>
#include <holy/machine/memory.h>
#include <holy/machine/kernel.h>
#include <holy/offsets.h>
#include <holy/mips/asm.h>

#define BASE_ADDR 8	

	.globl __start, _start, start
	.set noreorder
	.set nomacro
__start:
_start:
start:		
.extern __bss_start
.extern _end
	bal cont
	 nop

	.org holy_KERNEL_MACHINE_TOTAL_MODULE_SIZE
VARIABLE(holy_total_modules_size)
	.long 0

VARIABLE (holy_arch_busclock)
	.long 0
VARIABLE (holy_arch_cpuclock)
	.long 0
VARIABLE (holy_arch_memsize)
	.long 0
VARIABLE (holy_arch_highmemsize)
	.long 0
#ifdef holy_MACHINE_MIPS_LOONGSON
VARIABLE (holy_arch_machine)
	.long holy_ARCH_MACHINE_FULOONG2F
#endif
cont:
	/* Save our base.  */
	move $s0, $ra

#ifdef holy_MACHINE_MIPS_QEMU_MIPS
	lui $t1, %hi(holy_arch_busclock)
	addiu $t1, %lo(holy_arch_busclock)
	sw $s4, 8($t1)
#endif

#ifdef holy_MACHINE_MIPS_LOONGSON
	lui $t1, %hi(holy_arch_busclock)
	addiu $t1, %lo(holy_arch_busclock)
	sw $s2, 0($t1)
	sw $s3, 4($t1)
	sw $s4, 8($t1)
	sw $s5, 12($t1)
	sw $s7, 16($t1)
#endif

	/* Move the modules out of BSS.  */
	lui $t2, %hi(__bss_start)
	addiu $t2, %lo(__bss_start)
	
	lui $t1, %hi(_end)
	addiu $t1, %lo(_end)
	addiu $t1, (holy_KERNEL_MACHINE_MOD_ALIGN - 1)
	li $t3, (holy_KERNEL_MACHINE_MOD_ALIGN - 1)
	nor $t3, $t3, $0
	and $t1, $t1, $t3
	
	lw $t3, (holy_KERNEL_MACHINE_TOTAL_MODULE_SIZE - BASE_ADDR)($s0)

	/* Backward copy.  */
	add $t1, $t1, $t3
	add $t2, $t2, $t3
	addiu $t1, $t1, -1
	addiu $t2, $t2, -1

	/* $t2 is source. $t1 is destination. $t3 is size.  */
modulesmovcont:
	beq $t3, $0, modulesmovdone
         nop
	lb holy_ASM_T4, 0($t2)
	sb holy_ASM_T4, 0($t1)
	addiu $t2, $t2, -1
	addiu $t1, $t1, -1
	b modulesmovcont
	 addiu $t3, $t3, -1
modulesmovdone:

	/* Clean BSS.  */
	
	lui $t1, %hi(__bss_start)
	addiu $t1, $t1, %lo(__bss_start)
	lui $t2, %hi(_end)
	addiu $t2, $t2, %lo(_end)
bsscont:
	sb $0,0($t1)
	addiu $t1, $t1, 1
	sltu $t3, $t1, $t2
	bne $t3, $0, bsscont
         nop

	lui $t9, %hi(holy_main)
	addiu $t9, %lo(holy_main)

	lui $sp, %hi(holy_MACHINE_MEMORY_STACK_HIGH)
	jr $t9
	 addiu $sp, $sp, %lo(holy_MACHINE_MEMORY_STACK_HIGH)

