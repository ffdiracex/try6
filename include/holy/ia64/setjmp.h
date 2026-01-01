/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#define _JBLEN	70

/* the __jmp_buf element type should be __float80 per ABI... */
typedef long holy_jmp_buf[_JBLEN] __attribute__ ((aligned (16))); /* guarantees 128-bit alignment! */

int holy_setjmp (holy_jmp_buf env) RETURNS_TWICE;
void holy_longjmp (holy_jmp_buf env, int val) __attribute__ ((noreturn));
