/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/disk.h>
#include <holy/partition.h>
#include <holy/mm.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/err.h>
#include <holy/term.h>
#include <holy/efi/api.h>
#include <holy/efi/efi.h>
#include <holy/efi/disk.h>
#include <holy/serial.h>
#include <holy/types.h>
#include <holy/i18n.h>

/* GUID.  */
static holy_efi_guid_t serial_io_guid = holy_EFI_SERIAL_IO_GUID;

static void
do_real_config (struct holy_serial_port *port)
{
  holy_efi_status_t status = holy_EFI_SUCCESS;
  const holy_efi_parity_type_t parities[] = {
    [holy_SERIAL_PARITY_NONE] = holy_EFI_SERIAL_NO_PARITY,
    [holy_SERIAL_PARITY_ODD] = holy_EFI_SERIAL_ODD_PARITY,
    [holy_SERIAL_PARITY_EVEN] = holy_EFI_SERIAL_EVEN_PARITY
  };
  const holy_efi_stop_bits_t stop_bits[] = {
    [holy_SERIAL_STOP_BITS_1] = holy_EFI_SERIAL_1_STOP_BIT,
    [holy_SERIAL_STOP_BITS_1_5] = holy_EFI_SERIAL_1_5_STOP_BITS,
    [holy_SERIAL_STOP_BITS_2] = holy_EFI_SERIAL_2_STOP_BITS,
  };

  if (port->configured)
    return;

  status = efi_call_7 (port->interface->set_attributes, port->interface,
		       port->config.speed,
		       0, 0, parities[port->config.parity],
		       port->config.word_len,
		       stop_bits[port->config.stop_bits]);
  if (status != holy_EFI_SUCCESS)
    port->broken = 1;

  status = efi_call_2 (port->interface->set_control_bits, port->interface,
		       port->config.rtscts ? 0x4002 : 0x2);

  port->configured = 1;
}

/* Fetch a key.  */
static int
serial_hw_fetch (struct holy_serial_port *port)
{
  holy_efi_uintn_t bufsize = 1;
  char c;
  holy_efi_status_t status = holy_EFI_SUCCESS;
  do_real_config (port);
  if (port->broken)
    return -1;

  status = efi_call_3 (port->interface->read, port->interface, &bufsize, &c);
  if (status != holy_EFI_SUCCESS || bufsize == 0)
    return -1;

  return c;
}

/* Put a character.  */
static void
serial_hw_put (struct holy_serial_port *port, const int c)
{
  holy_efi_uintn_t bufsize = 1;
  char c0 = c;

  do_real_config (port);

  if (port->broken)
    return;

  efi_call_3 (port->interface->write, port->interface, &bufsize, &c0);
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
static holy_err_t
serial_hw_configure (struct holy_serial_port *port,
		     struct holy_serial_config *config)
{
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

struct holy_serial_driver holy_efiserial_driver =
  {
    .configure = serial_hw_configure,
    .fetch = serial_hw_fetch,
    .put = serial_hw_put
  };

void
holy_efiserial_init (void)
{
  holy_efi_uintn_t num_handles;
  holy_efi_handle_t *handles;
  holy_efi_handle_t *handle;
  int num_serial = 0;
  holy_err_t err;

  /* Find handles which support the disk io interface.  */
  handles = holy_efi_locate_handle (holy_EFI_BY_PROTOCOL, &serial_io_guid,
				    0, &num_handles);
  if (! handles)
    return;

  /* Make a linked list of devices.  */
  for (handle = handles; num_handles--; handle++)
    {
      struct holy_serial_port *port;
      struct holy_efi_serial_io_interface *sio;

      sio = holy_efi_open_protocol (*handle, &serial_io_guid,
				    holy_EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if (! sio)
	/* This should not happen... Why?  */
	continue;

      port = holy_zalloc (sizeof (*port));
      if (!port)
	return;

      port->name = holy_malloc (sizeof ("efiXXXXXXXXXXXXXXXXXXXX"));
      if (!port->name)
	{
	  holy_free (port);
	  return;
	}
      holy_snprintf (port->name, sizeof ("efiXXXXXXXXXXXXXXXXXXXX"),
		     "efi%d", num_serial++);

      port->driver = &holy_efiserial_driver;
      port->interface = sio;
      err = holy_serial_config_defaults (port);
      if (err)
	holy_print_error ();

      holy_serial_register (port);
    }

  holy_free (handles);

  return;
}
