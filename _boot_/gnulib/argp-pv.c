/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

const char *argp_program_version
#if defined __ELF__
  /* On ELF systems, variables in BSS behave well.  */
#else
  = (const char *) 0
#endif
  ;
