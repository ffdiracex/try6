/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/ieee1275/ieee1275.h>
#include <holy/types.h>

#define IEEE1275_PHANDLE_INVALID  ((holy_ieee1275_cell_t) -1)
#define IEEE1275_IHANDLE_INVALID  ((holy_ieee1275_cell_t) 0)
#define IEEE1275_CELL_INVALID     ((holy_ieee1275_cell_t) -1)



int
holy_ieee1275_finddevice (const char *name, holy_ieee1275_phandle_t *phandlep)
{
  struct find_device_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t device;
    holy_ieee1275_cell_t phandle;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "finddevice", 1, 1);
  args.device = (holy_ieee1275_cell_t) name;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *phandlep = args.phandle;
  if (args.phandle == IEEE1275_PHANDLE_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_get_property (holy_ieee1275_phandle_t phandle,
			    const char *property, void *buf,
			    holy_size_t size, holy_ssize_t *actual)
{
  struct get_property_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t phandle;
    holy_ieee1275_cell_t prop;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t buflen;
    holy_ieee1275_cell_t size;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "getprop", 4, 1);
  args.phandle = phandle;
  args.prop = (holy_ieee1275_cell_t) property;
  args.buf = (holy_ieee1275_cell_t) buf;
  args.buflen = (holy_ieee1275_cell_t) size;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = (holy_ssize_t) args.size;
  if (args.size == IEEE1275_CELL_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_get_integer_property (holy_ieee1275_phandle_t phandle,
				    const char *property, holy_uint32_t *buf,
				    holy_size_t size, holy_ssize_t *actual)
{
  int ret;
  ret = holy_ieee1275_get_property (phandle, property, (void *) buf, size, actual);
#ifndef holy_CPU_WORDS_BIGENDIAN
  /* Integer properties are always in big endian.  */
  if (ret == 0)
    {
      unsigned int i;
      size /= sizeof (holy_uint32_t);
      for (i = 0; i < size; i++)
	buf[i] = holy_be_to_cpu32 (buf[i]);
    }
#endif
  return ret;
}

int
holy_ieee1275_next_property (holy_ieee1275_phandle_t phandle, char *prev_prop,
			     char *prop)
{
  struct get_property_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t phandle;
    holy_ieee1275_cell_t prev_prop;
    holy_ieee1275_cell_t next_prop;
    holy_ieee1275_cell_t flags;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "nextprop", 3, 1);
  args.phandle = phandle;
  args.prev_prop = (holy_ieee1275_cell_t) prev_prop;
  args.next_prop = (holy_ieee1275_cell_t) prop;
  args.flags = (holy_ieee1275_cell_t) -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return (int) args.flags;
}

int
holy_ieee1275_get_property_length (holy_ieee1275_phandle_t phandle,
				   const char *prop, holy_ssize_t *length)
{
  struct get_property_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t phandle;
    holy_ieee1275_cell_t prop;
    holy_ieee1275_cell_t length;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "getproplen", 2, 1);
  args.phandle = phandle;
  args.prop = (holy_ieee1275_cell_t) prop;
  args.length = (holy_ieee1275_cell_t) -1;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *length = args.length;
  if (args.length == IEEE1275_CELL_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_instance_to_package (holy_ieee1275_ihandle_t ihandle,
				   holy_ieee1275_phandle_t *phandlep)
{
  struct instance_to_package_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t phandle;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "instance-to-package", 1, 1);
  args.ihandle = ihandle;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *phandlep = args.phandle;
  if (args.phandle == IEEE1275_PHANDLE_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_package_to_path (holy_ieee1275_phandle_t phandle,
			       char *path, holy_size_t len,
			       holy_ssize_t *actual)
{
  struct instance_to_package_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t phandle;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t buflen;
    holy_ieee1275_cell_t actual;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "package-to-path", 3, 1);
  args.phandle = phandle;
  args.buf = (holy_ieee1275_cell_t) path;
  args.buflen = (holy_ieee1275_cell_t) len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = args.actual;
  if (args.actual == IEEE1275_CELL_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_instance_to_path (holy_ieee1275_ihandle_t ihandle,
				char *path, holy_size_t len,
				holy_ssize_t *actual)
{
  struct instance_to_path_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t buflen;
    holy_ieee1275_cell_t actual;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "instance-to-path", 3, 1);
  args.ihandle = ihandle;
  args.buf = (holy_ieee1275_cell_t) path;
  args.buflen = (holy_ieee1275_cell_t) len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actual)
    *actual = args.actual;
  if (args.actual == IEEE1275_CELL_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_write (holy_ieee1275_ihandle_t ihandle, const void *buffer,
		     holy_size_t len, holy_ssize_t *actualp)
{
  struct write_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t len;
    holy_ieee1275_cell_t actual;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "write", 3, 1);
  args.ihandle = ihandle;
  args.buf = (holy_ieee1275_cell_t) buffer;
  args.len = (holy_ieee1275_cell_t) len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actualp)
    *actualp = args.actual;
  return 0;
}

int
holy_ieee1275_read (holy_ieee1275_ihandle_t ihandle, void *buffer,
		    holy_size_t len, holy_ssize_t *actualp)
{
  struct write_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t len;
    holy_ieee1275_cell_t actual;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "read", 3, 1);
  args.ihandle = ihandle;
  args.buf = (holy_ieee1275_cell_t) buffer;
  args.len = (holy_ieee1275_cell_t) len;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (actualp)
    *actualp = args.actual;
  return 0;
}

int
holy_ieee1275_seek (holy_ieee1275_ihandle_t ihandle, holy_disk_addr_t pos,
		    holy_ssize_t *result)
{
  struct write_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t pos_hi;
    holy_ieee1275_cell_t pos_lo;
    holy_ieee1275_cell_t result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "seek", 3, 1);
  args.ihandle = ihandle;
  /* To prevent stupid gcc warning.  */
#if holy_IEEE1275_CELL_SIZEOF >= 8
  args.pos_hi = 0;
  args.pos_lo = pos;
#else
  args.pos_hi = (holy_ieee1275_cell_t) (pos >> (8 * holy_IEEE1275_CELL_SIZEOF));
  args.pos_lo = (holy_ieee1275_cell_t)
    (pos & ((1ULL << (8 * holy_IEEE1275_CELL_SIZEOF)) - 1));
#endif

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  if (result)
    *result = args.result;
  return 0;
}

int
holy_ieee1275_peer (holy_ieee1275_phandle_t node,
		    holy_ieee1275_phandle_t *result)
{
  struct peer_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t node;
    holy_ieee1275_cell_t result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "peer", 1, 1);
  args.node = node;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  if (args.result == 0)
    return -1;
  return 0;
}

int
holy_ieee1275_child (holy_ieee1275_phandle_t node,
		     holy_ieee1275_phandle_t *result)
{
  struct child_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t node;
    holy_ieee1275_cell_t result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "child", 1, 1);
  args.node = node;
  args.result = IEEE1275_PHANDLE_INVALID;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  if (args.result == 0)
    return -1;
  return 0;
}

int
holy_ieee1275_parent (holy_ieee1275_phandle_t node,
		      holy_ieee1275_phandle_t *result)
{
  struct parent_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t node;
    holy_ieee1275_cell_t result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "parent", 1, 1);
  args.node = node;
  args.result = IEEE1275_PHANDLE_INVALID;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  return 0;
}

int
holy_ieee1275_interpret (const char *command, holy_ieee1275_cell_t *catch)
{
  struct enter_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t command;
    holy_ieee1275_cell_t catch;
  }
  args;

  if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_CANNOT_INTERPRET))
    return -1;

  INIT_IEEE1275_COMMON (&args.common, "interpret", 1, 1);
  args.command = (holy_ieee1275_cell_t) command;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (catch)
    *catch = args.catch;
  return 0;
}

int
holy_ieee1275_enter (void)
{
  struct enter_args
  {
    struct holy_ieee1275_common_hdr common;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "enter", 0, 0);

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return 0;
}

void
holy_ieee1275_exit (void)
{
  struct exit_args
  {
    struct holy_ieee1275_common_hdr common;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "exit", 0, 0);

  IEEE1275_CALL_ENTRY_FN (&args);
  for (;;) ;
}

int
holy_ieee1275_open (const char *path, holy_ieee1275_ihandle_t *result)
{
  struct open_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t path;
    holy_ieee1275_cell_t result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "open", 1, 1);
  args.path = (holy_ieee1275_cell_t) path;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *result = args.result;
  if (args.result == IEEE1275_IHANDLE_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_close (holy_ieee1275_ihandle_t ihandle)
{
  struct close_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t ihandle;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "close", 1, 0);
  args.ihandle = ihandle;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;

  return 0;
}

int
holy_ieee1275_claim (holy_addr_t addr, holy_size_t size, unsigned int align,
		     holy_addr_t *result)
{
  struct claim_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t addr;
    holy_ieee1275_cell_t size;
    holy_ieee1275_cell_t align;
    holy_ieee1275_cell_t base;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "claim", 3, 1);
  args.addr = (holy_ieee1275_cell_t) addr;
  args.size = (holy_ieee1275_cell_t) size;
  args.align = (holy_ieee1275_cell_t) align;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  if (result)
    *result = args.base;
  if (args.base == IEEE1275_CELL_INVALID)
    return -1;
  return 0;
}

int
holy_ieee1275_release (holy_addr_t addr, holy_size_t size)
{
 struct release_args
 {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t addr;
    holy_ieee1275_cell_t size;
 }
 args;

  INIT_IEEE1275_COMMON (&args.common, "release", 2, 0);
  args.addr = addr;
  args.size = size;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return 0;
}

int
holy_ieee1275_set_property (holy_ieee1275_phandle_t phandle,
			    const char *propname, const void *buf,
			    holy_size_t size, holy_ssize_t *actual)
{
  struct set_property_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t phandle;
    holy_ieee1275_cell_t propname;
    holy_ieee1275_cell_t buf;
    holy_ieee1275_cell_t size;
    holy_ieee1275_cell_t actual;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "setprop", 4, 1);
  args.size = (holy_ieee1275_cell_t) size;
  args.buf = (holy_ieee1275_cell_t) buf;
  args.propname = (holy_ieee1275_cell_t) propname;
  args.phandle = (holy_ieee1275_cell_t) phandle;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *actual = args.actual;
  if ((args.actual == IEEE1275_CELL_INVALID) || (args.actual != args.size))
    return -1;
  return 0;
}

int
holy_ieee1275_set_color (holy_ieee1275_ihandle_t ihandle,
			 int index, int r, int g, int b)
{
  struct set_color_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t method;
    holy_ieee1275_cell_t ihandle;
    holy_ieee1275_cell_t index;
    holy_ieee1275_cell_t b;
    holy_ieee1275_cell_t g;
    holy_ieee1275_cell_t r;
    holy_ieee1275_cell_t catch_result;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "call-method", 6, 1);
  args.method = (holy_ieee1275_cell_t) "color!";
  args.ihandle = ihandle;
  args.index = index;
  args.r = r;
  args.g = g;
  args.b = b;

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  return args.catch_result;
}

int
holy_ieee1275_milliseconds (holy_uint32_t *msecs)
{
  struct milliseconds_args
  {
    struct holy_ieee1275_common_hdr common;
    holy_ieee1275_cell_t msecs;
  }
  args;

  INIT_IEEE1275_COMMON (&args.common, "milliseconds", 0, 1);

  if (IEEE1275_CALL_ENTRY_FN (&args) == -1)
    return -1;
  *msecs = args.msecs;
  return 0;
}
