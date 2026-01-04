/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef HMAC256_H
#define HMAC256_H


struct hmac256_context;
typedef struct hmac256_context *hmac256_context_t;

hmac256_context_t _gcry_hmac256_new (const void *key, size_t keylen);
void _gcry_hmac256_update (hmac256_context_t hd, const void *buf, size_t len);
const void *_gcry_hmac256_finalize (hmac256_context_t hd, size_t *r_dlen);
void _gcry_hmac256_release (hmac256_context_t hd);

int _gcry_hmac256_file (void *result, size_t resultsize, const char *filename,
                        const void *key, size_t keylen);


#endif /*HMAC256_H*/
