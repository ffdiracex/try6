/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SETJMP_CPU_HEADER
#define holy_SETJMP_CPU_HEADER	1

typedef unsigned long holy_jmp_buf[21];

int holy_setjmp (holy_jmp_buf env) RETURNS_TWICE;
void holy_longjmp (holy_jmp_buf env, int val) __attribute__ ((noreturn));

#endif /* ! holy_SETJMP_CPU_HEADER */
