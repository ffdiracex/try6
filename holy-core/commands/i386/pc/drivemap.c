/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/extcmd.h>
#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/disk.h>
#include <holy/loader.h>
#include <holy/env.h>
#include <holy/machine/biosnum.h>
#include <holy/i18n.h>
#include <holy/memory.h>
#include <holy/machine/memory.h>

holy_MOD_LICENSE ("GPLv2+");

/* Real mode IVT slot (seg:off far pointer) for interrupt 0x13.  */
static holy_uint32_t *const int13slot = (holy_uint32_t *) (4 * 0x13);

/* Remember to update enum opt_idxs accordingly.  */
static const struct holy_arg_option options[] = {
  /* TRANSLATORS: In this file "mapping" refers to a change holy makes so if
     your language doesn't have an equivalent of "mapping" you can
     use the word like "rerouting".
   */
  {"list", 'l', 0, N_("Show the current mappings."), 0, 0},
  {"reset", 'r', 0, N_("Reset all mappings to the default values."), 0, 0},
  {"swap", 's', 0, N_("Perform both direct and reverse mappings."), 0, 0},
  {0, 0, 0, 0, 0, 0}
};

/* Remember to update options[] accordingly.  */
enum opt_idxs
{
  OPTIDX_LIST = 0,
  OPTIDX_RESET,
  OPTIDX_SWAP,
};

/* Realmode far ptr (2 * 16b) to the previous INT13h handler.  */
extern holy_uint32_t holy_drivemap_oldhandler;

/* The type "void" is used for imported assembly labels, takes no storage and
   serves just to take the address with &label.  */

/* The assembly function to replace the old INT13h handler. It does not follow
   any C callspecs and returns with IRET.  */
extern const void holy_drivemap_handler;

/* Start of the drive mappings area (space reserved at runtime).  */
extern const void holy_drivemap_mapstart;

typedef struct drivemap_node
{
  struct drivemap_node *next;
  holy_uint8_t newdrive;
  holy_uint8_t redirto;
} drivemap_node_t;

typedef struct holy_PACKED int13map_node
{
  holy_uint8_t disknum;
  holy_uint8_t mapto;
} int13map_node_t;

#define INT13H_OFFSET(x) \
	(((holy_uint8_t *)(x)) - ((holy_uint8_t *)&holy_drivemap_handler))

static drivemap_node_t *map_head;
static void *drivemap_hook;
static int drivemap_mmap;

/* Puts the specified mapping into the table, replacing an existing mapping
   for newdrive or adding a new one if required.  */
static holy_err_t
drivemap_set (holy_uint8_t newdrive, holy_uint8_t redirto)
{
  drivemap_node_t *mapping = 0;
  drivemap_node_t *search = map_head;
  while (search)
    {
      if (search->newdrive == newdrive)
	{
	  mapping = search;
	  break;
	}
      search = search->next;
    }

  /* Check for pre-existing mappings to modify before creating a new one.  */
  if (mapping)
    mapping->redirto = redirto;
  else
    {
      mapping = holy_malloc (sizeof (drivemap_node_t));
      if (! mapping)
	return holy_errno;
      mapping->newdrive = newdrive;
      mapping->redirto = redirto;
      mapping->next = map_head;
      map_head = mapping;
    }
  return holy_ERR_NONE;
}

/* Removes the mapping for newdrive from the table.  If there is no mapping,
   then this function behaves like a no-op on the map.  */
static void
drivemap_remove (holy_uint8_t newdrive)
{
  drivemap_node_t *mapping = 0;
  drivemap_node_t *search = map_head;
  drivemap_node_t *previous = 0;

  while (search)
    {
      if (search->newdrive == newdrive)
	{
	  mapping = search;
	  break;
	}
      previous = search;
      search = search->next;
    }

  if (mapping)
    {
      if (previous)
	previous->next = mapping->next;
      else
	map_head = mapping->next;
      holy_free (mapping);
    }
}

/* Given a holy-like device name and a convenient location, stores the
   related BIOS disk number.  Accepts devices like \((f|h)dN\), with
   0 <= N < 128.  */
static holy_err_t
tryparse_diskstring (const char *str, holy_uint8_t *output)
{
  /* Skip opening paren in order to allow both (hd0) and hd0.  */
  if (*str == '(')
    str++;
  if ((str[0] == 'f' || str[0] == 'h') && str[1] == 'd')
    {
      holy_uint8_t bios_num = (str[0] == 'h') ? 0x80 : 0x00;
      unsigned long drivenum = holy_strtoul (str + 2, 0, 0);
      if (holy_errno == holy_ERR_NONE && drivenum < 128)
	{
	  bios_num |= drivenum;
	  if (output)
	    *output = bios_num;
	  return holy_ERR_NONE;
	}
    }
  return holy_error (holy_ERR_BAD_ARGUMENT, "device format \"%s\" "
		     "invalid: must be (f|h)dN, with 0 <= N < 128", str);
}

static holy_err_t
list_mappings (void)
{
  /* Show: list mappings.  */
  if (! map_head)
    {
      holy_puts_ (N_("No drives have been remapped"));
      return holy_ERR_NONE;
    }

  /* TRANSLATORS:  This is the header of mapping list. 
     On the left is how OS will see the disks and
     on the right current holy vision.  */
  holy_puts_ (N_("OS disk #num ------> holy/BIOS device"));
  drivemap_node_t *curnode = map_head;
  while (curnode)
    {
      holy_printf ("%cD #%-3u (0x%02x)       %cd%d\n",
		   (curnode->newdrive & 0x80) ? 'H' : 'F',
		   curnode->newdrive & 0x7F, curnode->newdrive,
		   (curnode->redirto & 0x80) ? 'h' : 'f',
		   curnode->redirto & 0x7F
		   );
      curnode = curnode->next;
    }
  return holy_ERR_NONE;
}

static holy_err_t
holy_cmd_drivemap (struct holy_extcmd_context *ctxt, int argc, char **args)
{
  if (ctxt->state[OPTIDX_LIST].set)
    {
      return list_mappings ();
    }
  else if (ctxt->state[OPTIDX_RESET].set)
    {
      /* Reset: just delete all mappings, freeing their memory.  */
      drivemap_node_t *curnode = map_head;
      drivemap_node_t *prevnode = 0;
      while (curnode)
	{
	  prevnode = curnode;
	  curnode = curnode->next;
	  holy_free (prevnode);
	}
      map_head = 0;
      return holy_ERR_NONE;
    }
  else if (!ctxt->state[OPTIDX_SWAP].set && argc == 0)
    {
      /* No arguments */
      return list_mappings ();
    }

  /* Neither flag: put mapping.  */
  holy_uint8_t mapfrom = 0;
  holy_uint8_t mapto = 0xFF;
  holy_err_t err;

  if (argc != 2)
    return holy_error (holy_ERR_BAD_ARGUMENT, N_("two arguments expected"));

  err = tryparse_diskstring (args[0], &mapfrom);
  if (err != holy_ERR_NONE)
    return err;

  err = tryparse_diskstring (args[1], &mapto);
  if (err != holy_ERR_NONE)
    return err;

  if (mapto == mapfrom)
    {
      /* Reset to default.  */
      holy_dprintf ("drivemap", "Removing mapping for %s (%02x)\n",
		    args[0], mapfrom);
      drivemap_remove (mapfrom);
      return holy_ERR_NONE;
    }
  /* Set the mapping for the disk (overwrites any existing mapping).  */
  holy_dprintf ("drivemap", "%s %s (%02x) = %s (%02x)\n",
		ctxt->state[OPTIDX_SWAP].set ? "Swapping" : "Mapping",
		args[1], mapto, args[0], mapfrom);
  err = drivemap_set (mapto, mapfrom);
  /* If -s, perform the reverse mapping too (only if the first was OK).  */
  if (ctxt->state[OPTIDX_SWAP].set && err == holy_ERR_NONE)
    err = drivemap_set (mapfrom, mapto);
  return err;
}

/* Int13h handler installer - reserves conventional memory for the handler,
   copies it over and sets the IVT entry for int13h.
   This code rests on the assumption that holy does not activate any kind
   of memory mapping apart from identity paging, since it accesses
   realmode structures by their absolute addresses, like the IVT at 0;
   and transforms a pmode pointer into a rmode seg:off far ptr.  */
static holy_err_t
install_int13_handler (int noret __attribute__ ((unused)))
{
  /* Size of the full int13 handler "bundle", including code and map.  */
  holy_uint32_t total_size;
  /* Base address of the space reserved for the handler bundle.  */
  holy_uint8_t *handler_base = 0;
  /* Address of the map within the deployed bundle.  */
  int13map_node_t *handler_map;

  int i;
  int entries = 0;
  drivemap_node_t *curentry = map_head;

  /* Count entries to prepare a contiguous map block.  */
  while (curentry)
    {
      entries++;
      curentry = curentry->next;
    }
  if (entries == 0)
    {
      /* No need to install the int13h handler.  */
      holy_dprintf ("drivemap", "No drives marked as remapped, not installing "
		    "our int13h handler.\n");
      return holy_ERR_NONE;
    }

  holy_dprintf ("drivemap", "Installing our int13h handler\n");

  /* Save the pointer to the old handler.  */
  holy_drivemap_oldhandler = *int13slot;
  holy_dprintf ("drivemap", "Original int13 handler: %04x:%04x\n",
		(holy_drivemap_oldhandler >> 16) & 0x0ffff,
		holy_drivemap_oldhandler & 0x0ffff);

  /* Find a rmode-segment-aligned zone in conventional memory big
     enough to hold the handler and its data.  */
  total_size = INT13H_OFFSET (&holy_drivemap_mapstart)
    + (entries + 1) * sizeof (int13map_node_t);
  holy_dprintf ("drivemap", "Payload is %u bytes long\n", total_size);
  handler_base = holy_mmap_malign_and_register (16, ALIGN_UP (total_size, 16),
						&drivemap_mmap,
						holy_MEMORY_RESERVED,
						holy_MMAP_MALLOC_LOW);
  if (! handler_base)
    return holy_error (holy_ERR_OUT_OF_MEMORY, "couldn't reserve "
		       "memory for the int13h handler");

  /* Copy int13h handler bundle to reserved area.  */
  holy_dprintf ("drivemap", "Reserved memory at %p, copying handler\n",
		handler_base);
  holy_memcpy (handler_base, &holy_drivemap_handler,
	       INT13H_OFFSET (&holy_drivemap_mapstart));

  /* Copy the mappings to the reserved area.  */
  curentry = map_head;
  handler_map = (int13map_node_t *) (handler_base +
				     INT13H_OFFSET (&holy_drivemap_mapstart));
  holy_dprintf ("drivemap", "Target map at %p, copying mappings\n", handler_map);
  for (i = 0; i < entries; ++i, curentry = curentry->next)
    {
      handler_map[i].disknum = curentry->newdrive;
      handler_map[i].mapto = curentry->redirto;
      holy_dprintf ("drivemap", "\t#%d: 0x%02x <- 0x%02x\n", i,
		    handler_map[i].disknum, handler_map[i].mapto);
    }
  /* Signal end-of-map.  */
  handler_map[i].disknum = 0;
  handler_map[i].mapto = 0;
  holy_dprintf ("drivemap", "\t#%d: 0x00 <- 0x00 (end)\n", i);

  /* Install our function as the int13h handler in the IVT.  */
  *int13slot = ((holy_uint32_t) handler_base) << 12;	/* Segment address.  */
  holy_dprintf ("drivemap", "New int13 handler: %04x:%04x\n",
		(*int13slot >> 16) & 0x0ffff, *int13slot & 0x0ffff);

  return holy_ERR_NONE;
}

static holy_err_t
uninstall_int13_handler (void)
{
  if (! holy_drivemap_oldhandler)
    return holy_ERR_NONE;

  *int13slot = holy_drivemap_oldhandler;
  holy_mmap_free_and_unregister (drivemap_mmap);
  holy_drivemap_oldhandler = 0;
  holy_dprintf ("drivemap", "Restored int13 handler: %04x:%04x\n",
		(*int13slot >> 16) & 0x0ffff, *int13slot & 0x0ffff);

  return holy_ERR_NONE;
}

static int
holy_get_root_biosnumber_drivemap (void)
{
  const char *biosnum;
  int ret = -1;
  holy_device_t dev;

  biosnum = holy_env_get ("biosnum");

  if (biosnum)
    return holy_strtoul (biosnum, 0, 0);

  dev = holy_device_open (0);
  if (dev && dev->disk && dev->disk->dev
      && dev->disk->dev->id == holy_DISK_DEVICE_BIOSDISK_ID)
    {
      drivemap_node_t *curnode = map_head;
      ret = (int) dev->disk->id;
      while (curnode)
	{
	  if (curnode->redirto == ret)
	    {
	      ret = curnode->newdrive;
	      break;
	    }
	  curnode = curnode->next;
	}

    }

  if (dev)
    holy_device_close (dev);

  return ret;
}

static holy_extcmd_t cmd;
static int (*holy_get_root_biosnumber_saved) (void);

holy_MOD_INIT (drivemap)
{
  holy_get_root_biosnumber_saved = holy_get_root_biosnumber;
  holy_get_root_biosnumber = holy_get_root_biosnumber_drivemap;
  cmd = holy_register_extcmd ("drivemap", holy_cmd_drivemap, 0,
			      N_("-l | -r | [-s] holydev osdisk."),
			      N_("Manage the BIOS drive mappings."),
			      options);
  drivemap_hook =
    holy_loader_register_preboot_hook (&install_int13_handler,
				       &uninstall_int13_handler,
				       holy_LOADER_PREBOOT_HOOK_PRIO_NORMAL);
}

holy_MOD_FINI (drivemap)
{
  holy_get_root_biosnumber = holy_get_root_biosnumber_saved;
  holy_loader_unregister_preboot_hook (drivemap_hook);
  drivemap_hook = 0;
  holy_unregister_extcmd (cmd);
}
