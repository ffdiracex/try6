/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if !defined (__MINGW32__) && !defined (__CYGWIN__) && !defined (__AROS__)
#include "unix/compress.c"
#else
#include "basic/compress.c"
#endif
