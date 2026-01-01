/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SYSLINUX_PARSE_HEADER
#define holy_SYSLINUX_PARSE_HEADER 1

#include <holy/types.h>

typedef enum
  {
    holy_SYSLINUX_UNKNOWN,
    holy_SYSLINUX_ISOLINUX,
    holy_SYSLINUX_PXELINUX,
    holy_SYSLINUX_SYSLINUX,
  } holy_syslinux_flavour_t;

char *
holy_syslinux_config_file (const char *root, const char *target_root,
			   const char *cwd, const char *target_cwd,
			   const char *fname, holy_syslinux_flavour_t flav);

#endif
