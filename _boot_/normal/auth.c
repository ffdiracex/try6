/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/auth.h>
#include <holy/list.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/time.h>
#include <holy/i18n.h>

struct holy_auth_user
{
  struct holy_auth_user *next;
  struct holy_auth_user **prev;
  char *name;
  holy_auth_callback_t callback;
  void *arg;
  int authenticated;
};

static struct holy_auth_user *users = NULL;

holy_err_t
holy_auth_register_authentication (const char *user,
				   holy_auth_callback_t callback,
				   void *arg)
{
  struct holy_auth_user *cur;

  cur = holy_named_list_find (holy_AS_NAMED_LIST (users), user);
  if (!cur)
    cur = holy_zalloc (sizeof (*cur));
  if (!cur)
    return holy_errno;
  cur->callback = callback;
  cur->arg = arg;
  if (! cur->name)
    {
      cur->name = holy_strdup (user);
      if (!cur->name)
	{
	  holy_free (cur);
	  return holy_errno;
	}
      holy_list_push (holy_AS_LIST_P (&users), holy_AS_LIST (cur));
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_auth_unregister_authentication (const char *user)
{
  struct holy_auth_user *cur;
  cur = holy_named_list_find (holy_AS_NAMED_LIST (users), user);
  if (!cur)
    return holy_error (holy_ERR_BAD_ARGUMENT, "user '%s' not found", user);
  if (!cur->authenticated)
    {
      holy_free (cur->name);
      holy_list_remove (holy_AS_LIST (cur));
      holy_free (cur);
    }
  else
    {
      cur->callback = NULL;
      cur->arg = NULL;
    }
  return holy_ERR_NONE;
}

holy_err_t
holy_auth_authenticate (const char *user)
{
  struct holy_auth_user *cur;

  cur = holy_named_list_find (holy_AS_NAMED_LIST (users), user);
  if (!cur)
    cur = holy_zalloc (sizeof (*cur));
  if (!cur)
    return holy_errno;

  cur->authenticated = 1;

  if (! cur->name)
    {
      cur->name = holy_strdup (user);
      if (!cur->name)
	{
	  holy_free (cur);
	  return holy_errno;
	}
      holy_list_push (holy_AS_LIST_P (&users), holy_AS_LIST (cur));
    }

  return holy_ERR_NONE;
}

holy_err_t
holy_auth_deauthenticate (const char *user)
{
  struct holy_auth_user *cur;
  cur = holy_named_list_find (holy_AS_NAMED_LIST (users), user);
  if (!cur)
    return holy_error (holy_ERR_BAD_ARGUMENT, "user '%s' not found", user);
  if (!cur->callback)
    {
      holy_free (cur->name);
      holy_list_remove (holy_AS_LIST (cur));
      holy_free (cur);
    }
  else
    cur->authenticated = 0;
  return holy_ERR_NONE;
}

static int
is_authenticated (const char *userlist)
{
  const char *superusers;
  struct holy_auth_user *user;

  superusers = holy_env_get ("superusers");

  if (!superusers)
    return 1;

  FOR_LIST_ELEMENTS (user, users)
    {
      if (!(user->authenticated))
	continue;

      if ((userlist && holy_strword (userlist, user->name))
	  || holy_strword (superusers, user->name))
	return 1;
    }

  return 0;
}

static int
holy_username_get (char buf[], unsigned buf_size)
{
  unsigned cur_len = 0;
  int key;

  while (1)
    {
      key = holy_getkey ();
      if (key == '\n' || key == '\r')
	break;

      if (key == '\e')
	{
	  cur_len = 0;
	  break;
	}

      if (key == '\b')
	{
	  if (cur_len)
	    {
	      cur_len--;
	      holy_printf ("\b \b");
	    }
	  continue;
	}

      if (!holy_isprint (key))
	continue;

      if (cur_len + 2 < buf_size)
	{
	  buf[cur_len++] = key;
	  holy_printf ("%c", key);
	}
    }

  holy_memset (buf + cur_len, 0, buf_size - cur_len);

  holy_xputs ("\n");
  holy_refresh ();

  return (key != '\e');
}

holy_err_t
holy_auth_check_authentication (const char *userlist)
{
  char login[1024];
  struct holy_auth_user *cur = NULL;
  static unsigned long punishment_delay = 1;
  char entered[holy_AUTH_MAX_PASSLEN];
  struct holy_auth_user *user;

  holy_memset (login, 0, sizeof (login));

  if (is_authenticated (userlist))
    {
      punishment_delay = 1;
      return holy_ERR_NONE;
    }

  holy_puts_ (N_("Enter username: "));

  if (!holy_username_get (login, sizeof (login) - 1))
    goto access_denied;

  holy_puts_ (N_("Enter password: "));

  if (!holy_password_get (entered, holy_AUTH_MAX_PASSLEN))
    goto access_denied;

  FOR_LIST_ELEMENTS (user, users)
    {
      if (holy_strcmp (login, user->name) == 0)
	cur = user;
    }

  if (!cur || ! cur->callback)
    goto access_denied;

  cur->callback (login, entered, cur->arg);
  if (is_authenticated (userlist))
    {
      punishment_delay = 1;
      return holy_ERR_NONE;
    }

 access_denied:
  holy_sleep (punishment_delay);

  if (punishment_delay < holy_ULONG_MAX / 2)
    punishment_delay *= 2;

  return holy_ACCESS_DENIED;
}

static holy_err_t
holy_cmd_authenticate (struct holy_command *cmd __attribute__ ((unused)),
		       int argc, char **args)
{
  return holy_auth_check_authentication ((argc >= 1) ? args[0] : "");
}

static holy_command_t cmd;

void
holy_normal_auth_init (void)
{
  cmd = holy_register_command ("authenticate",
			       holy_cmd_authenticate,
			       N_("[USERLIST]"),
			       N_("Check whether user is in USERLIST."));

}

void
holy_normal_auth_fini (void)
{
  holy_unregister_command (cmd);
}
