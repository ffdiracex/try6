/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_USBDESC_H
#define	holy_USBDESC_H	1

#include <holy/types.h>
#include <holy/symbol.h>

typedef enum {
  holy_USB_DESCRIPTOR_DEVICE = 1,
  holy_USB_DESCRIPTOR_CONFIG,
  holy_USB_DESCRIPTOR_STRING,
  holy_USB_DESCRIPTOR_INTERFACE,
  holy_USB_DESCRIPTOR_ENDPOINT,
  holy_USB_DESCRIPTOR_DEBUG = 10,
  holy_USB_DESCRIPTOR_HUB = 0x29
} holy_usb_descriptor_t;

struct holy_usb_desc
{
  holy_uint8_t length;
  holy_uint8_t type;
} holy_PACKED;

struct holy_usb_desc_device
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint16_t usbrel;
  holy_uint8_t class;
  holy_uint8_t subclass;
  holy_uint8_t protocol;
  holy_uint8_t maxsize0;
  holy_uint16_t vendorid;
  holy_uint16_t prodid;
  holy_uint16_t devrel;
  holy_uint8_t strvendor;
  holy_uint8_t strprod;
  holy_uint8_t strserial;
  holy_uint8_t configcnt;
} holy_PACKED;

struct holy_usb_desc_config
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint16_t totallen;
  holy_uint8_t numif;
  holy_uint8_t config;
  holy_uint8_t strconfig;
  holy_uint8_t attrib;
  holy_uint8_t maxpower;
} holy_PACKED;

#if 0
struct holy_usb_desc_if_association
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint8_t firstif;
  holy_uint8_t ifcnt;
  holy_uint8_t class;
  holy_uint8_t subclass;
  holy_uint8_t protocol;
  holy_uint8_t function;
} holy_PACKED;
#endif

struct holy_usb_desc_if
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint8_t ifnum;
  holy_uint8_t altsetting;
  holy_uint8_t endpointcnt;
  holy_uint8_t class;
  holy_uint8_t subclass;
  holy_uint8_t protocol;
  holy_uint8_t strif;
} holy_PACKED;

struct holy_usb_desc_endp
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint8_t endp_addr;
  holy_uint8_t attrib;
  holy_uint16_t maxpacket;
  holy_uint8_t interval;
} holy_PACKED;

struct holy_usb_desc_str
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint16_t str[0];
} holy_PACKED;

struct holy_usb_desc_debug
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint8_t in_endp;
  holy_uint8_t out_endp;
} holy_PACKED;

struct holy_usb_usb_hubdesc
{
  holy_uint8_t length;
  holy_uint8_t type;
  holy_uint8_t portcnt;
  holy_uint16_t characteristics;
  holy_uint8_t pwdgood;
  holy_uint8_t current;
  /* Removable and power control bits follow.  */
} holy_PACKED;

#endif /* holy_USBDESC_H */
