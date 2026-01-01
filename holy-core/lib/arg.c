/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>

/* Built-in parser for default options.  */
static const struct holy_arg_option help_options[] =
  {
    {"help", 0, 0,
     N_("Display this help and exit."), 0, ARG_TYPE_NONE},
    {"usage", 0, 0,
     N_("Display the usage of this command and exit."), 0, ARG_TYPE_NONE},
    {0, 0, 0, 0, 0, 0}
  };

/* Helper for find_short.  */
static const struct holy_arg_option *
fnd_short (const struct holy_arg_option *opt, char c)
{
  while (opt->doc)
    {
      if (opt->shortarg == c)
	return opt;
      opt++;
    }
  return 0;
}

static const struct holy_arg_option *
find_short (const struct holy_arg_option *options, char c)
{
  const struct holy_arg_option *found = 0;

  if (options)
    found = fnd_short (options, c);

  if (! found)
    {
      switch (c)
	{
	case 'h':
	  found = help_options;
	  break;

	case 'u':
	  found = (help_options + 1);
	  break;

	default:
	  break;
	}
    }

  return found;
}

/* Helper for find_long.  */
static const struct holy_arg_option *
fnd_long (const struct holy_arg_option *opt, const char *s, int len)
{
  while (opt->doc)
    {
      if (opt->longarg && ! holy_strncmp (opt->longarg, s, len) &&
	  opt->longarg[len] == '\0')
	return opt;
      opt++;
    }
  return 0;
}

static const struct holy_arg_option *
find_long (const struct holy_arg_option *options, const char *s, int len)
{
  const struct holy_arg_option *found = 0;

  if (options)
    found = fnd_long (options, s, len);

  if (! found)
    found = fnd_long (help_options, s, len);

  return found;
}

static void
show_usage (holy_extcmd_t cmd)
{
  holy_printf ("%s %s %s\n", _("Usage:"), cmd->cmd->name, _(cmd->cmd->summary));
}

static void
showargs (const struct holy_arg_option *opt,
	  int h_is_used, int u_is_used)
{
  for (; opt->doc; opt++)
    {
      int spacing = 20;

      if (opt->shortarg && holy_isgraph (opt->shortarg))
	holy_printf ("-%c%c ", opt->shortarg, opt->longarg ? ',':' ');
      else if (opt == help_options && ! h_is_used)
	holy_printf ("-h, ");
      else if (opt == help_options + 1 && ! u_is_used)
	holy_printf ("-u, ");
      else
	holy_printf ("    ");

      if (opt->longarg)
	{
	  holy_printf ("--%s", opt->longarg);
	  spacing -= holy_strlen (opt->longarg) + 2;

	  if (opt->arg)
	    {
	      holy_printf ("=%s", opt->arg);
	      spacing -= holy_strlen (opt->arg) + 1;
	    }
	}

      if (spacing <= 0)
	spacing = 3;

      while (spacing--)
	holy_xputs (" ");

      holy_printf ("%s\n", _(opt->doc));
    }
}

void
holy_arg_show_help (holy_extcmd_t cmd)
{
  int h_is_used = 0;
  int u_is_used = 0;
  const struct holy_arg_option *opt;

  show_usage (cmd);
  holy_printf ("%s\n\n", _(cmd->cmd->description));

  for (opt = cmd->options; opt && opt->doc; opt++)
    switch (opt->shortarg)
      {
      case 'h':
	h_is_used = 1;
	break;

      case 'u':
	u_is_used = 1;
	break;
      }

  if (cmd->options)
    showargs (cmd->options, h_is_used, u_is_used);
  showargs (help_options, h_is_used, u_is_used);
#if 0
  holy_printf ("\nReport bugs to <%s>.\n", PACKAGE_BUGREPORT);
#endif
}


static int
parse_option (holy_extcmd_t cmd, const struct holy_arg_option *opt,
	      char *arg, struct holy_arg_list *usr)
{
  if (opt == help_options)
    {
      holy_arg_show_help (cmd);
      return -1;
    }

  if (opt == help_options + 1)
    {
      show_usage (cmd);
      return -1;
    }
  {
    int found = opt - cmd->options;

    if (opt->flags & holy_ARG_OPTION_REPEATABLE)
      {
	usr[found].args[usr[found].set++] = arg;
	usr[found].args[usr[found].set] = NULL;
      }
    else
      {
	usr[found].set = 1;
	usr[found].arg = arg;
      }
  }

  return 0;
}

static inline holy_err_t
add_arg (char ***argl, int *num, char *s)
{
  char **p = *argl;
  *argl = holy_realloc (*argl, (++(*num) + 1) * sizeof (char *));
  if (! *argl)
    {
      holy_free (p);
      return holy_errno;
    }
  (*argl)[(*num) - 1] = s;
  (*argl)[(*num)] = NULL;
  return 0;
}


int
holy_arg_parse (holy_extcmd_t cmd, int argc, char **argv,
		struct holy_arg_list *usr, char ***args, int *argnum)
{
  int curarg;
  int arglen;
  char **argl = 0;
  int num = 0;

  for (curarg = 0; curarg < argc; curarg++)
    {
      char *arg = argv[curarg];
      const struct holy_arg_option *opt;
      char *option = 0;

      /* No option is used.  */
      if ((num && (cmd->cmd->flags & holy_COMMAND_OPTIONS_AT_START))
	  || arg[0] != '-' || holy_strlen (arg) == 1)
	{
	  if (add_arg (&argl, &num, arg) != 0)
	    goto fail;

	  continue;
	}

      /* One or more short options.  */
      if (arg[1] != '-')
	{
	  char *curshort;

	  if (cmd->cmd->flags & holy_COMMAND_ACCEPT_DASH)
	    {
	      for (curshort = arg + 1; *curshort; curshort++)
		if (!find_short (cmd->options, *curshort))
		  break;
	    
	      if (*curshort)
		{
		  if (add_arg (&argl, &num, arg) != 0)
		    goto fail;
		  continue;
		}
	    }

	  curshort = arg + 1;

	  while (1)
	    {
	      opt = find_short (cmd->options, *curshort);

	      if (! opt)
		{
		  char tmp[3] = { '-', *curshort, 0 };
		  holy_error (holy_ERR_BAD_ARGUMENT,
			      N_("unknown argument `%s'"), tmp);
		  goto fail;
		}

	      curshort++;

	      /* Parse all arguments here except the last one because
		 it can have an argument value.  */
	      if (*curshort)
		{
		  if (parse_option (cmd, opt, 0, usr) || holy_errno)
		    goto fail;
		}
	      else
		{
		  if (opt->type != ARG_TYPE_NONE)
		    {
		      if (curarg + 1 < argc)
			{
			  char *nextarg = argv[curarg + 1];
			  if (!(opt->flags & holy_ARG_OPTION_OPTIONAL)
			      || (holy_strlen (nextarg) < 2 || nextarg[0] != '-'))
			    option = argv[++curarg];
			}
		    }
		  break;
		}
	    }

	}
      else /* The argument starts with "--".  */
	{
	  /* If the argument "--" is used just pass the other
	     arguments.  */
	  if (holy_strlen (arg) == 2)
	    {
	      for (curarg++; curarg < argc; curarg++)
		if (add_arg (&argl, &num, argv[curarg]) != 0)
		  goto fail;
	      break;
	    }

	  option = holy_strchr (arg, '=');
	  if (option)
	    {
	      arglen = option - arg - 2;
	      option++;
	    }
	  else
	    arglen = holy_strlen (arg) - 2;

	  opt = find_long (cmd->options, arg + 2, arglen);

	  if (!option && argv[curarg + 1] && argv[curarg + 1][0] != '-'
	      && opt && opt->type != ARG_TYPE_NONE)
	    option = argv[++curarg];

	  if (!opt && (cmd->cmd->flags & holy_COMMAND_ACCEPT_DASH))
	    {
	      if (add_arg (&argl, &num, arg) != 0)
		goto fail;
	      continue;
	    }

	  if (! opt)
	    {
	      holy_error (holy_ERR_BAD_ARGUMENT, N_("unknown argument `%s'"), arg);
	      goto fail;
	    }
	}

      if (! (opt->type == ARG_TYPE_NONE
	     || (! option && (opt->flags & holy_ARG_OPTION_OPTIONAL))))
	{
	  if (! option)
	    {
	      holy_error (holy_ERR_BAD_ARGUMENT,
			  N_("missing mandatory option for `%s'"), opt->longarg);
	      goto fail;
	    }

	  switch (opt->type)
	    {
	    case ARG_TYPE_NONE:
	      /* This will never happen.  */
	      break;

	    case ARG_TYPE_STRING:
		  /* No need to do anything.  */
	      break;

	    case ARG_TYPE_INT:
	      {
		char *tail;

		holy_strtoull (option, &tail, 0);
		if (tail == 0 || tail == option || *tail != '\0' || holy_errno)
		  {
		    holy_error (holy_ERR_BAD_ARGUMENT,
				N_("the argument `%s' requires an integer"),
				arg);

		    goto fail;
		  }
		break;
	      }

	    case ARG_TYPE_DEVICE:
	    case ARG_TYPE_DIR:
	    case ARG_TYPE_FILE:
	    case ARG_TYPE_PATHNAME:
	      /* XXX: Not implemented.  */
	      break;
	    }
	  if (parse_option (cmd, opt, option, usr) || holy_errno)
	    goto fail;
	}
      else
	{
	  if (option)
	    {
	      holy_error (holy_ERR_BAD_ARGUMENT,
			  N_("a value was assigned to the argument `%s' while it "
			     "doesn't require an argument"), arg);
	      goto fail;
	    }

	  if (parse_option (cmd, opt, 0, usr) || holy_errno)
	    goto fail;
	}
    }

  *args = argl;
  *argnum = num;
  return 1;

 fail:
  return 0;
}

struct holy_arg_list*
holy_arg_list_alloc(holy_extcmd_t extcmd, int argc,
		    char **argv __attribute__((unused)))
{
  int i;
  char **args;
  holy_size_t argcnt;
  struct holy_arg_list *list;
  const struct holy_arg_option *options;

  options = extcmd->options;
  if (! options)
    return 0;

  argcnt = 0;
  for (i = 0; options[i].doc; i++)
    {
      if (options[i].flags & holy_ARG_OPTION_REPEATABLE)
	argcnt += ((holy_size_t) argc + 1) / 2 + 1; /* max possible for any option */
    }

  list = holy_zalloc (sizeof (*list) * i + sizeof (char*) * argcnt);
  if (! list)
    return 0;

  args = (char**) (list + i);
  for (i = 0; options[i].doc; i++)
    {
      list[i].set = 0;
      list[i].arg = 0;

      if (options[i].flags & holy_ARG_OPTION_REPEATABLE)
	{
	  list[i].args = args;
	  args += (holy_size_t) argc / 2 + 1;
	}
    }
  return list;
}
