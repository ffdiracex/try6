#include <holy/symbol.h>
#include <holy/xen.h>

FUNCTION(holy_xen_hypercall)
	pushl   %ebp
	movl    %esp, %ebp
	pushl   %esi
	pushl   %edi
	pushl   %ebx

	/* call number already in %eax. */
	/* %edx -> %ebx*/
	/* %ecx -> %ecx*/
	movl   %edx,  %ebx
	movl    8(%ebp), %edx
	movl    12(%ebp), %esi
	movl    16(%ebp), %edi
	movl    20(%ebp), %ebp
	int     $0x82
	popl   %ebx
	popl   %edi
	popl   %esi
	popl   %ebp
	ret
