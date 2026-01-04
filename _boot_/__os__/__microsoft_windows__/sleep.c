/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config-util.h>
#include <config.h>

#include <windows.h>
#include <winioctl.h>
#include <holy/time.h>

void
holy_millisleep (holy_uint32_t ms)
{
  Sleep (ms);
}

