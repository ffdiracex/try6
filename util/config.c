/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <string.h>

#include <holy/emu/config.h>
#include <holy/util/misc.h>

void
holy_util_parse_config (FILE *f, struct holy_util_config *cfg, int simple)
{
  char *buffer = NULL;
  size_t sz = 0;
  while (getline (&buffer, &sz, f) >= 0)
    {
      const char *ptr;
      for (ptr = buffer; *ptr && holy_isspace (*ptr); ptr++);
      if (holy_strncmp (ptr, "holy_ENABLE_CRYPTODISK=",
			sizeof ("holy_ENABLE_CRYPTODISK=") - 1) == 0)
	{
	  ptr += sizeof ("holy_ENABLE_CRYPTODISK=") - 1;
	  if (*ptr == '"' || *ptr == '\'')
	    ptr++;
	  if (*ptr == 'y')
	    cfg->is_cryptodisk_enabled = 1;
	  continue;
	}
      if (holy_strncmp (ptr, "holy_DISTRIBUTOR=",
			sizeof ("holy_DISTRIBUTOR=") - 1) == 0)
	{
	  char *optr;
	  enum { NONE, SNGLQUOT, DBLQUOT } state;

	  ptr += sizeof ("holy_DISTRIBUTOR=") - 1;

	  if (simple)
	    {
	      char *ptr2;
	      free (cfg->holy_distributor);
	      cfg->holy_distributor = xstrdup (ptr);
	      for (ptr2 = cfg->holy_distributor
		     + holy_strlen (cfg->holy_distributor) - 1;
		   ptr2 >= cfg->holy_distributor
		     && (*ptr2 == '\r' || *ptr2 == '\n'); ptr2--);
	      ptr2[1] = '\0';
	      continue;
	    }
	  free (cfg->holy_distributor);
	  cfg->holy_distributor = xmalloc (strlen (ptr) + 1);
	  optr = cfg->holy_distributor;
	  state = NONE;

	  for (; *ptr; ptr++)
	    switch (*ptr)
	      {
	      case '\\':
		if (state == SNGLQUOT)
		  {
		    *optr++ = *ptr;
		    continue;
		  }
		if (ptr[1])
		  {
		    *optr++ = ptr[1];
		    ptr++;
		    continue;
		  }
		ptr++;
		break;
	      case '"':
		if (state == NONE)
		  {
		    state = DBLQUOT;
		    continue;
		  }
		if (state == DBLQUOT)
		  {
		    state = NONE;
		    continue;
		  }
		*optr++ = *ptr;
		continue;
	      case '\'':
		if (state == SNGLQUOT)
		  {
		    state = NONE;
		    continue;
		  }
		if (state == NONE)
		  {
		    state = SNGLQUOT;
		    continue;
		  }
		*optr++ = *ptr;
		continue;
	      default:
		*optr++ = *ptr;
		continue;
	      }
	  *optr = '\0';
	}
    }
}

