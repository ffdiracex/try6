/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef	holy_USB_H
#define	holy_USB_H	1

#include <holy/err.h>
#include <holy/usbdesc.h>
#include <holy/usbtrans.h>

typedef struct holy_usb_device *holy_usb_device_t;
typedef struct holy_usb_controller *holy_usb_controller_t;
typedef struct holy_usb_controller_dev *holy_usb_controller_dev_t;

typedef enum
  {
    holy_USB_ERR_NONE,
    holy_USB_ERR_WAIT,
    holy_USB_ERR_INTERNAL,
    holy_USB_ERR_STALL,
    holy_USB_ERR_DATA,
    holy_USB_ERR_NAK,
    holy_USB_ERR_BABBLE,
    holy_USB_ERR_TIMEOUT,
    holy_USB_ERR_BITSTUFF,
    holy_USB_ERR_UNRECOVERABLE,
    holy_USB_ERR_BADDEVICE
  } holy_usb_err_t;

typedef enum
  {
    holy_USB_SPEED_NONE,
    holy_USB_SPEED_LOW,
    holy_USB_SPEED_FULL,
    holy_USB_SPEED_HIGH
  } holy_usb_speed_t;

typedef int (*holy_usb_iterate_hook_t) (holy_usb_device_t dev, void *data);
typedef int (*holy_usb_controller_iterate_hook_t) (holy_usb_controller_t dev,
						   void *data);

/* Call HOOK with each device, until HOOK returns non-zero.  */
int holy_usb_iterate (holy_usb_iterate_hook_t hook, void *hook_data);

holy_usb_err_t holy_usb_device_initialize (holy_usb_device_t dev);

holy_usb_err_t holy_usb_get_descriptor (holy_usb_device_t dev,
					holy_uint8_t type, holy_uint8_t index,
					holy_size_t size, char *data);

holy_usb_err_t holy_usb_clear_halt (holy_usb_device_t dev, int endpoint);


holy_usb_err_t holy_usb_set_configuration (holy_usb_device_t dev,
					   int configuration);

void holy_usb_controller_dev_register (holy_usb_controller_dev_t usb);

void holy_usb_controller_dev_unregister (holy_usb_controller_dev_t usb);

int holy_usb_controller_iterate (holy_usb_controller_iterate_hook_t hook,
				 void *hook_data);


holy_usb_err_t holy_usb_control_msg (holy_usb_device_t dev, holy_uint8_t reqtype,
				     holy_uint8_t request, holy_uint16_t value,
				     holy_uint16_t index, holy_size_t size,
				     char *data);

holy_usb_err_t
holy_usb_bulk_read (holy_usb_device_t dev,
		    struct holy_usb_desc_endp *endpoint,
		    holy_size_t size, char *data);
holy_usb_err_t
holy_usb_bulk_write (holy_usb_device_t dev,
		     struct holy_usb_desc_endp *endpoint,
		     holy_size_t size, char *data);

holy_usb_err_t
holy_usb_root_hub (holy_usb_controller_t controller);



/* XXX: All handled by libusb for now.  */
struct holy_usb_controller_dev
{
  /* The device name.  */
  const char *name;

  int (*iterate) (holy_usb_controller_iterate_hook_t hook, void *hook_data);

  holy_usb_err_t (*setup_transfer) (holy_usb_controller_t dev,
				    holy_usb_transfer_t transfer);

  holy_usb_err_t (*check_transfer) (holy_usb_controller_t dev,
				    holy_usb_transfer_t transfer,
				    holy_size_t *actual);

  holy_usb_err_t (*cancel_transfer) (holy_usb_controller_t dev,
				     holy_usb_transfer_t transfer);

  int (*hubports) (holy_usb_controller_t dev);

  holy_usb_err_t (*portstatus) (holy_usb_controller_t dev, unsigned int port,
				unsigned int enable);

  holy_usb_speed_t (*detect_dev) (holy_usb_controller_t dev, int port, int *changed);

  /* Per controller flag - port reset pending, don't do another reset */
  holy_uint64_t pending_reset;

  /* Max. number of transfer descriptors used per one bulk transfer */
  /* The reason is to prevent "exhausting" of TD by large bulk */
  /* transfer - number of TD is limited in USB host driver */
  /* Value is calculated/estimated in driver - some TDs should be */
  /* reserved for posible concurrent control or "interrupt" transfers */
  holy_size_t max_bulk_tds;
  
  /* The next host controller.  */
  struct holy_usb_controller_dev *next;
};

struct holy_usb_controller
{
  /* The underlying USB Host Controller device.  */
  holy_usb_controller_dev_t dev;

  /* Data used by the USB Host Controller Driver.  */
  void *data;
};


struct holy_usb_interface
{
  struct holy_usb_desc_if *descif;

  struct holy_usb_desc_endp *descendp;

  /* A driver is handling this interface. Do we need to support multiple drivers
     for single interface?
   */
  int attached;

  void (*detach_hook) (struct holy_usb_device *dev, int config, int interface);

  void *detach_data;
};

struct holy_usb_configuration
{
  /* Configuration descriptors .  */
  struct holy_usb_desc_config *descconf;

  /* Interfaces associated to this configuration.  */
  struct holy_usb_interface interf[32];
};

struct holy_usb_hub_port
{
  holy_uint64_t soft_limit_time;
  holy_uint64_t hard_limit_time;
  enum { 
    PORT_STATE_NORMAL = 0,
    PORT_STATE_WAITING_FOR_STABLE_POWER = 1,
    PORT_STATE_FAILED_DEVICE = 2,
    PORT_STATE_STABLE_POWER = 3,
  } state;
};

struct holy_usb_device
{
  /* The device descriptor of this device.  */
  struct holy_usb_desc_device descdev;

  /* The controller the device is connected to.  */
  struct holy_usb_controller controller;

  /* Device configurations (after opening the device).  */
  struct holy_usb_configuration config[8];

  /* Device address.  */
  int addr;

  /* Device speed.  */
  holy_usb_speed_t speed;

  /* All descriptors are read if this is set to 1.  */
  int initialized;

  /* Data toggle values (used for bulk transfers only).  */
  int toggle[256];

  /* Used by libusb wrapper.  Schedulded for removal. */
  void *data;

  /* Hub information.  */

  /* Array of children for a hub.  */
  holy_usb_device_t *children;

  /* Number of hub ports.  */
  unsigned nports;

  struct holy_usb_hub_port *ports;

  holy_usb_transfer_t hub_transfer;

  holy_uint32_t statuschange;

  struct holy_usb_desc_endp *hub_endpoint;

  /* EHCI Split Transfer information */
  int split_hubport;

  int split_hubaddr;
};



typedef enum holy_usb_ep_type
  {
    holy_USB_EP_CONTROL,
    holy_USB_EP_ISOCHRONOUS,
    holy_USB_EP_BULK,
    holy_USB_EP_INTERRUPT
  } holy_usb_ep_type_t;

static inline enum holy_usb_ep_type
holy_usb_get_ep_type (struct holy_usb_desc_endp *ep)
{
  return ep->attrib & 3;
}

typedef enum
  {
    holy_USB_CLASS_NOTHERE,
    holy_USB_CLASS_AUDIO,
    holy_USB_CLASS_COMMUNICATION,
    holy_USB_CLASS_HID,
    holy_USB_CLASS_XXX,
    holy_USB_CLASS_PHYSICAL,
    holy_USB_CLASS_IMAGE,
    holy_USB_CLASS_PRINTER,
    holy_USB_CLASS_MASS_STORAGE,
    holy_USB_CLASS_HUB,
    holy_USB_CLASS_DATA_INTERFACE,
    holy_USB_CLASS_SMART_CARD,
    holy_USB_CLASS_CONTENT_SECURITY,
    holy_USB_CLASS_VIDEO
  } holy_usb_classes_t;

typedef enum
  {
    holy_USBMS_SUBCLASS_BULK = 0x06,
  	/* Experimental support for non-pure SCSI devices */
    holy_USBMS_SUBCLASS_RBC = 0x01,
    holy_USBMS_SUBCLASS_MMC2 = 0x02,
    holy_USBMS_SUBCLASS_UFI = 0x04,
    holy_USBMS_SUBCLASS_SFF8070 = 0x05
  } holy_usbms_subclass_t;

typedef enum
  {
    holy_USBMS_PROTOCOL_BULK = 0x50,
    /* Experimental support for Control/Bulk/Interrupt (CBI) devices */
    holy_USBMS_PROTOCOL_CBI = 0x00, /* CBI with interrupt */
    holy_USBMS_PROTOCOL_CB = 0x01  /* CBI wthout interrupt */
  } holy_usbms_protocol_t;

static inline struct holy_usb_desc_if *
holy_usb_get_config_interface (struct holy_usb_desc_config *config)
{
  struct holy_usb_desc_if *interf;

  interf = (struct holy_usb_desc_if *) (sizeof (*config) + (char *) config);
  return interf;
}

typedef int (*holy_usb_attach_hook_class) (holy_usb_device_t usbdev,
					   int configno, int interfno);

struct holy_usb_attach_desc
{
  struct holy_usb_attach_desc *next;
  struct holy_usb_attach_desc **prev;
  int class;
  holy_usb_attach_hook_class hook;
};

void holy_usb_register_attach_hook_class (struct holy_usb_attach_desc *desc);
void holy_usb_unregister_attach_hook_class (struct holy_usb_attach_desc *desc);

void holy_usb_poll_devices (int wait_for_completion);

void holy_usb_device_attach (holy_usb_device_t dev);
holy_usb_err_t
holy_usb_bulk_read_extended (holy_usb_device_t dev,
			     struct holy_usb_desc_endp *endpoint,
			     holy_size_t size, char *data,
			     int timeout, holy_size_t *actual);
holy_usb_transfer_t
holy_usb_bulk_read_background (holy_usb_device_t dev,
			       struct holy_usb_desc_endp *endpoint,
			       holy_size_t size, void *data);
holy_usb_err_t
holy_usb_check_transfer (holy_usb_transfer_t trans, holy_size_t *actual);
void
holy_usb_cancel_transfer (holy_usb_transfer_t trans);

#endif /* holy_USB_H */
