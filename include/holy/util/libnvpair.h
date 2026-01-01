/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_LIBNVPAIR_UTIL_HEADER
#define holy_LIBNVPAIR_UTIL_HEADER 1

#include <config.h>

#ifdef HAVE_LIBNVPAIR_H
#include <libnvpair.h>
#else /* ! HAVE_LIBNVPAIR_H */

#include <stdio.h>	/* FILE */

typedef void nvlist_t;

int nvlist_lookup_string (nvlist_t *, const char *, char **);
int nvlist_lookup_nvlist (nvlist_t *, const char *, nvlist_t **);
int nvlist_lookup_nvlist_array (nvlist_t *, const char *, nvlist_t ***, unsigned int *);

#endif /* ! HAVE_LIBNVPAIR_H */

#endif
