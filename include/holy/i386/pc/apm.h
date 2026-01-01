/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_APM_MACHINE_HEADER
#define holy_APM_MACHINE_HEADER	1

#include <holy/types.h>

struct holy_apm_info
{
  holy_uint16_t cseg;
  holy_uint32_t offset;
  holy_uint16_t cseg_16;
  holy_uint16_t dseg;
  holy_uint16_t flags;
  holy_uint16_t cseg_len;
  holy_uint16_t cseg_16_len;
  holy_uint16_t dseg_len;
  holy_uint16_t version;
};

enum
  {
    holy_APM_FLAGS_16BITPROTECTED_SUPPORTED = 1,
    holy_APM_FLAGS_32BITPROTECTED_SUPPORTED = 2,
    holy_APM_FLAGS_CPUIDLE_SLOWS_DOWN = 4,
    holy_APM_FLAGS_DISABLED = 8,
    holy_APM_FLAGS_DISENGAGED = 16,
  };

int holy_apm_get_info (struct holy_apm_info *info);

#endif
