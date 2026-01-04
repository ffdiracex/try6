/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/file.h>
#include <holy/command.h>
#include <holy/crypto.h>
#include <holy/i18n.h>
#include <holy/gcrypt/gcrypt.h>
#include <holy/pubkey.h>
#include <holy/env.h>
#include <holy/kernel.h>
#include <holy/extcmd.h>

holy_MOD_LICENSE ("GPLv2+");

struct holy_verified
{
  holy_file_t file;
  void *buf;
};
typedef struct holy_verified *holy_verified_t;

enum
  {
    OPTION_SKIP_SIG = 0
  };

static const struct holy_arg_option options[] =
  {
    {"skip-sig", 's', 0,
     N_("Skip signature-checking of the public key file."), 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

static holy_ssize_t
pseudo_read (struct holy_file *file, char *buf, holy_size_t len)
{
  holy_memcpy (buf, (holy_uint8_t *) file->data + file->offset, len);
  return len;
}

/* Filesystem descriptor.  */
struct holy_fs pseudo_fs =
  {
    .name = "pseudo",
    .read = pseudo_read
};

static holy_err_t
read_packet_header (holy_file_t sig, holy_uint8_t *out_type, holy_size_t *len)
{
  holy_uint8_t type;
  holy_uint8_t l;
  holy_uint16_t l16;
  holy_uint32_t l32;

  /* New format.  */
  switch (holy_file_read (sig, &type, sizeof (type)))
    {
    case 1:
      break;
    case 0:
      {
	*out_type = 0xff;
	return 0;
      }
    default:
      if (holy_errno)
	return holy_errno;
      /* TRANSLATORS: it's about GNUPG signatures.  */
      return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
    }

  if (type == 0)
    {
      *out_type = 0xfe;
      return 0;      
    }

  if (!(type & 0x80))
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
  if (type & 0x40)
    {
      *out_type = (type & 0x3f);
      if (holy_file_read (sig, &l, sizeof (l)) != 1)
	return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
      if (l < 192)
	{
	  *len = l;
	  return 0;
	}
      if (l < 224)
	{
	  *len = (l - 192) << holy_CHAR_BIT;
	  if (holy_file_read (sig, &l, sizeof (l)) != 1)
	    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  *len |= l;
	  return 0;
	}
      if (l == 255)
	{
	  if (holy_file_read (sig, &l32, sizeof (l32)) != sizeof (l32))
	    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  *len = holy_be_to_cpu32 (l32);
	  return 0;
	}
      return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
    }
  *out_type = ((type >> 2) & 0xf);
  switch (type & 0x3)
    {
    case 0:
      if (holy_file_read (sig, &l, sizeof (l)) != sizeof (l))
	return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = l;
      return 0;
    case 1:
      if (holy_file_read (sig, &l16, sizeof (l16)) != sizeof (l16))
	return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = holy_be_to_cpu16 (l16);
      return 0;
    case 2:
      if (holy_file_read (sig, &l32, sizeof (l32)) != sizeof (l32))
	return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
      *len = holy_be_to_cpu32 (l32);
      return 0;
    }
  return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
}

struct signature_v4_header
{
  holy_uint8_t type;
  holy_uint8_t pkeyalgo;
  holy_uint8_t hash;
  holy_uint16_t hashed_sub;
} holy_PACKED;

const char *hashes[] = {
  [0x01] = "md5",
  [0x02] = "sha1",
  [0x03] = "ripemd160",
  [0x08] = "sha256",
  [0x09] = "sha384",
  [0x0a] = "sha512",
  [0x0b] = "sha224"
};

struct gcry_pk_spec *holy_crypto_pk_dsa;
struct gcry_pk_spec *holy_crypto_pk_ecdsa;
struct gcry_pk_spec *holy_crypto_pk_rsa;

static int
dsa_pad (gcry_mpi_t *hmpi, holy_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct holy_public_subkey *sk);
static int
rsa_pad (gcry_mpi_t *hmpi, holy_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct holy_public_subkey *sk);

struct
{
  const char *name;
  holy_size_t nmpisig;
  holy_size_t nmpipub;
  struct gcry_pk_spec **algo;
  int (*pad) (gcry_mpi_t *hmpi, holy_uint8_t *hval,
	      const gcry_md_spec_t *hash, struct holy_public_subkey *sk);
  const char *module;
} pkalgos[] = 
  {
    [1] = { "rsa", 1, 2, &holy_crypto_pk_rsa, rsa_pad, "gcry_rsa" },
    [3] = { "rsa", 1, 2, &holy_crypto_pk_rsa, rsa_pad, "gcry_rsa" },
    [17] = { "dsa", 2, 4, &holy_crypto_pk_dsa, dsa_pad, "gcry_dsa" },
  };

struct holy_public_key
{
  struct holy_public_key *next;
  struct holy_public_subkey *subkeys;
};

struct holy_public_subkey
{
  struct holy_public_subkey *next;
  holy_uint8_t type;
  holy_uint32_t fingerprint[5];
  gcry_mpi_t mpis[10];
};

static void
free_pk (struct holy_public_key *pk)
{
  struct holy_public_subkey *nsk, *sk;
  for (sk = pk->subkeys; sk; sk = nsk)
    {
      holy_size_t i;
      for (i = 0; i < ARRAY_SIZE (sk->mpis); i++)
	if (sk->mpis[i])
	  gcry_mpi_release (sk->mpis[i]);
      nsk = sk->next;
      holy_free (sk);
    }
  holy_free (pk);
}

#define READBUF_SIZE 4096

struct holy_public_key *
holy_load_public_key (holy_file_t f)
{
  holy_err_t err;
  struct holy_public_key *ret;
  struct holy_public_subkey **last = 0;
  void *fingerprint_context = NULL;
  holy_uint8_t *buffer = NULL;

  ret = holy_zalloc (sizeof (*ret));
  if (!ret)
    {
      holy_free (fingerprint_context);
      return NULL;
    }

  buffer = holy_zalloc (READBUF_SIZE);
  fingerprint_context = holy_zalloc (holy_MD_SHA1->contextsize);

  if (!buffer || !fingerprint_context)
    goto fail;

  last = &ret->subkeys;

  while (1)
    {
      holy_uint8_t type;
      holy_size_t len;
      holy_uint8_t v, pk;
      holy_uint32_t creation_time;
      holy_off_t pend;
      struct holy_public_subkey *sk;
      holy_size_t i;
      holy_uint16_t len_be;

      err = read_packet_header (f, &type, &len);

      if (err)
	goto fail;
      if (type == 0xfe)
	continue;
      if (type == 0xff)
	{
	  holy_free (fingerprint_context);
	  holy_free (buffer);
	  return ret;
	}

      holy_dprintf ("crypt", "len = %x\n", (int) len);

      pend = holy_file_tell (f) + len;
      if (type != 6 && type != 14
	  && type != 5 && type != 7)
	{
	  holy_file_seek (f, pend);
	  continue;
	}

      if (holy_file_read (f, &v, sizeof (v)) != sizeof (v))
	{
	  holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      holy_dprintf ("crypt", "v = %x\n", v);

      if (v != 4)
	{
	  holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}
      if (holy_file_read (f, &creation_time, sizeof (creation_time)) != sizeof (creation_time))
	{
	  holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      holy_dprintf ("crypt", "time = %x\n", creation_time);

      if (holy_file_read (f, &pk, sizeof (pk)) != sizeof (pk))
	{
	  holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	  goto fail;
	}

      holy_dprintf ("crypt", "pk = %x\n", pk);

      if (pk >= ARRAY_SIZE (pkalgos) || pkalgos[pk].name == NULL)
	{
	  holy_file_seek (f, pend);
	  continue;
	}

      sk = holy_zalloc (sizeof (struct holy_public_subkey));
      if (!sk)
	goto fail;

      holy_memset (fingerprint_context, 0, holy_MD_SHA1->contextsize);
      holy_MD_SHA1->init (fingerprint_context);
      holy_MD_SHA1->write (fingerprint_context, "\x99", 1);
      len_be = holy_cpu_to_be16 (len);
      holy_MD_SHA1->write (fingerprint_context, &len_be, sizeof (len_be));
      holy_MD_SHA1->write (fingerprint_context, &v, sizeof (v));
      holy_MD_SHA1->write (fingerprint_context, &creation_time, sizeof (creation_time));
      holy_MD_SHA1->write (fingerprint_context, &pk, sizeof (pk));

      for (i = 0; i < pkalgos[pk].nmpipub; i++)
	{
	  holy_uint16_t l;
	  holy_size_t lb;
	  if (holy_file_read (f, &l, sizeof (l)) != sizeof (l))
	    {
	      holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	      break;
	    }
	  
	  lb = (holy_be_to_cpu16 (l) + holy_CHAR_BIT - 1) / holy_CHAR_BIT;
	  if (lb > READBUF_SIZE - sizeof (holy_uint16_t))
	    {
	      holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	      break;
	    }
	  if (holy_file_read (f, buffer + sizeof (holy_uint16_t), lb) != (holy_ssize_t) lb)
	    {
	      holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	      break;
	    }
	  holy_memcpy (buffer, &l, sizeof (l));

	  holy_MD_SHA1->write (fingerprint_context, buffer, lb + sizeof (holy_uint16_t));
 
	  if (gcry_mpi_scan (&sk->mpis[i], GCRYMPI_FMT_PGP,
			     buffer, lb + sizeof (holy_uint16_t), 0))
	    {
	      holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
	      break;
	    }
	}

      if (i < pkalgos[pk].nmpipub)
	{
	  holy_free (sk);
	  goto fail;
	}

      holy_MD_SHA1->final (fingerprint_context);

      holy_memcpy (sk->fingerprint, holy_MD_SHA1->read (fingerprint_context), 20);

      *last = sk;
      last = &sk->next;

      holy_dprintf ("crypt", "actual pos: %x, expected: %x\n", (int)holy_file_tell (f), (int)pend);

      holy_file_seek (f, pend);
    }
 fail:
  free_pk (ret);
  holy_free (fingerprint_context);
  holy_free (buffer);
  return NULL;
}

struct holy_public_key *holy_pk_trusted;

struct holy_public_subkey *
holy_crypto_pk_locate_subkey (holy_uint64_t keyid, struct holy_public_key *pkey)
{
  struct holy_public_subkey *sk;
  for (sk = pkey->subkeys; sk; sk = sk->next)
    if (holy_memcmp (sk->fingerprint + 3, &keyid, 8) == 0)
      return sk;
  return 0;
}

struct holy_public_subkey *
holy_crypto_pk_locate_subkey_in_trustdb (holy_uint64_t keyid)
{
  struct holy_public_key *pkey;
  struct holy_public_subkey *sk;
  for (pkey = holy_pk_trusted; pkey; pkey = pkey->next)
    {
      sk = holy_crypto_pk_locate_subkey (keyid, pkey);
      if (sk)
	return sk;
    }
  return 0;
}


static int
dsa_pad (gcry_mpi_t *hmpi, holy_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct holy_public_subkey *sk)
{
  unsigned nbits = gcry_mpi_get_nbits (sk->mpis[1]);
  holy_dprintf ("crypt", "must be %u bits got %d bits\n", nbits,
		(int)(8 * hash->mdlen));
  return gcry_mpi_scan (hmpi, GCRYMPI_FMT_USG, hval,
			nbits / 8 < (unsigned) hash->mdlen ? nbits / 8
			: (unsigned) hash->mdlen, 0);
}

static int
rsa_pad (gcry_mpi_t *hmpi, holy_uint8_t *hval,
	 const gcry_md_spec_t *hash, struct holy_public_subkey *sk)
{
  holy_size_t tlen, emlen, fflen;
  holy_uint8_t *em, *emptr;
  unsigned nbits = gcry_mpi_get_nbits (sk->mpis[0]);
  int ret;
  tlen = hash->mdlen + hash->asnlen;
  emlen = (nbits + 7) / 8;
  if (emlen < tlen + 11)
    return 1;

  em = holy_malloc (emlen);
  if (!em)
    return 1;

  em[0] = 0x00;
  em[1] = 0x01;
  fflen = emlen - tlen - 3;
  for (emptr = em + 2; emptr < em + 2 + fflen; emptr++)
    *emptr = 0xff;
  *emptr++ = 0x00;
  holy_memcpy (emptr, hash->asnoid, hash->asnlen);
  emptr += hash->asnlen;
  holy_memcpy (emptr, hval, hash->mdlen);

  ret = gcry_mpi_scan (hmpi, GCRYMPI_FMT_USG, em, emlen, 0);
  holy_free (em);
  return ret;
}

static holy_err_t
holy_verify_signature_real (char *buf, holy_size_t size,
			    holy_file_t f, holy_file_t sig,
			    struct holy_public_key *pkey)
{
  holy_size_t len;
  holy_uint8_t v;
  holy_uint8_t h;
  holy_uint8_t t;
  holy_uint8_t pk;
  const gcry_md_spec_t *hash;
  struct signature_v4_header v4;
  holy_err_t err;
  holy_size_t i;
  gcry_mpi_t mpis[10];
  holy_uint8_t type = 0;

  err = read_packet_header (sig, &type, &len);
  if (err)
    return err;

  if (type != 0x2)
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (holy_file_read (sig, &v, sizeof (v)) != sizeof (v))
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (v != 4)
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (holy_file_read (sig, &v4, sizeof (v4)) != sizeof (v4))
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  h = v4.hash;
  t = v4.type;
  pk = v4.pkeyalgo;
  
  if (t != 0)
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  if (h >= ARRAY_SIZE (hashes) || hashes[h] == NULL)
    return holy_error (holy_ERR_BAD_SIGNATURE, "unknown hash");

  if (pk >= ARRAY_SIZE (pkalgos) || pkalgos[pk].name == NULL)
    return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));

  hash = holy_crypto_lookup_md_by_name (hashes[h]);
  if (!hash)
    return holy_error (holy_ERR_BAD_SIGNATURE, "hash `%s' not loaded", hashes[h]);

  holy_dprintf ("crypt", "alive\n");

  {
    void *context = NULL;
    unsigned char *hval;
    holy_ssize_t rem = holy_be_to_cpu16 (v4.hashed_sub);
    holy_uint32_t headlen = holy_cpu_to_be32 (rem + 6);
    holy_uint8_t s;
    holy_uint16_t unhashed_sub;
    holy_ssize_t r;
    holy_uint8_t hash_start[2];
    gcry_mpi_t hmpi;
    holy_uint64_t keyid = 0;
    struct holy_public_subkey *sk;
    holy_uint8_t *readbuf = NULL;

    context = holy_zalloc (hash->contextsize);
    readbuf = holy_zalloc (READBUF_SIZE);
    if (!context || !readbuf)
      goto fail;

    hash->init (context);
    if (buf)
      hash->write (context, buf, size);
    else 
      while (1)
	{
	  r = holy_file_read (f, readbuf, READBUF_SIZE);
	  if (r < 0)
	    goto fail;
	  if (r == 0)
	    break;
	  hash->write (context, readbuf, r);
	}

    hash->write (context, &v, sizeof (v));
    hash->write (context, &v4, sizeof (v4));
    while (rem)
      {
	r = holy_file_read (sig, readbuf,
			    rem < READBUF_SIZE ? rem : READBUF_SIZE);
	if (r < 0)
	  goto fail;
	if (r == 0)
	  break;
	hash->write (context, readbuf, r);
	rem -= r;
      }
    hash->write (context, &v, sizeof (v));
    s = 0xff;
    hash->write (context, &s, sizeof (s));
    hash->write (context, &headlen, sizeof (headlen));
    r = holy_file_read (sig, &unhashed_sub, sizeof (unhashed_sub));
    if (r != sizeof (unhashed_sub))
      goto fail;
    {
      holy_uint8_t *ptr;
      holy_uint32_t l;
      rem = holy_be_to_cpu16 (unhashed_sub);
      if (rem > READBUF_SIZE)
	goto fail;
      r = holy_file_read (sig, readbuf, rem);
      if (r != rem)
	goto fail;
      for (ptr = readbuf; ptr < readbuf + rem; ptr += l)
	{
	  if (*ptr < 192)
	    l = *ptr++;
	  else if (*ptr < 255)
	    {
	      if (ptr + 1 >= readbuf + rem)
		break;
	      l = (((ptr[0] & ~192) << holy_CHAR_BIT) | ptr[1]) + 192;
	      ptr += 2;
	    }
	  else
	    {
	      if (ptr + 5 >= readbuf + rem)
		break;
	      l = holy_be_to_cpu32 (holy_get_unaligned32 (ptr + 1));
	      ptr += 5;
	    }
	  if (*ptr == 0x10 && l >= 8)
	    keyid = holy_get_unaligned64 (ptr + 1);
	}
    }

    hash->final (context);

    holy_dprintf ("crypt", "alive\n");

    hval = hash->read (context);

    if (holy_file_read (sig, hash_start, sizeof (hash_start)) != sizeof (hash_start))
      goto fail;
    if (holy_memcmp (hval, hash_start, sizeof (hash_start)) != 0)
      goto fail;

    holy_dprintf ("crypt", "@ %x\n", (int)holy_file_tell (sig));

    for (i = 0; i < pkalgos[pk].nmpisig; i++)
      {
	holy_uint16_t l;
	holy_size_t lb;
	holy_dprintf ("crypt", "alive\n");
	if (holy_file_read (sig, &l, sizeof (l)) != sizeof (l))
	  goto fail;
	holy_dprintf ("crypt", "alive\n");
	lb = (holy_be_to_cpu16 (l) + 7) / 8;
	holy_dprintf ("crypt", "l = 0x%04x\n", holy_be_to_cpu16 (l));
	if (lb > READBUF_SIZE - sizeof (holy_uint16_t))
	  goto fail;
	holy_dprintf ("crypt", "alive\n");
	if (holy_file_read (sig, readbuf + sizeof (holy_uint16_t), lb) != (holy_ssize_t) lb)
	  goto fail;
	holy_dprintf ("crypt", "alive\n");
	holy_memcpy (readbuf, &l, sizeof (l));
	holy_dprintf ("crypt", "alive\n");

	if (gcry_mpi_scan (&mpis[i], GCRYMPI_FMT_PGP,
			   readbuf, lb + sizeof (holy_uint16_t), 0))
	  goto fail;
	holy_dprintf ("crypt", "alive\n");
      }

    if (pkey)
      sk = holy_crypto_pk_locate_subkey (keyid, pkey);
    else
      sk = holy_crypto_pk_locate_subkey_in_trustdb (keyid);
    if (!sk)
      {
	/* TRANSLATORS: %08x is 32-bit key id.  */
	holy_error (holy_ERR_BAD_SIGNATURE, N_("public key %08x not found"),
		    keyid);
	goto fail;
      }

    if (pkalgos[pk].pad (&hmpi, hval, hash, sk))
      goto fail;
    if (!*pkalgos[pk].algo)
      {
	holy_dl_load (pkalgos[pk].module);
	holy_errno = holy_ERR_NONE;
      }

    if (!*pkalgos[pk].algo)
      {
	holy_error (holy_ERR_BAD_SIGNATURE, N_("module `%s' isn't loaded"),
		    pkalgos[pk].module);
	goto fail;
      }
    if ((*pkalgos[pk].algo)->verify (0, hmpi, mpis, sk->mpis, 0, 0))
      goto fail;

    holy_free (context);
    holy_free (readbuf);

    return holy_ERR_NONE;

  fail:
    holy_free (context);
    holy_free (readbuf);
    if (!holy_errno)
      return holy_error (holy_ERR_BAD_SIGNATURE, N_("bad signature"));
    return holy_errno;
  }
}

holy_err_t
holy_verify_signature (holy_file_t f, holy_file_t sig,
		       struct holy_public_key *pkey)
{
  return holy_verify_signature_real (0, 0, f, sig, pkey);
}

static holy_err_t
holy_cmd_trust (holy_extcmd_context_t ctxt,
		int argc, char **args)
{
  holy_file_t pkf;
  struct holy_public_key *pk = NULL;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  holy_file_filter_disable_compression ();
  if (ctxt->state[OPTION_SKIP_SIG].set)
    holy_file_filter_disable_pubkey ();
  pkf = holy_file_open (args[0]);
  if (!pkf)
    return holy_errno;
  pk = holy_load_public_key (pkf);
  if (!pk)
    {
      holy_file_close (pkf);
      return holy_errno;
    }
  holy_file_close (pkf);

  pk->next = holy_pk_trusted;
  holy_pk_trusted = pk;

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_trust_var (holy_command_t cmd __attribute__ ((unused)),
		    int argc, char **args)
{
  struct holy_file pseudo_file;
  const char *var;
  char *data;
  struct holy_public_key *pk = NULL;
  unsigned int i, idx0, idx1;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));

  var = holy_env_get (args[0]);
  if (!var)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("unknown variable"));

  data = holy_zalloc (holy_strlen (var) / 2);
  if (!data)
    return holy_error (holy_ERR_OUT_OF_MEMORY, N_("can't allocate memory for key"));

  /* For the want of sscanf() */
  for (i = 0; i < holy_strlen (var); i += 2)
    {
      if (var[i] < 0x40)
	idx0 = var[i] - 0x30;
      else
	idx0 = var[i] - 0x57;

      if (var[i+1] < 0x40)
	idx1 = var[i+1] - 0x30;
      else
	idx1 = var[i+1] - 0x57;

      data[i/2] = ((idx0 << 4) & 0xf0) | (idx1 & 0x0f);
    }

  holy_memset (&pseudo_file, 0, sizeof (pseudo_file));

  pseudo_file.fs = &pseudo_fs;
  pseudo_file.size = holy_strlen (var) / 2;
  pseudo_file.data = data;

  pk = holy_load_public_key (&pseudo_file);
  if (!pk)
    {
      holy_free(data);
      return holy_errno;
    }

  pk->next = holy_pk_trusted;
  holy_pk_trusted = pk;

  holy_free(data);
  return holy_ERR_NONE;

}

static holy_err_t
holy_cmd_list (holy_command_t cmd  __attribute__ ((unused)),
	       int argc __attribute__ ((unused)),
	       char **args __attribute__ ((unused)))
{
  struct holy_public_key *pk = NULL;
  struct holy_public_subkey *sk = NULL;

  for (pk = holy_pk_trusted; pk; pk = pk->next)
    for (sk = pk->subkeys; sk; sk = sk->next)
      {
	unsigned i;
	for (i = 0; i < 20; i += 2)
	  holy_printf ("%02x%02x ", ((holy_uint8_t *) sk->fingerprint)[i],
		       ((holy_uint8_t *) sk->fingerprint)[i + 1]);
	holy_printf ("\n");
      }

  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_distrust (holy_command_t cmd  __attribute__ ((unused)),
		   int argc, char **args)
{
  holy_uint32_t keyid, keyid_be;
  struct holy_public_key **pkey;
  struct holy_public_subkey *sk;

  if (argc < 1)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("one argument expected"));
  keyid = holy_strtoull (args[0], 0, 16);
  if (holy_errno)
    return holy_errno;
  keyid_be = holy_cpu_to_be32 (keyid);

  for (pkey = &holy_pk_trusted; *pkey; pkey = &((*pkey)->next))
    {
      struct holy_public_key *next;
      for (sk = (*pkey)->subkeys; sk; sk = sk->next)
	if (holy_memcmp (sk->fingerprint + 4, &keyid_be, 4) == 0)
	  break;
      if (!sk)
	continue;
      next = (*pkey)->next;
      free_pk (*pkey);
      *pkey = next;
      return holy_ERR_NONE;
    }
  /* TRANSLATORS: %08x is 32-bit key id.  */
  return holy_error (holy_ERR_BAD_SIGNATURE, N_("public key %08x not found"), keyid);
}

static holy_err_t
holy_cmd_verify_signature (holy_extcmd_context_t ctxt,
			   int argc, char **args)
{
  holy_file_t f = NULL, sig = NULL;
  holy_err_t err = holy_ERR_NONE;
  struct holy_public_key *pk = NULL;

  holy_dprintf ("crypt", "alive\n");

  if (argc < 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  holy_dprintf ("crypt", "alive\n");

  if (argc > 2)
    {
      holy_file_t pkf;
      holy_file_filter_disable_compression ();
      if (ctxt->state[OPTION_SKIP_SIG].set)
	holy_file_filter_disable_pubkey ();
      pkf = holy_file_open (args[2]);
      if (!pkf)
	return holy_errno;
      pk = holy_load_public_key (pkf);
      if (!pk)
	{
	  holy_file_close (pkf);
	  return holy_errno;
	}
      holy_file_close (pkf);
    }

  holy_file_filter_disable_all ();
  f = holy_file_open (args[0]);
  if (!f)
    {
      err = holy_errno;
      goto fail;
    }

  holy_file_filter_disable_all ();
  sig = holy_file_open (args[1]);
  if (!sig)
    {
      err = holy_errno;
      goto fail;
    }

  err = holy_verify_signature (f, sig, pk);
 fail:
  if (sig)
    holy_file_close (sig);
  if (f)
    holy_file_close (f);
  if (pk)
    free_pk (pk);
  return err;
}

static int sec = 0;

static void
verified_free (holy_verified_t verified)
{
  if (verified)
    {
      holy_free (verified->buf);
      holy_free (verified);
    }
}

static holy_ssize_t
verified_read (struct holy_file *file, char *buf, holy_size_t len)
{
  holy_verified_t verified = file->data;

  holy_memcpy (buf, (char *) verified->buf + file->offset, len);
  return len;
}

static holy_err_t
verified_close (struct holy_file *file)
{
  holy_verified_t verified = file->data;

  holy_file_close (verified->file);
  verified_free (verified);
  file->data = 0;

  /* device and name are freed by parent */
  file->device = 0;
  file->name = 0;

  return holy_errno;
}

struct holy_fs verified_fs =
{
  .name = "verified_read",
  .read = verified_read,
  .close = verified_close
};

static holy_file_t
holy_pubkey_open (holy_file_t io, const char *filename)
{
  holy_file_t sig;
  char *fsuf, *ptr;
  holy_err_t err;
  holy_file_filter_t curfilt[holy_FILE_FILTER_MAX];
  holy_file_t ret;
  holy_verified_t verified;

  if (!sec)
    return io;
  if (io->device->disk && 
      (io->device->disk->dev->id == holy_DISK_DEVICE_MEMDISK_ID
       || io->device->disk->dev->id == holy_DISK_DEVICE_PROCFS_ID))
    return io;
  fsuf = holy_malloc (holy_strlen (filename) + sizeof (".sig"));
  if (!fsuf)
    return NULL;
  ptr = holy_stpcpy (fsuf, filename);
  holy_memcpy (ptr, ".sig", sizeof (".sig"));

  holy_memcpy (curfilt, holy_file_filters_enabled,
	       sizeof (curfilt));
  holy_file_filter_disable_all ();
  sig = holy_file_open (fsuf);
  holy_memcpy (holy_file_filters_enabled, curfilt,
	       sizeof (curfilt));
  holy_free (fsuf);
  if (!sig)
    return NULL;

  ret = holy_malloc (sizeof (*ret));
  if (!ret)
    {
      holy_file_close (sig);
      return NULL;
    }
  *ret = *io;

  ret->fs = &verified_fs;
  ret->not_easily_seekable = 0;
  if (ret->size >> (sizeof (holy_size_t) * holy_CHAR_BIT - 1))
    {
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "big file signature isn't implemented yet");
      holy_file_close (sig);
      holy_free (ret);
      return NULL;
    }
  verified = holy_malloc (sizeof (*verified));
  if (!verified)
    {
      holy_file_close (sig);
      holy_free (ret);
      return NULL;
    }
  verified->buf = holy_malloc (ret->size);
  if (!verified->buf)
    {
      holy_file_close (sig);
      holy_free (verified);
      holy_free (ret);
      return NULL;
    }
  if (holy_file_read (io, verified->buf, ret->size) != (holy_ssize_t) ret->size)
    {
      if (!holy_errno)
	holy_error (holy_ERR_FILE_READ_ERROR, N_("premature end of file %s"),
		    filename);
      holy_file_close (sig);
      verified_free (verified);
      holy_free (ret);
      return NULL;
    }

  err = holy_verify_signature_real (verified->buf, ret->size, 0, sig, NULL);
  holy_file_close (sig);
  if (err)
    {
      verified_free (verified);
      holy_free (ret);
      return NULL;
    }
  verified->file = io;
  ret->data = verified;
  return ret;
}

static char *
holy_env_write_sec (struct holy_env_var *var __attribute__ ((unused)),
		    const char *val)
{
  sec = (*val == '1') || (*val == 'e');
  return holy_strdup (sec ? "enforce" : "no");
}

static holy_extcmd_t cmd, cmd_trust;
static holy_command_t cmd_trust_var, cmd_distrust, cmd_list;

holy_MOD_INIT(verify)
{
  const char *val;
  struct holy_module_header *header;

  val = holy_env_get ("check_signatures");
  if (val && (val[0] == '1' || val[0] == 'e'))
    sec = 1;
  else
    sec = 0;
    
  holy_file_filter_register (holy_FILE_FILTER_PUBKEY, holy_pubkey_open);

  holy_register_variable_hook ("check_signatures", 0, holy_env_write_sec);
  holy_env_export ("check_signatures");

  holy_pk_trusted = 0;
  FOR_MODULES (header)
  {
    struct holy_file pseudo_file;
    struct holy_public_key *pk = NULL;

    holy_memset (&pseudo_file, 0, sizeof (pseudo_file));

    /* Not an ELF module, skip.  */
    if (header->type != OBJ_TYPE_PUBKEY)
      continue;

    pseudo_file.fs = &pseudo_fs;
    pseudo_file.size = (header->size - sizeof (struct holy_module_header));
    pseudo_file.data = (char *) header + sizeof (struct holy_module_header);

    pk = holy_load_public_key (&pseudo_file);
    if (!pk)
      holy_fatal ("error loading initial key: %s\n", holy_errmsg);

    pk->next = holy_pk_trusted;
    holy_pk_trusted = pk;
  }

  if (!val)
    holy_env_set ("check_signatures", holy_pk_trusted ? "enforce" : "no");

  cmd = holy_register_extcmd ("verify_detached", holy_cmd_verify_signature, 0,
			      N_("[-s|--skip-sig] FILE SIGNATURE_FILE [PUBKEY_FILE]"),
			      N_("Verify detached signature."),
			      options);
  cmd_trust = holy_register_extcmd ("trust", holy_cmd_trust, 0,
				     N_("[-s|--skip-sig] PUBKEY_FILE"),
				     N_("Add PUBKEY_FILE to trusted keys."),
				     options);
  cmd_trust_var = holy_register_command ("trust_var", holy_cmd_trust_var,
					 N_("PUBKEY_VAR"),
					 N_("Add the contents of PUBKEY_VAR to trusted keys."));
  cmd_list = holy_register_command ("list_trusted", holy_cmd_list,
				    0,
				    N_("Show the list of trusted keys."));
  cmd_distrust = holy_register_command ("distrust", holy_cmd_distrust,
					N_("PUBKEY_ID"),
					N_("Remove PUBKEY_ID from trusted keys."));
}

holy_MOD_FINI(verify)
{
  holy_file_filter_unregister (holy_FILE_FILTER_PUBKEY);
  holy_unregister_extcmd (cmd);
  holy_unregister_extcmd (cmd_trust);
  holy_unregister_command (cmd_trust_var);
  holy_unregister_command (cmd_list);
  holy_unregister_command (cmd_distrust);
}
