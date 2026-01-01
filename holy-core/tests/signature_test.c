/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/env.h>
#include <holy/test.h>
#include <holy/mm.h>
#include <holy/procfs.h>

#include "signatures.h"

holy_MOD_LICENSE ("GPLv2+");

static char *
get_hi_dsa_sig (holy_size_t *sz)
{
  char *ret;
  *sz = sizeof (hi_dsa_sig);
  ret = holy_malloc (sizeof (hi_dsa_sig));
  if (ret)
    holy_memcpy (ret, hi_dsa_sig, sizeof (hi_dsa_sig));
  return ret;
}

static struct holy_procfs_entry hi_dsa_sig_entry =
{
  .name = "hi_dsa.sig",
  .get_contents = get_hi_dsa_sig
};

static char *
get_hi_dsa_pub (holy_size_t *sz)
{
  char *ret;
  *sz = sizeof (hi_dsa_pub);
  ret = holy_malloc (sizeof (hi_dsa_pub));
  if (ret)
    holy_memcpy (ret, hi_dsa_pub, sizeof (hi_dsa_pub));
  return ret;
}

static struct holy_procfs_entry hi_dsa_pub_entry =
{
  .name = "hi_dsa.pub",
  .get_contents = get_hi_dsa_pub
};

static char *
get_hi_rsa_sig (holy_size_t *sz)
{
  char *ret;
  *sz = sizeof (hi_rsa_sig);
  ret = holy_malloc (sizeof (hi_rsa_sig));
  if (ret)
    holy_memcpy (ret, hi_rsa_sig, sizeof (hi_rsa_sig));
  return ret;
}

static struct holy_procfs_entry hi_rsa_sig_entry =
{
  .name = "hi_rsa.sig",
  .get_contents = get_hi_rsa_sig
};

static char *
get_hi_rsa_pub (holy_size_t *sz)
{
  char *ret;
  *sz = sizeof (hi_rsa_pub);
  ret = holy_malloc (sizeof (hi_rsa_pub));
  if (ret)
    holy_memcpy (ret, hi_rsa_pub, sizeof (hi_rsa_pub));
  return ret;
}

static struct holy_procfs_entry hi_rsa_pub_entry =
{
  .name = "hi_rsa.pub",
  .get_contents = get_hi_rsa_pub
};

static char *
get_hi (holy_size_t *sz)
{
  *sz = 3;
  return holy_strdup ("hi\n");
}

struct holy_procfs_entry hi =
{
  .name = "hi",
  .get_contents = get_hi
};

static char *
get_hj (holy_size_t *sz)
{
  *sz = 3;
  return holy_strdup ("hj\n");
}

struct holy_procfs_entry hj =
{
  .name = "hj",
  .get_contents = get_hj
};

static void
do_verify (const char *f, const char *sig, const char *pub, int is_valid)
{
  holy_command_t cmd;
  char *args[] = { (char *) f, (char *) sig,
		   (char *) pub, NULL };
  holy_err_t err;

  cmd = holy_command_find ("verify_detached");
  if (!cmd)
    {
      holy_test_assert (0, "can't find command `%s'", "verify_detached");
      return;
    }
  err = (cmd->func) (cmd, 3, args);

  holy_test_assert (err == (is_valid ? 0 : holy_ERR_BAD_SIGNATURE),
		    "verification failed: %d: %s", holy_errno, holy_errmsg);
  holy_errno = holy_ERR_NONE;

}
static void
signature_test (void)
{
  holy_procfs_register ("hi", &hi);
  holy_procfs_register ("hj", &hj);
  holy_procfs_register ("hi_dsa.pub", &hi_dsa_pub_entry);
  holy_procfs_register ("hi_dsa.sig", &hi_dsa_sig_entry);
  holy_procfs_register ("hi_rsa.pub", &hi_rsa_pub_entry);
  holy_procfs_register ("hi_rsa.sig", &hi_rsa_sig_entry);

  do_verify ("(proc)/hi", "(proc)/hi_dsa.sig", "(proc)/hi_dsa.pub", 1);
  do_verify ("(proc)/hi", "(proc)/hi_dsa.sig", "(proc)/hi_dsa.pub", 1);
  do_verify ("(proc)/hj", "(proc)/hi_dsa.sig", "(proc)/hi_dsa.pub", 0);

  do_verify ("(proc)/hi", "(proc)/hi_rsa.sig", "(proc)/hi_rsa.pub", 1);
  do_verify ("(proc)/hj", "(proc)/hi_rsa.sig", "(proc)/hi_rsa.pub", 0);

  holy_procfs_unregister (&hi);
  holy_procfs_unregister (&hj);
  holy_procfs_unregister (&hi_dsa_sig_entry);
  holy_procfs_unregister (&hi_dsa_pub_entry);
}

holy_FUNCTIONAL_TEST (signature_test, signature_test);
