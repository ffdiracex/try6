/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_ENVBLK_HEADER
#define holy_ENVBLK_HEADER	1

#define holy_ENVBLK_SIGNATURE	"# holy Environment Block\n"
#define holy_ENVBLK_DEFCFG	"holyenv"

#ifndef ASM_FILE

struct holy_envblk
{
  char *buf;
  holy_size_t size;
};
typedef struct holy_envblk *holy_envblk_t;

holy_envblk_t holy_envblk_open (char *buf, holy_size_t size);
int holy_envblk_set (holy_envblk_t envblk, const char *name, const char *value);
void holy_envblk_delete (holy_envblk_t envblk, const char *name);
void holy_envblk_iterate (holy_envblk_t envblk,
                          void *hook_data,
                          int hook (const char *name, const char *value, void *hook_data));
void holy_envblk_close (holy_envblk_t envblk);

static inline char *
holy_envblk_buffer (const holy_envblk_t envblk)
{
  return envblk->buf;
}

static inline holy_size_t
holy_envblk_size (const holy_envblk_t envblk)
{
  return envblk->size;
}

#endif /* ! ASM_FILE */

#endif /* ! holy_ENVBLK_HEADER */
