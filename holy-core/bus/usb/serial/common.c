/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/serial.h>
#include <holy/usbserial.h>
#include <holy/dl.h>

holy_MOD_LICENSE ("GPLv2+");

void
holy_usbserial_fini (struct holy_serial_port *port)
{
  port->usbdev->config[port->configno].interf[port->interfno].detach_hook = 0;
  port->usbdev->config[port->configno].interf[port->interfno].attached = 0;
}

void
holy_usbserial_detach (holy_usb_device_t usbdev, int configno, int interfno)
{
  static struct holy_serial_port *port;
  port = usbdev->config[configno].interf[interfno].detach_data;

  holy_serial_unregister (port);
}

static int usbnum = 0;

int
holy_usbserial_attach (holy_usb_device_t usbdev, int configno, int interfno,
		       struct holy_serial_driver *driver, int in_endp,
		       int out_endp)
{
  struct holy_serial_port *port;
  int j;
  struct holy_usb_desc_if *interf;
  holy_usb_err_t err = holy_USB_ERR_NONE;

  interf = usbdev->config[configno].interf[interfno].descif;

  port = holy_zalloc (sizeof (*port));
  if (!port)
    {
      holy_print_error ();
      return 0;
    }

  port->name = holy_xasprintf ("usb%d", usbnum++);
  if (!port->name)
    {
      holy_free (port);
      holy_print_error ();
      return 0;
    }

  port->usbdev = usbdev;
  port->driver = driver;
  for (j = 0; j < interf->endpointcnt; j++)
    {
      struct holy_usb_desc_endp *endp;
      endp = &usbdev->config[0].interf[interfno].descendp[j];

      if ((endp->endp_addr & 128) && (endp->attrib & 3) == 2
	  && (in_endp == holy_USB_SERIAL_ENDPOINT_LAST_MATCHING
	      || in_endp == endp->endp_addr))
	{
	  /* Bulk IN endpoint.  */
	  port->in_endp = endp;
	}
      else if (!(endp->endp_addr & 128) && (endp->attrib & 3) == 2
	       && (out_endp == holy_USB_SERIAL_ENDPOINT_LAST_MATCHING
		   || out_endp == endp->endp_addr))
	{
	  /* Bulk OUT endpoint.  */
	  port->out_endp = endp;
	}
    }

  /* Configure device */
  if (port->out_endp && port->in_endp)
    err = holy_usb_set_configuration (usbdev, configno + 1);
  
  if (!port->out_endp || !port->in_endp || err)
    {
      holy_free (port->name);
      holy_free (port);
      return 0;
    }

  port->configno = configno;
  port->interfno = interfno;

  holy_serial_config_defaults (port);
  holy_serial_register (port);

  port->usbdev->config[port->configno].interf[port->interfno].detach_hook
    = holy_usbserial_detach;
  port->usbdev->config[port->configno].interf[port->interfno].detach_data
    = port;

  return 1;
}

int
holy_usbserial_fetch (struct holy_serial_port *port, holy_size_t header_size)
{
  holy_usb_err_t err;
  holy_size_t actual;

  if (port->bufstart < port->bufend)
    return port->buf[port->bufstart++];

  err = holy_usb_bulk_read_extended (port->usbdev, port->in_endp,
				     sizeof (port->buf), port->buf, 10,
				     &actual);
  if (err != holy_USB_ERR_NONE)
    return -1;

  port->bufstart = header_size;
  port->bufend = actual;
  if (port->bufstart >= port->bufend)
    return -1;

  return port->buf[port->bufstart++];
}
