/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/kernel.h>
#include <holy/mm.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static holy_err_t (*holy_loader_boot_func) (void);
static holy_err_t (*holy_loader_unload_func) (void);
static int holy_loader_flags;

struct holy_preboot
{
  holy_err_t (*preboot_func) (int);
  holy_err_t (*preboot_rest_func) (void);
  holy_loader_preboot_hook_prio_t prio;
  struct holy_preboot *next;
  struct holy_preboot *prev;
};

static int holy_loader_loaded;
static struct holy_preboot *preboots_head = 0,
  *preboots_tail = 0;

int
holy_loader_is_loaded (void)
{
  return holy_loader_loaded;
}

/* Register a preboot hook. */
struct holy_preboot *
holy_loader_register_preboot_hook (holy_err_t (*preboot_func) (int flags),
				   holy_err_t (*preboot_rest_func) (void),
				   holy_loader_preboot_hook_prio_t prio)
{
  struct holy_preboot *cur, *new_preboot;

  if (! preboot_func && ! preboot_rest_func)
    return 0;

  new_preboot = (struct holy_preboot *)
    holy_malloc (sizeof (struct holy_preboot));
  if (! new_preboot)
    return 0;

  new_preboot->preboot_func = preboot_func;
  new_preboot->preboot_rest_func = preboot_rest_func;
  new_preboot->prio = prio;

  for (cur = preboots_head; cur && cur->prio > prio; cur = cur->next);

  if (cur)
    {
      new_preboot->next = cur;
      new_preboot->prev = cur->prev;
      cur->prev = new_preboot;
    }
  else
    {
      new_preboot->next = 0;
      new_preboot->prev = preboots_tail;
      preboots_tail = new_preboot;
    }
  if (new_preboot->prev)
    new_preboot->prev->next = new_preboot;
  else
    preboots_head = new_preboot;

  return new_preboot;
}

void
holy_loader_unregister_preboot_hook (struct holy_preboot *hnd)
{
  struct holy_preboot *preb = hnd;

  if (preb->next)
    preb->next->prev = preb->prev;
  else
    preboots_tail = preb->prev;
  if (preb->prev)
    preb->prev->next = preb->next;
  else
    preboots_head = preb->next;

  holy_free (preb);
}

void
holy_loader_set (holy_err_t (*boot) (void),
		 holy_err_t (*unload) (void),
		 int flags)
{
  if (holy_loader_loaded && holy_loader_unload_func)
    holy_loader_unload_func ();

  holy_loader_boot_func = boot;
  holy_loader_unload_func = unload;
  holy_loader_flags = flags;

  holy_loader_loaded = 1;
}

void
holy_loader_unset(void)
{
  if (holy_loader_loaded && holy_loader_unload_func)
    holy_loader_unload_func ();

  holy_loader_boot_func = 0;
  holy_loader_unload_func = 0;

  holy_loader_loaded = 0;
}

holy_err_t
holy_loader_boot (void)
{
  holy_err_t err = holy_ERR_NONE;
  struct holy_preboot *cur;

  if (! holy_loader_loaded)
    return holy_error (holy_ERR_NO_KERNEL,
		       N_("you need to load the kernel first"));

  holy_machine_fini (holy_loader_flags);

  for (cur = preboots_head; cur; cur = cur->next)
    {
      err = cur->preboot_func (holy_loader_flags);
      if (err)
	{
	  for (cur = cur->prev; cur; cur = cur->prev)
	    cur->preboot_rest_func ();
	  return err;
	}
    }
  err = (holy_loader_boot_func) ();

  for (cur = preboots_tail; cur; cur = cur->prev)
    if (! err)
      err = cur->preboot_rest_func ();
    else
      cur->preboot_rest_func ();

  return err;
}

/* boot */
static holy_err_t
holy_cmd_boot (struct holy_command *cmd __attribute__ ((unused)),
		    int argc __attribute__ ((unused)),
		    char *argv[] __attribute__ ((unused)))
{
  return holy_loader_boot ();
}



static holy_command_t cmd_boot;

holy_MOD_INIT(boot)
{
  cmd_boot =
    holy_register_command ("boot", holy_cmd_boot,
			   0, N_("Boot an operating system."));
}

holy_MOD_FINI(boot)
{
  holy_unregister_command (cmd_boot);
}
