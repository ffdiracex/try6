/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if defined (__MINGW32__) || defined (__CYGWIN__)
#include "windows/relpath.c"
#elif defined (__AROS__)
#include "aros/relpath.c"
#else
#include "unix/relpath.c"
#endif
