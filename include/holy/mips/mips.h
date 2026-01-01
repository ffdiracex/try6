/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_REGISTERS_CPU_HEADER
#define holy_REGISTERS_CPU_HEADER	1

#ifdef ASM_FILE
#define holy_CPU_REGISTER_WRAP(x) x
#else
#define holy_CPU_REGISTER_WRAP(x) #x
#endif

#define holy_CPU_MIPS_COP0_TIMER_COUNT holy_CPU_REGISTER_WRAP($9)

#endif
