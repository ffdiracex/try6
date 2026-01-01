/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARCHELP_HEADER
#define holy_ARCHELP_HEADER	1

#include <holy/fs.h>
#include <holy/file.h>

typedef enum
  {
    holy_ARCHELP_ATTR_TYPE = 0160000,
    holy_ARCHELP_ATTR_FILE = 0100000,
    holy_ARCHELP_ATTR_DIR = 0040000,
    holy_ARCHELP_ATTR_LNK = 0120000,
    holy_ARCHELP_ATTR_NOTIME = 0x80000000,
    holy_ARCHELP_ATTR_END = 0xffffffff
  } holy_archelp_mode_t;

struct holy_archelp_data;

struct holy_archelp_ops
{
  holy_err_t
  (*find_file) (struct holy_archelp_data *data, char **name,
		holy_int32_t *mtime,
		holy_archelp_mode_t *mode);

  char *
  (*get_link_target) (struct holy_archelp_data *data);

  void
  (*rewind) (struct holy_archelp_data *data);
};

holy_err_t
holy_archelp_dir (struct holy_archelp_data *data,
		  struct holy_archelp_ops *ops,
		  const char *path_in,
		  holy_fs_dir_hook_t hook, void *hook_data);

holy_err_t
holy_archelp_open (struct holy_archelp_data *data,
		   struct holy_archelp_ops *ops,
		   const char *name_in);

#endif
