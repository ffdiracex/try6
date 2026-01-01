/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ENV_PRIVATE_HEADER
#define holy_ENV_PRIVATE_HEADER	1

#include <holy/env.h>

/* The size of the hash table.  */
#define	HASHSZ	13

/* A hashtable for quick lookup of variables.  */
struct holy_env_context
{
  /* A hash table for variables.  */
  struct holy_env_var *vars[HASHSZ];

  /* One level deeper on the stack.  */
  struct holy_env_context *prev;
};

/* This is used for sorting only.  */
struct holy_env_sorted_var
{
  struct holy_env_var *var;
  struct holy_env_sorted_var *next;
};

extern struct holy_env_context *EXPORT_VAR(holy_current_context);

#endif /* ! holy_ENV_PRIVATE_HEADER */
