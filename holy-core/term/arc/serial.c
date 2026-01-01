/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/serial.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/i18n.h>
#include <holy/arc/arc.h>


static void
do_real_config (struct holy_serial_port *port)
{
  char *name;
  if (port->configured)
    return;

  name = holy_arc_alt_name_to_norm (port->name, "");

  if (holy_ARC_FIRMWARE_VECTOR->open (name,holy_ARC_FILE_ACCESS_OPEN_RW,
				      &port->handle))
    port->handle_valid = 0;
  else
    port->handle_valid = 1;

  port->configured = 1;
}

/* Fetch a key.  */
static int
serial_hw_fetch (struct holy_serial_port *port)
{
  unsigned long actual;
  char c;

  do_real_config (port);

  if (!port->handle_valid)
    return -1;
  if (holy_ARC_FIRMWARE_VECTOR->read (port->handle, &c,
				      1, &actual) || actual <= 0)
    return -1;
  return c;
}

/* Put a character.  */
static void
serial_hw_put (struct holy_serial_port *port, const int c)
{
  unsigned long actual;
  char c0 = c;

  do_real_config (port);

  if (!port->handle_valid)
    return;

  holy_ARC_FIRMWARE_VECTOR->write (port->handle, &c0,
				   1, &actual);
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
static holy_err_t
serial_hw_configure (struct holy_serial_port *port __attribute__ ((unused)),
		     struct holy_serial_config *config __attribute__ ((unused)))
{
  /*  FIXME: no ARC serial config available.  */

  return holy_ERR_NONE;
}

struct holy_serial_driver holy_arcserial_driver =
  {
    .configure = serial_hw_configure,
    .fetch = serial_hw_fetch,
    .put = serial_hw_put
  };

const char *
holy_arcserial_add_port (const char *path)
{
  struct holy_serial_port *port;
  holy_err_t err;

  port = holy_zalloc (sizeof (*port));
  if (!port)
    return NULL;
  port->name = holy_strdup (path);
  if (!port->name)
    return NULL;

  port->driver = &holy_arcserial_driver;
  err = holy_serial_config_defaults (port);
  if (err)
    holy_print_error ();

  holy_serial_register (port);

  return port->name;
}

static int
dev_iterate (const char *name,
	     const struct holy_arc_component *comp __attribute__ ((unused)),
	     void *data __attribute__ ((unused)))
{
  /* We should check consolein/consoleout flags as
     well but some implementations are buggy.  */
  if ((comp->flags & (holy_ARC_COMPONENT_FLAG_IN | holy_ARC_COMPONENT_FLAG_OUT))
      != (holy_ARC_COMPONENT_FLAG_IN | holy_ARC_COMPONENT_FLAG_OUT))
    return 0;
  if (!holy_arc_is_device_serial (name, 1))
    return 0;
  holy_arcserial_add_port (name);
  return 0;
}

void
holy_arcserial_init (void)
{
  holy_arc_iterate_devs (dev_iterate, 0, 1);
}

