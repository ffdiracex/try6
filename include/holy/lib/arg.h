/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ARG_HEADER
#define holy_ARG_HEADER	1

#include <holy/symbol.h>
#include <holy/err.h>
#include <holy/types.h>

enum holy_arg_type
  {
    ARG_TYPE_NONE,
    ARG_TYPE_STRING,
    ARG_TYPE_INT,
    ARG_TYPE_DEVICE,
    ARG_TYPE_FILE,
    ARG_TYPE_DIR,
    ARG_TYPE_PATHNAME
  };

typedef enum holy_arg_type holy_arg_type_t;

/* Flags for the option field op holy_arg_option.  */
#define holy_ARG_OPTION_OPTIONAL	(1 << 1)
/* Flags for an option that can appear multiple times.  */
#define holy_ARG_OPTION_REPEATABLE      (1 << 2)

enum holy_key_type
  {
    holy_KEY_ARG = -1,
    holy_KEY_END = -2
  };
typedef enum holy_key_type holy_arg_key_type_t;

struct holy_arg_option
{
  const char *longarg;
  int shortarg;
  int flags;
  const char *doc;
  const char *arg;
  holy_arg_type_t type;
};

struct holy_arg_list
{
  int set;
  union {
    char *arg;
    char **args;
  };
};

struct holy_extcmd;

int holy_arg_parse (struct holy_extcmd *cmd, int argc, char **argv,
		    struct holy_arg_list *usr, char ***args, int *argnum);

void EXPORT_FUNC(holy_arg_show_help) (struct holy_extcmd *cmd);
struct holy_arg_list* holy_arg_list_alloc (struct holy_extcmd *cmd,
					   int argc, char *argv[]);

#endif /* ! holy_ARG_HEADER */
