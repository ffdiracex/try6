#include "sysdep.h"
#include "asm-syntax.h"


/*******************
 * mpi_limb_t
 * _gcry_mpih_submul_1( mpi_ptr_t res_ptr,      (sp + 4)
 *		     mpi_ptr_t s1_ptr,	     (sp + 8)
 *		     mpi_size_t s1_size,     (sp + 12)
 *		     mpi_limb_t s2_limb)     (sp + 16)
 */

#define res_ptr edi
#define s1_ptr	esi
#define size	ecx
#define s2_limb ebp

	TEXT
	ALIGN (3)
	GLOBL	C_SYMBOL_NAME(_gcry_mpih_submul_1)
C_SYMBOL_NAME(_gcry_mpih_submul_1:)

	INSN1(push,l	,R(edi))
	INSN1(push,l	,R(esi))
	INSN1(push,l	,R(ebx))
	INSN1(push,l	,R(ebp))

	INSN2(mov,l	,R(res_ptr),MEM_DISP(esp,20))
	INSN2(mov,l	,R(s1_ptr),MEM_DISP(esp,24))
	INSN2(mov,l	,R(size),MEM_DISP(esp,28))
	INSN2(mov,l	,R(s2_limb),MEM_DISP(esp,32))

	INSN2(lea,l	,R(res_ptr),MEM_INDEX(res_ptr,size,4))
	INSN2(lea,l	,R(s1_ptr),MEM_INDEX(s1_ptr,size,4))
	INSN1(neg,l	,R(size))
	INSN2(xor,l	,R(ebx),R(ebx))
	ALIGN (3)

Loop:	INSN2(adc,l	,R(ebx),$0)
	INSN2(mov,l	,R(eax),MEM_INDEX(s1_ptr,size,4))

	INSN1(mul,l	,R(s2_limb))

	INSN2(add,l	,R(eax),R(ebx))
	INSN2(mov,l	,R(ebx),MEM_INDEX(res_ptr,size,4))

	INSN2(adc,l	,R(edx),$0)
	INSN2(sub,l	,R(ebx),R(eax))

	INSN2(mov,l	,MEM_INDEX(res_ptr,size,4),R(ebx))
	INSN1(inc,l	,R(size))

	INSN2(mov,l	,R(ebx),R(edx))
	INSN1(jnz,	,Loop)

	INSN2(adc,l	,R(ebx),$0)
	INSN2(mov,l	,R(eax),R(ebx))
	INSN1(pop,l	,R(ebp))
	INSN1(pop,l	,R(ebx))
	INSN1(pop,l	,R(esi))
	INSN1(pop,l	,R(edi))
	ret

