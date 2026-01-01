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

/* Convert speed to divisor.  */
static holy_uint32_t
is_speed_supported (unsigned int speed)
{
  unsigned int i;
  unsigned int supported[] = { 2400, 4800, 9600, 19200, 38400, 57600, 115200};

  for (i = 0; i < ARRAY_SIZE (supported); i++)
    if (supported[i] == speed)
      return 1;
  return 0;
}

#define holy_PL2303_REQUEST_SET_CONFIG 0x20
#define holy_PL2303_STOP_BITS_1 0x0
#define holy_PL2303_STOP_BITS_1_5 0x1
#define holy_PL2303_STOP_BITS_2 0x2

#define holy_PL2303_PARITY_NONE 0
#define holy_PL2303_PARITY_ODD  1
#define holy_PL2303_PARITY_EVEN 2

struct holy_pl2303_config
{
  holy_uint32_t speed;
  holy_uint8_t stop_bits;
  holy_uint8_t parity;
  holy_uint8_t word_len;
} holy_PACKED;

static void
real_config (struct holy_serial_port *port)
{
  struct holy_pl2303_config config_pl2303;
  char xx;

  if (port->configured)
    return;

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8484, 0, 1, &xx);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 0x0404, 0, 0, 0);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8484, 0, 1, &xx);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8383, 0, 1, &xx);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8484, 0, 1, &xx);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 0x0404, 1, 0, 0);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8484, 0, 1, &xx);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_IN,
			1, 0x8383, 0, 1, &xx);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 0, 1, 0, 0);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 1, 0, 0, 0);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 2, 0x44, 0, 0);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 8, 0, 0, 0);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 9, 0, 0, 0);

  if (port->config.stop_bits == holy_SERIAL_STOP_BITS_2)
    config_pl2303.stop_bits = holy_PL2303_STOP_BITS_2;
  else if (port->config.stop_bits == holy_SERIAL_STOP_BITS_1_5)
    config_pl2303.stop_bits = holy_PL2303_STOP_BITS_1_5;
  else
    config_pl2303.stop_bits = holy_PL2303_STOP_BITS_1;

  switch (port->config.parity)
    {
    case holy_SERIAL_PARITY_NONE:
      config_pl2303.parity = holy_PL2303_PARITY_NONE;
      break;
    case holy_SERIAL_PARITY_ODD:
      config_pl2303.parity = holy_PL2303_PARITY_ODD;
      break;
    case holy_SERIAL_PARITY_EVEN:
      config_pl2303.parity = holy_PL2303_PARITY_EVEN;
      break;
    }

  config_pl2303.word_len = port->config.word_len;
  config_pl2303.speed = port->config.speed;
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
			holy_PL2303_REQUEST_SET_CONFIG, 0, 0,
			sizeof (config_pl2303), (char *) &config_pl2303);
  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
			0x22, 3, 0, 0, 0);

  holy_usb_control_msg (port->usbdev, holy_USB_REQTYPE_VENDOR_OUT,
			1, 0, port->config.rtscts ? 0x61 : 0, 0, 0);
  port->configured = 1;
}

/* Fetch a key.  */
static int
pl2303_hw_fetch (struct holy_serial_port *port)
{
  real_config (port);

  return holy_usbserial_fetch (port, 0);
}

/* Put a character.  */
static void
pl2303_hw_put (struct holy_serial_port *port, const int c)
{
  char cc = c;

  real_config (port);

  holy_usb_bulk_write (port->usbdev, port->out_endp, 1, &cc);
}

static holy_err_t
pl2303_hw_configure (struct holy_serial_port *port,
			struct holy_serial_config *config)
{
  if (!is_speed_supported (config->speed))
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

static struct holy_serial_driver holy_pl2303_driver =
  {
    .configure = pl2303_hw_configure,
    .fetch = pl2303_hw_fetch,
    .put = pl2303_hw_put,
    .fini = holy_usbserial_fini
  };

static const struct 
{
  holy_uint16_t vendor, product;
} products[] =
  {
    {0x067b, 0x2303}
  };

static int
holy_pl2303_attach (holy_usb_device_t usbdev, int configno, int interfno)
{
  unsigned j;

  for (j = 0; j < ARRAY_SIZE (products); j++)
    if (usbdev->descdev.vendorid == products[j].vendor
	&& usbdev->descdev.prodid == products[j].product)
      break;
  if (j == ARRAY_SIZE (products))
    return 0;

  return holy_usbserial_attach (usbdev, configno, interfno,
				&holy_pl2303_driver,
				holy_USB_SERIAL_ENDPOINT_LAST_MATCHING,
				holy_USB_SERIAL_ENDPOINT_LAST_MATCHING);
}

static struct holy_usb_attach_desc attach_hook =
{
  .class = 0xff,
  .hook = holy_pl2303_attach
};

holy_MOD_INIT(usbserial_pl2303)
{
  holy_usb_register_attach_hook_class (&attach_hook);
}

holy_MOD_FINI(usbserial_pl2303)
{
  holy_serial_unregister_driver (&holy_pl2303_driver);
  holy_usb_unregister_attach_hook_class (&attach_hook);
}
