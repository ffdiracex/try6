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

holy_MOD_LICENSE ("GPLv2+");

struct holy_escc_descriptor
{
  volatile holy_uint8_t *escc_ctrl;
  volatile holy_uint8_t *escc_data;
};

static void
do_real_config (struct holy_serial_port *port)
{
  holy_uint8_t bitsspec;
  holy_uint8_t parity_stop_spec;
  if (port->configured)
    return;

  /* Make sure the port is waiting for address now.  */
  (void) *port->escc_desc->escc_ctrl;
  switch (port->config.speed)
    {
    case 57600:
      *port->escc_desc->escc_ctrl = 13;
      *port->escc_desc->escc_ctrl = 0;
      *port->escc_desc->escc_ctrl = 12;
      *port->escc_desc->escc_ctrl = 0;
      *port->escc_desc->escc_ctrl = 14;
      *port->escc_desc->escc_ctrl = 1;
      *port->escc_desc->escc_ctrl = 11;
      *port->escc_desc->escc_ctrl = 0x50;
      break;
    case 38400:
      *port->escc_desc->escc_ctrl = 13;
      *port->escc_desc->escc_ctrl = 0;
      *port->escc_desc->escc_ctrl = 12;
      *port->escc_desc->escc_ctrl = 1;
      *port->escc_desc->escc_ctrl = 14;
      *port->escc_desc->escc_ctrl = 1;
      *port->escc_desc->escc_ctrl = 11;
      *port->escc_desc->escc_ctrl = 0x50;
      break;
    }

  parity_stop_spec = 0;
  switch (port->config.parity)
    {
    case holy_SERIAL_PARITY_NONE:
      parity_stop_spec |= 0;
      break;
    case holy_SERIAL_PARITY_ODD:
      parity_stop_spec |= 1;
      break;
    case holy_SERIAL_PARITY_EVEN:
      parity_stop_spec |= 3;
      break;
    }

  switch (port->config.stop_bits)
    {
    case holy_SERIAL_STOP_BITS_1:
      parity_stop_spec |= 0x4;
      break;
    case holy_SERIAL_STOP_BITS_1_5:
      parity_stop_spec |= 0x8;
      break;
    case holy_SERIAL_STOP_BITS_2:
      parity_stop_spec |= 0xc;
      break;      
    }

  *port->escc_desc->escc_ctrl = 4;
  *port->escc_desc->escc_ctrl = 0x40 | parity_stop_spec;

  bitsspec = port->config.word_len - 5;
  bitsspec = ((bitsspec >> 1) | (bitsspec << 1)) & 3;

  *port->escc_desc->escc_ctrl = 3;
  *port->escc_desc->escc_ctrl = (bitsspec << 6) | 0x1;

  port->configured = 1;

  return;
}

/* Fetch a key.  */
static int
serial_hw_fetch (struct holy_serial_port *port)
{
  do_real_config (port);

  *port->escc_desc->escc_ctrl = 0;
  if (*port->escc_desc->escc_ctrl & 1)
    return *port->escc_desc->escc_data;
  return -1;
}

/* Put a character.  */
static void
serial_hw_put (struct holy_serial_port *port, const int c)
{
  holy_uint64_t endtime;

  do_real_config (port);

  if (port->broken > 5)
    endtime = holy_get_time_ms ();
  else if (port->broken > 1)
    endtime = holy_get_time_ms () + 50;
  else
    endtime = holy_get_time_ms () + 200;
  /* Wait until the transmitter holding register is empty.  */
  while (1)
    {
      *port->escc_desc->escc_ctrl = 0;
      if (*port->escc_desc->escc_ctrl & 4)
	break;
      if (holy_get_time_ms () > endtime)
	{
	  port->broken++;
	  /* There is something wrong. But what can I do?  */
	  return;
	}
    }

  if (port->broken)
    port->broken--;

  *port->escc_desc->escc_data = c;
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
  if (config->speed != 38400 && config->speed != 57600)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port speed"));

  if (config->parity != holy_SERIAL_PARITY_NONE
      && config->parity != holy_SERIAL_PARITY_ODD
      && config->parity != holy_SERIAL_PARITY_EVEN)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port parity"));

  if (config->stop_bits != holy_SERIAL_STOP_BITS_1
      && config->stop_bits != holy_SERIAL_STOP_BITS_1_5
      && config->stop_bits != holy_SERIAL_STOP_BITS_2)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port stop bits number"));

  if (config->word_len < 5 || config->word_len > 8)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port word length"));

  port->config = *config;
  port->configured = 0;

  /*  FIXME: should check if the serial terminal was found.  */

  return holy_ERR_NONE;
}

struct holy_serial_driver holy_escc_driver =
  {
    .configure = serial_hw_configure,
    .fetch = serial_hw_fetch,
    .put = serial_hw_put
  };

static struct holy_escc_descriptor escc_descs[2];
static char *macio = 0;

static void
add_device (holy_addr_t addr, int channel)
{
  struct holy_serial_port *port;
  holy_err_t err;
  struct holy_serial_config config =
    {
      .speed = 38400,
      .word_len = 8,
      .parity = holy_SERIAL_PARITY_NONE,
      .stop_bits = holy_SERIAL_STOP_BITS_1
    };

  escc_descs[channel].escc_ctrl
    = (volatile holy_uint8_t *) (holy_addr_t) addr;
  escc_descs[channel].escc_data = escc_descs[channel].escc_ctrl + 16;

  port = holy_zalloc (sizeof (*port));
  if (!port)
    {
      holy_errno = 0;
      return;
    }

  port->name = holy_xasprintf ("escc-ch-%c", channel + 'a');
  if (!port->name)
    {
      holy_errno = 0;
      return;
    }

  port->escc_desc = &escc_descs[channel];

  port->driver = &holy_escc_driver;

  err = port->driver->configure (port, &config);
  if (err)
    holy_print_error ();

  holy_serial_register (port);
}

static int
find_macio (struct holy_ieee1275_devalias *alias)
{
  if (holy_strcmp (alias->type, "mac-io") != 0)
    return 0;
  macio = holy_strdup (alias->path);
  return 1;
}

holy_MOD_INIT (escc)
{
  holy_uint32_t macio_addr[4];
  holy_uint32_t escc_addr[2];
  holy_ieee1275_phandle_t dev;
  struct holy_ieee1275_devalias alias;
  char *escc = 0;

  holy_ieee1275_devices_iterate (find_macio);
  if (!macio)
    return;

  FOR_IEEE1275_DEVCHILDREN(macio, alias)
    if (holy_strcmp (alias.type, "escc") == 0)
      {
	escc = holy_strdup (alias.path);
	break;
      }
  holy_ieee1275_devalias_free (&alias);
  if (!escc)
    {
      holy_free (macio);
      return;
    }

  if (holy_ieee1275_finddevice (macio, &dev))
    {
      holy_free (macio);
      holy_free (escc);
      return;
    }
  if (holy_ieee1275_get_integer_property (dev, "assigned-addresses",
					  macio_addr, sizeof (macio_addr), 0))
    {
      holy_free (macio);
      holy_free (escc);
      return;
    }

  if (holy_ieee1275_finddevice (escc, &dev))
    {
      holy_free (macio);
      holy_free (escc);
      return;
    }

  if (holy_ieee1275_get_integer_property (dev, "reg",
					  escc_addr, sizeof (escc_addr), 0))
    {
      holy_free (macio);
      holy_free (escc);
      return;
    }

  add_device (macio_addr[2] + escc_addr[0] + 32, 0);
  add_device (macio_addr[2] + escc_addr[0], 1);

  holy_free (macio);
  holy_free (escc);
}

holy_MOD_FINI (escc)
{
}
