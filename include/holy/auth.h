/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_AUTH_HEADER
#define holy_AUTH_HEADER	1

#include <holy/err.h>
#include <holy/crypto.h>

#define holy_AUTH_MAX_PASSLEN 1024

typedef holy_err_t (*holy_auth_callback_t) (const char *, const char *, void *);

holy_err_t holy_auth_register_authentication (const char *user,
					      holy_auth_callback_t callback,
					      void *arg);
holy_err_t holy_auth_unregister_authentication (const char *user);

holy_err_t holy_auth_authenticate (const char *user);
holy_err_t holy_auth_deauthenticate (const char *user);
holy_err_t holy_auth_check_authentication (const char *userlist);

#endif /* ! holy_AUTH_HEADER */
