/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/test.h>
#include <holy/dl.h>
#include <holy/setjmp.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_jmp_buf jmp_point;
static int expected, ctr;

/* This fixes GCC7 "unintentional fallthrough" warning */
static void jmp0 (void) __attribute__ ((noreturn));
static void jmp1 (void) __attribute__ ((noreturn));
static void jmp2 (void) __attribute__ ((noreturn));

static void
jmp0 (void)
{
  holy_longjmp (jmp_point, 0);
}

static void
jmp1 (void)
{
  holy_longjmp (jmp_point, 1);
}

static void
jmp2 (void)
{
  holy_longjmp (jmp_point, 2);
}

static void
setjmp_test (void)
{
  int val;

  expected = 0;
  ctr = 0;
  val = holy_setjmp (jmp_point);

  holy_test_assert (val == expected, "setjmp returned %d instead of %d",
		    val, expected);
  switch (ctr++)
    {
    case 0:
      expected = 1;
      jmp0 ();
    case 1:
      expected = 1;
      jmp1 ();
    case 2:
      expected = 2;
      jmp2 ();
    case 3:
      return;
    }
  holy_test_assert (0, "setjmp didn't return enough times");
}

/* Register example_test method as a functional test.  */
holy_FUNCTIONAL_TEST (setjmp_test, setjmp_test);
