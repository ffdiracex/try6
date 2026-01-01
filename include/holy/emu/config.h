/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CONFIG_EMU_HEADER
#define holy_CONFIG_EMU_HEADER	1

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/emu/hostfile.h>
#include <stdio.h>

const char *
holy_util_get_config_filename (void);
const char *
holy_util_get_pkgdatadir (void);
const char *
holy_util_get_pkglibdir (void);
const char *
holy_util_get_localedir (void);

struct holy_util_config
{
  int is_cryptodisk_enabled;
  char *holy_distributor;
};

void
holy_util_load_config (struct holy_util_config *cfg);

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple);

#endif
