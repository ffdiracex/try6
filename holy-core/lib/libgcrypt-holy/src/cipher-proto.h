/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef G10_CIPHER_PROTO_H
#define G10_CIPHER_PROTO_H

/* Definition of a function used to report selftest failures.
   DOMAIN is a string describing the function block:
          "cipher", "digest", "pubkey or "random",
   ALGO   is the algorithm under test,
   WHAT   is a string describing what has been tested,
   DESC   is a string describing the error. */
typedef void (*selftest_report_func_t)(const char *domain,
                                       int algo,
                                       const char *what,
                                       const char *errdesc);

/* Definition of the selftest functions.  */
typedef gpg_err_code_t (*selftest_func_t)
     (int algo, int extended, selftest_report_func_t report);


/* An extended type of the generate function.  */
typedef gcry_err_code_t (*pk_ext_generate_t)
     (int algo,
      unsigned int nbits,
      unsigned long evalue,
      gcry_sexp_t genparms,
      gcry_mpi_t *skey,
      gcry_mpi_t **retfactors,
      gcry_sexp_t *extrainfo);

/* The type used to compute the keygrip.  */
typedef gpg_err_code_t (*pk_comp_keygrip_t)
     (gcry_md_hd_t md, gcry_sexp_t keyparm);

/* The type used to query ECC curve parameters.  */
typedef gcry_err_code_t (*pk_get_param_t)
     (const char *name, gcry_mpi_t *pkey);

/* The type used to query an ECC curve name.  */
typedef const char *(*pk_get_curve_t)(gcry_mpi_t *pkey, int iterator,
                                      unsigned int *r_nbits);

/* The type used to query ECC curve parameters by name.  */
typedef gcry_sexp_t (*pk_get_curve_param_t)(const char *name);

/* The type used to convey additional information to a cipher.  */
typedef gpg_err_code_t (*cipher_set_extra_info_t)
     (void *c, int what, const void *buffer, size_t buflen);


/* Extra module specification structures.  These are used for internal
   modules which provide more functions than available through the
   public algorithm register APIs.  */
typedef struct cipher_extra_spec
{
  selftest_func_t selftest;
  cipher_set_extra_info_t set_extra_info;
} cipher_extra_spec_t;

typedef struct md_extra_spec
{
  selftest_func_t selftest;
} md_extra_spec_t;

typedef struct pk_extra_spec
{
  selftest_func_t selftest;
  pk_ext_generate_t ext_generate;
  pk_comp_keygrip_t comp_keygrip;
  pk_get_param_t get_param;
  pk_get_curve_t get_curve;
  pk_get_curve_param_t get_curve_param;
} pk_extra_spec_t;



/* The private register functions. */
gcry_error_t _gcry_cipher_register (gcry_cipher_spec_t *cipher,
                                    cipher_extra_spec_t *extraspec,
                                    int *algorithm_id,
                                    gcry_module_t *module);
gcry_error_t _gcry_md_register (gcry_md_spec_t *cipher,
                                md_extra_spec_t *extraspec,
                                unsigned int *algorithm_id,
                                gcry_module_t *module);
gcry_error_t _gcry_pk_register (gcry_pk_spec_t *cipher,
                                pk_extra_spec_t *extraspec,
                                unsigned int *algorithm_id,
                                gcry_module_t *module);

/* The selftest functions.  */
gcry_error_t _gcry_cipher_selftest (int algo, int extended,
                                    selftest_report_func_t report);
gcry_error_t _gcry_md_selftest (int algo, int extended,
                                selftest_report_func_t report);
gcry_error_t _gcry_pk_selftest (int algo, int extended,
                                selftest_report_func_t report);
gcry_error_t _gcry_hmac_selftest (int algo, int extended,
                                  selftest_report_func_t report);

gcry_error_t _gcry_random_selftest (selftest_report_func_t report);

#endif /*G10_CIPHER_PROTO_H*/
