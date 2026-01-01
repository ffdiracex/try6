/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_CRYPTODISK_HEADER
#define holy_CRYPTODISK_HEADER	1

#include <holy/disk.h>
#include <holy/crypto.h>
#include <holy/list.h>
#ifdef holy_UTIL
#include <holy/emu/hostdisk.h>
#endif

typedef enum
  {
    holy_CRYPTODISK_MODE_ECB,
    holy_CRYPTODISK_MODE_CBC,
    holy_CRYPTODISK_MODE_PCBC,
    holy_CRYPTODISK_MODE_XTS,
    holy_CRYPTODISK_MODE_LRW
  } holy_cryptodisk_mode_t;

typedef enum
  {
    holy_CRYPTODISK_MODE_IV_NULL,
    holy_CRYPTODISK_MODE_IV_PLAIN,
    holy_CRYPTODISK_MODE_IV_PLAIN64,
    holy_CRYPTODISK_MODE_IV_ESSIV,
    holy_CRYPTODISK_MODE_IV_BENBI,
    holy_CRYPTODISK_MODE_IV_BYTECOUNT64,
    holy_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH
  } holy_cryptodisk_mode_iv_t;

#define holy_CRYPTODISK_MAX_UUID_LENGTH 71

#define holy_CRYPTODISK_GF_LOG_SIZE 7
#define holy_CRYPTODISK_GF_SIZE (1U << holy_CRYPTODISK_GF_LOG_SIZE)
#define holy_CRYPTODISK_GF_LOG_BYTES (holy_CRYPTODISK_GF_LOG_SIZE - 3)
#define holy_CRYPTODISK_GF_BYTES (1U << holy_CRYPTODISK_GF_LOG_BYTES)
#define holy_CRYPTODISK_MAX_KEYLEN 128

struct holy_cryptodisk;

typedef gcry_err_code_t
(*holy_cryptodisk_rekey_func_t) (struct holy_cryptodisk *dev,
				 holy_uint64_t zoneno);

struct holy_cryptodisk
{
  struct holy_cryptodisk *next;
  struct holy_cryptodisk **prev;

  char *source;
  holy_disk_addr_t offset;
  holy_disk_addr_t total_length;
  holy_disk_t source_disk;
  int ref;
  holy_crypto_cipher_handle_t cipher;
  holy_crypto_cipher_handle_t secondary_cipher;
  holy_crypto_cipher_handle_t essiv_cipher;
  const gcry_md_spec_t *essiv_hash, *hash, *iv_hash;
  holy_cryptodisk_mode_t mode;
  holy_cryptodisk_mode_iv_t mode_iv;
  int benbi_log;
  unsigned long id, source_id;
  enum holy_disk_dev_id source_dev_id;
  char uuid[holy_CRYPTODISK_MAX_UUID_LENGTH + 1];
  holy_uint8_t lrw_key[holy_CRYPTODISK_GF_BYTES];
  holy_uint8_t *lrw_precalc;
  holy_uint8_t iv_prefix[64];
  holy_size_t iv_prefix_len;
  holy_uint8_t key[holy_CRYPTODISK_MAX_KEYLEN];
  holy_size_t keysize;
#ifdef holy_UTIL
  char *cheat;
  holy_util_fd_t cheat_fd;
#endif
  const char *modname;
  int log_sector_size;
  holy_cryptodisk_rekey_func_t rekey;
  int rekey_shift;
  holy_uint8_t rekey_key[64];
  holy_uint64_t last_rekey;
  int rekey_derived_size;
  holy_disk_addr_t partition_start;
};
typedef struct holy_cryptodisk *holy_cryptodisk_t;

struct holy_cryptodisk_dev
{
  struct holy_cryptodisk_dev *next;
  struct holy_cryptodisk_dev **prev;

  holy_cryptodisk_t (*scan) (holy_disk_t disk, const char *check_uuid,
			     int boot_only);
  holy_err_t (*recover_key) (holy_disk_t disk, holy_cryptodisk_t dev);
};
typedef struct holy_cryptodisk_dev *holy_cryptodisk_dev_t;

extern holy_cryptodisk_dev_t EXPORT_VAR (holy_cryptodisk_list);

#ifndef holy_LST_GENERATOR
static inline void
holy_cryptodisk_dev_register (holy_cryptodisk_dev_t cr)
{
  holy_list_push (holy_AS_LIST_P (&holy_cryptodisk_list), holy_AS_LIST (cr));
}
#endif

static inline void
holy_cryptodisk_dev_unregister (holy_cryptodisk_dev_t cr)
{
  holy_list_remove (holy_AS_LIST (cr));
}

#define FOR_CRYPTODISK_DEVS(var) FOR_LIST_ELEMENTS((var), (holy_cryptodisk_list))

gcry_err_code_t
holy_cryptodisk_setkey (holy_cryptodisk_t dev,
			holy_uint8_t *key, holy_size_t keysize);
gcry_err_code_t
holy_cryptodisk_decrypt (struct holy_cryptodisk *dev,
			 holy_uint8_t * data, holy_size_t len,
			 holy_disk_addr_t sector);
holy_err_t
holy_cryptodisk_insert (holy_cryptodisk_t newdev, const char *name,
			holy_disk_t source);
#ifdef holy_UTIL
holy_err_t
holy_cryptodisk_cheat_insert (holy_cryptodisk_t newdev, const char *name,
			      holy_disk_t source, const char *cheat);
void
holy_util_cryptodisk_get_abstraction (holy_disk_t disk,
				      void (*cb) (const char *val, void *data),
				      void *data);

char *
holy_util_get_geli_uuid (const char *dev);
#endif

holy_cryptodisk_t holy_cryptodisk_get_by_uuid (const char *uuid);
holy_cryptodisk_t holy_cryptodisk_get_by_source_disk (holy_disk_t disk);

#endif
