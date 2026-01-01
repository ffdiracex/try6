/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/crypto.h>
#include <holy/legacy_parse.h>
#include <holy/auth.h>

holy_MOD_LICENSE ("GPLv2+");

static struct
{
  char **args;
  int argc;
  char entered[holy_AUTH_MAX_PASSLEN];
  int exp;
} vectors[] = {
  { (char * []) { (char *) "hello", NULL }, 1, "hello", 1 },
  { (char * []) { (char *) "hello", NULL }, 1, "hi", 0 },
  { (char * []) { (char *) "hello", NULL }, 1, "hillo", 0 },
  { (char * []) { (char *) "hello", NULL }, 1, "hellw", 0 },
  { (char * []) { (char *) "hello", NULL }, 1, "hell", 0 },
  { (char * []) { (char *) "hello", NULL }, 1, "h", 0 },
  { (char * []) { (char *) "--md5", (char *) "$1$maL$OKEF0PD2k6eQ0Po8u4Gjr/",
		  NULL }, 2, "hello", 1 },
  { (char * []) { (char *) "--md5", (char *) "$1$maL$OKEF0PD2k6eQ0Po8u4Gjr/",
		  NULL }, 2, "hell", 0 },
  { (char * []) { (char *) "--md5", (char *) "$1$naL$BaFO8zGgmss1E76GsrAec1",
		  NULL }, 2, "hello", 1 },
  { (char * []) { (char *) "--md5", (char *) "$1$naL$BaFO8zGgmss1E76GsrAec1",
		  NULL }, 2, "hell", 0 },
  { (char * []) { (char *) "--md5", (char *) "$1$oaL$eyrazuM7TkxVkKgBim1WH1",
		  NULL }, 2, "hi", 1 },
  { (char * []) { (char *) "--md5", (char *) "$1$oaL$eyrazuM7TkxVkKgBim1WH1",
		  NULL }, 2, "hello", 0 },
};

static void
legacy_password_test (void)
{
  holy_size_t i;

  for (i = 0; i < ARRAY_SIZE (vectors); i++)
    holy_test_assert (holy_legacy_check_md5_password (vectors[i].argc,
						      vectors[i].args,
						      vectors[i].entered)
		      == vectors[i].exp, "Bad password check (%d)", (int) i);
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (legacy_password_test, legacy_password_test);
