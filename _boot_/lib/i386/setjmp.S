#include <holy/symbol.h>
#include <holy/dl.h>

	.file	"setjmp.S"

holy_MOD_LICENSE "GPLv2+"

	.text

/*
 * int holy_setjmp (holy_jmp_buf env)
 */
FUNCTION(holy_setjmp)
	movl	%ebx, 0(%eax)	/* EBX */
	movl	%esi, 4(%eax)	/* ESI */
	movl	%edi, 8(%eax)	/* EDI */
	movl	%ebp, 12(%eax)	/* EBP */
	popl	%ecx
	movl	%esp, 16(%eax)	/* ESP */
	movl	%ecx, 20(%eax)	/* EIP */
	xorl	%eax, %eax
	jmp	*%ecx


/*
 * int holy_longjmp (holy_jmp_buf env, int val)
 */
FUNCTION(holy_longjmp)
	movl	0(%eax), %ebx
	movl	4(%eax), %esi
	movl	8(%eax), %edi
	movl	12(%eax), %ebp
	movl	16(%eax), %esp
	movl	20(%eax), %ecx

	movl	%edx, %eax
	testl	%eax, %eax
	jnz	1f
	incl	%eax
1:	jmp	*%ecx

