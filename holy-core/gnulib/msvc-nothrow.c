/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

/* Specification.  */
#include "msvc-nothrow.h"

/* Get declarations of the native Windows API functions.  */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "msvc-inval.h"

#undef _get_osfhandle

#if HAVE_MSVC_INVALID_PARAMETER_HANDLER
intptr_t
_gl_nothrow_get_osfhandle (int fd)
{
  intptr_t result;

  TRY_MSVC_INVAL
    {
      result = _get_osfhandle (fd);
    }
  CATCH_MSVC_INVAL
    {
      result = (intptr_t) INVALID_HANDLE_VALUE;
    }
  DONE_MSVC_INVAL;

  return result;
}
#endif
