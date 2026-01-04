/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_RMD_H
#define G10_RMD_H


/* We need this here because random.c must have direct access. */
typedef struct
{
  u32  h0,h1,h2,h3,h4;
  u32  nblocks;
  byte buf[64];
  int  count;
} RMD160_CONTEXT;

void _gcry_rmd160_init ( void *context );
void _gcry_rmd160_mixblock ( RMD160_CONTEXT *hd, void *blockof64byte );

#endif /*G10_RMD_H*/
