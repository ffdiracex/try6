/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <wchar.h>

/* Internal state used by the functions mbsrtowcs() and mbsnrtowcs().  */
mbstate_t _gl_mbsrtowcs_state
/* The state must initially be in the "initial state"; so, zero-initialize it.
   On most systems, putting it into BSS is sufficient.  Not so on Mac OS X 10.3,
   */
#if defined __ELF__
  /* On ELF systems, variables in BSS behave well.  */
#else
  /* Use braces, to be on the safe side.  */
  = { 0 }
#endif
  ;
