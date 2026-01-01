/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_USBTRANS_H
#define	holy_USBTRANS_H	1

#define MAX_USB_TRANSFER_LEN 0x0800

typedef enum
  {
    holy_USB_TRANSFER_TYPE_IN,
    holy_USB_TRANSFER_TYPE_OUT,
    holy_USB_TRANSFER_TYPE_SETUP
  } holy_transfer_type_t;

typedef enum
  {
    holy_USB_TRANSACTION_TYPE_CONTROL,
    holy_USB_TRANSACTION_TYPE_BULK
  } holy_transaction_type_t;

struct holy_usb_transaction
{
  int size;
  int toggle;
  holy_transfer_type_t pid;
  holy_uint32_t data;
  holy_size_t preceding;
};
typedef struct holy_usb_transaction *holy_usb_transaction_t;

struct holy_usb_transfer
{
  int devaddr;

  int endpoint;

  int size;

  int transcnt;

  int max;

  holy_transaction_type_t type;

  holy_transfer_type_t dir;

  struct holy_usb_device *dev;

  struct holy_usb_transaction *transactions;
  
  int last_trans;
  /* Index of last processed transaction in OHCI/UHCI driver. */

  void *controller_data;

  /* Used when finishing transfer to copy data back.  */
  struct holy_pci_dma_chunk *data_chunk;
  void *data;
};
typedef struct holy_usb_transfer *holy_usb_transfer_t;



enum
  {
    holy_USB_REQTYPE_TARGET_DEV = (0 << 0),
    holy_USB_REQTYPE_TARGET_INTERF = (1 << 0),
    holy_USB_REQTYPE_TARGET_ENDP = (2 << 0),
    holy_USB_REQTYPE_TARGET_OTHER = (3 << 0),
    holy_USB_REQTYPE_STANDARD = (0 << 5),
    holy_USB_REQTYPE_CLASS = (1 << 5),
    holy_USB_REQTYPE_VENDOR = (2 << 5),
    holy_USB_REQTYPE_OUT = (0 << 7),
    holy_USB_REQTYPE_IN	= (1 << 7),
    holy_USB_REQTYPE_CLASS_INTERFACE_OUT = holy_USB_REQTYPE_TARGET_INTERF
    | holy_USB_REQTYPE_CLASS | holy_USB_REQTYPE_OUT,
    holy_USB_REQTYPE_VENDOR_OUT = holy_USB_REQTYPE_VENDOR | holy_USB_REQTYPE_OUT,
    holy_USB_REQTYPE_CLASS_INTERFACE_IN = holy_USB_REQTYPE_TARGET_INTERF
    | holy_USB_REQTYPE_CLASS | holy_USB_REQTYPE_IN,
    holy_USB_REQTYPE_VENDOR_IN = holy_USB_REQTYPE_VENDOR | holy_USB_REQTYPE_IN
  };

enum
  {
    holy_USB_REQ_GET_STATUS = 0x00,
    holy_USB_REQ_CLEAR_FEATURE = 0x01,
    holy_USB_REQ_SET_FEATURE = 0x03,
    holy_USB_REQ_SET_ADDRESS = 0x05,
    holy_USB_REQ_GET_DESCRIPTOR = 0x06,
    holy_USB_REQ_SET_DESCRIPTOR	= 0x07,
    holy_USB_REQ_GET_CONFIGURATION = 0x08,
    holy_USB_REQ_SET_CONFIGURATION = 0x09,
    holy_USB_REQ_GET_INTERFACE = 0x0A,
    holy_USB_REQ_SET_INTERFACE = 0x0B,
    holy_USB_REQ_SYNC_FRAME = 0x0C
  };

#define holy_USB_FEATURE_ENDP_HALT	0x00
#define holy_USB_FEATURE_DEV_REMOTE_WU	0x01
#define holy_USB_FEATURE_TEST_MODE	0x02

enum
  {
    holy_USB_HUB_FEATURE_PORT_RESET = 0x04,
    holy_USB_HUB_FEATURE_PORT_POWER = 0x08,
    holy_USB_HUB_FEATURE_C_PORT_CONNECTED = 0x10,
    holy_USB_HUB_FEATURE_C_PORT_ENABLED = 0x11,
    holy_USB_HUB_FEATURE_C_PORT_SUSPEND = 0x12,
    holy_USB_HUB_FEATURE_C_PORT_OVERCURRENT = 0x13,
    holy_USB_HUB_FEATURE_C_PORT_RESET = 0x14
  };

enum
  {
    holy_USB_HUB_STATUS_PORT_CONNECTED = (1 << 0),
    holy_USB_HUB_STATUS_PORT_ENABLED = (1 << 1),
    holy_USB_HUB_STATUS_PORT_SUSPEND = (1 << 2),
    holy_USB_HUB_STATUS_PORT_OVERCURRENT = (1 << 3),
    holy_USB_HUB_STATUS_PORT_POWERED = (1 << 8),
    holy_USB_HUB_STATUS_PORT_LOWSPEED = (1 << 9),
    holy_USB_HUB_STATUS_PORT_HIGHSPEED = (1 << 10),
    holy_USB_HUB_STATUS_C_PORT_CONNECTED = (1 << 16),
    holy_USB_HUB_STATUS_C_PORT_ENABLED = (1 << 17),
    holy_USB_HUB_STATUS_C_PORT_SUSPEND = (1 << 18),
    holy_USB_HUB_STATUS_C_PORT_OVERCURRENT = (1 << 19),
    holy_USB_HUB_STATUS_C_PORT_RESET = (1 << 20)
  };

struct holy_usb_packet_setup
{
  holy_uint8_t reqtype;
  holy_uint8_t request;
  holy_uint16_t value;
  holy_uint16_t index;
  holy_uint16_t length;
} holy_PACKED;


#endif /* holy_USBTRANS_H */
