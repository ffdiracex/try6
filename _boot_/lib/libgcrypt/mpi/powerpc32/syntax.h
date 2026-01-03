/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define USE_PPC_PATCHES 1

/* This seems to always be the case on PPC.  */
#define ALIGNARG(log2) log2
/* For ELF we need the `.type' directive to make shared libs work right.  */
#define ASM_TYPE_DIRECTIVE(name,typearg) .type name,typearg;
#define ASM_SIZE_DIRECTIVE(name) .size name,.-name
#define ASM_GLOBAL_DIRECTIVE   .globl

#ifdef __STDC__
#define C_LABEL(name) C_SYMBOL_NAME(name)##:
#else
#define C_LABEL(name) C_SYMBOL_NAME(name)/**/:
#endif

#ifdef __STDC__
#define L(body) .L##body
#else
#define L(body) .L/**/body
#endif

/* No profiling of gmp's assembly for now... */
#define CALL_MCOUNT /* no profiling */

#define        ENTRY(name)				    \
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);		    \
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)	    \
  .align ALIGNARG(2);					    \
  C_LABEL(name) 					    \
  CALL_MCOUNT

#define EALIGN_W_0  /* No words to insert.  */
#define EALIGN_W_1  nop
#define EALIGN_W_2  nop;nop
#define EALIGN_W_3  nop;nop;nop
#define EALIGN_W_4  EALIGN_W_3;nop
#define EALIGN_W_5  EALIGN_W_4;nop
#define EALIGN_W_6  EALIGN_W_5;nop
#define EALIGN_W_7  EALIGN_W_6;nop

/* EALIGN is like ENTRY, but does alignment to 'words'*4 bytes
   past a 2^align boundary.  */
#define EALIGN(name, alignt, words)			\
  ASM_GLOBAL_DIRECTIVE C_SYMBOL_NAME(name);		\
  ASM_TYPE_DIRECTIVE (C_SYMBOL_NAME(name),@function)	\
  .align ALIGNARG(alignt);				\
  EALIGN_W_##words;					\
  C_LABEL(name)

#undef END
#define END(name)		     \
  ASM_SIZE_DIRECTIVE(name)

