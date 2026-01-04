/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/serial.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/usb.h>
#include <holy/usbserial.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

enum
  {
    holy_FTDI_MODEM_CTRL = 0x01,
    holy_FTDI_FLOW_CTRL = 0x02,
    holy_FTDI_SPEED_CTRL = 0x03,
    holy_FTDI_DATA_CTRL = 0x04
  };

#define holy_FTDI_MODEM_CTRL_DTRRTS 3
#define holy_FTDI_FLOW_CTRL_DTRRTS 3

/* Convert speed to divisor.  */
static holy_uint32_t
get_divisor (unsigned int speed)
{
  unsigned int i;

  /* The structure for speed vs. divisor.  */
  struct divisor
  {
    unsigned int speed;
    holy_uint32_t div;
  };

  /* The table which lists common configurations.  */
  /* Computed with a division formula with 3MHz as base frequency. */
  static struct divisor divisor_tab[] =
    {
      { 2400,   0x04e2 },
      { 4800,   0x0271 },
      { 9600,   0x4138 },
      { 19200,  0x809c },
      { 38400,  0xc04e },
      { 57600,  0xc034 },
      { 115200, 0x001a }
    };

  /* Set the baud rate.  */
  for (i = 0; i < ARRAY_SIZE (divisor_tab); i++)
    if (divisor_tab[i].speed == speed)
      return divisor_tab[i].div;
  return 0;
}

static void
real_config (struct holy_serial_port *port)
{
  holy_uint32_t divisor;
  const holy_uint16_t parities[] = {
    [holy_SERIAL_PARITY_NONE] = 0x0000,
    [holy_SERIAL_PARITY_ODD] = 0x0100,
    [holy_SERIAL_PARITY_EVEN] = 0x0200
  };
  const holy_uint16_t stop_bits[] = {
    [holy_SERIAL_STOP_BITS_1] = 0x0000,
    [holy_SERIAL_STOP_BITS_1_5] = 0x0800,
    [holy_SERIAL_STOP_BITS_2] = 0x1000,
  };

  if (port->configured)
    return;

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			holy_FTDI_MODEM_CTRL,
			port->config.rtscts ? holy_FTDI_MODEM_CTRL_DTRRTS : 0,
			0, 0, 0);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			holy_FTDI_FLOW_CTRL,
			port->config.rtscts ? holy_FTDI_FLOW_CTRL_DTRRTS : 0,
			0, 0, 0);

  divisor = get_divisor (port->config.speed);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			holy_FTDI_SPEED_CTRL,
			divisor & 0xffff, divisor >> 16, 0, 0);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			holy_FTDI_DATA_CTRL,
			parities[port->config.parity]
			| stop_bits[port->config.stop_bits]
			| port->config.word_len, 0, 0, 0);

  port->configured = 1;
}

/* Fetch a key.  */
static int
ftdi_hw_fetch (struct holy_serial_port *port)
{
  real_config (port);

  return holy_usbserial_fetch (port, 2);
}

/* Put a character.  */
static void
ftdi_hw_put (struct holy_serial_port *port, const int c)
{
  char cc = c;

  real_config (port);

  holy_usb_bulk_write (port->usbdev, port->out_endp, 1, &cc);
}

static holy_err_t
ftdi_hw_configure (struct holy_serial_port *port,
			struct holy_serial_config *config)
{
  holy_uint16_t divisor;

  divisor = get_divisor (config->speed);
  if (divisor == 0)
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

  return holy_ERR_NONE;
}

static struct holy_serial_driver holy_ftdi_driver =
  {
    .configure = ftdi_hw_configure,
    .fetch = ftdi_hw_fetch,
    .put = ftdi_hw_put,
    .fini = holy_usbserial_fini
  };

static const struct 
{
  holy_uint16_t vendor, product;
} products[] =
  {
    {0x0403, 0x6001} /* QEMU virtual USBserial.  */
  };

static int
holy_ftdi_attach (holy_usb_device_t usbdev, int configno, int interfno)
{
  unsigned j;

  for (j = 0; j < ARRAY_SIZE (products); j++)
    if (usbdev->descdev.vendorid == products[j].vendor
	&& usbdev->descdev.prodid == products[j].product)
      break;
  if (j == ARRAY_SIZE (products))
    return 0;

  return holy_usbserial_attach (usbdev, configno, interfno,
				&holy_ftdi_driver,
				holy_USB_SERIAL_ENDPOINT_LAST_MATCHING,
				holy_USB_SERIAL_ENDPOINT_LAST_MATCHING);
}

static struct holy_usb_attach_desc attach_hook =
{
  .class = 0xff,
  .hook = holy_ftdi_attach
};

holy_MOD_INIT(usbserial_ftdi)
{
  holy_usb_register_attach_hook_class (&attach_hook);
}

holy_MOD_FINI(usbserial_ftdi)
{
  holy_serial_unregister_driver (&holy_ftdi_driver);
  holy_usb_unregister_attach_hook_class (&attach_hook);
}
