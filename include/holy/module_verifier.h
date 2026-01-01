/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdint.h>
#include <stdlib.h>

#include <holy/types.h>

#define holy_MODULE_VERIFY_SUPPORTS_REL 1
#define holy_MODULE_VERIFY_SUPPORTS_RELA 2

struct holy_module_verifier_arch {
  const char *name;
  int voidp_sizeof;
  int bigendian;
  int machine;
  int flags;
  const int *supported_relocations;
  const int *short_relocations;
};

void holy_module_verify64(void *module_img, size_t module_size, const struct holy_module_verifier_arch *arch, const char **whitelist_empty);
void holy_module_verify32(void *module_img, size_t module_size, const struct holy_module_verifier_arch *arch, const char **whitelist_empty);
