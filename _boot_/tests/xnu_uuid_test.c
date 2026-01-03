/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/time.h>
#include <holy/misc.h>
#include <holy/dl.h>
#include <holy/command.h>
#include <holy/env.h>
#include <holy/test.h>
#include <holy/mm.h>

holy_MOD_LICENSE ("GPLv2+");

static void
xnu_uuid_test (void)
{
  holy_command_t cmd;
  cmd = holy_command_find ("xnu_uuid");
  char *args[] = { (char *) "fedcba98", (char *) "tstvar", NULL };
  const char *val;

  if (!cmd)
    {
      holy_test_assert (0, "can't find command `%s'", "xnu_uuid");
      return;
    }
  if ((cmd->func) (cmd, 2, args))
    {
      holy_test_assert (0, "%d: %s", holy_errno, holy_errmsg);
      return;
    }

  val = holy_env_get ("tstvar");
  if (!val)
    {
      holy_test_assert (0, "tstvar isn't set");
      return;
    }
  holy_test_assert (holy_strcmp (val, "944F9DED-DBED-391C-9402-77C8CEE04173")
		    == 0, "UUIDs don't match");
}

holy_FUNCTIONAL_TEST (xnu_uuid_test, xnu_uuid_test);
