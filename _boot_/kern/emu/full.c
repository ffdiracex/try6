/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/kernel.h>
#include <holy/misc.h>
#include <holy/emu/misc.h>
#include <holy/disk.h>

const int holy_no_modules = 1;

void
holy_register_exported_symbols (void)
{
}

holy_err_t
holy_arch_dl_check_header (void *ehdr)
{
  (void) ehdr;
  return holy_ERR_BAD_MODULE;
}

holy_err_t
holy_arch_dl_relocate_symbols (holy_dl_t mod, void *ehdr,
			       Elf_Shdr *s, holy_dl_segment_t seg)
{
  (void) mod;
  (void) ehdr;
  (void) s;
  (void) seg;
  return holy_ERR_BAD_MODULE;
}

#if !defined (__i386__) && !defined (__x86_64__)
holy_err_t
holy_arch_dl_get_tramp_got_size (const void *ehdr __attribute__ ((unused)),
			         holy_size_t *tramp, holy_size_t *got)
{
  *tramp = 0;
  *got = 0;
  return holy_ERR_BAD_MODULE;
}
#endif

#ifdef holy_LINKER_HAVE_INIT
void
holy_arch_dl_init_linker (void)
{
}
#endif

