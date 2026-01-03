	.set noreorder
	.set noat
.text
	.align	3
	.globl	__udiv_qrnnd
	.ent	__udiv_qrnnd
__udiv_qrnnd:
	.frame $30,0,$26,0
	.prologue 0
#define cnt	$2
#define tmp	$3
#define rem_ptr $16
#define n1	$17
#define n0	$18
#define d	$19
#define qb	$20

	ldiq	cnt,16
	blt	d,.Largedivisor

.Loop1: cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	d,n1,qb
	subq	n1,d,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	d,n1,qb
	subq	n1,d,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	d,n1,qb
	subq	n1,d,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	d,n1,qb
	subq	n1,d,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	subq	cnt,1,cnt
	bgt	cnt,.Loop1
	stq	n1,0(rem_ptr)
	bis	$31,n0,$0
	ret	$31,($26),1

.Largedivisor:
	and	n0,1,$4

	srl	n0,1,n0
	sll	n1,63,tmp
	or	tmp,n0,n0
	srl	n1,1,n1

	and	d,1,$6
	srl	d,1,$5
	addq	$5,$6,$5

.Loop2: cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	$5,n1,qb
	subq	n1,$5,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	$5,n1,qb
	subq	n1,$5,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	$5,n1,qb
	subq	n1,$5,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	cmplt	n0,0,tmp
	addq	n1,n1,n1
	bis	n1,tmp,n1
	addq	n0,n0,n0
	cmpule	$5,n1,qb
	subq	n1,$5,tmp
	cmovne	qb,tmp,n1
	bis	n0,qb,n0
	subq	cnt,1,cnt
	bgt	cnt,.Loop2

	addq	n1,n1,n1
	addq	$4,n1,n1
	bne	$6,.LOdd
	stq	n1,0(rem_ptr)
	bis	$31,n0,$0
	ret	$31,($26),1

.LOdd:
	/* q' in n0. r' in n1 */
	addq	n1,n0,n1
	cmpult	n1,n0,tmp	# tmp := carry from addq
	beq	tmp,.LLp6
	addq	n0,1,n0
	subq	n1,d,n1
.LLp6:	cmpult	n1,d,tmp
	bne	tmp,.LLp7
	addq	n0,1,n0
	subq	n1,d,n1
.LLp7:
	stq	n1,0(rem_ptr)
	bis	$31,n0,$0
	ret	$31,($26),1

	.end	__udiv_qrnnd
