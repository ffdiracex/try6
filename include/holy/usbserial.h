/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_USBSERIAL_HEADER
#define holy_USBSERIAL_HEADER	1

void holy_usbserial_fini (struct holy_serial_port *port);

void holy_usbserial_detach (holy_usb_device_t usbdev, int configno,
			    int interfno);

int
holy_usbserial_attach (holy_usb_device_t usbdev, int configno, int interfno,
		       struct holy_serial_driver *driver, int in_endp,
		       int out_endp);
enum
  {
    holy_USB_SERIAL_ENDPOINT_LAST_MATCHING = -1
  };
int
holy_usbserial_fetch (struct holy_serial_port *port, holy_size_t header_size);

#endif
