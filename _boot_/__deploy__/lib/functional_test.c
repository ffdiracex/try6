/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/extcmd.h>
#include <holy/test.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t
holy_functional_test (holy_extcmd_context_t ctxt __attribute__ ((unused)),
		      int argc __attribute__ ((unused)),
		      char **args __attribute__ ((unused)))
{
  holy_test_t test;
  int ok = 1;

  FOR_LIST_ELEMENTS (test, holy_test_list)
    {
      holy_errno = 0;
      ok = ok && !holy_test_run (test);
      holy_errno = 0;
    }
  if (ok)
    holy_printf ("ALL TESTS PASSED\n");
  else
    holy_printf ("TEST FAILURE\n");
  return holy_ERR_NONE;
}

static holy_err_t
holy_functional_all_tests (holy_extcmd_context_t ctxt __attribute__ ((unused)),
			   int argc __attribute__ ((unused)),
			   char **args __attribute__ ((unused)))
{
  holy_test_t test;
  int ok = 1;

  holy_dl_load ("legacy_password_test");
  holy_errno = holy_ERR_NONE;
  holy_dl_load ("exfctest");
  holy_dl_load ("videotest_checksum");
  holy_dl_load ("gfxterm_menu");
  holy_dl_load ("setjmp_test");
  holy_dl_load ("cmdline_cat_test");
  holy_dl_load ("div_test");
  holy_dl_load ("xnu_uuid_test");
  holy_dl_load ("pbkdf2_test");
  holy_dl_load ("signature_test");
  holy_dl_load ("sleep_test");
  holy_dl_load ("bswap_test");
  holy_dl_load ("ctz_test");
  holy_dl_load ("cmp_test");
  holy_dl_load ("mul_test");
  holy_dl_load ("shift_test");

  FOR_LIST_ELEMENTS (test, holy_test_list)
    ok = !holy_test_run (test) && ok;
  if (ok)
    holy_printf ("ALL TESTS PASSED\n");
  else
    holy_printf ("TEST FAILURE\n");
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT (functional_test)
{
  cmd = holy_register_extcmd ("functional_test", holy_functional_test, 0, 0,
			      "Run all loaded functional tests.", 0);
  cmd = holy_register_extcmd ("all_functional_test", holy_functional_all_tests, 0, 0,
			      "Run all functional tests.", 0);
}

holy_MOD_FINI (functional_test)
{
  holy_unregister_extcmd (cmd);
}
