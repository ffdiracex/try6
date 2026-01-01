/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/types.h>
#include <holy/crypto.h>
#include <holy/auth.h>
#include <holy/emu/misc.h>
#include <holy/util/misc.h>
#include <holy/i18n.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
holy_get_random (void *out, holy_size_t len)
{
#warning "No random number generator is available for your OS. \
Some functions like holy-mkpaswd and installing on UUID-less disks will be \
disabled."
  holy_util_error ("%s",
		   /* TRANSLATORS: The OS itself may very well have a random
		      number generator but holy doesn't know how to access it.  */
		   _("no random number generator is available for your OS"));
}
