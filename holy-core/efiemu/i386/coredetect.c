/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/efiemu/efiemu.h>
#include <holy/command.h>
#include <holy/i386/cpuid.h>

const char *
holy_efiemu_get_default_core_name (void)
{
  return holy_cpuid_has_longmode ? "efiemu64.o" : "efiemu32.o";
}
