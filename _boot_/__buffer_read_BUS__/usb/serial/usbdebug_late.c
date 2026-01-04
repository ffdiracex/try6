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


/* Fetch a key.  */
static int
usbdebug_late_hw_fetch (struct holy_serial_port *port)
{
  return holy_usbserial_fetch (port, 0);
}

/* Put a character.  */
static void
usbdebug_late_hw_put (struct holy_serial_port *port, const int c)
{
  char cc = c;

  holy_usb_bulk_write (port->usbdev, port->out_endp, 1, &cc);
}

static holy_err_t
usbdebug_late_hw_configure (struct holy_serial_port *port __attribute__ ((unused)),
			    struct holy_serial_config *config __attribute__ ((unused)))
{
  return holy_ERR_NONE;
}

static struct holy_serial_driver holy_usbdebug_late_driver =
  {
    .configure = usbdebug_late_hw_configure,
    .fetch = usbdebug_late_hw_fetch,
    .put = usbdebug_late_hw_put,
    .fini = holy_usbserial_fini
  };

static int
holy_usbdebug_late_attach (holy_usb_device_t usbdev, int configno, int interfno)
{
  holy_usb_err_t err;
  struct holy_usb_desc_debug debugdesc;

  err = holy_usb_get_descriptor (usbdev, holy_USB_DESCRIPTOR_DEBUG, configno,
				 sizeof (debugdesc), (char *) &debugdesc);
  if (err)
    return 0;

  return holy_usbserial_attach (usbdev, configno, interfno,
				&holy_usbdebug_late_driver,
				debugdesc.in_endp, debugdesc.out_endp);
}

static struct holy_usb_attach_desc attach_hook =
{
  .class = 0xff,
  .hook = holy_usbdebug_late_attach
};

holy_MOD_INIT(usbserial_usbdebug_late)
{
  holy_usb_register_attach_hook_class (&attach_hook);
}

holy_MOD_FINI(usbserial_usbdebug_late)
{
  holy_serial_unregister_driver (&holy_usbdebug_late_driver);
  holy_usb_unregister_attach_hook_class (&attach_hook);
}
