/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/emu/exec.h>
#include <holy/util/install.h>

int 
holy_install_compress_gzip (const char *src, const char *dest)
{
  return holy_util_exec_redirect ((const char * []) { "gzip", "--best",
	"--stdout", NULL }, src, dest);
}

int 
holy_install_compress_xz (const char *src, const char *dest)
{
  return holy_util_exec_redirect ((const char * []) { "xz",
	"--lzma2=dict=128KiB", "--check=none", "--stdout", NULL }, src, dest);
}

int 
holy_install_compress_lzop (const char *src, const char *dest)
{
  return holy_util_exec_redirect ((const char * []) { "lzop", "-9",  "-c",
	NULL }, src, dest);
}
