/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef GCRY_HASH_COMMON_H
#define GCRY_HASH_COMMON_H


const char * _gcry_hash_selftest_check_one
/**/         (int algo,
              int datamode, const void *data, size_t datalen,
              const void *expect, size_t expectlen);





#endif /*GCRY_HASH_COMMON_H*/
