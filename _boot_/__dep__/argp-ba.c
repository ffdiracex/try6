/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

const char *argp_program_bug_address
/* This variable should be zero-initialized.  On most systems, putting it into
   BSS is sufficient.  Not so on Mac OS X 10.3 and 10.4, */
#if defined __ELF__
  /* On ELF systems, variables in BSS behave well.  */
#else
  = (const char *) 0
#endif
  ;
