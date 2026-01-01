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

#define MAX_PASSPHRASE 256

#define LUKS_KEY_ENABLED  0x00AC71F3

/* On disk LUKS header */
struct holy_luks_phdr
{
  holy_uint8_t magic[6];
#define LUKS_MAGIC        "LUKS\xBA\xBE"
  holy_uint16_t version;
  char cipherName[32];
  char cipherMode[32];
  char hashSpec[32];
  holy_uint32_t payloadOffset;
  holy_uint32_t keyBytes;
  holy_uint8_t mkDigest[20];
  holy_uint8_t mkDigestSalt[32];
  holy_uint32_t mkDigestIterations;
  char uuid[40];
  struct
  {
    holy_uint32_t active;
    holy_uint32_t passwordIterations;
    holy_uint8_t passwordSalt[32];
    holy_uint32_t keyMaterialOffset;
    holy_uint32_t stripes;
  } keyblock[8];
} holy_PACKED;

typedef struct holy_luks_phdr *holy_luks_phdr_t;

gcry_err_code_t AF_merge (const gcry_md_spec_t * hash, holy_uint8_t * src,
			  holy_uint8_t * dst, holy_size_t blocksize,
			  holy_size_t blocknumbers);

static holy_cryptodisk_t
configure_ciphers (holy_disk_t disk, const char *check_uuid,
		   int check_boot)
{
  holy_cryptodisk_t newdev;
  const char *iptr;
  struct holy_luks_phdr header;
  char *optr;
  char uuid[sizeof (header.uuid) + 1];
  char ciphername[sizeof (header.cipherName) + 1];
  char ciphermode[sizeof (header.cipherMode) + 1];
  char *cipheriv = NULL;
  char hashspec[sizeof (header.hashSpec) + 1];
  holy_crypto_cipher_handle_t cipher = NULL, secondary_cipher = NULL;
  holy_crypto_cipher_handle_t essiv_cipher = NULL;
  const gcry_md_spec_t *hash = NULL, *essiv_hash = NULL;
  const struct gcry_cipher_spec *ciph;
  holy_cryptodisk_mode_t mode;
  holy_cryptodisk_mode_iv_t mode_iv = holy_CRYPTODISK_MODE_IV_PLAIN64;
  int benbi_log = 0;
  holy_err_t err;

  if (check_boot)
    return NULL;

  /* Read the LUKS header.  */
  err = holy_disk_read (disk, 0, 0, sizeof (header), &header);
  if (err)
    {
      if (err == holy_ERR_OUT_OF_RANGE)
	holy_errno = holy_ERR_NONE;
      return NULL;
    }

  /* Look for LUKS magic sequence.  */
  if (holy_memcmp (header.magic, LUKS_MAGIC, sizeof (header.magic))
      || holy_be_to_cpu16 (header.version) != 1)
    return NULL;

  optr = uuid;
  for (iptr = header.uuid; iptr < &header.uuid[ARRAY_SIZE (header.uuid)];
       iptr++)
    {
      if (*iptr != '-')
	*optr++ = *iptr;
    }
  *optr = 0;

  if (check_uuid && holy_strcasecmp (check_uuid, uuid) != 0)
    {
      holy_dprintf ("luks", "%s != %s\n", uuid, check_uuid);
      return NULL;
    }

  /* Make sure that strings are null terminated.  */
  holy_memcpy (ciphername, header.cipherName, sizeof (header.cipherName));
  ciphername[sizeof (header.cipherName)] = 0;
  holy_memcpy (ciphermode, header.cipherMode, sizeof (header.cipherMode));
  ciphermode[sizeof (header.cipherMode)] = 0;
  holy_memcpy (hashspec, header.hashSpec, sizeof (header.hashSpec));
  hashspec[sizeof (header.hashSpec)] = 0;

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

  if (holy_be_to_cpu32 (header.keyBytes) > 1024)
    {
      holy_error (holy_ERR_BAD_ARGUMENT, "invalid keysize %d",
		  holy_be_to_cpu32 (header.keyBytes));
      holy_crypto_cipher_close (cipher);
      return NULL;
    }

  /* Configure the cipher mode.  */
  if (holy_strcmp (ciphermode, "ecb") == 0)
    {
      mode = holy_CRYPTODISK_MODE_ECB;
      mode_iv = holy_CRYPTODISK_MODE_IV_PLAIN;
      cipheriv = NULL;
    }
  else if (holy_strcmp (ciphermode, "plain") == 0)
    {
      mode = holy_CRYPTODISK_MODE_CBC;
      mode_iv = holy_CRYPTODISK_MODE_IV_PLAIN;
      cipheriv = NULL;
    }
  else if (holy_memcmp (ciphermode, "cbc-", sizeof ("cbc-") - 1) == 0)
    {
      mode = holy_CRYPTODISK_MODE_CBC;
      cipheriv = ciphermode + sizeof ("cbc-") - 1;
    }
  else if (holy_memcmp (ciphermode, "pcbc-", sizeof ("pcbc-") - 1) == 0)
    {
      mode = holy_CRYPTODISK_MODE_PCBC;
      cipheriv = ciphermode + sizeof ("pcbc-") - 1;
    }
  else if (holy_memcmp (ciphermode, "xts-", sizeof ("xts-") - 1) == 0)
    {
      mode = holy_CRYPTODISK_MODE_XTS;
      cipheriv = ciphermode + sizeof ("xts-") - 1;
      secondary_cipher = holy_crypto_cipher_open (ciph);
      if (!secondary_cipher)
	{
	  holy_crypto_cipher_close (cipher);
	  return NULL;
	}
      if (cipher->cipher->blocksize != holy_CRYPTODISK_GF_BYTES)
	{
	  holy_error (holy_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      cipher->cipher->blocksize);
	  holy_crypto_cipher_close (cipher);
	  holy_crypto_cipher_close (secondary_cipher);
	  return NULL;
	}
      if (secondary_cipher->cipher->blocksize != holy_CRYPTODISK_GF_BYTES)
	{
	  holy_crypto_cipher_close (cipher);
	  holy_error (holy_ERR_BAD_ARGUMENT, "Unsupported XTS block size: %d",
		      secondary_cipher->cipher->blocksize);
	  holy_crypto_cipher_close (secondary_cipher);
	  return NULL;
	}
    }
  else if (holy_memcmp (ciphermode, "lrw-", sizeof ("lrw-") - 1) == 0)
    {
      mode = holy_CRYPTODISK_MODE_LRW;
      cipheriv = ciphermode + sizeof ("lrw-") - 1;
      if (cipher->cipher->blocksize != holy_CRYPTODISK_GF_BYTES)
	{
	  holy_error (holy_ERR_BAD_ARGUMENT, "Unsupported LRW block size: %d",
		      cipher->cipher->blocksize);
	  holy_crypto_cipher_close (cipher);
	  return NULL;
	}
    }
  else
    {
      holy_crypto_cipher_close (cipher);
      holy_error (holy_ERR_BAD_ARGUMENT, "Unknown cipher mode: %s",
		  ciphermode);
      return NULL;
    }

  if (cipheriv == NULL);
  else if (holy_memcmp (cipheriv, "plain", sizeof ("plain") - 1) == 0)
      mode_iv = holy_CRYPTODISK_MODE_IV_PLAIN;
  else if (holy_memcmp (cipheriv, "plain64", sizeof ("plain64") - 1) == 0)
      mode_iv = holy_CRYPTODISK_MODE_IV_PLAIN64;
  else if (holy_memcmp (cipheriv, "benbi", sizeof ("benbi") - 1) == 0)
    {
      if (cipher->cipher->blocksize & (cipher->cipher->blocksize - 1)
	  || cipher->cipher->blocksize == 0)
	holy_error (holy_ERR_BAD_ARGUMENT, "Unsupported benbi blocksize: %d",
		    cipher->cipher->blocksize);
	/* FIXME should we return an error here? */
      for (benbi_log = 0; 
	   (cipher->cipher->blocksize << benbi_log) < holy_DISK_SECTOR_SIZE;
	   benbi_log++);
      mode_iv = holy_CRYPTODISK_MODE_IV_BENBI;
    }
  else if (holy_memcmp (cipheriv, "null", sizeof ("null") - 1) == 0)
      mode_iv = holy_CRYPTODISK_MODE_IV_NULL;
  else if (holy_memcmp (cipheriv, "essiv:", sizeof ("essiv:") - 1) == 0)
    {
      char *hash_str = cipheriv + 6;

      mode_iv = holy_CRYPTODISK_MODE_IV_ESSIV;

      /* Configure the hash and cipher used for ESSIV.  */
      essiv_hash = holy_crypto_lookup_md_by_name (hash_str);
      if (!essiv_hash)
	{
	  holy_crypto_cipher_close (cipher);
	  holy_crypto_cipher_close (secondary_cipher);
	  holy_error (holy_ERR_FILE_NOT_FOUND,
		      "Couldn't load %s hash", hash_str);
	  return NULL;
	}
      essiv_cipher = holy_crypto_cipher_open (ciph);
      if (!essiv_cipher)
	{
	  holy_crypto_cipher_close (cipher);
	  holy_crypto_cipher_close (secondary_cipher);
	  return NULL;
	}
    }
  else
    {
      holy_crypto_cipher_close (cipher);
      holy_crypto_cipher_close (secondary_cipher);
      holy_error (holy_ERR_BAD_ARGUMENT, "Unknown IV mode: %s",
		  cipheriv);
      return NULL;
    }

  /* Configure the hash used for the AF splitter and HMAC.  */
  hash = holy_crypto_lookup_md_by_name (hashspec);
  if (!hash)
    {
      holy_crypto_cipher_close (cipher);
      holy_crypto_cipher_close (essiv_cipher);
      holy_crypto_cipher_close (secondary_cipher);
      holy_error (holy_ERR_FILE_NOT_FOUND, "Couldn't load %s hash",
		  hashspec);
      return NULL;
    }

  newdev = holy_zalloc (sizeof (struct holy_cryptodisk));
  if (!newdev)
    {
      holy_crypto_cipher_close (cipher);
      holy_crypto_cipher_close (essiv_cipher);
      holy_crypto_cipher_close (secondary_cipher);
      return NULL;
    }
  newdev->cipher = cipher;
  newdev->offset = holy_be_to_cpu32 (header.payloadOffset);
  newdev->source_disk = NULL;
  newdev->benbi_log = benbi_log;
  newdev->mode = mode;
  newdev->mode_iv = mode_iv;
  newdev->secondary_cipher = secondary_cipher;
  newdev->essiv_cipher = essiv_cipher;
  newdev->essiv_hash = essiv_hash;
  newdev->hash = hash;
  newdev->log_sector_size = 9;
  newdev->total_length = holy_disk_get_size (disk) - newdev->offset;
  holy_memcpy (newdev->uuid, uuid, sizeof (newdev->uuid));
  newdev->modname = "luks";
  COMPILE_TIME_ASSERT (sizeof (newdev->uuid) >= sizeof (uuid));
  return newdev;
}

static holy_err_t
luks_recover_key (holy_disk_t source,
		  holy_cryptodisk_t dev)
{
  struct holy_luks_phdr header;
  holy_size_t keysize;
  holy_uint8_t *split_key = NULL;
  char passphrase[MAX_PASSPHRASE] = "";
  holy_uint8_t candidate_digest[sizeof (header.mkDigest)];
  unsigned i;
  holy_size_t length;
  holy_err_t err;
  holy_size_t max_stripes = 1;
  char *tmp;

  err = holy_disk_read (source, 0, 0, sizeof (header), &header);
  if (err)
    return err;

  holy_puts_ (N_("Attempting to decrypt master key..."));
  keysize = holy_be_to_cpu32 (header.keyBytes);
  if (keysize > holy_CRYPTODISK_MAX_KEYLEN)
    return holy_error (holy_ERR_BAD_FS, "key is too long");

  for (i = 0; i < ARRAY_SIZE (header.keyblock); i++)
    if (holy_be_to_cpu32 (header.keyblock[i].active) == LUKS_KEY_ENABLED
	&& holy_be_to_cpu32 (header.keyblock[i].stripes) > max_stripes)
      max_stripes = holy_be_to_cpu32 (header.keyblock[i].stripes);

  split_key = holy_malloc (keysize * max_stripes);
  if (!split_key)
    return holy_errno;

  /* Get the passphrase from the user.  */
  tmp = NULL;
  if (source->partition)
    tmp = holy_partition_get_name (source->partition);
  holy_printf_ (N_("Enter passphrase for %s%s%s (%s): "), source->name,
	       source->partition ? "," : "", tmp ? : "",
	       dev->uuid);
  holy_free (tmp);
  if (!holy_password_get (passphrase, MAX_PASSPHRASE))
    {
      holy_free (split_key);
      return holy_error (holy_ERR_BAD_ARGUMENT, "Passphrase not supplied");
    }

  /* Try to recover master key from each active keyslot.  */
  for (i = 0; i < ARRAY_SIZE (header.keyblock); i++)
    {
      gcry_err_code_t gcry_err;
      holy_uint8_t candidate_key[holy_CRYPTODISK_MAX_KEYLEN];
      holy_uint8_t digest[holy_CRYPTODISK_MAX_KEYLEN];

      /* Check if keyslot is enabled.  */
      if (holy_be_to_cpu32 (header.keyblock[i].active) != LUKS_KEY_ENABLED)
	continue;

      holy_dprintf ("luks", "Trying keyslot %d\n", i);

      /* Calculate the PBKDF2 of the user supplied passphrase.  */
      gcry_err = holy_crypto_pbkdf2 (dev->hash, (holy_uint8_t *) passphrase,
				     holy_strlen (passphrase),
				     header.keyblock[i].passwordSalt,
				     sizeof (header.keyblock[i].passwordSalt),
				     holy_be_to_cpu32 (header.keyblock[i].
						       passwordIterations),
				     digest, keysize);

      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      holy_dprintf ("luks", "PBKDF2 done\n");

      gcry_err = holy_cryptodisk_setkey (dev, digest, keysize);
      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      length = (keysize * holy_be_to_cpu32 (header.keyblock[i].stripes));

      /* Read and decrypt the key material from the disk.  */
      err = holy_disk_read (source,
			    holy_be_to_cpu32 (header.keyblock
					      [i].keyMaterialOffset), 0,
			    length, split_key);
      if (err)
	{
	  holy_free (split_key);
	  return err;
	}

      gcry_err = holy_cryptodisk_decrypt (dev, split_key, length, 0);
      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      /* Merge the decrypted key material to get the candidate master key.  */
      gcry_err = AF_merge (dev->hash, split_key, candidate_key, keysize,
			   holy_be_to_cpu32 (header.keyblock[i].stripes));
      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      holy_dprintf ("luks", "candidate key recovered\n");

      /* Calculate the PBKDF2 of the candidate master key.  */
      gcry_err = holy_crypto_pbkdf2 (dev->hash, candidate_key,
				     holy_be_to_cpu32 (header.keyBytes),
				     header.mkDigestSalt,
				     sizeof (header.mkDigestSalt),
				     holy_be_to_cpu32
				     (header.mkDigestIterations),
				     candidate_digest,
				     sizeof (candidate_digest));
      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      /* Compare the calculated PBKDF2 to the digest stored
         in the header to see if it's correct.  */
      if (holy_memcmp (candidate_digest, header.mkDigest,
		       sizeof (header.mkDigest)) != 0)
	{
	  holy_dprintf ("luks", "bad digest\n");
	  continue;
	}

      /* TRANSLATORS: It's a cryptographic key slot: one element of an array
	 where each element is either empty or holds a key.  */
      holy_printf_ (N_("Slot %d opened\n"), i);

      /* Set the master key.  */
      gcry_err = holy_cryptodisk_setkey (dev, candidate_key, keysize);
      if (gcry_err)
	{
	  holy_free (split_key);
	  return holy_crypto_gcry_error (gcry_err);
	}

      holy_free (split_key);

      return holy_ERR_NONE;
    }

  holy_free (split_key);
  return holy_ACCESS_DENIED;
}

struct holy_cryptodisk_dev luks_crypto = {
  .scan = configure_ciphers,
  .recover_key = luks_recover_key
};

holy_MOD_INIT (luks)
{
  COMPILE_TIME_ASSERT (sizeof (((struct holy_luks_phdr *) 0)->uuid)
		       < holy_CRYPTODISK_MAX_UUID_LENGTH);
  holy_cryptodisk_dev_register (&luks_crypto);
}

holy_MOD_FINI (luks)
{
  holy_cryptodisk_dev_unregister (&luks_crypto);
}
