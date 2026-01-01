/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_RELOCATOR_CPU_HEADER
#define holy_RELOCATOR_CPU_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/relocator.h>

struct holy_relocator32_state
{
  /* gpr[0] is ignored since it's hardwired to 0.  */
  holy_uint32_t gpr[32];
  /* Register holding target $pc.  */
  int jumpreg;
};

holy_err_t
holy_relocator32_boot (struct holy_relocator *rel,
		       struct holy_relocator32_state state);

#endif /* ! holy_RELOCATOR_CPU_HEADER */
