/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/misc.h>
#include <stdarg.h>
#include <holy/i18n.h>

#define holy_ERROR_STACK_SIZE	10

holy_err_t holy_errno;
char holy_errmsg[holy_MAX_ERRMSG];
int holy_err_printed_errors;

static struct holy_error_saved holy_error_stack_items[holy_ERROR_STACK_SIZE];

static int holy_error_stack_pos;
static int holy_error_stack_assert;

holy_err_t
holy_error (holy_err_t n, const char *fmt, ...)
{
  va_list ap;

  holy_errno = n;

  va_start (ap, fmt);
  holy_vsnprintf (holy_errmsg, sizeof (holy_errmsg), _(fmt), ap);
  va_end (ap);

  return n;
}

void
holy_error_push (void)
{
  /* Only add items to stack, if there is enough room.  */
  if (holy_error_stack_pos < holy_ERROR_STACK_SIZE)
    {
      /* Copy active error message to stack.  */
      holy_error_stack_items[holy_error_stack_pos].holy_errno = holy_errno;
      holy_memcpy (holy_error_stack_items[holy_error_stack_pos].errmsg,
                   holy_errmsg,
                   sizeof (holy_errmsg));

      /* Advance to next error stack position.  */
      holy_error_stack_pos++;
    }
  else
    {
      /* There is no room for new error message. Discard new error message
         and mark error stack assertion flag.  */
      holy_error_stack_assert = 1;
    }

  /* Allow further operation of other components by resetting
     active errno to holy_ERR_NONE.  */
  holy_errno = holy_ERR_NONE;
}

int
holy_error_pop (void)
{
  if (holy_error_stack_pos > 0)
    {
      /* Pop error message from error stack to current active error.  */
      holy_error_stack_pos--;

      holy_errno = holy_error_stack_items[holy_error_stack_pos].holy_errno;
      holy_memcpy (holy_errmsg,
                   holy_error_stack_items[holy_error_stack_pos].errmsg,
                   sizeof (holy_errmsg));

      return 1;
    }
  else
    {
      /* There is no more items on error stack, reset to no error state.  */
      holy_errno = holy_ERR_NONE;

      return 0;
    }
}

void
holy_print_error (void)
{
  /* Print error messages in reverse order. First print active error message
     and then empty error stack.  */
  do
    {
      if (holy_errno != holy_ERR_NONE)
	{
	  holy_err_printf (_("error: %s.\n"), holy_errmsg);
	  holy_err_printed_errors++;
	}
    }
  while (holy_error_pop ());

  /* If there was an assert while using error stack, report about it.  */
  if (holy_error_stack_assert)
    {
      holy_err_printf ("assert: error stack overflow detected!\n");
      holy_error_stack_assert = 0;
    }
}
