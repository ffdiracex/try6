/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/cryptodisk.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/fs.h>
#include <holy/file.h>
#include <holy/procfs.h>
#include <holy/partition.h>

#ifdef holy_UTIL
#include <holy/emu/hostdisk.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

holy_cryptodisk_dev_t holy_cryptodisk_list;

static const struct holy_arg_option options[] =
  {
    {"uuid", 'u', 0, N_("Mount by UUID."), 0, 0},
    /* TRANSLATORS: It's still restricted to cryptodisks only.  */
    {"all", 'a', 0, N_("Mount all."), 0, 0},
    {"boot", 'b', 0, N_("Mount all volumes with `boot' flag set."), 0, 0},
    {0, 0, 0, 0, 0, 0}
  };

/* Our irreducible polynom is x^128+x^7+x^2+x+1. Lowest byte of it is:  */
#define GF_POLYNOM 0x87
static inline int GF_PER_SECTOR (const struct holy_cryptodisk *dev)
{
  return 1U << (dev->log_sector_size - holy_CRYPTODISK_GF_LOG_BYTES);
}

static holy_cryptodisk_t cryptodisk_list = NULL;
static holy_uint8_t last_cryptodisk_id = 0;

static void
gf_mul_x (holy_uint8_t *g)
{
  int over = 0, over2 = 0;
  unsigned j;

  for (j = 0; j < holy_CRYPTODISK_GF_BYTES; j++)
    {
      over2 = !!(g[j] & 0x80);
      g[j] <<= 1;
      g[j] |= over;
      over = over2;
    }
  if (over)
    g[0] ^= GF_POLYNOM;
}


static void
gf_mul_x_be (holy_uint8_t *g)
{
  int over = 0, over2 = 0;
  int j;

  for (j = (int) holy_CRYPTODISK_GF_BYTES - 1; j >= 0; j--)
    {
      over2 = !!(g[j] & 0x80);
      g[j] <<= 1;
      g[j] |= over;
      over = over2;
    }
  if (over)
    g[holy_CRYPTODISK_GF_BYTES - 1] ^= GF_POLYNOM;
}

static void
gf_mul_be (holy_uint8_t *o, const holy_uint8_t *a, const holy_uint8_t *b)
{
  unsigned i;
  holy_uint8_t t[holy_CRYPTODISK_GF_BYTES];
  holy_memset (o, 0, holy_CRYPTODISK_GF_BYTES);
  holy_memcpy (t, b, holy_CRYPTODISK_GF_BYTES);
  for (i = 0; i < holy_CRYPTODISK_GF_SIZE; i++)
    {
      if (((a[holy_CRYPTODISK_GF_BYTES - i / holy_CHAR_BIT - 1] >> (i % holy_CHAR_BIT))) & 1)
	holy_crypto_xor (o, o, t, holy_CRYPTODISK_GF_BYTES);
      gf_mul_x_be (t);
    }
}

static gcry_err_code_t
holy_crypto_pcbc_decrypt (holy_crypto_cipher_handle_t cipher,
			 void *out, void *in, holy_size_t size,
			 void *iv)
{
  holy_uint8_t *inptr, *outptr, *end;
  holy_uint8_t ivt[holy_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  holy_size_t blocksize;
  if (!cipher->cipher->decrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  if (blocksize > holy_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;
  end = (holy_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    {
      holy_memcpy (ivt, inptr, blocksize);
      cipher->cipher->decrypt (cipher->ctx, outptr, inptr);
      holy_crypto_xor (outptr, outptr, iv, blocksize);
      holy_crypto_xor (iv, ivt, outptr, blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

static gcry_err_code_t
holy_crypto_pcbc_encrypt (holy_crypto_cipher_handle_t cipher,
			  void *out, void *in, holy_size_t size,
			  void *iv)
{
  holy_uint8_t *inptr, *outptr, *end;
  holy_uint8_t ivt[holy_CRYPTO_MAX_CIPHER_BLOCKSIZE];
  holy_size_t blocksize;
  if (!cipher->cipher->encrypt)
    return GPG_ERR_NOT_SUPPORTED;
  blocksize = cipher->cipher->blocksize;
  if (blocksize > holy_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;
  if (blocksize == 0 || (((blocksize - 1) & blocksize) != 0)
      || ((size & (blocksize - 1)) != 0))
    return GPG_ERR_INV_ARG;
  end = (holy_uint8_t *) in + size;
  for (inptr = in, outptr = out; inptr < end;
       inptr += blocksize, outptr += blocksize)
    {
      holy_memcpy (ivt, inptr, blocksize);
      holy_crypto_xor (outptr, outptr, iv, blocksize);
      cipher->cipher->encrypt (cipher->ctx, outptr, inptr);
      holy_crypto_xor (iv, ivt, outptr, blocksize);
    }
  return GPG_ERR_NO_ERROR;
}

struct lrw_sector
{
  holy_uint8_t low[holy_CRYPTODISK_GF_BYTES];
  holy_uint8_t high[holy_CRYPTODISK_GF_BYTES];
  holy_uint8_t low_byte, low_byte_c;
};

static void
generate_lrw_sector (struct lrw_sector *sec,
		     const struct holy_cryptodisk *dev,
		     const holy_uint8_t *iv)
{
  holy_uint8_t idx[holy_CRYPTODISK_GF_BYTES];
  holy_uint16_t c;
  int j;
  holy_memcpy (idx, iv, holy_CRYPTODISK_GF_BYTES);
  sec->low_byte = (idx[holy_CRYPTODISK_GF_BYTES - 1]
		   & (GF_PER_SECTOR (dev) - 1));
  sec->low_byte_c = (((GF_PER_SECTOR (dev) - 1) & ~sec->low_byte) + 1);
  idx[holy_CRYPTODISK_GF_BYTES - 1] &= ~(GF_PER_SECTOR (dev) - 1);
  gf_mul_be (sec->low, dev->lrw_key, idx);
  if (!sec->low_byte)
    return;

  c = idx[holy_CRYPTODISK_GF_BYTES - 1] + GF_PER_SECTOR (dev);
  if (c & 0x100)
    {
      for (j = holy_CRYPTODISK_GF_BYTES - 2; j >= 0; j--)
	{
	  idx[j]++;
	  if (idx[j] != 0)
	    break;
	}
    }
  idx[holy_CRYPTODISK_GF_BYTES - 1] = c;
  gf_mul_be (sec->high, dev->lrw_key, idx);
}

static void __attribute__ ((unused))
lrw_xor (const struct lrw_sector *sec,
	 const struct holy_cryptodisk *dev,
	 holy_uint8_t *b)
{
  unsigned i;

  for (i = 0; i < sec->low_byte_c * holy_CRYPTODISK_GF_BYTES;
       i += holy_CRYPTODISK_GF_BYTES)
    holy_crypto_xor (b + i, b + i, sec->low, holy_CRYPTODISK_GF_BYTES);
  holy_crypto_xor (b, b, dev->lrw_precalc + holy_CRYPTODISK_GF_BYTES * sec->low_byte,
		   sec->low_byte_c * holy_CRYPTODISK_GF_BYTES);
  if (!sec->low_byte)
    return;

  for (i = sec->low_byte_c * holy_CRYPTODISK_GF_BYTES;
       i < (1U << dev->log_sector_size); i += holy_CRYPTODISK_GF_BYTES)
    holy_crypto_xor (b + i, b + i, sec->high, holy_CRYPTODISK_GF_BYTES);
  holy_crypto_xor (b + sec->low_byte_c * holy_CRYPTODISK_GF_BYTES,
		   b + sec->low_byte_c * holy_CRYPTODISK_GF_BYTES,
		   dev->lrw_precalc, sec->low_byte * holy_CRYPTODISK_GF_BYTES);
}

static gcry_err_code_t
holy_cryptodisk_endecrypt (struct holy_cryptodisk *dev,
			   holy_uint8_t * data, holy_size_t len,
			   holy_disk_addr_t sector, int do_encrypt)
{
  holy_size_t i;
  gcry_err_code_t err;

  if (dev->cipher->cipher->blocksize > holy_CRYPTO_MAX_CIPHER_BLOCKSIZE)
    return GPG_ERR_INV_ARG;

  /* The only mode without IV.  */
  if (dev->mode == holy_CRYPTODISK_MODE_ECB && !dev->rekey)
    return (do_encrypt ? holy_crypto_ecb_encrypt (dev->cipher, data, data, len)
	    : holy_crypto_ecb_decrypt (dev->cipher, data, data, len));

  for (i = 0; i < len; i += (1U << dev->log_sector_size))
    {
      holy_size_t sz = ((dev->cipher->cipher->blocksize
			 + sizeof (holy_uint32_t) - 1)
			/ sizeof (holy_uint32_t));
      holy_uint32_t iv[(holy_CRYPTO_MAX_CIPHER_BLOCKSIZE + 3) / 4];

      if (dev->rekey)
	{
	  holy_uint64_t zone = sector >> dev->rekey_shift;
	  if (zone != dev->last_rekey)
	    {
	      err = dev->rekey (dev, zone);
	      if (err)
		return err;
	      dev->last_rekey = zone;
	    }
	}

      holy_memset (iv, 0, sizeof (iv));
      switch (dev->mode_iv)
	{
	case holy_CRYPTODISK_MODE_IV_NULL:
	  break;
	case holy_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH:
	  {
	    holy_uint64_t tmp;
	    void *ctx;

	    ctx = holy_zalloc (dev->iv_hash->contextsize);
	    if (!ctx)
	      return GPG_ERR_OUT_OF_MEMORY;

	    tmp = holy_cpu_to_le64 (sector << dev->log_sector_size);
	    dev->iv_hash->init (ctx);
	    dev->iv_hash->write (ctx, dev->iv_prefix, dev->iv_prefix_len);
	    dev->iv_hash->write (ctx, &tmp, sizeof (tmp));
	    dev->iv_hash->final (ctx);

	    holy_memcpy (iv, dev->iv_hash->read (ctx), sizeof (iv));
	    holy_free (ctx);
	  }
	  break;
	case holy_CRYPTODISK_MODE_IV_PLAIN64:
	  iv[1] = holy_cpu_to_le32 (sector >> 32);
	  /* FALLTHROUGH */
	case holy_CRYPTODISK_MODE_IV_PLAIN:
	  iv[0] = holy_cpu_to_le32 (sector & 0xFFFFFFFF);
	  break;
	case holy_CRYPTODISK_MODE_IV_BYTECOUNT64:
	  iv[1] = holy_cpu_to_le32 (sector >> (32 - dev->log_sector_size));
	  iv[0] = holy_cpu_to_le32 ((sector << dev->log_sector_size)
				    & 0xFFFFFFFF);
	  break;
	case holy_CRYPTODISK_MODE_IV_BENBI:
	  {
	    holy_uint64_t num = (sector << dev->benbi_log) + 1;
	    iv[sz - 2] = holy_cpu_to_be32 (num >> 32);
	    iv[sz - 1] = holy_cpu_to_be32 (num & 0xFFFFFFFF);
	  }
	  break;
	case holy_CRYPTODISK_MODE_IV_ESSIV:
	  iv[0] = holy_cpu_to_le32 (sector & 0xFFFFFFFF);
	  err = holy_crypto_ecb_encrypt (dev->essiv_cipher, iv, iv,
					 dev->cipher->cipher->blocksize);
	  if (err)
	    return err;
	}

      switch (dev->mode)
	{
	case holy_CRYPTODISK_MODE_CBC:
	  if (do_encrypt)
	    err = holy_crypto_cbc_encrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size), iv);
	  else
	    err = holy_crypto_cbc_decrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size), iv);
	  if (err)
	    return err;
	  break;

	case holy_CRYPTODISK_MODE_PCBC:
	  if (do_encrypt)
	    err = holy_crypto_pcbc_encrypt (dev->cipher, data + i, data + i,
					    (1U << dev->log_sector_size), iv);
	  else
	    err = holy_crypto_pcbc_decrypt (dev->cipher, data + i, data + i,
					    (1U << dev->log_sector_size), iv);
	  if (err)
	    return err;
	  break;
	case holy_CRYPTODISK_MODE_XTS:
	  {
	    unsigned j;
	    err = holy_crypto_ecb_encrypt (dev->secondary_cipher, iv, iv,
					   dev->cipher->cipher->blocksize);
	    if (err)
	      return err;
	    
	    for (j = 0; j < (1U << dev->log_sector_size);
		 j += dev->cipher->cipher->blocksize)
	      {
		holy_crypto_xor (data + i + j, data + i + j, iv,
				 dev->cipher->cipher->blocksize);
		if (do_encrypt)
		  err = holy_crypto_ecb_encrypt (dev->cipher, data + i + j,
						 data + i + j,
						 dev->cipher->cipher->blocksize);
		else
		  err = holy_crypto_ecb_decrypt (dev->cipher, data + i + j,
						 data + i + j,
						 dev->cipher->cipher->blocksize);
		if (err)
		  return err;
		holy_crypto_xor (data + i + j, data + i + j, iv,
				 dev->cipher->cipher->blocksize);
		gf_mul_x ((holy_uint8_t *) iv);
	      }
	  }
	  break;
	case holy_CRYPTODISK_MODE_LRW:
	  {
	    struct lrw_sector sec;

	    generate_lrw_sector (&sec, dev, (holy_uint8_t *) iv);
	    lrw_xor (&sec, dev, data + i);

	    if (do_encrypt)
	      err = holy_crypto_ecb_encrypt (dev->cipher, data + i,
					     data + i,
					     (1U << dev->log_sector_size));
	    else
	      err = holy_crypto_ecb_decrypt (dev->cipher, data + i,
					     data + i,
					     (1U << dev->log_sector_size));
	    if (err)
	      return err;
	    lrw_xor (&sec, dev, data + i);
	  }
	  break;
	case holy_CRYPTODISK_MODE_ECB:
	  if (do_encrypt)
	    err = holy_crypto_ecb_encrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size));
	  else
	    err = holy_crypto_ecb_decrypt (dev->cipher, data + i, data + i,
					   (1U << dev->log_sector_size));
	  if (err)
	    return err;
	  break;
	default:
	  return GPG_ERR_NOT_IMPLEMENTED;
	}
      sector++;
    }
  return GPG_ERR_NO_ERROR;
}

gcry_err_code_t
holy_cryptodisk_decrypt (struct holy_cryptodisk *dev,
			 holy_uint8_t * data, holy_size_t len,
			 holy_disk_addr_t sector)
{
  return holy_cryptodisk_endecrypt (dev, data, len, sector, 0);
}

gcry_err_code_t
holy_cryptodisk_setkey (holy_cryptodisk_t dev, holy_uint8_t *key, holy_size_t keysize)
{
  gcry_err_code_t err;
  int real_keysize;

  real_keysize = keysize;
  if (dev->mode == holy_CRYPTODISK_MODE_XTS)
    real_keysize /= 2;
  if (dev->mode == holy_CRYPTODISK_MODE_LRW)
    real_keysize -= dev->cipher->cipher->blocksize;
	
  /* Set the PBKDF2 output as the cipher key.  */
  err = holy_crypto_cipher_set_key (dev->cipher, key, real_keysize);
  if (err)
    return err;
  holy_memcpy (dev->key, key, keysize);
  dev->keysize = keysize;

  /* Configure ESSIV if necessary.  */
  if (dev->mode_iv == holy_CRYPTODISK_MODE_IV_ESSIV)
    {
      holy_size_t essiv_keysize = dev->essiv_hash->mdlen;
      holy_uint8_t hashed_key[holy_CRYPTO_MAX_MDLEN];
      if (essiv_keysize > holy_CRYPTO_MAX_MDLEN)
	return GPG_ERR_INV_ARG;

      holy_crypto_hash (dev->essiv_hash, hashed_key, key, keysize);
      err = holy_crypto_cipher_set_key (dev->essiv_cipher,
					hashed_key, essiv_keysize);
      if (err)
	return err;
    }
  if (dev->mode == holy_CRYPTODISK_MODE_XTS)
    {
      err = holy_crypto_cipher_set_key (dev->secondary_cipher,
					key + real_keysize,
					keysize / 2);
      if (err)
	return err;
    }

  if (dev->mode == holy_CRYPTODISK_MODE_LRW)
    {
      unsigned i;
      holy_uint8_t idx[holy_CRYPTODISK_GF_BYTES];

      holy_free (dev->lrw_precalc);
      holy_memcpy (dev->lrw_key, key + real_keysize,
		   dev->cipher->cipher->blocksize);
      dev->lrw_precalc = holy_malloc ((1U << dev->log_sector_size));
      if (!dev->lrw_precalc)
	return GPG_ERR_OUT_OF_MEMORY;
      holy_memset (idx, 0, holy_CRYPTODISK_GF_BYTES);
      for (i = 0; i < (1U << dev->log_sector_size);
	   i += holy_CRYPTODISK_GF_BYTES)
	{
	  idx[holy_CRYPTODISK_GF_BYTES - 1] = i / holy_CRYPTODISK_GF_BYTES;
	  gf_mul_be (dev->lrw_precalc + i, idx, dev->lrw_key);
	}
    }
  return GPG_ERR_NO_ERROR;
}

static int
holy_cryptodisk_iterate (holy_disk_dev_iterate_hook_t hook, void *hook_data,
			 holy_disk_pull_t pull)
{
  holy_cryptodisk_t i;

  if (pull != holy_DISK_PULL_NONE)
    return 0;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    {
      char buf[30];
      holy_snprintf (buf, sizeof (buf), "crypto%lu", i->id);
      if (hook (buf, hook_data))
	return 1;
    }

  return holy_ERR_NONE;
}

static holy_err_t
holy_cryptodisk_open (const char *name, holy_disk_t disk)
{
  holy_cryptodisk_t dev;

  if (holy_memcmp (name, "crypto", sizeof ("crypto") - 1) != 0)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "No such device");

  if (holy_memcmp (name, "cryptouuid/", sizeof ("cryptouuid/") - 1) == 0)
    {
      for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
	if (holy_strcasecmp (name + sizeof ("cryptouuid/") - 1, dev->uuid) == 0)
	  break;
    }
  else
    {
      unsigned long id = holy_strtoul (name + sizeof ("crypto") - 1, 0, 0);
      if (holy_errno)
	return holy_error (holy_ERR_UNKNOWN_DEVICE, "No such device");
      /* Search for requested device in the list of CRYPTODISK devices.  */
      for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
	if (dev->id == id)
	  break;
    }
  if (!dev)
    return holy_error (holy_ERR_UNKNOWN_DEVICE, "No such device");

  disk->log_sector_size = dev->log_sector_size;

#ifdef holy_UTIL
  if (dev->cheat)
    {
      if (!holy_UTIL_FD_IS_VALID (dev->cheat_fd))
	dev->cheat_fd = holy_util_fd_open (dev->cheat, holy_UTIL_FD_O_RDONLY);
      if (!holy_UTIL_FD_IS_VALID (dev->cheat_fd))
	return holy_error (holy_ERR_IO, N_("cannot open `%s': %s"),
			   dev->cheat, holy_util_fd_strerror ());
    }
#endif

  if (!dev->source_disk)
    {
      holy_dprintf ("cryptodisk", "Opening device %s\n", name);
      /* Try to open the source disk and populate the requested disk.  */
      dev->source_disk = holy_disk_open (dev->source);
      if (!dev->source_disk)
	return holy_errno;
    }

  disk->data = dev;
  disk->total_sectors = dev->total_length;
  disk->max_agglomerate = holy_DISK_MAX_MAX_AGGLOMERATE;
  disk->id = dev->id;
  dev->ref++;
  return holy_ERR_NONE;
}

static void
holy_cryptodisk_close (holy_disk_t disk)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;
  holy_dprintf ("cryptodisk", "Closing disk\n");

  dev->ref--;

  if (dev->ref != 0)
    return;
#ifdef holy_UTIL
  if (dev->cheat)
    {
      holy_util_fd_close (dev->cheat_fd);
      dev->cheat_fd = holy_UTIL_FD_INVALID;
    }
#endif
  holy_disk_close (dev->source_disk);
  dev->source_disk = NULL;
}

static holy_err_t
holy_cryptodisk_read (holy_disk_t disk, holy_disk_addr_t sector,
		      holy_size_t size, char *buf)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;
  holy_err_t err;
  gcry_err_code_t gcry_err;

#ifdef holy_UTIL
  if (dev->cheat)
    {
      int r;
      r = holy_util_fd_seek (dev->cheat_fd, sector << disk->log_sector_size);
      if (r)
	return holy_error (holy_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
			   dev->cheat, holy_util_fd_strerror ());
      if (holy_util_fd_read (dev->cheat_fd, buf, size << disk->log_sector_size)
	  != (ssize_t) (size << disk->log_sector_size))
	return holy_error (holy_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   dev->cheat, holy_util_fd_strerror ());
      return holy_ERR_NONE;
    }
#endif

  holy_dprintf ("cryptodisk",
		"Reading %" PRIuholy_SIZE " sectors from sector 0x%"
		PRIxholy_UINT64_T " with offset of %" PRIuholy_UINT64_T "\n",
		size, sector, dev->offset);

  err = holy_disk_read (dev->source_disk,
			(sector << (disk->log_sector_size
				   - holy_DISK_SECTOR_BITS)) + dev->offset, 0,
			size << disk->log_sector_size, buf);
  if (err)
    {
      holy_dprintf ("cryptodisk", "holy_disk_read failed with error %d\n", err);
      return err;
    }
  gcry_err = holy_cryptodisk_endecrypt (dev, (holy_uint8_t *) buf,
					size << disk->log_sector_size,
					sector, 0);
  return holy_crypto_gcry_error (gcry_err);
}

static holy_err_t
holy_cryptodisk_write (holy_disk_t disk, holy_disk_addr_t sector,
		       holy_size_t size, const char *buf)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;
  gcry_err_code_t gcry_err;
  char *tmp;
  holy_err_t err;

#ifdef holy_UTIL
  if (dev->cheat)
    {
      int r;
      r = holy_util_fd_seek (dev->cheat_fd, sector << disk->log_sector_size);
      if (r)
	return holy_error (holy_ERR_BAD_DEVICE, N_("cannot seek `%s': %s"),
			   dev->cheat, holy_util_fd_strerror ());
      if (holy_util_fd_write (dev->cheat_fd, buf, size << disk->log_sector_size)
	  != (ssize_t) (size << disk->log_sector_size))
	return holy_error (holy_ERR_READ_ERROR, N_("cannot read `%s': %s"),
			   dev->cheat, holy_util_fd_strerror ());
      return holy_ERR_NONE;
    }
#endif

  tmp = holy_malloc (size << disk->log_sector_size);
  if (!tmp)
    return holy_errno;
  holy_memcpy (tmp, buf, size << disk->log_sector_size);

  holy_dprintf ("cryptodisk",
		"Writing %" PRIuholy_SIZE " sectors to sector 0x%"
		PRIxholy_UINT64_T " with offset of %" PRIuholy_UINT64_T "\n",
		size, sector, dev->offset);

  gcry_err = holy_cryptodisk_endecrypt (dev, (holy_uint8_t *) tmp,
					size << disk->log_sector_size,
					sector, 1);
  if (gcry_err)
    {
      holy_free (tmp);
      return holy_crypto_gcry_error (gcry_err);
    }

  /* Since ->write was called so disk.mod is loaded but be paranoid  */
  
  if (holy_disk_write_weak)
    err = holy_disk_write_weak (dev->source_disk,
				(sector << (disk->log_sector_size
					    - holy_DISK_SECTOR_BITS))
				+ dev->offset,
				0, size << disk->log_sector_size, tmp);
  else
    err = holy_error (holy_ERR_BUG, "disk.mod not loaded");
  holy_free (tmp);
  return err;
}

#ifdef holy_UTIL
static holy_disk_memberlist_t
holy_cryptodisk_memberlist (holy_disk_t disk)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;
  holy_disk_memberlist_t list = NULL;

  list = holy_malloc (sizeof (*list));
  if (list)
    {
      list->disk = dev->source_disk;
      list->next = NULL;
    }

  return list;
}
#endif

static void
cryptodisk_cleanup (void)
{
#if 0
  holy_cryptodisk_t dev = cryptodisk_list;
  holy_cryptodisk_t tmp;

  while (dev != NULL)
    {
      holy_free (dev->source);
      holy_free (dev->cipher);
      holy_free (dev->secondary_cipher);
      holy_free (dev->essiv_cipher);
      tmp = dev->next;
      holy_free (dev);
      dev = tmp;
    }
#endif
}

holy_err_t
holy_cryptodisk_insert (holy_cryptodisk_t newdev, const char *name,
			holy_disk_t source)
{
  newdev->source = holy_strdup (name);
  if (!newdev->source)
    {
      holy_free (newdev);
      return holy_errno;
    }

  newdev->id = last_cryptodisk_id++;
  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->partition_start = holy_partition_get_start (source->partition);
  newdev->next = cryptodisk_list;
  cryptodisk_list = newdev;

  return holy_ERR_NONE;
}

holy_cryptodisk_t
holy_cryptodisk_get_by_uuid (const char *uuid)
{
  holy_cryptodisk_t dev;
  for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
    if (holy_strcasecmp (dev->uuid, uuid) == 0)
      return dev;
  return NULL;
}

holy_cryptodisk_t
holy_cryptodisk_get_by_source_disk (holy_disk_t disk)
{
  holy_cryptodisk_t dev;
  for (dev = cryptodisk_list; dev != NULL; dev = dev->next)
    if (dev->source_id == disk->id && dev->source_dev_id == disk->dev->id)
      if ((disk->partition && holy_partition_get_start (disk->partition) == dev->partition_start) ||
          (!disk->partition && dev->partition_start == 0))
        return dev;
  return NULL;
}

#ifdef holy_UTIL
holy_err_t
holy_cryptodisk_cheat_insert (holy_cryptodisk_t newdev, const char *name,
			      holy_disk_t source, const char *cheat)
{
  newdev->cheat = holy_strdup (cheat);
  newdev->source = holy_strdup (name);
  if (!newdev->source || !newdev->cheat)
    {
      holy_free (newdev->source);
      holy_free (newdev->cheat);
      return holy_errno;
    }

  newdev->cheat_fd = holy_UTIL_FD_INVALID;
  newdev->source_id = source->id;
  newdev->source_dev_id = source->dev->id;
  newdev->partition_start = holy_partition_get_start (source->partition);
  newdev->id = last_cryptodisk_id++;
  newdev->next = cryptodisk_list;
  cryptodisk_list = newdev;

  return holy_ERR_NONE;
}

void
holy_util_cryptodisk_get_abstraction (holy_disk_t disk,
				      void (*cb) (const char *val, void *data),
				      void *data)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;

  cb ("cryptodisk", data);
  cb (dev->modname, data);

  if (dev->cipher)
    cb (dev->cipher->cipher->modname, data);
  if (dev->secondary_cipher)
    cb (dev->secondary_cipher->cipher->modname, data);
  if (dev->essiv_cipher)
    cb (dev->essiv_cipher->cipher->modname, data);
  if (dev->hash)
    cb (dev->hash->modname, data);
  if (dev->essiv_hash)
    cb (dev->essiv_hash->modname, data);
  if (dev->iv_hash)
    cb (dev->iv_hash->modname, data);
}

const char *
holy_util_cryptodisk_get_uuid (holy_disk_t disk)
{
  holy_cryptodisk_t dev = (holy_cryptodisk_t) disk->data;
  return dev->uuid;
}

#endif

static int check_boot, have_it;
static char *search_uuid;

static void
cryptodisk_close (holy_cryptodisk_t dev)
{
  holy_crypto_cipher_close (dev->cipher);
  holy_crypto_cipher_close (dev->secondary_cipher);
  holy_crypto_cipher_close (dev->essiv_cipher);
  holy_free (dev);
}

static holy_err_t
holy_cryptodisk_scan_device_real (const char *name, holy_disk_t source)
{
  holy_err_t err;
  holy_cryptodisk_t dev;
  holy_cryptodisk_dev_t cr;

  dev = holy_cryptodisk_get_by_source_disk (source);

  if (dev)
    return holy_ERR_NONE;

  FOR_CRYPTODISK_DEVS (cr)
  {
    dev = cr->scan (source, search_uuid, check_boot);
    if (holy_errno)
      return holy_errno;
    if (!dev)
      continue;
    
    err = cr->recover_key (source, dev);
    if (err)
    {
      cryptodisk_close (dev);
      return err;
    }

    holy_cryptodisk_insert (dev, name, source);

    have_it = 1;

    return holy_ERR_NONE;
  }
  return holy_ERR_NONE;
}

#ifdef holy_UTIL
#include <holy/util/misc.h>
holy_err_t
holy_cryptodisk_cheat_mount (const char *sourcedev, const char *cheat)
{
  holy_err_t err;
  holy_cryptodisk_t dev;
  holy_cryptodisk_dev_t cr;
  holy_disk_t source;

  /* Try to open disk.  */
  source = holy_disk_open (sourcedev);
  if (!source)
    return holy_errno;

  dev = holy_cryptodisk_get_by_source_disk (source);

  if (dev)
    {
      holy_disk_close (source);
      return holy_ERR_NONE;
    }

  FOR_CRYPTODISK_DEVS (cr)
  {
    dev = cr->scan (source, search_uuid, check_boot);
    if (holy_errno)
      return holy_errno;
    if (!dev)
      continue;

    holy_util_info ("cheatmounted %s (%s) at %s", sourcedev, dev->modname,
		    cheat);
    err = holy_cryptodisk_cheat_insert (dev, sourcedev, source, cheat);
    holy_disk_close (source);
    if (err)
      holy_free (dev);

    return holy_ERR_NONE;
  }

  holy_disk_close (source);

  return holy_ERR_NONE;
}
#endif

static int
holy_cryptodisk_scan_device (const char *name,
			     void *data __attribute__ ((unused)))
{
  holy_err_t err;
  holy_disk_t source;

  /* Try to open disk.  */
  source = holy_disk_open (name);
  if (!source)
    {
      holy_print_error ();
      return 0;
    }

  err = holy_cryptodisk_scan_device_real (name, source);

  holy_disk_close (source);
  
  if (err)
    holy_print_error ();
  return have_it && search_uuid ? 1 : 0;
}

static holy_err_t
holy_cmd_cryptomount (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;

  if (argc < 1 && !state[1].set && !state[2].set)
    return holy_error (holy_ERR_BAD_ARGUMENT, "device name required");

  have_it = 0;
  if (state[0].set)
    {
      holy_cryptodisk_t dev;

      dev = holy_cryptodisk_get_by_uuid (args[0]);
      if (dev)
	{
	  holy_dprintf ("cryptodisk",
			"already mounted as crypto%lu\n", dev->id);
	  return holy_ERR_NONE;
	}

      check_boot = state[2].set;
      search_uuid = args[0];
      holy_device_iterate (&holy_cryptodisk_scan_device, NULL);
      search_uuid = NULL;

      if (!have_it)
	return holy_error (holy_ERR_BAD_ARGUMENT, "no such cryptodisk found");
      return holy_ERR_NONE;
    }
  else if (state[1].set || (argc == 0 && state[2].set))
    {
      search_uuid = NULL;
      check_boot = state[2].set;
      holy_device_iterate (&holy_cryptodisk_scan_device, NULL);
      search_uuid = NULL;
      return holy_ERR_NONE;
    }
  else
    {
      holy_err_t err;
      holy_disk_t disk;
      holy_cryptodisk_t dev;
      char *diskname;
      char *disklast = NULL;
      holy_size_t len;

      search_uuid = NULL;
      check_boot = state[2].set;
      diskname = args[0];
      len = holy_strlen (diskname);
      if (len && diskname[0] == '(' && diskname[len - 1] == ')')
	{
	  disklast = &diskname[len - 1];
	  *disklast = '\0';
	  diskname++;
	}

      disk = holy_disk_open (diskname);
      if (!disk)
	{
	  if (disklast)
	    *disklast = ')';
	  return holy_errno;
	}

      dev = holy_cryptodisk_get_by_source_disk (disk);
      if (dev)
	{
	  holy_dprintf ("cryptodisk", "already mounted as crypto%lu\n", dev->id);
	  holy_disk_close (disk);
	  if (disklast)
	    *disklast = ')';
	  return holy_ERR_NONE;
	}

      err = holy_cryptodisk_scan_device_real (diskname, disk);

      holy_disk_close (disk);
      if (disklast)
	*disklast = ')';

      return err;
    }
}

static struct holy_disk_dev holy_cryptodisk_dev = {
  .name = "cryptodisk",
  .id = holy_DISK_DEVICE_CRYPTODISK_ID,
  .iterate = holy_cryptodisk_iterate,
  .open = holy_cryptodisk_open,
  .close = holy_cryptodisk_close,
  .read = holy_cryptodisk_read,
  .write = holy_cryptodisk_write,
#ifdef holy_UTIL
  .memberlist = holy_cryptodisk_memberlist,
#endif
  .next = 0
};

static char
hex (holy_uint8_t val)
{
  if (val < 10)
    return '0' + val;
  return 'a' + val - 10;
}

/* Open a file named NAME and initialize FILE.  */
static char *
luks_script_get (holy_size_t *sz)
{
  holy_cryptodisk_t i;
  holy_size_t size = 0;
  char *ptr, *ret;

  *sz = 0;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    if (holy_strcmp (i->modname, "luks") == 0)
      {
	size += sizeof ("luks_mount ");
	size += holy_strlen (i->uuid);
	size += holy_strlen (i->cipher->cipher->name);
	size += 54;
	if (i->essiv_hash)
	  size += holy_strlen (i->essiv_hash->name);
	size += i->keysize * 2;
      }

  ret = holy_malloc (size + 1);
  if (!ret)
    return 0;

  ptr = ret;

  for (i = cryptodisk_list; i != NULL; i = i->next)
    if (holy_strcmp (i->modname, "luks") == 0)
      {
	unsigned j;
	const char *iptr;
	ptr = holy_stpcpy (ptr, "luks_mount ");
	ptr = holy_stpcpy (ptr, i->uuid);
	*ptr++ = ' ';
	holy_snprintf (ptr, 21, "%" PRIuholy_UINT64_T " ", i->offset);
	while (*ptr)
	  ptr++;
	for (iptr = i->cipher->cipher->name; *iptr; iptr++)
	  *ptr++ = holy_tolower (*iptr);
	switch (i->mode)
	  {
	  case holy_CRYPTODISK_MODE_ECB:
	    ptr = holy_stpcpy (ptr, "-ecb");
	    break;
	  case holy_CRYPTODISK_MODE_CBC:
	    ptr = holy_stpcpy (ptr, "-cbc");
	    break;
	  case holy_CRYPTODISK_MODE_PCBC:
	    ptr = holy_stpcpy (ptr, "-pcbc");
	    break;
	  case holy_CRYPTODISK_MODE_XTS:
	    ptr = holy_stpcpy (ptr, "-xts");
	    break;
	  case holy_CRYPTODISK_MODE_LRW:
	    ptr = holy_stpcpy (ptr, "-lrw");
	    break;
	  }

	switch (i->mode_iv)
	  {
	  case holy_CRYPTODISK_MODE_IV_NULL:
	    ptr = holy_stpcpy (ptr, "-null");
	    break;
	  case holy_CRYPTODISK_MODE_IV_PLAIN:
	    ptr = holy_stpcpy (ptr, "-plain");
	    break;
	  case holy_CRYPTODISK_MODE_IV_PLAIN64:
	    ptr = holy_stpcpy (ptr, "-plain64");
	    break;
	  case holy_CRYPTODISK_MODE_IV_BENBI:
	    ptr = holy_stpcpy (ptr, "-benbi");
	    break;
	  case holy_CRYPTODISK_MODE_IV_ESSIV:
	    ptr = holy_stpcpy (ptr, "-essiv:");
	    ptr = holy_stpcpy (ptr, i->essiv_hash->name);
	    break;
	  case holy_CRYPTODISK_MODE_IV_BYTECOUNT64:
	  case holy_CRYPTODISK_MODE_IV_BYTECOUNT64_HASH:
	    break;
	  }
	*ptr++ = ' ';
	for (j = 0; j < i->keysize; j++)
	  {
	    *ptr++ = hex (i->key[j] >> 4);
	    *ptr++ = hex (i->key[j] & 0xf);
	  }
	*ptr++ = '\n';
      }
  *ptr = '\0';
  *sz = ptr - ret;
  return ret;
}

struct holy_procfs_entry luks_script =
{
  .name = "luks_script",
  .get_contents = luks_script_get
};

static holy_extcmd_t cmd;

holy_MOD_INIT (cryptodisk)
{
  holy_disk_dev_register (&holy_cryptodisk_dev);
  cmd = holy_register_extcmd ("cryptomount", holy_cmd_cryptomount, 0,
			      N_("SOURCE|-u UUID|-a|-b"),
			      N_("Mount a crypto device."), options);
  holy_procfs_register ("luks_script", &luks_script);
}

holy_MOD_FINI (cryptodisk)
{
  holy_disk_dev_unregister (&holy_cryptodisk_dev);
  cryptodisk_cleanup ();
  holy_procfs_unregister (&luks_script);
}
