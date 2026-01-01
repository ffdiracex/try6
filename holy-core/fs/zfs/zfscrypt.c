/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/file.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/dl.h>
#include <holy/types.h>
#include <holy/zfs/zfs.h>
#include <holy/zfs/zio.h>
#include <holy/zfs/dnode.h>
#include <holy/zfs/uberblock_impl.h>
#include <holy/zfs/vdev_impl.h>
#include <holy/zfs/zio_checksum.h>
#include <holy/zfs/zap_impl.h>
#include <holy/zfs/zap_leaf.h>
#include <holy/zfs/zfs_znode.h>
#include <holy/zfs/dmu.h>
#include <holy/zfs/dmu_objset.h>
#include <holy/zfs/sa_impl.h>
#include <holy/zfs/dsl_dir.h>
#include <holy/zfs/dsl_dataset.h>
#include <holy/crypto.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/*
  Mostly based on following article: 
  https://blogs.oracle.com/darren/entry/zfs_encryption_what_is_on
 */

enum holy_zfs_algo
  {
    holy_ZFS_ALGO_CCM,
    holy_ZFS_ALGO_GCM,
  };

struct holy_zfs_key
{
  holy_uint64_t algo;
  holy_uint8_t enc_nonce[13];
  holy_uint8_t unused[3];
  holy_uint8_t enc_key[48];
  holy_uint8_t unknown_purpose_nonce[13];
  holy_uint8_t unused2[3];
  holy_uint8_t unknown_purpose_key[48];
};

struct holy_zfs_wrap_key
{
  struct holy_zfs_wrap_key *next;
  holy_size_t keylen;
  int is_passphrase;
  holy_uint64_t key[0];
};

static struct holy_zfs_wrap_key *zfs_wrap_keys;

holy_err_t
holy_zfs_add_key (holy_uint8_t *key_in,
		  holy_size_t keylen,
		  int passphrase)
{
  struct holy_zfs_wrap_key *key;
  if (!passphrase && keylen > 32)
    keylen = 32;
  key = holy_malloc (sizeof (*key) + keylen);
  if (!key)
    return holy_errno;
  key->is_passphrase = passphrase;
  key->keylen = keylen;
  holy_memcpy (key->key, key_in, keylen);
  key->next = zfs_wrap_keys;
  zfs_wrap_keys = key;
  return holy_ERR_NONE;
}

static gcry_err_code_t
holy_ccm_decrypt (holy_crypto_cipher_handle_t cipher,
		  holy_uint8_t *out, const holy_uint8_t *in,
		  holy_size_t psize,
		  void *mac_out, const void *nonce,
		  unsigned l, unsigned m)
{
  holy_uint8_t iv[16];
  holy_uint8_t mul[16];
  holy_uint32_t mac[4];
  unsigned i, j;
  gcry_err_code_t err;

  holy_memcpy (iv + 1, nonce, 15 - l);

  iv[0] = (l - 1) | (((m-2) / 2) << 3);
  for (j = 0; j < l; j++)
    iv[15 - j] = psize >> (8 * j);
  err = holy_crypto_ecb_encrypt (cipher, mac, iv, 16);
  if (err)
    return err;

  iv[0] = l - 1;

  for (i = 0; i < (psize + 15) / 16; i++)
    {
      holy_size_t csize;
      csize = 16;
      if (csize > psize - 16 * i)
	csize = psize - 16 * i;
      for (j = 0; j < l; j++)
	iv[15 - j] = (i + 1) >> (8 * j);
      err = holy_crypto_ecb_encrypt (cipher, mul, iv, 16);
      if (err)
	return err;
      holy_crypto_xor (out + 16 * i, in + 16 * i, mul, csize);
      holy_crypto_xor (mac, mac, out + 16 * i, csize);
      err = holy_crypto_ecb_encrypt (cipher, mac, mac, 16);
      if (err)
	return err;
    }
  for (j = 0; j < l; j++)
    iv[15 - j] = 0;
  err = holy_crypto_ecb_encrypt (cipher, mul, iv, 16);
  if (err)
    return err;
  if (mac_out)
    holy_crypto_xor (mac_out, mac, mul, m);
  return GPG_ERR_NO_ERROR;
}

static void
holy_gcm_mul_x (holy_uint8_t *a)
{
  int i;
  int c = 0, d = 0;
  for (i = 0; i < 16; i++)
    {
      c = a[i] & 0x1;
      a[i] = (a[i] >> 1) | (d << 7);
      d = c;
    }
  if (d)
    a[0] ^= 0xe1;
}

static void
holy_gcm_mul (holy_uint8_t *a, const holy_uint8_t *b)
{
  holy_uint8_t res[16], bs[16];
  int i;
  holy_memcpy (bs, b, 16);
  holy_memset (res, 0, 16);
  for (i = 0; i < 128; i++)
    {
      if ((a[i / 8] << (i % 8)) & 0x80)
	holy_crypto_xor (res, res, bs, 16);
      holy_gcm_mul_x (bs);
    }
 
  holy_memcpy (a, res, 16);
}

static gcry_err_code_t
holy_gcm_decrypt (holy_crypto_cipher_handle_t cipher,
		  holy_uint8_t *out, const holy_uint8_t *in,
		  holy_size_t psize,
		  void *mac_out, const void *nonce,
		  unsigned nonce_len, unsigned m)
{
  holy_uint8_t iv[16];
  holy_uint8_t mul[16];
  holy_uint8_t mac[16], h[16], mac_xor[16];
  unsigned i, j;
  gcry_err_code_t err;

  holy_memset (mac, 0, sizeof (mac));

  err = holy_crypto_ecb_encrypt (cipher, h, mac, 16);
  if (err)
    return err;

  if (nonce_len == 12)
    {
      holy_memcpy (iv, nonce, 12);
      iv[12] = 0;
      iv[13] = 0;
      iv[14] = 0;
      iv[15] = 1;
    }
  else
    {
      holy_memset (iv, 0, sizeof (iv));
      holy_memcpy (iv, nonce, nonce_len);
      holy_gcm_mul (iv, h);
      iv[15] ^= nonce_len * 8;
      holy_gcm_mul (iv, h);
    }

  err = holy_crypto_ecb_encrypt (cipher, mac_xor, iv, 16);
  if (err)
    return err;

  for (i = 0; i < (psize + 15) / 16; i++)
    {
      holy_size_t csize;
      csize = 16;
      if (csize > psize - 16 * i)
	csize = psize - 16 * i;
      for (j = 0; j < 4; j++)
	{
	  iv[15 - j]++;
	  if (iv[15 - j] != 0)
	    break;
	}
      holy_crypto_xor (mac, mac, in + 16 * i, csize);
      holy_gcm_mul (mac, h);
      err = holy_crypto_ecb_encrypt (cipher, mul, iv, 16);
      if (err)
	return err;
      holy_crypto_xor (out + 16 * i, in + 16 * i, mul, csize);
    }
  for (j = 0; j < 8; j++)
    mac[15 - j] ^= ((((holy_uint64_t) psize) * 8) >> (8 * j));
  holy_gcm_mul (mac, h);

  if (mac_out)
    holy_crypto_xor (mac_out, mac, mac_xor, m);

  return GPG_ERR_NO_ERROR;
}


static gcry_err_code_t
algo_decrypt (holy_crypto_cipher_handle_t cipher, holy_uint64_t algo,
	      holy_uint8_t *out, const holy_uint8_t *in,
	      holy_size_t psize,
	      void *mac_out, const void *nonce,
	      unsigned l, unsigned m)
{
  switch (algo)
    {
    case 0:
      return holy_ccm_decrypt (cipher, out, in, psize,
			       mac_out, nonce, l, m);
    case 1:
      return holy_gcm_decrypt (cipher, out, in, psize,
			       mac_out, nonce,
			       15 - l, m);
    default:
      return GPG_ERR_CIPHER_ALGO;
    }
}

static holy_err_t
holy_zfs_decrypt_real (holy_crypto_cipher_handle_t cipher,
		       holy_uint64_t algo,
		       void *nonce,
		       char *buf, holy_size_t size,
		       const holy_uint32_t *expected_mac,
		       holy_zfs_endian_t endian)
{
  holy_uint32_t mac[4];
  unsigned i;
  holy_uint32_t sw[4];
  gcry_err_code_t err;
      
  holy_memcpy (sw, nonce, 16);
  if (endian != holy_ZFS_BIG_ENDIAN)
    for (i = 0; i < 4; i++)
      sw[i] = holy_swap_bytes32 (sw[i]);

  if (!cipher)
    return holy_error (holy_ERR_ACCESS_DENIED,
		       N_("no decryption key available"));
  err = algo_decrypt (cipher, algo,
		      (holy_uint8_t *) buf,
		      (holy_uint8_t *) buf,
		      size, mac,
		      sw + 1, 3, 12);
  if (err)
    return holy_crypto_gcry_error (err);
  
  for (i = 0; i < 3; i++)
    if (holy_zfs_to_cpu32 (expected_mac[i], endian)
	!= holy_be_to_cpu32 (mac[i]))
      return holy_error (holy_ERR_BAD_FS, N_("MAC verification failed"));
  return holy_ERR_NONE;
}

static holy_crypto_cipher_handle_t
holy_zfs_load_key_real (const struct holy_zfs_key *key,
			holy_size_t keysize,
			holy_uint64_t salt,
			holy_uint64_t algo)
{
  unsigned keylen;
  struct holy_zfs_wrap_key *wrap_key;
  holy_crypto_cipher_handle_t ret = NULL;

  if (keysize != sizeof (*key))
    {
      holy_dprintf ("zfs", "Unexpected key size %" PRIuholy_SIZE "\n", keysize);
      return 0;
    }

  if (holy_memcmp (key->enc_key + 32, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16)
      == 0)
    keylen = 16;
  else if (holy_memcmp (key->enc_key + 40, "\0\0\0\0\0\0\0\0", 8) == 0)
    keylen = 24;
  else
    keylen = 32;

  for (wrap_key = zfs_wrap_keys; wrap_key; wrap_key = wrap_key->next)
    {
      holy_crypto_cipher_handle_t cipher;
      holy_uint8_t decrypted[32], mac[32], wrap_key_real[32];
      gcry_err_code_t err;
      cipher = holy_crypto_cipher_open (holy_CIPHER_AES);
      if (!cipher)
	{
	  holy_errno = holy_ERR_NONE;
	  return 0;
	}
      holy_memset (wrap_key_real, 0, sizeof (wrap_key_real));
      err = 0;
      if (!wrap_key->is_passphrase)
	holy_memcpy(wrap_key_real, wrap_key->key,
		    wrap_key->keylen < keylen ? wrap_key->keylen : keylen);
      else
	err = holy_crypto_pbkdf2 (holy_MD_SHA1,
				  (const holy_uint8_t *) wrap_key->key,
				  wrap_key->keylen,
				  (const holy_uint8_t *) &salt, sizeof (salt),
				  1000, wrap_key_real, keylen);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (cipher);
	  continue;
	}
		    
      err = holy_crypto_cipher_set_key (cipher, wrap_key_real,
					keylen);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (cipher);
	  continue;
	}
      
      err = algo_decrypt (cipher, algo, decrypted, key->unknown_purpose_key, 32,
			  mac, key->unknown_purpose_nonce, 2, 16);
      if (err || (holy_crypto_memcmp (mac, key->unknown_purpose_key + 32, 16)
		  != 0))
	{
	  holy_dprintf ("zfs", "key loading failed\n");
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (cipher);
	  continue;
	}

      err = algo_decrypt (cipher, algo, decrypted, key->enc_key, keylen, mac,
			  key->enc_nonce, 2, 16);
      if (err || holy_crypto_memcmp (mac, key->enc_key + keylen, 16) != 0)
	{
	  holy_dprintf ("zfs", "key loading failed\n");
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (cipher);
	  continue;
	}
      ret = holy_crypto_cipher_open (holy_CIPHER_AES);
      if (!ret)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (cipher);
	  continue;
	}
      err = holy_crypto_cipher_set_key (ret, decrypted, keylen);
      if (err)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_crypto_cipher_close (ret);
	  holy_crypto_cipher_close (cipher);
	  continue;
	}
      holy_crypto_cipher_close (cipher);
      return ret;
    }
  return NULL;
}

static const struct holy_arg_option options[] =
  {
    {"raw", 'r', 0, N_("Assume input is raw."), 0, 0},
    {"hex", 'h', 0, N_("Assume input is hex."), 0, 0},
    {"passphrase", 'p', 0, N_("Assume input is passphrase."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

static holy_err_t
holy_cmd_zfs_key (holy_extcmd_context_t ctxt, int argc, char **args)
{
  holy_uint8_t buf[1024];
  holy_ssize_t real_size;

  if (argc > 0)
    {
      holy_file_t file;
      file = holy_file_open (args[0]);
      if (!file)
	return holy_errno;
      real_size = holy_file_read (file, buf, 1024);
      if (real_size < 0)
	return holy_errno;
    }
  else
    {
      holy_xputs (_("Enter ZFS password: "));
      if (!holy_password_get ((char *) buf, 1023))
	return holy_errno;
      real_size = holy_strlen ((char *) buf);
    }

  if (ctxt->state[1].set)
    {
      int i;
      holy_err_t err;
      for (i = 0; i < real_size / 2; i++)
	{
	  char c1 = holy_tolower (buf[2 * i]) - '0';
	  char c2 = holy_tolower (buf[2 * i + 1]) - '0';
	  if (c1 > 9)
	    c1 += '0' - 'a' + 10;
	  if (c2 > 9)
	    c2 += '0' - 'a' + 10;
	  buf[i] = (c1 << 4) | c2;
	}
      err = holy_zfs_add_key (buf, real_size / 2, 0);
      if (err)
	return err;
      return holy_ERR_NONE;
    }

  return holy_zfs_add_key (buf, real_size,
			   ctxt->state[2].set
			   || (argc == 0 && !ctxt->state[0].set
			       && !ctxt->state[1].set));
}

static holy_extcmd_t cmd_key;

holy_MOD_INIT(zfscrypt)
{
  holy_zfs_decrypt = holy_zfs_decrypt_real;
  holy_zfs_load_key = holy_zfs_load_key_real;
  cmd_key = holy_register_extcmd ("zfskey", holy_cmd_zfs_key, 0,
				  N_("[-h|-p|-r] [FILE]"),
				  N_("Import ZFS wrapping key stored in FILE."),
				  options);
}

holy_MOD_FINI(zfscrypt)
{
  holy_zfs_decrypt = 0;
  holy_zfs_load_key = 0;
  holy_unregister_extcmd (cmd_key);
}
