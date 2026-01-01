/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef __MINGW32__
#include "windows/cputime.c"
#else
#include "unix/cputime.c"
#endif
