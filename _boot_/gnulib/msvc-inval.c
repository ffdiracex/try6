/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include "msvc-inval.h"

#if HAVE_MSVC_INVALID_PARAMETER_HANDLER \
    && !(MSVC_INVALID_PARAMETER_HANDLING == SANE_LIBRARY_HANDLING)

/* Get _invalid_parameter_handler type and _set_invalid_parameter_handler
   declaration.  */
# include <stdlib.h>

# if MSVC_INVALID_PARAMETER_HANDLING == DEFAULT_HANDLING

static void __cdecl
gl_msvc_invalid_parameter_handler (const wchar_t *expression,
                                   const wchar_t *function,
                                   const wchar_t *file,
                                   unsigned int line,
                                   uintptr_t dummy)
{
}

# else

/* Get declarations of the native Windows API functions.  */
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>

#  if defined _MSC_VER

static void cdecl
gl_msvc_invalid_parameter_handler (const wchar_t *expression,
                                   const wchar_t *function,
                                   const wchar_t *file,
                                   unsigned int line,
                                   uintptr_t dummy)
{
  RaiseException (STATUS_GNULIB_INVALID_PARAMETER, 0, 0, NULL);
}

#  else

/* An index to thread-local storage.  */
static DWORD tls_index;
static int tls_initialized /* = 0 */;

/* Used as a fallback only.  */
static struct gl_msvc_inval_per_thread not_per_thread;

struct gl_msvc_inval_per_thread *
gl_msvc_inval_current (void)
{
  if (!tls_initialized)
    {
      tls_index = TlsAlloc ();
      tls_initialized = 1;
    }
  if (tls_index == TLS_OUT_OF_INDEXES)
    /* TlsAlloc had failed.  */
    return &not_per_thread;
  else
    {
      struct gl_msvc_inval_per_thread *pointer =
        (struct gl_msvc_inval_per_thread *) TlsGetValue (tls_index);
      if (pointer == NULL)
        {
          /* First call.  Allocate a new 'struct gl_msvc_inval_per_thread'.  */
          pointer =
            (struct gl_msvc_inval_per_thread *)
            malloc (sizeof (struct gl_msvc_inval_per_thread));
          if (pointer == NULL)
            /* Could not allocate memory.  Use the global storage.  */
            pointer = &not_per_thread;
          TlsSetValue (tls_index, pointer);
        }
      return pointer;
    }
}

static void cdecl
gl_msvc_invalid_parameter_handler (const wchar_t *expression,
                                   const wchar_t *function,
                                   const wchar_t *file,
                                   unsigned int line,
                                   uintptr_t dummy)
{
  struct gl_msvc_inval_per_thread *current = gl_msvc_inval_current ();
  if (current->restart_valid)
    longjmp (current->restart, 1);
  else
    /* An invalid parameter notification from outside the gnulib code.
       Give the caller a chance to intervene.  */
    RaiseException (STATUS_GNULIB_INVALID_PARAMETER, 0, 0, NULL);
}

#  endif

# endif

static int gl_msvc_inval_initialized /* = 0 */;

void
gl_msvc_inval_ensure_handler (void)
{
  if (gl_msvc_inval_initialized == 0)
    {
      _set_invalid_parameter_handler (gl_msvc_invalid_parameter_handler);
      gl_msvc_inval_initialized = 1;
    }
}

#endif
