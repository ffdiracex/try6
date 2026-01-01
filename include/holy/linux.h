/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/file.h>

struct holy_linux_initrd_component;

struct holy_linux_initrd_context
{
  int nfiles;
  struct holy_linux_initrd_component *components;
  holy_size_t size;
};

holy_err_t
holy_initrd_init (int argc, char *argv[],
		  struct holy_linux_initrd_context *ctx);

holy_size_t
holy_get_initrd_size (struct holy_linux_initrd_context *ctx);

void
holy_initrd_close (struct holy_linux_initrd_context *initrd_ctx);

holy_err_t
holy_initrd_load (struct holy_linux_initrd_context *initrd_ctx,
		  char *argv[], void *target);
