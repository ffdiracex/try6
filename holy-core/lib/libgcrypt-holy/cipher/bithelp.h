/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <holy/dl.h>
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_BITHELP_H
#define G10_BITHELP_H


/****************
 * Rotate the 32 bit unsigned integer X by N bits left/right
 */
#if defined(__GNUC__) && defined(__i386__)
static inline u32
rol( u32 x, int n)
{
	__asm__("roll %%cl,%0"
		:"=r" (x)
		:"0" (x),"c" (n));
	return x;
}
#else
#define rol(x,n) ( ((x) << (n)) | ((x) >> (32-(n))) )
#endif

#if defined(__GNUC__) && defined(__i386__)
static inline u32
ror(u32 x, int n)
{
	__asm__("rorl %%cl,%0"
		:"=r" (x)
		:"0" (x),"c" (n));
	return x;
}
#else
#define ror(x,n) ( ((x) >> (n)) | ((x) << (32-(n))) )
#endif


#endif /*G10_BITHELP_H*/
