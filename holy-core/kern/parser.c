/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/parser.h>
#include <holy/env.h>
#include <holy/misc.h>
#include <holy/mm.h>


/* All the possible state transitions on the command line.  If a
   transition can not be found, it is assumed that there is no
   transition and keep_value is assumed to be 1.  */
static struct holy_parser_state_transition state_transitions[] = {
  {holy_PARSER_STATE_TEXT, holy_PARSER_STATE_QUOTE, '\'', 0},
  {holy_PARSER_STATE_TEXT, holy_PARSER_STATE_DQUOTE, '\"', 0},
  {holy_PARSER_STATE_TEXT, holy_PARSER_STATE_VAR, '$', 0},
  {holy_PARSER_STATE_TEXT, holy_PARSER_STATE_ESC, '\\', 0},

  {holy_PARSER_STATE_ESC, holy_PARSER_STATE_TEXT, 0, 1},

  {holy_PARSER_STATE_QUOTE, holy_PARSER_STATE_TEXT, '\'', 0},

  {holy_PARSER_STATE_DQUOTE, holy_PARSER_STATE_TEXT, '\"', 0},
  {holy_PARSER_STATE_DQUOTE, holy_PARSER_STATE_QVAR, '$', 0},

  {holy_PARSER_STATE_VAR, holy_PARSER_STATE_VARNAME2, '{', 0},
  {holy_PARSER_STATE_VAR, holy_PARSER_STATE_VARNAME, 0, 1},
  {holy_PARSER_STATE_VARNAME, holy_PARSER_STATE_TEXT, ' ', 1},
  {holy_PARSER_STATE_VARNAME, holy_PARSER_STATE_TEXT, '\t', 1},
  {holy_PARSER_STATE_VARNAME2, holy_PARSER_STATE_TEXT, '}', 0},

  {holy_PARSER_STATE_QVAR, holy_PARSER_STATE_QVARNAME2, '{', 0},
  {holy_PARSER_STATE_QVAR, holy_PARSER_STATE_QVARNAME, 0, 1},
  {holy_PARSER_STATE_QVARNAME, holy_PARSER_STATE_TEXT, '\"', 0},
  {holy_PARSER_STATE_QVARNAME, holy_PARSER_STATE_DQUOTE, ' ', 1},
  {holy_PARSER_STATE_QVARNAME, holy_PARSER_STATE_DQUOTE, '\t', 1},
  {holy_PARSER_STATE_QVARNAME2, holy_PARSER_STATE_DQUOTE, '}', 0},

  {0, 0, 0, 0}
};


/* Determines the state following STATE, determined by C.  */
holy_parser_state_t
holy_parser_cmdline_state (holy_parser_state_t state, char c, char *result)
{
  struct holy_parser_state_transition *transition;
  struct holy_parser_state_transition default_transition;

  default_transition.to_state = state;
  default_transition.keep_value = 1;

  /* Look for a good translation.  */
  for (transition = state_transitions; transition->from_state; transition++)
    {
      if (transition->from_state != state)
	continue;
      /* An exact match was found, use it.  */
      if (transition->input == c)
	break;

      if (holy_isspace (transition->input) && !holy_isalpha (c)
	  && !holy_isdigit (c) && c != '_')
	break;

      /* A less perfect match was found, use this one if no exact
         match can be found.  */
      if (transition->input == 0)
	break;
    }

  if (!transition->from_state)
    transition = &default_transition;

  if (transition->keep_value)
    *result = c;
  else
    *result = 0;
  return transition->to_state;
}


/* Helper for holy_parser_split_cmdline.  */
static inline int
check_varstate (holy_parser_state_t s)
{
  return (s == holy_PARSER_STATE_VARNAME
	  || s == holy_PARSER_STATE_VARNAME2
	  || s == holy_PARSER_STATE_QVARNAME
	  || s == holy_PARSER_STATE_QVARNAME2);
}


static void
add_var (char *varname, char **bp, char **vp,
	 holy_parser_state_t state, holy_parser_state_t newstate)
{
  const char *val;

  /* Check if a variable was being read in and the end of the name
     was reached.  */
  if (!(check_varstate (state) && !check_varstate (newstate)))
    return;

  *((*vp)++) = '\0';
  val = holy_env_get (varname);
  *vp = varname;
  if (!val)
    return;

  /* Insert the contents of the variable in the buffer.  */
  for (; *val; val++)
    *((*bp)++) = *val;
}

holy_err_t
holy_parser_split_cmdline (const char *cmdline,
			   holy_reader_getline_t getline, void *getline_data,
			   int *argc, char ***argv)
{
  holy_parser_state_t state = holy_PARSER_STATE_TEXT;
  /* XXX: Fixed size buffer, perhaps this buffer should be dynamically
     allocated.  */
  char buffer[1024];
  char *bp = buffer;
  char *rd = (char *) cmdline;
  char varname[200];
  char *vp = varname;
  char *args;
  int i;

  *argc = 0;
  do
    {
      if (!rd || !*rd)
	{
	  if (getline)
	    getline (&rd, 1, getline_data);
	  else
	    break;
	}

      if (!rd)
	break;

      for (; *rd; rd++)
	{
	  holy_parser_state_t newstate;
	  char use;

	  newstate = holy_parser_cmdline_state (state, *rd, &use);

	  /* If a variable was being processed and this character does
	     not describe the variable anymore, write the variable to
	     the buffer.  */
	  add_var (varname, &bp, &vp, state, newstate);

	  if (check_varstate (newstate))
	    {
	      if (use)
		*(vp++) = use;
	    }
	  else
	    {
	      if (newstate == holy_PARSER_STATE_TEXT
		  && state != holy_PARSER_STATE_ESC && holy_isspace (use))
		{
		  /* Don't add more than one argument if multiple
		     spaces are used.  */
		  if (bp != buffer && *(bp - 1))
		    {
		      *(bp++) = '\0';
		      (*argc)++;
		    }
		}
	      else if (use)
		*(bp++) = use;
	    }
	  state = newstate;
	}
    }
  while (state != holy_PARSER_STATE_TEXT && !check_varstate (state));

  /* A special case for when the last character was part of a
     variable.  */
  add_var (varname, &bp, &vp, state, holy_PARSER_STATE_TEXT);

  if (bp != buffer && *(bp - 1))
    {
      *(bp++) = '\0';
      (*argc)++;
    }

  /* Reserve memory for the return values.  */
  args = holy_malloc (bp - buffer);
  if (!args)
    return holy_errno;
  holy_memcpy (args, buffer, bp - buffer);

  *argv = holy_malloc (sizeof (char *) * (*argc + 1));
  if (!*argv)
    {
      holy_free (args);
      return holy_errno;
    }

  /* The arguments are separated with 0's, setup argv so it points to
     the right values.  */
  bp = args;
  for (i = 0; i < *argc; i++)
    {
      (*argv)[i] = bp;
      while (*bp)
	bp++;
      bp++;
    }

  return 0;
}

/* Helper for holy_parser_execute.  */
static holy_err_t
holy_parser_execute_getline (char **line, int cont __attribute__ ((unused)),
			     void *data)
{
  char **source = data;
  char *p;

  if (!*source)
    {
      *line = 0;
      return 0;
    }

  p = holy_strchr (*source, '\n');

  if (p)
    *line = holy_strndup (*source, p - *source);
  else
    *line = holy_strdup (*source);
  *source = p ? p + 1 : 0;
  return 0;
}

holy_err_t
holy_parser_execute (char *source)
{
  while (source)
    {
      char *line;

      holy_parser_execute_getline (&line, 0, &source);
      holy_rescue_parse_line (line, holy_parser_execute_getline, &source);
      holy_free (line);
      holy_print_error ();
    }

  return holy_errno;
}
