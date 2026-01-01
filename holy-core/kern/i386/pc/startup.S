#include <config.h>
#include <holy/symbol.h>
#include <multiboot.h>
#ifdef __APPLE__
#include <holy/i386/pc/memory.h>
#endif

	.file	"startup.S"

	.text

	.globl	start, _start, __start
start:
_start:
__start:
#ifdef __APPLE__
LOCAL(start):
#endif
	.code32

	movl	%ecx, (LOCAL(real_to_prot_addr) - _start) (%esi)
	movl	%edi, (LOCAL(prot_to_real_addr) - _start) (%esi)
	movl	%eax, (EXT_C(holy_realidt) - _start) (%esi)

	/* copy back the decompressed part (except the modules) */
#ifdef __APPLE__
	movl	$EXT_C(_edata), %ecx
	subl    $LOCAL(start), %ecx
#else
	movl	$(_edata - _start), %ecx
#endif
	movl	$(_start), %edi
	rep
	movsb

	movl	$LOCAL (cont), %esi
	jmp	*%esi
LOCAL(cont):

#if 0
	/* copy modules before cleaning out the bss */
	movl	EXT_C(holy_total_module_size), %ecx
	movl	EXT_C(holy_kernel_image_size), %esi
	addl	%ecx, %esi
	addl	$_start, %esi
	decl	%esi
	movl	$END_SYMBOL, %edi
	addl	%ecx, %edi
	decl	%edi
	std
	rep
	movsb
#endif

#ifdef __APPLE__
	/* clean out the bss */
	movl	$EXT_C(_edata), %edi

	/* compute the bss length */
	movl	$holy_MEMORY_MACHINE_SCRATCH_ADDR, %ecx
#else
	/* clean out the bss */
	movl	$BSS_START_SYMBOL, %edi

	/* compute the bss length */
	movl	$END_SYMBOL, %ecx
#endif
	subl	%edi, %ecx

	/* clean out */
	xorl	%eax, %eax
	cld
	rep
	stosb

	movl	%edx, EXT_C(holy_boot_device)

	/*
	 *  Call the start of main body of C code.
	 */
	call EXT_C(holy_main)

LOCAL(real_to_prot_addr):
	.long 0
LOCAL(prot_to_real_addr):
	.long 0

	.macro PROT_TO_REAL
	movl	LOCAL(prot_to_real_addr), %eax
	call	*%eax
	.endm

	.macro REAL_TO_PROT
	movl	LOCAL(real_to_prot_addr), %eax
	calll	*%eax
	.endm

/*
 * holy_exit()
 *
 * Exit the system.
 */
FUNCTION(holy_exit)
	PROT_TO_REAL
	.code16
	/* Tell the BIOS a boot failure. If this does not work, reboot.  */
	int	$0x18
	/* set 0x472 to 0x0000 for cold boot (0x1234 for warm boot) */
	xorw	%ax, %ax
	movw	$0x0472, %di
	movw	%ax, (%di)
	ljmp	$0xf000, $0xfff0
	.code32

/*
 * int holy_pxe_call (int func, void* data, holy_uint32_t pxe_rm_entry);
 */
FUNCTION(holy_pxe_call)
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%edi
	pushl	%ebx

	movl	%ecx, %ebx
	movl	%eax, %ecx
	movl	%edx, %eax
	andl	$0xF, %eax
	shrl	$4, %edx
	shll	$16, %edx
	addl	%eax, %edx

	PROT_TO_REAL
	.code16

	pushl	%ebx
	pushl	%edx
	pushw	%cx
	movw	%sp, %bx
	lcall	*%ss:6(%bx)
	cld
	addw	$10, %sp
	movw	%ax, %cx

	REAL_TO_PROT
	.code32

	movzwl	%cx, %eax

	popl	%ebx
	popl	%edi
	popl	%esi
	popl	%ebp
	ret

#include "../int.S"

VARIABLE(holy_realidt)
	.long 0

#ifdef __APPLE__
	/* Older versions of objconv assume that there is the same number
	   of text and data sections. Hence this dummy.  */
	.section __TEXT, __zz_dummy
	.byte 0
	.globl EXT_C(_edata)
	.globl EXT_C(holy_boot_device)
	.zerofill __DATA, __aa_before_bss, EXT_C(_edata), 1, 0
	.zerofill __DATA, __bss, EXT_C(holy_boot_device), 4, 2
#else
	.bss
VARIABLE(holy_boot_device)
	.long	0
#endif
