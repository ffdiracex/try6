/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PUBKEY_HEADER
#define holy_PUBKEY_HEADER 1

#include <holy/crypto.h>

struct holy_public_key *
holy_load_public_key (holy_file_t f);

holy_err_t
holy_verify_signature (holy_file_t f, holy_file_t sig,
		       struct holy_public_key *pk);


struct holy_public_subkey *
holy_crypto_pk_locate_subkey (holy_uint64_t keyid, struct holy_public_key *pkey);

struct holy_public_subkey *
holy_crypto_pk_locate_subkey_in_trustdb (holy_uint64_t keyid);

#endif
