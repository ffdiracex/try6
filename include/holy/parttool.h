/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PARTTOOL_HEADER
#define holy_PARTTOOL_HEADER	1

struct holy_parttool_argdesc
{
  const char *name;
  const char *desc;
  enum {holy_PARTTOOL_ARG_END, holy_PARTTOOL_ARG_BOOL, holy_PARTTOOL_ARG_VAL}
    type;
};

struct holy_parttool_args
{
  int set;
  union
  {
    int bool;
    char *str;
  };
};

typedef holy_err_t (*holy_parttool_function_t) (const holy_device_t dev,
						const struct holy_parttool_args *args);

struct holy_parttool
{
  struct holy_parttool *next;
  char *name;
  int handle;
  int nargs;
  struct holy_parttool_argdesc *args;
  holy_parttool_function_t func;
};

int holy_parttool_register(const char *part_name,
			   const holy_parttool_function_t func,
			   const struct holy_parttool_argdesc *args);
void holy_parttool_unregister (int handle);

#endif /* ! holy_PARTTOOL_HEADER*/
