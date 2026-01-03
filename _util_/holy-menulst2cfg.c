/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/legacy_parse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <holy/util/misc.h>
#include <holy/misc.h>
#include <holy/i18n.h>

int
main (int argc, char **argv)
{
  FILE *in, *out;
  char *entryname = NULL;
  char *buf = NULL;
  size_t bufsize = 0;
  char *suffix = xstrdup ("");
  int suffixlen = 0;
  const char *out_fname = 0;

  holy_util_host_init (&argc, &argv);

  if (argc >= 2 && argv[1][0] == '-')
    {
      fprintf (stdout, _("Usage: %s [INFILE [OUTFILE]]\n"), argv[0]);
      return 0;
    }

  if (argc >= 2)
    {
      in = holy_util_fopen (argv[1], "r");
      if (!in)
	{
	  fprintf (stderr, _("cannot open `%s': %s"),
		   argv[1], strerror (errno));
	  return 1;
	}
    }
  else
    in = stdin;

  if (argc >= 3)
    {
      out = holy_util_fopen (argv[2], "w");
      if (!out)
	{					
	  if (in != stdin)
	    fclose (in);
	  fprintf (stderr, _("cannot open `%s': %s"),
		   argv[2], strerror (errno));
	  return 1;
	}
      out_fname = argv[2];
    }
  else
    out = stdout;

  while (1)
    {
      char *parsed;

      if (getline (&buf, &bufsize, in) < 0)
	break;

      {
	char *oldname = NULL;
	char *newsuffix;

	oldname = entryname;
	parsed = holy_legacy_parse (buf, &entryname, &newsuffix);
	if (newsuffix)
	  {
	    suffixlen += strlen (newsuffix);
	    suffix = xrealloc (suffix, suffixlen + 1);
	    strcat (suffix, newsuffix);
	  }
	if (oldname != entryname && oldname)
	  fprintf (out, "}\n\n");
	if (oldname != entryname)
	  {
	    char *escaped = holy_legacy_escape (entryname, strlen (entryname));
	    fprintf (out, "menuentry \'%s\' {\n", escaped);
	    free (escaped);
	    free (oldname);
	  }
      }

      if (parsed)
	fprintf (out, "%s%s", entryname ? "  " : "", parsed);
      free (parsed);
      parsed = NULL;
    }

  if (entryname)
    fprintf (out, "}\n\n");

  if (fwrite (suffix, 1, suffixlen, out) != suffixlen)
    {
      if (out_fname)
	holy_util_error ("cannot write to `%s': %s",
			 out_fname, strerror (errno));
      else
	holy_util_error ("cannot write to the stdout: %s", strerror (errno));
    }

  free (buf);
  free (suffix);
  free (entryname);

  if (in != stdin)
    fclose (in);
  if (out != stdout)
    fclose (out);

  return 0;
}
