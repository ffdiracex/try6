/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CRYPTO_HEADER
#define holy_CRYPTO_HEADER 1

#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/err.h>
#include <holy/mm.h>

typedef enum 
  {
    GPG_ERR_NO_ERROR,
    GPG_ERR_BAD_MPI,
    GPG_ERR_BAD_SECKEY,
    GPG_ERR_BAD_SIGNATURE,
    GPG_ERR_CIPHER_ALGO,
    GPG_ERR_CONFLICT,
    GPG_ERR_DECRYPT_FAILED,
    GPG_ERR_DIGEST_ALGO,
    GPG_ERR_GENERAL,
    GPG_ERR_INTERNAL,
    GPG_ERR_INV_ARG,
    GPG_ERR_INV_CIPHER_MODE,
    GPG_ERR_INV_FLAG,
    GPG_ERR_INV_KEYLEN,
    GPG_ERR_INV_OBJ,
    GPG_ERR_INV_OP,
    GPG_ERR_INV_SEXP,
    GPG_ERR_INV_VALUE,
    GPG_ERR_MISSING_VALUE,
    GPG_ERR_NO_ENCRYPTION_SCHEME,
    GPG_ERR_NO_OBJ,
    GPG_ERR_NO_PRIME,
    GPG_ERR_NO_SIGNATURE_SCHEME,
    GPG_ERR_NOT_FOUND,
    GPG_ERR_NOT_IMPLEMENTED,
    GPG_ERR_NOT_SUPPORTED,
    GPG_ERROR_CFLAGS,
    GPG_ERR_PUBKEY_ALGO,
    GPG_ERR_SELFTEST_FAILED,
    GPG_ERR_TOO_SHORT,
    GPG_ERR_UNSUPPORTED,
    GPG_ERR_WEAK_KEY,
    GPG_ERR_WRONG_KEY_USAGE,
    GPG_ERR_WRONG_PUBKEY_ALGO,
    GPG_ERR_OUT_OF_MEMORY,
    GPG_ERR_TOO_LARGE,
    GPG_ERR_ENOMEM
  } gpg_err_code_t;
typedef gpg_err_code_t gpg_error_t;
typedef gpg_error_t gcry_error_t;
typedef gpg_err_code_t gcry_err_code_t;
#define gcry_error_t gcry_err_code_t
#if 0
enum gcry_cipher_modes 
  {
    GCRY_CIPHER_MODE_NONE   = 0,  /* Not yet specified. */
    GCRY_CIPHER_MODE_ECB    = 1,  /* Electronic codebook. */
    GCRY_CIPHER_MODE_CFB    = 2,  /* Cipher feedback. */
    GCRY_CIPHER_MODE_CBC    = 3,  /* Cipher block chaining. */
    GCRY_CIPHER_MODE_STREAM = 4,  /* Used with stream ciphers. */
    GCRY_CIPHER_MODE_OFB    = 5,  /* Outer feedback. */
    GCRY_CIPHER_MODE_CTR    = 6   /* Counter. */
  };
#endif

/* Don't rely on this. Check!  */
#define holy_CRYPTO_MAX_MDLEN 64
#define holy_CRYPTO_MAX_CIPHER_BLOCKSIZE 16
#define holy_CRYPTO_MAX_MD_CONTEXT_SIZE 256

/* Type for the cipher_setkey function.  */
typedef gcry_err_code_t (*gcry_cipher_setkey_t) (void *c,
						 const unsigned char *key,
						 unsigned keylen);

/* Type for the cipher_encrypt function.  */
typedef void (*gcry_cipher_encrypt_t) (void *c,
				       unsigned char *outbuf,
				       const unsigned char *inbuf);

/* Type for the cipher_decrypt function.  */
typedef void (*gcry_cipher_decrypt_t) (void *c,
				       unsigned char *outbuf,
				       const unsigned char *inbuf);

/* Type for the cipher_stencrypt function.  */
typedef void (*gcry_cipher_stencrypt_t) (void *c,
					 unsigned char *outbuf,
					 const unsigned char *inbuf,
					 unsigned int n);

/* Type for the cipher_stdecrypt function.  */
typedef void (*gcry_cipher_stdecrypt_t) (void *c,
					 unsigned char *outbuf,
					 const unsigned char *inbuf,
					 unsigned int n);

typedef struct gcry_cipher_oid_spec
{
  const char *oid;
  int mode;
} gcry_cipher_oid_spec_t;

/* Module specification structure for ciphers.  */
typedef struct gcry_cipher_spec
{
  const char *name;
  const char **aliases;
  gcry_cipher_oid_spec_t *oids;
  holy_size_t blocksize;
  holy_size_t keylen;
  holy_size_t contextsize;
  gcry_cipher_setkey_t setkey;
  gcry_cipher_encrypt_t encrypt;
  gcry_cipher_decrypt_t decrypt;
  gcry_cipher_stencrypt_t stencrypt;
  gcry_cipher_stdecrypt_t stdecrypt;
#ifdef holy_UTIL
  const char *modname;
#endif
  struct gcry_cipher_spec *next;
} gcry_cipher_spec_t;

/* Type for the md_init function.  */
typedef void (*gcry_md_init_t) (void *c);

/* Type for the md_write function.  */
typedef void (*gcry_md_write_t) (void *c, const void *buf, holy_size_t nbytes);

/* Type for the md_final function.  */
typedef void (*gcry_md_final_t) (void *c);

/* Type for the md_read function.  */
typedef unsigned char *(*gcry_md_read_t) (void *c);

typedef struct gcry_md_oid_spec
{
  const char *oidstring;
} gcry_md_oid_spec_t;

/* Module specification structure for message digests.  */
typedef struct gcry_md_spec
{
  const char *name;
  unsigned char *asnoid;
  int asnlen;
  gcry_md_oid_spec_t *oids;
  holy_size_t mdlen;
  gcry_md_init_t init;
  gcry_md_write_t write;
  gcry_md_final_t final;
  gcry_md_read_t read;
  holy_size_t contextsize; /* allocate this amount of context */
  /* Block size, needed for HMAC.  */
  holy_size_t blocksize;
#ifdef holy_UTIL
  const char *modname;
#endif
  struct gcry_md_spec *next;
} gcry_md_spec_t;

struct gcry_mpi;
typedef struct gcry_mpi *gcry_mpi_t;

/* Type for the pk_generate function.  */
typedef gcry_err_code_t (*gcry_pk_generate_t) (int algo,
					       unsigned int nbits,
					       unsigned long use_e,
					       gcry_mpi_t *skey,
					       gcry_mpi_t **retfactors);

/* Type for the pk_check_secret_key function.  */
typedef gcry_err_code_t (*gcry_pk_check_secret_key_t) (int algo,
						       gcry_mpi_t *skey);

/* Type for the pk_encrypt function.  */
typedef gcry_err_code_t (*gcry_pk_encrypt_t) (int algo,
					      gcry_mpi_t *resarr,
					      gcry_mpi_t data,
					      gcry_mpi_t *pkey,
					      int flags);

/* Type for the pk_decrypt function.  */
typedef gcry_err_code_t (*gcry_pk_decrypt_t) (int algo,
					      gcry_mpi_t *result,
					      gcry_mpi_t *data,
					      gcry_mpi_t *skey,
					      int flags);

/* Type for the pk_sign function.  */
typedef gcry_err_code_t (*gcry_pk_sign_t) (int algo,
					   gcry_mpi_t *resarr,
					   gcry_mpi_t data,
					   gcry_mpi_t *skey);

/* Type for the pk_verify function.  */
typedef gcry_err_code_t (*gcry_pk_verify_t) (int algo,
					     gcry_mpi_t hash,
					     gcry_mpi_t *data,
					     gcry_mpi_t *pkey,
					     int (*cmp) (void *, gcry_mpi_t),
					     void *opaquev);

/* Type for the pk_get_nbits function.  */
typedef unsigned (*gcry_pk_get_nbits_t) (int algo, gcry_mpi_t *pkey);

/* Module specification structure for message digests.  */
typedef struct gcry_pk_spec
{
  const char *name;
  const char **aliases;
  const char *elements_pkey;
  const char *elements_skey;
  const char *elements_enc;
  const char *elements_sig;
  const char *elements_grip;
  int use;
  gcry_pk_generate_t generate;
  gcry_pk_check_secret_key_t check_secret_key;
  gcry_pk_encrypt_t encrypt;
  gcry_pk_decrypt_t decrypt;
  gcry_pk_sign_t sign;
  gcry_pk_verify_t verify;
  gcry_pk_get_nbits_t get_nbits;
#ifdef holy_UTIL
  const char *modname;
#endif
} gcry_pk_spec_t;

struct holy_crypto_cipher_handle
{
  const struct gcry_cipher_spec *cipher;
  char ctx[0];
};

typedef struct holy_crypto_cipher_handle *holy_crypto_cipher_handle_t;

struct holy_crypto_hmac_handle;

const gcry_cipher_spec_t *
holy_crypto_lookup_cipher_by_name (const char *name);

holy_crypto_cipher_handle_t
holy_crypto_cipher_open (const struct gcry_cipher_spec *cipher);

gcry_err_code_t
holy_crypto_cipher_set_key (holy_crypto_cipher_handle_t cipher,
			    const unsigned char *key,
			    unsigned keylen);

static inline void
holy_crypto_cipher_close (holy_crypto_cipher_handle_t cipher)
{
  holy_free (cipher);
}

static inline void
holy_crypto_xor (void *out, const void *in1, const void *in2, holy_size_t size)
{
  const holy_uint8_t *in1ptr = in1, *in2ptr = in2;
  holy_uint8_t *outptr = out;
  while (size && (((holy_addr_t) in1ptr & (sizeof (holy_uint64_t) - 1))
		  || ((holy_addr_t) in2ptr & (sizeof (holy_uint64_t) - 1))
		  || ((holy_addr_t) outptr & (sizeof (holy_uint64_t) - 1))))
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
  while (size >= sizeof (holy_uint64_t))
    {
      /* We've already checked that all pointers are aligned.  */
      *(holy_uint64_t *) (void *) outptr
	= (*(const holy_uint64_t *) (const void *) in1ptr
	   ^ *(const holy_uint64_t *) (const void *) in2ptr);
      in1ptr += sizeof (holy_uint64_t);
      in2ptr += sizeof (holy_uint64_t);
      outptr += sizeof (holy_uint64_t);
      size -= sizeof (holy_uint64_t);
    }
  while (size)
    {
      *outptr = *in1ptr ^ *in2ptr;
      in1ptr++;
      in2ptr++;
      outptr++;
      size--;
    }
}

gcry_err_code_t
holy_crypto_ecb_decrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size);

gcry_err_code_t
holy_crypto_ecb_encrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size);
gcry_err_code_t
holy_crypto_cbc_encrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size,
			 void *iv_in);
gcry_err_code_t
holy_crypto_cbc_decrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, const void *in, holy_size_t size,
			 void *iv);
void 
holy_cipher_register (gcry_cipher_spec_t *cipher);
void
holy_cipher_unregister (gcry_cipher_spec_t *cipher);
void 
holy_md_register (gcry_md_spec_t *digest);
void 
holy_md_unregister (gcry_md_spec_t *cipher);

extern struct gcry_pk_spec *holy_crypto_pk_dsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdsa;
extern struct gcry_pk_spec *holy_crypto_pk_ecdh;
extern struct gcry_pk_spec *holy_crypto_pk_rsa;

void
holy_crypto_hash (const gcry_md_spec_t *hash, void *out, const void *in,
		  holy_size_t inlen);
const gcry_md_spec_t *
holy_crypto_lookup_md_by_name (const char *name);

holy_err_t
holy_crypto_gcry_error (gcry_err_code_t in);

void holy_burn_stack (holy_size_t size);

struct holy_crypto_hmac_handle *
holy_crypto_hmac_init (const struct gcry_md_spec *md,
		       const void *key, holy_size_t keylen);
void
holy_crypto_hmac_write (struct holy_crypto_hmac_handle *hnd,
			const void *data,
			holy_size_t datalen);
gcry_err_code_t
holy_crypto_hmac_fini (struct holy_crypto_hmac_handle *hnd, void *out);

gcry_err_code_t
holy_crypto_hmac_buffer (const struct gcry_md_spec *md,
			 const void *key, holy_size_t keylen,
			 const void *data, holy_size_t datalen, void *out);

extern gcry_md_spec_t _gcry_digest_spec_md5;
extern gcry_md_spec_t _gcry_digest_spec_sha1;
extern gcry_md_spec_t _gcry_digest_spec_sha256;
extern gcry_md_spec_t _gcry_digest_spec_sha512;
extern gcry_md_spec_t _gcry_digest_spec_crc32;
extern gcry_cipher_spec_t _gcry_cipher_spec_aes;
#define holy_MD_MD5 ((const gcry_md_spec_t *) &_gcry_digest_spec_md5)
#define holy_MD_SHA1 ((const gcry_md_spec_t *) &_gcry_digest_spec_sha1)
#define holy_MD_SHA256 ((const gcry_md_spec_t *) &_gcry_digest_spec_sha256)
#define holy_MD_SHA512 ((const gcry_md_spec_t *) &_gcry_digest_spec_sha512)
#define holy_MD_CRC32 ((const gcry_md_spec_t *) &_gcry_digest_spec_crc32)
#define holy_CIPHER_AES ((const gcry_cipher_spec_t *) &_gcry_cipher_spec_aes)

/* Implement PKCS#5 PBKDF2 as per RFC 2898.  The PRF to use is HMAC variant
   of digest supplied by MD.  Inputs are the password P of length PLEN,
   the salt S of length SLEN, the iteration counter C (> 0), and the
   desired derived output length DKLEN.  Output buffer is DK which
   must have room for at least DKLEN octets.  The output buffer will
   be filled with the derived data.  */
gcry_err_code_t
holy_crypto_pbkdf2 (const struct gcry_md_spec *md,
		    const holy_uint8_t *P, holy_size_t Plen,
		    const holy_uint8_t *S, holy_size_t Slen,
		    unsigned int c,
		    holy_uint8_t *DK, holy_size_t dkLen);

int
holy_crypto_memcmp (const void *a, const void *b, holy_size_t n);

int
holy_password_get (char buf[], unsigned buf_size);

/* For indistinguishibility.  */
#define holy_ACCESS_DENIED holy_error (holy_ERR_ACCESS_DENIED, N_("access denied"))

extern void (*holy_crypto_autoload_hook) (const char *name);

void _gcry_assert_failed (const char *expr, const char *file, int line,
                          const char *func) __attribute__ ((noreturn));

void _gcry_burn_stack (int bytes);
void _gcry_log_error( const char *fmt, ... )  __attribute__ ((format (__printf__, 1, 2)));


#ifdef holy_UTIL
void holy_gcry_init_all (void);
void holy_gcry_fini_all (void);

int
holy_get_random (void *out, holy_size_t len);

#endif

#endif
