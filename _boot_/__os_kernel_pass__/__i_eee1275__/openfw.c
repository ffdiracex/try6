/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/err.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/ieee1275/ieee1275.h>
#include <holy/net.h>

enum holy_ieee1275_parse_type
{
  holy_PARSE_FILENAME,
  holy_PARSE_PARTITION,
  holy_PARSE_DEVICE,
  holy_PARSE_DEVICE_TYPE
};

static int
fill_alias (struct holy_ieee1275_devalias *alias)
{
  holy_ssize_t actual;

  if (holy_ieee1275_get_property (alias->phandle, "device_type", alias->type,
				  IEEE1275_MAX_PROP_LEN, &actual))
    alias->type[0] = 0;

  if (alias->parent_dev == alias->phandle)
    return 0;

  if (holy_ieee1275_package_to_path (alias->phandle, alias->path,
				     IEEE1275_MAX_PATH_LEN, &actual))
    return 0;

  if (holy_strcmp (alias->parent_path, alias->path) == 0)
    return 0;

  if (holy_ieee1275_get_property (alias->phandle, "name", alias->name,
				  IEEE1275_MAX_PROP_LEN, &actual))
    return 0;
  holy_dprintf ("devalias", "device path=%s\n", alias->path);
  return 1;
}

void
holy_ieee1275_devalias_free (struct holy_ieee1275_devalias *alias)
{
  holy_free (alias->name);
  holy_free (alias->type);
  holy_free (alias->path);
  holy_free (alias->parent_path);
  alias->name = 0;
  alias->type = 0;
  alias->path = 0;
  alias->parent_path = 0;
  alias->phandle = holy_IEEE1275_PHANDLE_INVALID;
}

void
holy_ieee1275_children_peer (struct holy_ieee1275_devalias *alias)
{
  while (holy_ieee1275_peer (alias->phandle, &alias->phandle) != -1)
    if (fill_alias (alias))
      return;
  holy_ieee1275_devalias_free (alias);
}

void
holy_ieee1275_children_first (const char *devpath,
			      struct holy_ieee1275_devalias *alias)
{
  holy_ieee1275_phandle_t dev;

  holy_dprintf ("devalias", "iterating children of %s\n",
		devpath);

  alias->name = 0;
  alias->path = 0;
  alias->parent_path = 0;
  alias->type = 0;

  if (holy_ieee1275_finddevice (devpath, &dev))
    return;

  if (holy_ieee1275_child (dev, &alias->phandle))
    return;

  alias->type = holy_malloc (IEEE1275_MAX_PROP_LEN);
  if (!alias->type)
    return;
  alias->path = holy_malloc (IEEE1275_MAX_PATH_LEN);
  if (!alias->path)
    {
      holy_free (alias->type);
      return;
    }
  alias->parent_path = holy_strdup (devpath);
  if (!alias->parent_path)
    {
      holy_free (alias->path);
      holy_free (alias->type);
      return;
    }

  alias->name = holy_malloc (IEEE1275_MAX_PROP_LEN);
  if (!alias->name)
    {
      holy_free (alias->path);
      holy_free (alias->type);
      holy_free (alias->parent_path);
      return;
    }
  if (!fill_alias (alias))
    holy_ieee1275_children_peer (alias);
}

static int
iterate_recursively (const char *path,
		     int (*hook) (struct holy_ieee1275_devalias *alias))
{
  struct holy_ieee1275_devalias alias;
  int ret = 0;

  FOR_IEEE1275_DEVCHILDREN(path, alias)
    {
      ret = hook (&alias);
      if (ret)
	break;
      ret = iterate_recursively (alias.path, hook);
      if (ret)
	break;
    }
  holy_ieee1275_devalias_free (&alias);
  return ret;
}

int
holy_ieee1275_devices_iterate (int (*hook) (struct holy_ieee1275_devalias *alias))
{
  return iterate_recursively ("/", hook);
}

void
holy_ieee1275_devalias_init_iterator (struct holy_ieee1275_devalias *alias)
{
  alias->name = 0;
  alias->path = 0;
  alias->parent_path = 0;
  alias->type = 0;

  holy_dprintf ("devalias", "iterating aliases\n");

  if (holy_ieee1275_finddevice ("/aliases", &alias->parent_dev))
    return;

  alias->name = holy_malloc (IEEE1275_MAX_PROP_LEN);
  if (!alias->name)
    return;

  alias->type = holy_malloc (IEEE1275_MAX_PROP_LEN);
  if (!alias->type)
    {
      holy_free (alias->name);
      alias->name = 0;
      return;
    }

  alias->name[0] = '\0';
}

int
holy_ieee1275_devalias_next (struct holy_ieee1275_devalias *alias)
{
  if (!alias->name)
    return 0;
  while (1)
    {
      holy_ssize_t pathlen;
      holy_ssize_t actual;
      char *tmp;

      if (alias->path)
	{
	  holy_free (alias->path);
	  alias->path = 0;
	}
      tmp = holy_strdup (alias->name);
      if (holy_ieee1275_next_property (alias->parent_dev, tmp,
				       alias->name) <= 0)
	{
	  holy_free (tmp);
	  holy_ieee1275_devalias_free (alias);
	  return 0;
	}
      holy_free (tmp);

      holy_dprintf ("devalias", "devalias name = %s\n", alias->name);

      holy_ieee1275_get_property_length (alias->parent_dev, alias->name, &pathlen);

      /* The property `name' is a special case we should skip.  */
      if (holy_strcmp (alias->name, "name") == 0)
	continue;

      /* Sun's OpenBoot often doesn't zero terminate the device alias
	 strings, so we will add a NULL byte at the end explicitly.  */
      pathlen += 1;

      alias->path = holy_malloc (pathlen + 1);
      if (! alias->path)
	{
	  holy_ieee1275_devalias_free (alias);
	  return 0;
	}

      if (holy_ieee1275_get_property (alias->parent_dev, alias->name, alias->path,
				      pathlen, &actual) || actual < 0)
	{
	  holy_dprintf ("devalias", "get_property (%s) failed\n", alias->name);
	  holy_free (alias->path);
	  continue;
	}
      if (actual > pathlen)
	actual = pathlen;
      alias->path[actual] = '\0';
      alias->path[pathlen] = '\0';

      if (holy_ieee1275_finddevice (alias->path, &alias->phandle))
	{
	  holy_dprintf ("devalias", "finddevice (%s) failed\n", alias->path);
	  holy_free (alias->path);
	  alias->path = 0;
	  continue;
	}

      if (holy_ieee1275_get_property (alias->phandle, "device_type", alias->type,
				      IEEE1275_MAX_PROP_LEN, &actual))
	{
	  /* NAND device don't have device_type property.  */
          alias->type[0] = 0;
	}
      return 1;
    }
}

/* Call the "map" method of /chosen/mmu.  */
int
holy_ieee1275_map (holy_addr_t phys, holy_addr_t virt, holy_size_t size,
		   holy_uint32_t mode)
{
  struct map_args {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t mode;
    holy_ieee1275_cell_t size;
    holy_ieee1275_cell_t virt;
#ifdef __sparc__
    holy_ieee1275_cell_t phys_high;
#endif
    holy_ieee1275_cell_t phys_low;
    holy_ieee1275_cell_t catch_result;
  } args;

  INIT_IEEE1275_COMMON (&args.common, "call-method",
#ifdef __sparc__
			7,
#else
			6,
#endif
			1);
  args.method = (holy_ieee1275_cell_t) "map";
  args.ihandle = holy_ieee1275_mmu;
#ifdef __sparc__
  args.phys_high = 0;
#endif
  args.phys_low = phys;
  args.virt = virt;
  args.size = size;
  args.mode = mode; /* Format is WIMG0PP.  */
  args.catch_result = (holy_ieee1275_cell_t) -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return args.catch_result;
}

holy_err_t
holy_claimmap (holy_addr_t addr, holy_size_t size)
{
  if (holy_ieee1275_claim (addr, size, 0, 0))
    return -1;

  if (! holy_ieee1275_test_flag (holy_IEEE1275_FLAG_REAL_MODE)
      && holy_ieee1275_map (addr, addr, size, 0x00))
    {
      holy_error (holy_ERR_OUT_OF_MEMORY, "map failed: address 0x%llx, size 0x%llx\n",
		  (long long) addr, (long long) size);
      holy_ieee1275_release (addr, size);
      return holy_errno;
    }

  return holy_ERR_NONE;
}

/* Get the device arguments of the Open Firmware node name `path'.  */
static char *
holy_ieee1275_get_devargs (const char *path)
{
  char *colon = holy_strchr (path, ':');

  if (! colon)
    return 0;

  return holy_strdup (colon + 1);
}

/* Get the device path of the Open Firmware node name `path'.  */
char *
holy_ieee1275_get_devname (const char *path)
{
  char *colon = holy_strchr (path, ':');
  int pathlen = holy_strlen (path);
  struct holy_ieee1275_devalias curalias;
  if (colon)
    pathlen = (int)(colon - path);

  /* Try to find an alias for this device.  */
  FOR_IEEE1275_DEVALIASES (curalias)
    /* briQ firmware can change capitalization in /chosen/bootpath.  */
    if (holy_strncasecmp (curalias.path, path, pathlen) == 0
	&& curalias.path[pathlen] == 0)
      {
	char *newpath;
	newpath = holy_strdup (curalias.name);
	holy_ieee1275_devalias_free (&curalias);
	return newpath;
      }

  return holy_strndup (path, pathlen);
}

static char *
holy_ieee1275_parse_args (const char *path, enum holy_ieee1275_parse_type ptype)
{
  char type[64]; /* XXX check size.  */
  char *device = holy_ieee1275_get_devname (path);
  char *ret = 0;
  holy_ieee1275_phandle_t dev;

  /* We need to know what type of device it is in order to parse the full
     file path properly.  */
  if (holy_ieee1275_finddevice (device, &dev))
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE, "device %s not found", device);
      goto fail;
    }
  if (holy_ieee1275_get_property (dev, "device_type", &type, sizeof type, 0))
    {
      holy_error (holy_ERR_UNKNOWN_DEVICE,
		  "device %s lacks a device_type property", device);
      goto fail;
    }

  switch (ptype)
    {
    case holy_PARSE_DEVICE:
      ret = holy_strdup (device);
      break;
    case holy_PARSE_DEVICE_TYPE:
      ret = holy_strdup (type);
      break;
    case holy_PARSE_FILENAME:
      {
	char *comma;
	char *args;

	args = holy_ieee1275_get_devargs (path);
	if (!args)
	  /* Shouldn't happen.  */
	  return 0;

	/* The syntax of the device arguments is defined in the CHRP and PReP
	   IEEE1275 bindings: "[partition][,[filename]]".  */
	comma = holy_strchr (args, ',');

	if (comma)
	  {
	    char *filepath = comma + 1;
	    
	    /* Make sure filepath has leading backslash.  */
	    if (filepath[0] != '\\')
	      ret = holy_xasprintf ("\\%s", filepath);
	    else
	      ret = holy_strdup (filepath);
	    }
	holy_free (args);
	}
      break;
    case holy_PARSE_PARTITION:
      {
	char *comma;
	char *args;

	if (holy_strcmp ("block", type) != 0)
	  goto unknown;

	args = holy_ieee1275_get_devargs (path);
	if (!args)
	  /* Shouldn't happen.  */
	  return 0;

	comma = holy_strchr (args, ',');
	if (!comma)
	  ret = holy_strdup (args);
	else
	  ret = holy_strndup (args, (holy_size_t)(comma - args));
	/* Consistently provide numbered partitions to holy.
	   OpenBOOT traditionally uses alphabetical partition
	   specifiers.  */
	if (ret[0] >= 'a' && ret[0] <= 'z')
	    ret[0] = '1' + (ret[0] - 'a');
	holy_free (args);
      }
      break;
    default:
    unknown:
      holy_error (holy_ERR_NOT_IMPLEMENTED_YET,
		  "unsupported type %s for device %s", type, device);
    }

fail:
  holy_free (device);
  return ret;
}

char *
holy_ieee1275_get_device_type (const char *path)
{
  return holy_ieee1275_parse_args (path, holy_PARSE_DEVICE_TYPE);
}

char *
holy_ieee1275_get_aliasdevname (const char *path)
{
  return holy_ieee1275_parse_args (path, holy_PARSE_DEVICE);
}

char *
holy_ieee1275_get_filename (const char *path)
{
  return holy_ieee1275_parse_args (path, holy_PARSE_FILENAME);
}

/* Convert a device name from IEEE1275 syntax to holy syntax.  */
char *
holy_ieee1275_encode_devname (const char *path)
{
  char *device = holy_ieee1275_get_devname (path);
  char *partition;
  char *encoding;
  char *optr;
  const char *iptr;

  encoding = holy_malloc (sizeof ("ieee1275/") + 2 * holy_strlen (device)
			  + sizeof (",XXXXXXXXXXXX"));
  if (!encoding)
    {
      holy_free (device);
      return 0;
    }

  partition = holy_ieee1275_parse_args (path, holy_PARSE_PARTITION);

  optr = holy_stpcpy (encoding, "ieee1275/");
  for (iptr = device; *iptr; )
    {
      if (*iptr == ',')
	*optr++ ='\\';
      *optr++ = *iptr++;
    }
  if (partition && partition[0])
    {
      unsigned int partno = holy_strtoul (partition, 0, 0);

      *optr++ = ',';

      if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_0_BASED_PARTITIONS))
	/* holy partition 1 is OF partition 0.  */
	partno++;

      holy_snprintf (optr, sizeof ("XXXXXXXXXXXX"), "%d", partno);
    }
  else
    *optr = '\0';

  holy_free (partition);
  holy_free (device);

  return encoding;
}

/* Resolve aliases.  */
char *
holy_ieee1275_canonicalise_devname (const char *path)
{
  struct canon_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t path;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t inlen;
    holy_ieee1275_cell_t outlen;
  }
  args;
  char *buf = NULL;
  holy_size_t bufsize = 64;
  int i;

  for (i = 0; i < 2; i++)
    {
      holy_free (buf);

      buf = holy_malloc (bufsize);
      if (!buf)
	return NULL;

      INIT_IEEE1275_COMMON (&args.common, "canon", 3, 1);
      args.path = (holy_ieee1275_cell_t) path;
      args.buf = (holy_ieee1275_cell_t) buf;
      args.inlen = (holy_ieee1275_cell_t) (bufsize - 1);

      if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
	return 0;
      if (args.outlen > bufsize - 1)
	{
	  bufsize = args.outlen + 2;
	  continue;
	}
      return buf;
    }
  /* Shouldn't reach here.  */
  holy_free (buf);
  return NULL;
}

