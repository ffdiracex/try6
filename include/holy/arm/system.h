/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SYSTEM_CPU_HEADER
#define holy_SYSTEM_CPU_HEADER

#include <holy/types.h>

enum
  {
    holy_ARM_MACHINE_TYPE_RASPBERRY_PI = 3138,
    holy_ARM_MACHINE_TYPE_FDT = 0xFFFFFFFF
  };

void EXPORT_FUNC(holy_arm_disable_caches_mmu) (void);
void holy_arm_enable_caches_mmu (void);
void holy_arm_enable_mmu (holy_uint32_t *mmu_tables);
void holy_arm_clear_mmu_v6 (void);

#endif /* ! holy_SYSTEM_CPU_HEADER */

