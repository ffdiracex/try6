/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/parser.h>
#include <holy/extcmd.h>
#include <holy/charset.h>

/* The current word.  */
static const char *current_word;

/* The matched string.  */
static char *match;

/* The count of candidates.  */
static int num_found;

/* The string to be appended.  */
static const char *suffix;

/* The callback function to print items.  */
static void (*print_func) (const char *, holy_completion_type_t, int);

/* The state the command line is in.  */
static holy_parser_state_t cmdline_state;


/* Add a string to the list of possible completions. COMPLETION is the
   string that should be added. EXTRA will be appended if COMPLETION
   matches uniquely. The type TYPE specifies what kind of data is added.  */
static int
add_completion (const char *completion, const char *extra,
		holy_completion_type_t type)
{
  if (holy_strncmp (current_word, completion, holy_strlen (current_word)) == 0)
    {
      num_found++;

      switch (num_found)
	{
	case 1:
	  match = holy_strdup (completion);
	  if (! match)
	    return 1;
	  suffix = extra;
	  break;

	case 2:
	  if (print_func)
	    print_func (match, type, 0);

	  /* Fall through.  */

	default:
	  {
	    char *s = match;
	    const char *t = completion;

	    if (print_func)
	      print_func (completion, type, num_found - 1);

	    /* Detect the matched portion.  */
	    while (*s && *t && *s == *t)
	      {
		s++;
		t++;
	      }
	    s = match + holy_getend (match, s);

	    *s = '\0';
	  }
	  break;
	}
    }

  return 0;
}

static int
iterate_partition (holy_disk_t disk, const holy_partition_t p,
		   void *data __attribute__ ((unused)))
{
  const char *disk_name = disk->name;
  char *name;
  int ret;
  char *part_name;

  part_name = holy_partition_get_name (p);
  if (! part_name)
    return 1;

  name = holy_xasprintf ("%s,%s", disk_name, part_name);
  holy_free (part_name);

  if (! name)
    return 1;

  ret = add_completion (name, ")", holy_COMPLETION_TYPE_PARTITION);
  holy_free (name);
  return ret;
}

static int
iterate_dir (const char *filename, const struct holy_dirhook_info *info,
	     void *data __attribute__ ((unused)))
{
  if (! info->dir)
    {
      const char *prefix;
      if (cmdline_state == holy_PARSER_STATE_DQUOTE)
	prefix = "\" ";
      else if (cmdline_state == holy_PARSER_STATE_QUOTE)
	prefix = "\' ";
      else
	prefix = " ";

      if (add_completion (filename, prefix, holy_COMPLETION_TYPE_FILE))
	return 1;
    }
  else if (holy_strcmp (filename, ".") && holy_strcmp (filename, ".."))
    {
      char *fname;

      fname = holy_xasprintf ("%s/", filename);
      if (add_completion (fname, "", holy_COMPLETION_TYPE_FILE))
	{
	  holy_free (fname);
	  return 1;
	}
      holy_free (fname);
    }

  return 0;
}

static int
iterate_dev (const char *devname, void *data __attribute__ ((unused)))
{
  holy_device_t dev;

  /* Complete the partition part.  */
  dev = holy_device_open (devname);

  if (!dev)
    {
      holy_errno = holy_ERR_NONE;
      return 0;
    }

  if (holy_strcmp (devname, current_word) == 0)
    {
      if (add_completion (devname, ")", holy_COMPLETION_TYPE_PARTITION))
	{
	  holy_device_close (dev);
	  return 1;
	}

      if (dev->disk)
	if (holy_partition_iterate (dev->disk, iterate_partition, NULL))
	  {
	    holy_device_close (dev);
	    return 1;
	  }
    }
  else if (add_completion (devname, "", holy_COMPLETION_TYPE_DEVICE))
    {
      holy_device_close (dev);
      return 1;
    }

  holy_device_close (dev);
  holy_errno = holy_ERR_NONE;
  return 0;
}

/* Complete a device.  */
static int
complete_device (void)
{
  /* Check if this is a device or a partition.  */
  char *p = holy_strchr (++current_word, ',');
  holy_device_t dev;

  if (! p)
    {
      /* Complete the disk part.  */
      if (holy_disk_dev_iterate (iterate_dev, NULL))
	return 1;
    }
  else
    {
      /* Complete the partition part.  */
      *p = '\0';
      dev = holy_device_open (current_word);
      *p = ',';
      holy_errno = holy_ERR_NONE;

      if (dev)
	{
	  if (dev->disk)
	    {
	      if (holy_partition_iterate (dev->disk, iterate_partition, NULL))
		{
		  holy_device_close (dev);
		  return 1;
		}
	    }

	  holy_device_close (dev);
	}
      else
	return 1;
    }

  return 0;
}

/* Complete a file.  */
static int
complete_file (void)
{
  char *device;
  char *dir;
  char *last_dir;
  holy_fs_t fs;
  holy_device_t dev;
  int ret = 0;

  device = holy_file_get_device_name (current_word);
  if (holy_errno != holy_ERR_NONE)
    return 1;

  dev = holy_device_open (device);
  if (! dev)
    {
      ret = 1;
      goto fail;
    }

  fs = holy_fs_probe (dev);
  if (! fs)
    {
      ret = 1;
      goto fail;
    }

  dir = holy_strchr (current_word + (device ? 2 + holy_strlen (device) : 0),
		     '/');
  last_dir = holy_strrchr (current_word, '/');
  if (dir)
    {
      char *dirfile;

      current_word = last_dir + 1;

      dir = holy_strdup (dir);
      if (! dir)
	{
	  ret = 1;
	  goto fail;
	}

      /* Cut away the filename part.  */
      dirfile = holy_strrchr (dir, '/');
      dirfile[1] = '\0';

      /* Iterate the directory.  */
      (fs->dir) (dev, dir, iterate_dir, NULL);

      holy_free (dir);

      if (holy_errno)
	{
	  ret = 1;
	  goto fail;
	}
    }
  else
    {
      current_word += holy_strlen (current_word);
      match = holy_strdup ("/");
      if (! match)
	{
	  ret = 1;
	  goto fail;
	}

      suffix = "";
      num_found = 1;
    }

 fail:
  if (dev)
    holy_device_close (dev);
  holy_free (device);
  return ret;
}

/* Complete an argument.  */
static int
complete_arguments (char *command)
{
  holy_command_t cmd;
  holy_extcmd_t ext;
  const struct holy_arg_option *option;
  char shortarg[] = "- ";

  cmd = holy_command_find (command);

  if (!cmd || !(cmd->flags & holy_COMMAND_FLAG_EXTCMD))
    return 0;

  ext = cmd->data;
  if (!ext->options)
    return 0;

  if (add_completion ("-u", " ", holy_COMPLETION_TYPE_ARGUMENT))
    return 1;

  /* Add the short arguments.  */
  for (option = ext->options; option->doc; option++)
    {
      if (! option->shortarg)
	continue;

      shortarg[1] = option->shortarg;
      if (add_completion (shortarg, " ", holy_COMPLETION_TYPE_ARGUMENT))
	return 1;

    }

  /* First add the built-in arguments.  */
  if (add_completion ("--help", " ", holy_COMPLETION_TYPE_ARGUMENT))
    return 1;
  if (add_completion ("--usage", " ", holy_COMPLETION_TYPE_ARGUMENT))
    return 1;

  /* Add the long arguments.  */
  for (option = ext->options; option->doc; option++)
    {
      char *longarg;
      if (!option->longarg)
	continue;

      longarg = holy_xasprintf ("--%s", option->longarg);
      if (!longarg)
	return 1;

      if (add_completion (longarg, " ", holy_COMPLETION_TYPE_ARGUMENT))
	{
	  holy_free (longarg);
	  return 1;
	}
      holy_free (longarg);
    }

  return 0;
}


static holy_parser_state_t
get_state (const char *cmdline)
{
  holy_parser_state_t state = holy_PARSER_STATE_TEXT;
  char use;

  while (*cmdline)
    state = holy_parser_cmdline_state (state, *(cmdline++), &use);
  return state;
}


/* Try to complete the string in BUF. Return the characters that
   should be added to the string.  This command outputs the possible
   completions by calling HOOK, in that case set RESTORE to 1 so the
   caller can restore the prompt.  */
char *
holy_normal_do_completion (char *buf, int *restore,
			   void (*hook) (const char *, holy_completion_type_t, int))
{
  int argc;
  char **argv;

  /* Initialize variables.  */
  match = 0;
  num_found = 0;
  suffix = "";
  print_func = hook;

  *restore = 1;

  if (holy_parser_split_cmdline (buf, 0, 0, &argc, &argv))
    return 0;

  if (argc == 0)
    current_word = "";
  else
    current_word = argv[argc - 1];

  if (argc > 1 && ! holy_strcmp (argv[0], "set"))
    {
      char *equals = holy_strchr (current_word, '=');
      if (equals)
	/* Complete the value of the variable.  */
	current_word = equals + 1;
    }

  /* Determine the state the command line is in, depending on the
     state, it can be determined how to complete.  */
  cmdline_state = get_state (buf);

  if (argc == 1 || argc == 0)
    {
      /* Complete a command.  */
      holy_command_t cmd;
      FOR_COMMANDS(cmd)
      {
	if (cmd->prio & holy_COMMAND_FLAG_ACTIVE)
	  {
	    if (add_completion (cmd->name, " ", holy_COMPLETION_TYPE_COMMAND))
	      goto fail;
	  }
      }
    }
  else if (*current_word == '-')
    {
      if (complete_arguments (buf))
	goto fail;
    }
  else if (*current_word == '(' && ! holy_strchr (current_word, ')'))
    {
      /* Complete a device.  */
      if (complete_device ())
	    goto fail;
    }
  else
    {
      /* Complete a file.  */
      if (complete_file ())
	goto fail;
    }

  /* If more than one match is found those matches will be printed and
     the prompt should be restored.  */
  if (num_found > 1)
    *restore = 1;
  else
    *restore = 0;

  /* Return the part that matches.  */
  if (match)
    {
      char *ret;
      char *escstr;
      char *newstr;
      int current_len;
      int match_len;
      int spaces = 0;

      current_len = holy_strlen (current_word);
      match_len = holy_strlen (match);

      /* Count the number of spaces that have to be escaped.  XXX:
	 More than just spaces have to be escaped.  */
      for (escstr = match + current_len; *escstr; escstr++)
	if (*escstr == ' ')
	  spaces++;

      ret = holy_malloc (match_len - current_len + holy_strlen (suffix) + spaces + 1);
      newstr = ret;
      for (escstr = match + current_len; *escstr; escstr++)
	{
	  if (*escstr == ' ' && cmdline_state != holy_PARSER_STATE_QUOTE
	      && cmdline_state != holy_PARSER_STATE_QUOTE)
	    *(newstr++) = '\\';
	  *(newstr++) = *escstr;
	}
      *newstr = '\0';

      if (num_found == 1)
	holy_strcpy (newstr, suffix);

      if (*ret == '\0')
	{
	  holy_free (ret);
          goto fail;
	}

      if (argc != 0)
	holy_free (argv[0]);
      holy_free (match);
      return ret;
    }

 fail:
  if (argc != 0)
    {
      holy_free (argv[0]);
      holy_free (argv);
    }
  holy_free (match);
  holy_errno = holy_ERR_NONE;

  return 0;
}
