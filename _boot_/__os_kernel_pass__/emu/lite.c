/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/emu/misc.h>

#ifndef holy_MACHINE_EMU
#error "This source is only meant for holy-emu platform"
#endif

#if defined(__i386__)
#include "../i386/dl.c"
#elif defined(__x86_64__)
#include "../x86_64/dl.c"
#elif defined(__sparc__)
#include "../sparc64/dl.c"
#elif defined(__mips__)
#include "../mips/dl.c"
#elif defined(__powerpc__)
#include "../powerpc/dl.c"
#elif defined(__ia64__)
#include "../ia64/dl_helper.c"
#include "../ia64/dl.c"
#elif defined(__arm__)
#include "../arm/dl_helper.c"
#include "../arm/dl.c"
#elif defined(__aarch64__)
#include "../arm64/dl_helper.c"
#include "../arm64/dl.c"
#else
#error "No target cpu type is defined"
#endif

const int holy_no_modules = 0;

/* holy-emu-lite supports dynamic module loading, so it won't have any
   embedded modules.  */
void
holy_init_all (void)
{
  return;
}

void
holy_fini_all (void)
{
  return;
}
