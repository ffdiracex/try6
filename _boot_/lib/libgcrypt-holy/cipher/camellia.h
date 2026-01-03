/* This file was automatically imported with 
   import_gcry.py. Please don't modify it */
#include <holy/dl.h>
#include <holy/misc.h>
void camellia_setup128(const unsigned char *key, holy_uint32_t *subkey);
void camellia_setup192(const unsigned char *key, holy_uint32_t *subkey);
void camellia_setup256(const unsigned char *key, holy_uint32_t *subkey);
void camellia_encrypt128(const holy_uint32_t *subkey, holy_uint32_t *io);
void camellia_encrypt192(const holy_uint32_t *subkey, holy_uint32_t *io);
void camellia_encrypt256(const holy_uint32_t *subkey, holy_uint32_t *io);
void camellia_decrypt128(const holy_uint32_t *subkey, holy_uint32_t *io);
void camellia_decrypt192(const holy_uint32_t *subkey, holy_uint32_t *io);
void camellia_decrypt256(const holy_uint32_t *subkey, holy_uint32_t *io);
#define memcpy holy_memcpy
/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef HEADER_CAMELLIA_H
#define HEADER_CAMELLIA_H

/* To use Camellia with libraries it is often useful to keep the name
 * space of the library clean.  The following macro is thus useful:
 *
 *     #define CAMELLIA_EXT_SYM_PREFIX foo_
 *
 * This prefixes all external symbols with "foo_".
 */
#ifdef HAVE_CONFIG_H
#endif
#ifdef CAMELLIA_EXT_SYM_PREFIX
#define CAMELLIA_PREFIX1(x,y) x ## y
#define CAMELLIA_PREFIX2(x,y) CAMELLIA_PREFIX1(x,y)
#define CAMELLIA_PREFIX(x)    CAMELLIA_PREFIX2(CAMELLIA_EXT_SYM_PREFIX,x)
#define Camellia_Ekeygen      CAMELLIA_PREFIX(Camellia_Ekeygen)
#define Camellia_EncryptBlock CAMELLIA_PREFIX(Camellia_EncryptBlock)
#define Camellia_DecryptBlock CAMELLIA_PREFIX(Camellia_DecryptBlock)
#define camellia_decrypt128   CAMELLIA_PREFIX(camellia_decrypt128)
#define camellia_decrypt256   CAMELLIA_PREFIX(camellia_decrypt256)
#define camellia_encrypt128   CAMELLIA_PREFIX(camellia_encrypt128)
#define camellia_encrypt256   CAMELLIA_PREFIX(camellia_encrypt256)
#define camellia_setup128     CAMELLIA_PREFIX(camellia_setup128)
#define camellia_setup192     CAMELLIA_PREFIX(camellia_setup192)
#define camellia_setup256     CAMELLIA_PREFIX(camellia_setup256)
#endif /*CAMELLIA_EXT_SYM_PREFIX*/


#ifdef  __cplusplus
extern "C" {
#endif

#define CAMELLIA_BLOCK_SIZE 16
#define CAMELLIA_TABLE_BYTE_LEN 272
#define CAMELLIA_TABLE_WORD_LEN (CAMELLIA_TABLE_BYTE_LEN / 4)

typedef unsigned int KEY_TABLE_TYPE[CAMELLIA_TABLE_WORD_LEN];


void Camellia_Ekeygen(const int keyBitLength,
		      const unsigned char *rawKey,
		      KEY_TABLE_TYPE keyTable);

void Camellia_EncryptBlock(const int keyBitLength,
			   const unsigned char *plaintext,
			   const KEY_TABLE_TYPE keyTable,
			   unsigned char *cipherText);

void Camellia_DecryptBlock(const int keyBitLength,
			   const unsigned char *cipherText,
			   const KEY_TABLE_TYPE keyTable,
			   unsigned char *plaintext);


#ifdef  __cplusplus
}
#endif

#endif /* HEADER_CAMELLIA_H */
