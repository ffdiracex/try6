/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <holy/list.h>
#include <holy/test.h>

int
main (int argc __attribute__ ((unused)),
      char *argv[] __attribute__ ((unused)))
{
  int status = 0;

  holy_test_t test;

  holy_unit_test_init ();
  FOR_LIST_ELEMENTS (test, holy_test_list)
    status = holy_test_run (test) ? : status;

  holy_unit_test_fini ();

  exit (status);
}
