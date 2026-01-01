/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SYMBOL_HEADER
#define holy_SYMBOL_HEADER	1

#include <config.h>

/* Apple assembler requires local labels to start with a capital L */
#define LOCAL(sym)	L_ ## sym

/* Add an underscore to a C symbol in assembler code if needed. */
#ifndef holy_UTIL

#ifdef __APPLE__
#define MACRO_DOLLAR(x) $$ ## x
#else
#define MACRO_DOLLAR(x) $ ## x
#endif

#if HAVE_ASM_USCORE
#ifdef ASM_FILE
# define EXT_C(sym)	_ ## sym
#else
# define EXT_C(sym)	"_" sym
#endif
#else
# define EXT_C(sym)	sym
#endif

#ifdef __arm__
#define END .end
#endif

#if defined (__APPLE__)
#define FUNCTION(x)	.globl EXT_C(x) ; EXT_C(x):
#define VARIABLE(x)	.globl EXT_C(x) ; EXT_C(x):
#elif defined (__CYGWIN__) || defined (__MINGW32__)
/* .type not supported for non-ELF targets.  XXX: Check this in configure? */
#define FUNCTION(x)	.globl EXT_C(x) ; .def EXT_C(x); .scl 2; .type 32; .endef; EXT_C(x):
#define VARIABLE(x)	.globl EXT_C(x) ; .def EXT_C(x); .scl 2; .type 0; .endef; EXT_C(x):
#elif defined (__arm__)
#define FUNCTION(x)	.globl EXT_C(x) ; .type EXT_C(x), %function ; EXT_C(x):
#define VARIABLE(x)	.globl EXT_C(x) ; .type EXT_C(x), %object ; EXT_C(x):
#else
#define FUNCTION(x)	.globl EXT_C(x) ; .type EXT_C(x), @function ; EXT_C(x):
#define VARIABLE(x)	.globl EXT_C(x) ; .type EXT_C(x), @object ; EXT_C(x):
#endif
#endif

/* Mark an exported symbol.  */
#ifndef holy_SYMBOL_GENERATOR
# define EXPORT_FUNC(x)	x
# define EXPORT_VAR(x)	x
#endif /* ! holy_SYMBOL_GENERATOR */

#endif /* ! holy_SYMBOL_HEADER */
