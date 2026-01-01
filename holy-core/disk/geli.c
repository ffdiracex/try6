/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/cryptodisk.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/err.h>
#include <holy/disk.h>
#include <holy/crypto.h>
#include <holy/partition.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

/* Dirty trick to solve circular dependency.  */
#ifdef holy_UTIL

#include <holy/util/misc.h>

#undef holy_MD_SHA256
#undef holy_MD_SHA512

static const gcry_md_spec_t *
holy_md_sha256_real (void)
{
  const gcry_md_spec_t *ret;
  ret = holy_crypto_lookup_md_by_name ("sha256");
  if (!ret)
    holy_util_error ("%s", _("Couldn't load sha256"));
  return ret;
}

static const gcry_md_spec_t *
holy_md_sha512_real (void)
{
  const gcry_md_spec_t *ret;
  ret = holy_crypto_lookup_md_by_name ("sha512");
  if (!ret)
    holy_util_error ("%s", _("Couldn't load sha512"));
  return ret;
}

#define holy_MD_SHA256 holy_md_sha256_real()
#define holy_MD_SHA512 holy_md_sha512_real()
#endif

struct holy_geli_key
{
  holy_uint8_t iv_key[64];
  holy_uint8_t cipher_key[64];
  holy_uint8_t hmac[64];
} holy_PACKED;

struct holy_geli_phdr
{
  holy_uint8_t magic[16];
#define GELI_MAGIC "GEOM::ELI"
  holy_uint32_t version;
  holy_uint32_t flags;
  holy_uint16_t alg;
  holy_uint16_t keylen;
  holy_uint16_t unused3[5];
  holy_uint32_t sector_size;
  holy_uint8_t keys_used;
  holy_uint32_t niter;
  holy_uint8_t salt[64];
  struct holy_geli_key keys[2];
} holy_PACKED;

enum
  {
    holy_GELI_FLAGS_ONETIME = 1,
    holy_GELI_FLAGS_BOOT = 2,
  };

/* FIXME: support version 0.  */
/* FIXME: support big-endian pre-version-4 volumes.  */
/* FIXME: support for keyfiles.  */
/* FIXME: support for HMAC.  */
const char *algorithms[] = {
  [0x01] = "des",
  [0x02] = "3des",
  [0x03] = "blowfish",
  [0x04] = "cast5",
  /* FIXME: 0x05 is skipjack, but we don't have it.  */
  [0x0b] = "aes",
  /* FIXME: 0x10 is null.  */
  [0x15] = "camellia128",
  [0x16] = "aes"
};

#define MAX_PASSPHRASE 256

static gcry_err_code_t
geli_rekey (struct holy_cryptodisk *dev, holy_uint64_t zoneno)
{
  gcry_err_code_t gcry_err;
  const struct {
    char magic[4];
    holy_uint64_t zone;
  } holy_PACKED tohash
      = { {'e', 'k', 'e', 'y'}, holy_cpu_to_le64 (zoneno) };
  holy_PROPERLY_ALIGNED_ARRAY (key, holy_CRYPTO_MAX_MDLEN);

  if (dev->hash->mdlen > holy_CRYPTO_MAX_MDLEN)
    return GPG_ERR_INV_ARG;

  holy_dprintf ("geli", "rekeying %" PRIuholy_UINT64_T " keysize=%d\n",
		zoneno, dev->rekey_derived_size);
  gcry_err = holy_crypto_hmac_buffer (dev->hash, dev->rekey_key, 64,
				      &tohash, sizeof (tohash), key);
  if (gcry_err)
    return gcry_err;

  return holy_cryptodisk_setkey (dev, (holy_uint8_t *) key,
				 dev->rekey_derived_size); 
}

static inline gcry_err_code_t
make_uuid (const struct holy_geli_phdr *header,
	   char *uuid)
{
  holy_uint8_t uuidbin[holy_CRYPTODISK_MAX_UUID_LENGTH];
  gcry_err_code_t err;
  holy_uint8_t *iptr;
  char *optr;

  if (2 * holy_MD_SHA256->mdlen + 1 > holy_CRYPTODISK_MAX_UUID_LENGTH)
    return GPG_ERR_TOO_LARGE;
  err = holy_crypto_hmac_buffer (holy_MD_SHA256,
				 header->salt, sizeof (header->salt),
				 "uuid", sizeof ("uuid") - 1, uuidbin);
  if (err)
    return err;

  optr = uuid;
  for (iptr = uuidbin; iptr < &uuidbin[holy_MD_SHA256->mdlen]; iptr++)
    {
      holy_snprintf (optr, 3, "%02x", *iptr);
      optr += 2;
    }
  *optr = 0;
  return GPG_ERR_NO_ERROR;
}

#ifdef holy_UTIL

#include <holy/emu/hostdisk.h>
#include <holy/emu/misc.h>

char *
holy_util_get_geli_uuid (const char *dev)
{
  holy_util_fd_t fd;
  holy_uint64_t s;
  unsigned log_secsize;
  holy_uint8_t hdr[512];
  struct holy_geli_phdr *header;
  char *uuid; 
  gcry_err_code_t err;

  fd = holy_util_fd_open (dev, holy_UTIL_FD_O_RDONLY);

  if (!holy_UTIL_FD_IS_VALID (fd))
    return NULL;

  s = holy_util_get_fd_size (fd, dev, &log_secsize);
  s >>= log_secsize;
  if (holy_util_fd_seek (fd, (s << log_secsize) - 512) < 0)
    holy_util_error ("%s", _("couldn't read ELI metadata"));

  uuid = xmalloc (holy_MD_SHA256->mdlen * 2 + 1);
  if (holy_util_fd_read (fd, (void *) &hdr, 512) < 0)
    holy_util_error ("%s", _("couldn't read ELI metadata"));

  holy_util_fd_close (fd);
	  
  COMPILE_TIME_ASSERT (sizeof (header) <= 512);
  header = (void *) &hdr;

  /* Look for GELI magic sequence.  */
  if (holy_memcmp (header->magic, GELI_MAGIC, sizeof (GELI_MAGIC))
      || holy_le_to_cpu32 (header->version) > 7
      || holy_le_to_cpu32 (header->version) < 1)
    holy_util_error ("%s", _("wrong ELI magic or version"));

  err = make_uuid ((void *) &hdr, uuid);
  if (err)
    {
      holy_free (uuid);
      return NULL;
    }

  return uuid;
}
#endif

static holy_cryptodisk_t
configure_ciphers (holy_disk_t disk, const char *check_uuid,
		   int boot_only)
{
  holy_cryptodisk_t newdev;
  struct holy_geli_phdr header;
  holy_crypto_cipher_handle_t cipher = NULL, secondary_cipher = NULL;
  const struct gcry_cipher_spec *ciph;
  const char *ciphername = NULL;
  gcry_err_code_t gcry_err;
  char uuid[holy_CRYPTODISK_MAX_UUID_LENGTH];
  holy_disk_addr_t sector;
  holy_err_t err;

  if (2 * holy_MD_SHA256->mdlen + 1 > holy_CRYPTODISK_MAX_UUID_LENGTH)
    return NULL;

  sector = holy_disk_get_size (disk);
  if (sector == holy_DISK_SIZE_UNKNOWN || sector == 0)
    return NULL;

  /* Read the GELI header.  */
  err = holy_disk_read (disk, sector - 1, 0, sizeof (header), &header);
  if (err)
    return NULL;

  /* Look for GELI magic sequence.  */
  if (holy_memcmp (header.magic, GELI_MAGIC, sizeof (GELI_MAGIC))
      || holy_le_to_cpu32 (header.version) > 7
      || holy_le_to_cpu32 (header.version) < 1)
    {
      holy_dprintf ("geli", "wrong magic %02x\n", header.magic[0]);
      return NULL;
    }

  if ((holy_le_to_cpu32 (header.sector_size)
       & (holy_le_to_cpu32 (header.sector_size) - 1))
      || holy_le_to_cpu32 (header.sector_size) == 0)
    {
      holy_dprintf ("geli", "incorrect sector size %d\n",
		    holy_le_to_cpu32 (header.sector_size));
      return NULL;
    }

  if (holy_le_to_cpu32 (header.flags) & holy_GELI_FLAGS_ONETIME)
    {
      holy_dprintf ("geli", "skipping one-time volume\n");
      return NULL;
    }

  if (boot_only && !(holy_le_to_cpu32 (header.flags) & holy_GELI_FLAGS_BOOT))
    {
      holy_dprintf ("geli", "not a boot volume\n");
      return NULL;
    }    

  gcry_err = make_uuid (&header, uuid);
  if (gcry_err)
    {
      holy_crypto_gcry_error (gcry_err);
      return NULL;
    }

  if (check_uuid && holy_strcasecmp (check_uuid, uuid) != 0)
    {
      holy_dprintf ("geli", "%s != %s\n", uuid, check_uuid);
      return NULL;
    }

  if (holy_le_to_cpu16 (header.alg) >= ARRAY_SIZE (algorithms)
      || algorithms[holy_le_to_cpu16 (header.alg)] == NULL)
    {
      holy_error (holy_ERR_FILE_NOT_FOUND, "Cipher 0x%x unknown",
		  holy_le_to_cpu16 (header.alg));
      return NULL;
    }

  ciphername = algorithms[holy_le_to_cpu16 (header.alg)];
  ciph = holy_crypto_lookup_cipher_by_name (ciphername);
  if (!ciph)
    {
      holy_error (holy_ERR_FILE_NOT_FOUND, "Cipher %s isn't available",
		  ciphername);
      return NULL;
    }

  /* Configure the cipher used for the bulk data.  */
  cipher = holy_crypto_cipher_open (ciph);
  if (!cipher)
    return NULL;

  if (holy_le_to_cpu16 (header.alg) == 0x16)
    {
      secondary_cipher = holy_crypto_cipher_open (ciph);
      if (!secondary_cipher)
	{
	  holy_crypto_cipher_close (cipher);
	  return NULL;
	}

    }

  if (holy_le_to_cpu16 (header.keylen) > 1024)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "invalid keysize %d",
		  holy_le_to_cpu16 (header.keylen));
      holy_crypto_cipher_close (cipher);
      holy_crypto_cipher_close (secondary_cipher);
      return NULL;
    }

  newdev = holy_zalloc (sizeof (struct holy_cryptodisk));
  if (!newdev)
    {
      holy_crypto_cipher_close (cipher);
      holy_crypto_cipher_close (secondary_cipher);
      return NULL;
    }
  newdev->cipher = cipher;
  newdev->secondary_cipher = secondary_cipher;
  newdev->offset = 0;
  newdev->source_disk = NULL;
  newdev->benbi_log = 0;
  if (holy_le_to_cpu16 (header.alg) == 0x16)
    {
      newdev->mode = holy_CRYPTODISK_MODE_XTS;
      newdev->mode_iv = holy_CRYPTODISK_MODE_IV_BYTECOUNT64;
    }
  else
    {
      newdev->mode = holy_CRYPTODISK_MODE_CBC;
      newdev->mode_iv = holy_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH;
    }
  newdev->essiv_cipher = NULL;
  newdev->essiv_hash = NULL;
  newdev->hash = holy_MD_SHA512;
  newdev->iv_hash = holy_MD_SHA256;

  for (newdev->log_sector_size = 0;
       (1U << newdev->log_sector_size) < holy_le_to_cpu32 (header.sector_size);
       newdev->log_sector_size++);

  if (holy_le_to_cpu32 (header.version) >= 5)
    {
      newdev->rekey = geli_rekey;
      newdev->rekey_shift = 20;
    }

  newdev->modname = "geli";

  newdev->total_length = holy_disk_get_size (disk) - 1;
  holy_memcpy (newdev->uuid, uuid, sizeof (newdev->uuid));
  COMPILE_TIME_ASSERT (sizeof (newdev->uuid) >= 32 * 2 + 1);
  return newdev;
}

static holy_err_t
recover_key (holy_disk_t source, holy_cryptodisk_t dev)
{
  holy_size_t keysize;
  holy_uint8_t digest[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t geomkey[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t verify_key[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t zero[holy_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  holy_uint8_t geli_cipher_key[64];
  char passphrase[MAX_PASSPHRASE] = "";
  unsigned i;
  gcry_err_code_t gcry_err;
  struct holy_geli_phdr header;
  char *tmp;
  holy_disk_addr_t sector;
  holy_err_t err;

  if (dev->cipher->cipher->blocksize > holy_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return holy_error (holy_ERR_BUG, "cipher block is too long");

  if (dev->hash->mdlen > holy_CRYPTO_MAX_MDLEN)
    return holy_error (holy_ERR_BUG, "mdlen is too long");

  sector = holy_disk_get_size (source);
  if (sector == holy_DISK_SIZE_UNKNOWN || sector == 0)
    return holy_error (holy_ERR_BUG, "not a geli");

  /* Read the GELI header.  */
  err = holy_disk_read (source, sector - 1, 0, sizeof (header), &header);
  if (err)
    return err;

  keysize = holy_le_to_cpu16 (header.keylen) / holy_CHAR_BIT;
  holy_memset (zero, 0, sizeof (zero));

  holy_puts_ (N_("Attempting to decrypt master key..."));

  /* Get the passphrase from the user.  */
  tmp = NULL;
  if (source->partition)
    tmp = holy_partition_get_name (source->partition);
  holy_printf_ (N_("Enter passphrase for %s%s%s (%s): "), source->name,
		source->partition ? "," : "", tmp ? : "",
		dev->uuid);
  holy_free (tmp);
  if (!holy_password_get (passphrase, MAX_PASSPHRASE))
    return holy_error (holy_ERR_BAD_ARGUMENT, "Passphrase not supplied");

  /* Calculate the PBKDF2 of the user supplied passphrase.  */
  if (holy_le_to_cpu32 (header.niter) != 0)
    {
      holy_uint8_t pbkdf_key[64];
      gcry_err = holy_crypto_pbkdf2 (dev->hash, (holy_uint8_t *) passphrase,
				     holy_strlen (passphrase),
				     header.salt,
				     sizeof (header.salt),
				     holy_le_to_cpu32 (header.niter),
				     pbkdf_key, sizeof (pbkdf_key));

      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);

      gcry_err = holy_crypto_hmac_buffer (dev->hash, NULL, 0, pbkdf_key,
					  sizeof (pbkdf_key), geomkey);
      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);
    }
  else
    {
      struct holy_crypto_hmac_handle *hnd;

      hnd = holy_crypto_hmac_init (dev->hash, NULL, 0);
      if (!hnd)
	return holy_crypto_gcry_error (GPG_ERR_OUT_OF_MEMORY);

      holy_crypto_hmac_write (hnd, header.salt, sizeof (header.salt));
      holy_crypto_hmac_write (hnd, passphrase, holy_strlen (passphrase));

      gcry_err = holy_crypto_hmac_fini (hnd, geomkey);
      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);
    }

  gcry_err = holy_crypto_hmac_buffer (dev->hash, geomkey,
				      dev->hash->mdlen, "\1", 1, digest);
  if (gcry_err)
    return holy_crypto_gcry_error (gcry_err);

  gcry_err = holy_crypto_hmac_buffer (dev->hash, geomkey,
				      dev->hash->mdlen, "\0", 1, verify_key);
  if (gcry_err)
    return holy_crypto_gcry_error (gcry_err);

  holy_dprintf ("geli", "keylen = %" PRIuholy_SIZE "\n", keysize);

  /* Try to recover master key from each active keyslot.  */
  for (i = 0; i < ARRAY_SIZE (header.keys); i++)
    {
      struct holy_geli_key candidate_key;
      holy_uint8_t key_hmac[holy_CRYPTO_MAX_MDLEN];

      /* Check if keyslot is enabled.  */
      if (! (header.keys_used & (1 << i)))
	  continue;

      holy_dprintf ("geli", "Trying keyslot %d\n", i);

      gcry_err = holy_crypto_cipher_set_key (dev->cipher,
					     digest, keysize);
      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);

      gcry_err = holy_crypto_cbc_decrypt (dev->cipher, &candidate_key,
					  &header.keys[i],
					  sizeof (candidate_key),
					  zero);
      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);

      gcry_err = holy_crypto_hmac_buffer (dev->hash, verify_key,
					  dev->hash->mdlen,
					  &candidate_key,
					  (sizeof (candidate_key)
					   - sizeof (candidate_key.hmac)),
					  key_hmac);
      if (gcry_err)
	return holy_crypto_gcry_error (gcry_err);

      if (holy_memcmp (candidate_key.hmac, key_hmac, dev->hash->mdlen) != 0)
	continue;
      holy_printf_ (N_("Slot %d opened\n"), i);

      if (holy_le_to_cpu32 (header.version) >= 7)
        {
          /* GELI >=7 uses the cipher_key */
	  holy_memcpy (geli_cipher_key, candidate_key.cipher_key,
		sizeof (candidate_key.cipher_key));
        }
      else
        {
          /* GELI <=6 uses the iv_key */
	  holy_memcpy (geli_cipher_key, candidate_key.iv_key,
		sizeof (candidate_key.iv_key));
        }

      /* Set the master key.  */
      if (!dev->rekey)
	{
	  holy_size_t real_keysize = keysize;
	  if (holy_le_to_cpu16 (header.alg) == 0x16)
	    real_keysize *= 2;
	  gcry_err = holy_cryptodisk_setkey (dev, candidate_key.cipher_key,
					     real_keysize); 
	  if (gcry_err)
	    return holy_crypto_gcry_error (gcry_err);
	}
      else
	{
	  holy_size_t real_keysize = keysize;
	  if (holy_le_to_cpu16 (header.alg) == 0x16)
	    real_keysize *= 2;

	  holy_memcpy (dev->rekey_key, geli_cipher_key,
		       sizeof (geli_cipher_key));
	  dev->rekey_derived_size = real_keysize;
	  dev->last_rekey = -1;
	  COMPILE_TIME_ASSERT (sizeof (dev->rekey_key)
		       >= sizeof (geli_cipher_key));
	}

      dev->iv_prefix_len = sizeof (candidate_key.iv_key);
      holy_memcpy (dev->iv_prefix, candidate_key.iv_key,
		   sizeof (candidate_key.iv_key));

      COMPILE_TIME_ASSERT (sizeof (dev->iv_prefix) >= sizeof (candidate_key.iv_key));

      return holy_ERR_NONE;
    }

  return holy_ACCESS_DENIED;
}

struct holy_cryptodisk_dev geli_crypto = {
  .scan = configure_ciphers,
  .recover_key = recover_key
};

holy_MOD_INIT (geli)
{
  holy_cryptodisk_dev_register (&geli_crypto);
}

holy_MOD_FINI (geli)
{
  holy_cryptodisk_dev_unregister (&geli_crypto);
}
