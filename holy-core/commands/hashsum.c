/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/crypto.h>
#include <holy/normal.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] = {
  {"hash", 'h', 0, N_("Specify hash to use."), N_("HASH"), ARG_TYPE_STRING},
  {"check", 'c', 0, N_("Check hashes of files with hash list FILE."),
   N_("FILE"), ARG_TYPE_STRING},
  {"prefix", 'p', 0, N_("Base directory for hash list."), N_("DIR"),
   ARG_TYPE_STRING},
  {"keep-going", 'k', 0, N_("Don't stop after first error."), 0, 0},
  {"uncompress", 'u', 0, N_("Uncompress file before checksumming."), 0, 0},
  {0, 0, 0, 0, 0, 0}
};

static struct { const char *name; const char *hashname; } aliases[] = 
  {
    {"sha256sum", "sha256"},
    {"sha512sum", "sha512"},
    {"sha1sum", "sha1"},
    {"md5sum", "md5"},
    {"crc", "crc32"},
  };

static inline int
hextoval (char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  return -1;
}

static holy_err_t
hash_file (holy_file_t file, const gcry_md_spec_t *hash, void *result)
{
  void *context;
  holy_uint8_t *readbuf;
#define BUF_SIZE 4096
  readbuf = holy_malloc (BUF_SIZE);
  if (!readbuf)
    return holy_errno;
  context = holy_zalloc (hash->contextsize);
  if (!readbuf || !context)
    goto fail;

  hash->init (context);
  while (1)
    {
      holy_ssize_t r;
      r = holy_file_read (file, readbuf, BUF_SIZE);
      if (r < 0)
	goto fail;
      if (r == 0)
	break;
      hash->write (context, readbuf, r);
    }
  hash->final (context);
  holy_memcpy (result, hash->read (context), hash->mdlen);

  holy_free (readbuf);
  holy_free (context);

  return holy_ERR_NONE;

 fail:
  holy_free (readbuf);
  holy_free (context);
  return holy_errno;
}

static holy_err_t
check_list (const gcry_md_spec_t *hash, const char *hashfilename,
	    const char *prefix, int keep, int uncompress)
{
  holy_file_t hashlist, file;
  char *buf = NULL;
  holy_uint8_t expected[holy_CRYPTO_MAX_MDLEN];
  holy_uint8_t actual[holy_CRYPTO_MAX_MDLEN];
  holy_err_t err;
  unsigned i;
  unsigned unread = 0, mismatch = 0;

  if (hash->mdlen > holy_CRYPTO_MAX_MDLEN)
    return holy_error (holy_ERR_BUG, "mdlen is too long");

  hashlist = holy_file_open (hashfilename);
  if (!hashlist)
    return holy_errno;
  
  while (holy_free (buf), (buf = holy_file_getline (hashlist)))
    {
      const char *p = buf;
      while (holy_isspace (p[0]))
	p++;
      for (i = 0; i < hash->mdlen; i++)
	{
	  int high, low;
	  high = hextoval (*p++);
	  low = hextoval (*p++);
	  if (high < 0 || low < 0)
	    return holy_error (holy_ERR_BAD_FILE_TYPE, "invalid hash list");
	  expected[i] = (high << 4) | low;
	}
      if ((p[0] != ' ' && p[0] != '\t') || (p[1] != ' ' && p[1] != '\t'))
	return holy_error (holy_ERR_BAD_FILE_TYPE, "invalid hash list");
      p += 2;
      if (prefix)
	{
	  char *filename;
	  
	  filename = holy_xasprintf ("%s/%s", prefix, p);
	  if (!filename)
	    return holy_errno;
	  if (!uncompress)
	    holy_file_filter_disable_compression ();
	  file = holy_file_open (filename);
	  holy_free (filename);
	}
      else
	{
	  if (!uncompress)
	    holy_file_filter_disable_compression ();
	  file = holy_file_open (p);
	}
      if (!file)
	{
	  holy_file_close (hashlist);
	  holy_free (buf);
	  return holy_errno;
	}
      err = hash_file (file, hash, actual);
      holy_file_close (file);
      if (err)
	{
	  holy_printf_ (N_("%s: READ ERROR\n"), p);
	  if (!keep)
	    {
	      holy_file_close (hashlist);
	      holy_free (buf);
	      return err;
	    }
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  unread++;
	  continue;
	}
      if (holy_crypto_memcmp (expected, actual, hash->mdlen) != 0)
	{
	  holy_printf_ (N_("%s: HASH MISMATCH\n"), p);
	  if (!keep)
	    {
	      holy_file_close (hashlist);
	      holy_free (buf);
	      return holy_error (holy_ERR_TEST_FAILURE,
				 "hash of '%s' mismatches", p);
	    }
	  mismatch++;
	  continue;	  
	}
      holy_printf_ (N_("%s: OK\n"), p);
    }
  if (mismatch || unread)
    return holy_error (holy_ERR_TEST_FAILURE,
		       "%d files couldn't be read and hash "
		       "of %d files mismatches", unread, mismatch);
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_hashsum (struct holy_extcmd_context *ctxt,
		  int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  const char *hashname = NULL;
  const char *prefix = NULL;
  const gcry_md_spec_t *hash;
  unsigned i;
  int keep = state[3].set;
  int uncompress = state[4].set;
  unsigned unread = 0;

  for (i = 0; i < ARRAY_SIZE (aliases); i++)
    if (holy_strcmp (ctxt->extcmd->cmd->name, aliases[i].name) == 0)
      hashname = aliases[i].hashname;
  if (state[0].set)
    hashname = state[0].arg;

  if (!hashname)
    return holy_error (holy_ERR_BAD_ARGUMENT, "no hash specified");

  hash = holy_crypto_lookup_md_by_name (hashname);
  if (!hash)
    return holy_error (holy_ERR_BAD_ARGUMENT, "unknown hash");

  if (hash->mdlen > holy_CRYPTO_MAX_MDLEN)
    return holy_error (holy_ERR_BUG, "mdlen is too long");

  if (state[2].set)
    prefix = state[2].arg;

  if (state[1].set)
    {
      if (argc != 0)
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   "--check is incompatible with file list");
      return check_list (hash, state[1].arg, prefix, keep, uncompress);
    }

  for (i = 0; i < (unsigned) argc; i++)
    {
      holy_PROPERLY_ALIGNED_ARRAY (result, holy_CRYPTO_MAX_MDLEN);
      holy_file_t file;
      holy_err_t err;
      unsigned j;
      if (!uncompress)
	holy_file_filter_disable_compression ();
      file = holy_file_open (args[i]);
      if (!file)
	{
	  if (!keep)
	    return holy_errno;
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  unread++;
	  continue;
	}
      err = hash_file (file, hash, result);
      holy_file_close (file);
      if (err)
	{
	  if (!keep)
	    return err;
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  unread++;
	  continue;
	}
      for (j = 0; j < hash->mdlen; j++)
	holy_printf ("%02x", ((holy_uint8_t *) result)[j]);
      holy_printf ("  %s\n", args[i]);
    }

  if (unread)
    return holy_error (holy_ERR_TEST_FAILURE, "%d files couldn't be read",
		       unread);
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd, cmd_md5, cmd_sha1, cmd_sha256, cmd_sha512, cmd_crc;

holy_MOD_INIT(hashsum)
{
  cmd = holy_register_extcmd ("hashsum", holy_cmd_hashsum, 0,
			      N_("-h HASH [-c FILE [-p PREFIX]] "
				 "[FILE1 [FILE2 ...]]"),
			      /* TRANSLATORS: "hash checksum" is just to
				 be a bit more precise, you can treat it as
				 just "hash".  */
			      N_("Compute or check hash checksum."),
			      options);
  cmd_md5 = holy_register_extcmd ("md5sum", holy_cmd_hashsum, 0,
				  N_("[-c FILE [-p PREFIX]] "
				     "[FILE1 [FILE2 ...]]"),
				  N_("Compute or check hash checksum."),
				  options);
  cmd_sha1 = holy_register_extcmd ("sha1sum", holy_cmd_hashsum, 0,
				   N_("[-c FILE [-p PREFIX]] "
				      "[FILE1 [FILE2 ...]]"),
				   N_("Compute or check hash checksum."),
				   options);
  cmd_sha256 = holy_register_extcmd ("sha256sum", holy_cmd_hashsum, 0,
				     N_("[-c FILE [-p PREFIX]] "
					"[FILE1 [FILE2 ...]]"),
				     N_("Compute or check hash checksum."),
				     options);
  cmd_sha512 = holy_register_extcmd ("sha512sum", holy_cmd_hashsum, 0,
				     N_("[-c FILE [-p PREFIX]] "
					"[FILE1 [FILE2 ...]]"),
				     N_("Compute or check hash checksum."),
				     options);

  cmd_crc = holy_register_extcmd ("crc", holy_cmd_hashsum, 0,
				     N_("[-c FILE [-p PREFIX]] "
					"[FILE1 [FILE2 ...]]"),
				     N_("Compute or check hash checksum."),
				     options);
}

holy_MOD_FINI(hashsum)
{
  holy_unregister_extcmd (cmd);
  holy_unregister_extcmd (cmd_md5);
  holy_unregister_extcmd (cmd_sha1);
  holy_unregister_extcmd (cmd_sha256);
  holy_unregister_extcmd (cmd_sha512);
  holy_unregister_extcmd (cmd_crc);
}
