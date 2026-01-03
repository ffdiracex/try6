/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/test.h>

struct holy_test_failure
{
  /* The next failure.  */
  struct holy_test_failure *next;
  struct holy_test_failure **prev;

  /* The test source file name.  */
  char *file;

  /* The test function name.  */
  char *funp;

  /* The test call line number.  */
  holy_uint32_t line;

  /* The test failure message.  */
  char *message;
};
typedef struct holy_test_failure *holy_test_failure_t;

holy_test_t holy_test_list;
static holy_test_failure_t failure_list;

static holy_test_failure_t
failure_start(const char *file, const char *funp, holy_uint32_t line);
static holy_test_failure_t
failure_start(const char *file, const char *funp, holy_uint32_t line)
{
  holy_test_failure_t failure;

  failure = (holy_test_failure_t) holy_malloc (sizeof (*failure));
  if (!failure)
    return NULL;

  failure->file = holy_strdup (file ? : "<unknown_file>");
  if (!failure->file)
    {
      holy_free(failure);
      return NULL;
    }

  failure->funp = holy_strdup (funp ? : "<unknown_function>");
  if (!failure->funp)
    {
      holy_free(failure->file);
      holy_free(failure);
      return NULL;
    }

  failure->line = line;

  failure->message = NULL;

  return failure;
}

static void
failure_append_vtext(holy_test_failure_t failure, const char *fmt, va_list args);
static void
failure_append_vtext(holy_test_failure_t failure, const char *fmt, va_list args)
{
  char *msg = holy_xvasprintf(fmt, args);
  if (failure->message)
    {
      char *oldmsg = failure->message;

      failure->message = holy_xasprintf("%s%s", oldmsg, msg);
      holy_free (oldmsg);
      holy_free (msg);
    }
  else
    {
      failure->message = msg;
    }
}

static void
failure_append_text(holy_test_failure_t failure, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  failure_append_vtext(failure, fmt, args);
  va_end(args);
}

static void
add_failure (const char *file,
	     const char *funp,
	     holy_uint32_t line, const char *fmt, va_list args)
{
  holy_test_failure_t failure = failure_start(file, funp, line);
  failure_append_text(failure, fmt, args);
  holy_list_push (holy_AS_LIST_P (&failure_list), holy_AS_LIST (failure));
}

static void
free_failures (void)
{
  holy_test_failure_t item;

  while (failure_list)
    {
      item = failure_list;
      failure_list = item->next;
      if (item->message)
	holy_free (item->message);

      if (item->funp)
	holy_free (item->funp);

      if (item->file)
	holy_free (item->file);

      holy_free (item);
    }
  failure_list = 0;
}

void
holy_test_nonzero (int cond,
		   const char *file,
		   const char *funp, holy_uint32_t line, const char *fmt, ...)
{
  va_list ap;

  if (cond)
    return;

  va_start (ap, fmt);
  add_failure (file, funp, line, fmt, ap);
  va_end (ap);
}

void
holy_test_assert_helper (int cond, const char *file, const char *funp,
			 holy_uint32_t line, const char *condstr,
			 const char *fmt, ...)
{
  va_list ap;
  holy_test_failure_t failure;

  if (cond)
    return;

  failure = failure_start(file, funp, line);
  failure_append_text(failure, "assert failed: %s ", condstr);

  va_start(ap, fmt);

  failure_append_vtext(failure, fmt, ap);

  va_end(ap);

  holy_list_push (holy_AS_LIST_P (&failure_list), holy_AS_LIST (failure));
}

void
holy_test_register (const char *name, void (*test_main) (void))
{
  holy_test_t test;

  test = (holy_test_t) holy_malloc (sizeof (*test));
  if (!test)
    return;

  test->name = holy_strdup (name);
  test->main = test_main;

  holy_list_push (holy_AS_LIST_P (&holy_test_list), holy_AS_LIST (test));
}

void
holy_test_unregister (const char *name)
{
  holy_test_t test;

  test = (holy_test_t) holy_named_list_find
    (holy_AS_NAMED_LIST (holy_test_list), name);

  if (test)
    {
      holy_list_remove (holy_AS_LIST (test));

      if (test->name)
	holy_free (test->name);

      holy_free (test);
    }
}

int
holy_test_run (holy_test_t test)
{
  holy_test_failure_t failure;

  test->main ();

  holy_printf ("%s:\n", test->name);
  FOR_LIST_ELEMENTS (failure, failure_list)
    holy_printf (" %s:%s:%u: %s\n",
		 (failure->file ? : "<unknown_file>"),
		 (failure->funp ? : "<unknown_function>"),
		 failure->line, (failure->message ? : "<no message>"));

  if (!failure_list)
    {
      holy_printf ("%s: PASS\n", test->name);
      return holy_ERR_NONE;
    }
  else
    {
      holy_printf ("%s: FAIL\n", test->name);
      free_failures ();
      return holy_ERR_TEST_FAILURE;
    }
}
