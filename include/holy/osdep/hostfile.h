/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#if defined (__AROS__)
#include "hostfile_aros.h"
#elif defined (__CYGWIN__) || defined (__MINGW32__)
#include "hostfile_windows.h"
#else
#include "hostfile_unix.h"
#endif
