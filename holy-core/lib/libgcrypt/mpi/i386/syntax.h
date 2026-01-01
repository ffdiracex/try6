/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#undef ALIGN

#if defined (BSD_SYNTAX) || defined (ELF_SYNTAX)
#define R(r) %r
#define MEM(base)(base)
#define MEM_DISP(base,displacement)displacement(R(base))
#define MEM_INDEX(base,index,size)(R(base),R(index),size)
#ifdef __STDC__
#define INSN1(mnemonic,size_suffix,dst)mnemonic##size_suffix dst
#define INSN2(mnemonic,size_suffix,dst,src)mnemonic##size_suffix src,dst
#else
#define INSN1(mnemonic,size_suffix,dst)mnemonic/**/size_suffix dst
#define INSN2(mnemonic,size_suffix,dst,src)mnemonic/**/size_suffix src,dst
#endif
#define TEXT .text
#if defined (BSD_SYNTAX)
#define ALIGN(log) .align log
#endif
#if defined (ELF_SYNTAX)
#define ALIGN(log) .align 1<<(log)
#endif
#define GLOBL .globl
#endif

#ifdef INTEL_SYNTAX
#define R(r) r
#define MEM(base)[base]
#define MEM_DISP(base,displacement)[base+(displacement)]
#define MEM_INDEX(base,index,size)[base+index*size]
#define INSN1(mnemonic,size_suffix,dst)mnemonic dst
#define INSN2(mnemonic,size_suffix,dst,src)mnemonic dst,src
#define TEXT .text
#define ALIGN(log) .align log
#define GLOBL .globl
#endif

#ifdef X86_BROKEN_ALIGN
#undef ALIGN
#define ALIGN(log) .align log,0x90
#endif
