#include <holy/symbol.h>
#include <holy/dl.h>
#include <holy/mips/asm.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	.text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	holy_ASM_REG_S $s0, 0($a0)
	holy_ASM_REG_S $s1, 8($a0)
	holy_ASM_REG_S $s2, 16($a0)
	holy_ASM_REG_S $s3, 24($a0)
	holy_ASM_REG_S $s4, 32($a0)
	holy_ASM_REG_S $s5, 40($a0)
	holy_ASM_REG_S $s6, 48($a0)
	holy_ASM_REG_S $s7, 56($a0)
	holy_ASM_REG_S $s8, 64($a0)
	holy_ASM_REG_S $gp, 72($a0)
	holy_ASM_REG_S $sp, 80($a0)
	holy_ASM_REG_S $ra, 88($a0)
	move $v0, $zero
	move $v1, $zero
	jr $ra
	 nop
/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	holy_ASM_REG_L $s0, 0($a0)
	holy_ASM_REG_L $s1, 8($a0)
	holy_ASM_REG_L $s2, 16($a0)
	holy_ASM_REG_L $s3, 24($a0)
	holy_ASM_REG_L $s4, 32($a0)
	holy_ASM_REG_L $s5, 40($a0)
	holy_ASM_REG_L $s6, 48($a0)
	holy_ASM_REG_L $s7, 56($a0)
	holy_ASM_REG_L $s8, 64($a0)
	holy_ASM_REG_L $gp, 72($a0)
	holy_ASM_REG_L $sp, 80($a0)
	holy_ASM_REG_L $ra, 88($a0)
	move $v0, $a1
	bne  $v0, $zero, 1f
	addiu $v0, $v0, 1
1:	
	move $v1, $zero
	jr $ra
	 nop
