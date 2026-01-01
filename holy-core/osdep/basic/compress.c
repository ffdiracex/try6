/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>
#include <holy/util/install.h>
#include <holy/util/misc.h>

int 
holy_install_compress_gzip (const char *src, const char *dest)
{
  holy_util_error (_("no compression is available for your platform"));
}

int 
holy_install_compress_xz (const char *src, const char *dest)
{
  holy_util_error (_("no compression is available for your platform"));
}

int 
holy_install_compress_lzop (const char *src, const char *dest)
{
  holy_util_error (_("no compression is available for your platform"));
}
