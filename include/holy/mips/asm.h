/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_MIPS_ASM_HEADER
#define holy_MIPS_ASM_HEADER	1

#if defined(_MIPS_SIM) && defined(_ABIN32) && _MIPS_SIM == _ABIN32
#define holy_ASM_T4 $a4
#define holy_ASM_T5 $a5
#define holy_ASM_SZREG 8
#define holy_ASM_REG_S sd
#define holy_ASM_REG_L ld
#else
#define holy_ASM_T4 $t4
#define holy_ASM_T5 $t5
#define holy_ASM_SZREG 4
#define holy_ASM_REG_S sw
#define holy_ASM_REG_L lw
#endif

#endif
