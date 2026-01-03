/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/i18n.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/tpm.h>
#include <holy/term.h>

holy_err_t
holy_tpm_measure (unsigned char *buf, holy_size_t size, holy_uint8_t pcr,
		  const char *kind, const char *description)
{
  holy_err_t ret;
  char *desc = holy_xasprintf("%s %s", kind, description);
  if (!desc)
    return holy_ERR_OUT_OF_MEMORY;
  ret = holy_tpm_log_event(buf, size, pcr, desc);
  holy_free(desc);
  return ret;
}
