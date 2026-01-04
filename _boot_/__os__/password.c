/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if defined (__MINGW32__) && !defined (__CYGWIN__)
#include "windows/password.c"
#else
#include "unix/password.c"
#endif
