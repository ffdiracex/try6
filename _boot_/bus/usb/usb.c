/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/usb.h>
#include <holy/misc.h>
#include <holy/list.h>
#include <holy/term.h>

holy_MOD_LICENSE ("GPLv2+");

static struct holy_usb_attach_desc *attach_hooks;

#if 0
/* Context for holy_usb_controller_iterate.  */
struct holy_usb_controller_iterate_ctx
{
  holy_usb_controller_iterate_hook_t hook;
  void *hook_data;
  holy_usb_controller_dev_t p;
};

/* Helper for holy_usb_controller_iterate.  */
static int
holy_usb_controller_iterate_iter (holy_usb_controller_t dev, void *data)
{
  struct holy_usb_controller_iterate_ctx *ctx = data;

  dev->dev = ctx->p;
  if (ctx->hook (dev, ctx->hook_data))
    return 1;
  return 0;
}

int
holy_usb_controller_iterate (holy_usb_controller_iterate_hook_t hook,
			     void *hook_data)
{
  struct holy_usb_controller_iterate_ctx ctx = {
    .hook = hook,
    .hook_data = hook_data
  };

  /* Iterate over all controller drivers.  */
  for (ctx.p = holy_usb_list; ctx.p; ctx.p = ctx.p->next)
    {
      /* Iterate over the busses of the controllers.  XXX: Actually, a
	 hub driver should do this.  */
      if (ctx.p->iterate (holy_usb_controller_iterate_iter, &ctx))
	return 1;
    }

  return 0;
}
#endif


holy_usb_err_t
holy_usb_clear_halt (holy_usb_device_t dev, int endpoint)
{
  dev->toggle[endpoint] = 0;
  return holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				     | holy_USB_REQTYPE_STANDARD
				     | holy_USB_REQTYPE_TARGET_ENDP),
			       holy_USB_REQ_CLEAR_FEATURE,
			       holy_USB_FEATURE_ENDP_HALT,
			       endpoint, 0, 0);
}

holy_usb_err_t
holy_usb_set_configuration (holy_usb_device_t dev, int configuration)
{
  holy_memset (dev->toggle, 0, sizeof (dev->toggle));

  return holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				     | holy_USB_REQTYPE_STANDARD
				     | holy_USB_REQTYPE_TARGET_DEV),
			       holy_USB_REQ_SET_CONFIGURATION, configuration,
			       0, 0, NULL);
}

holy_usb_err_t
holy_usb_get_descriptor (holy_usb_device_t dev,
			 holy_uint8_t type, holy_uint8_t index,
			 holy_size_t size, char *data)
{
  return holy_usb_control_msg (dev, (holy_USB_REQTYPE_IN
				     | holy_USB_REQTYPE_STANDARD
				     | holy_USB_REQTYPE_TARGET_DEV),
			       holy_USB_REQ_GET_DESCRIPTOR,
			       (type << 8) | index,
			       0, size, data);
}

holy_usb_err_t
holy_usb_device_initialize (holy_usb_device_t dev)
{
  struct holy_usb_desc_device *descdev;
  struct holy_usb_desc_config config;
  holy_usb_err_t err;
  int i;

  /* First we have to read first 8 bytes only and determine
   * max. size of packet */
  dev->descdev.maxsize0 = 0; /* invalidating, for safety only, can be removed if it is sure it is zero here */
  err = holy_usb_get_descriptor (dev, holy_USB_DESCRIPTOR_DEVICE,
                                 0, 8, (char *) &dev->descdev);
  if (err)
    return err;

  /* Now we have valid value in dev->descdev.maxsize0,
   * so we can read whole device descriptor */
  err = holy_usb_get_descriptor (dev, holy_USB_DESCRIPTOR_DEVICE,
				 0, sizeof (struct holy_usb_desc_device),
				 (char *) &dev->descdev);
  if (err)
    return err;
  descdev = &dev->descdev;

  for (i = 0; i < 8; i++)
    dev->config[i].descconf = NULL;

  if (descdev->configcnt == 0)
    {
      err = holy_USB_ERR_BADDEVICE;
      goto fail;
    }    

  for (i = 0; i < descdev->configcnt; i++)
    {
      int pos;
      int currif;
      char *data;
      struct holy_usb_desc *desc;

      /* First just read the first 4 bytes of the configuration
	 descriptor, after that it is known how many bytes really have
	 to be read.  */
      err = holy_usb_get_descriptor (dev, holy_USB_DESCRIPTOR_CONFIG, i, 4,
				     (char *) &config);

      data = holy_malloc (config.totallen);
      if (! data)
	{
	  err = holy_USB_ERR_INTERNAL;
	  goto fail;
	}

      dev->config[i].descconf = (struct holy_usb_desc_config *) data;
      err = holy_usb_get_descriptor (dev, holy_USB_DESCRIPTOR_CONFIG, i,
				     config.totallen, data);
      if (err)
	goto fail;

      /* Skip the configuration descriptor.  */
      pos = dev->config[i].descconf->length;

      /* Read all interfaces.  */
      for (currif = 0; currif < dev->config[i].descconf->numif; currif++)
	{
	  while (pos < config.totallen)
            {
              desc = (struct holy_usb_desc *)&data[pos];
              if (desc->type == holy_USB_DESCRIPTOR_INTERFACE)
                break;
              if (!desc->length)
                {
                  err = holy_USB_ERR_BADDEVICE;
                  goto fail;
                }
              pos += desc->length;
            }

	  dev->config[i].interf[currif].descif
	    = (struct holy_usb_desc_if *) &data[pos];
	  pos += dev->config[i].interf[currif].descif->length;

	  while (pos < config.totallen)
            {
              desc = (struct holy_usb_desc *)&data[pos];
              if (desc->type == holy_USB_DESCRIPTOR_ENDPOINT)
                break;
              if (!desc->length)
                {
                  err = holy_USB_ERR_BADDEVICE;
                  goto fail;
                }
              pos += desc->length;
            }

	  /* Point to the first endpoint.  */
	  dev->config[i].interf[currif].descendp
	    = (struct holy_usb_desc_endp *) &data[pos];
	  pos += (sizeof (struct holy_usb_desc_endp)
		  * dev->config[i].interf[currif].descif->endpointcnt);
	}
    }

  return holy_USB_ERR_NONE;

 fail:

  for (i = 0; i < 8; i++)
    holy_free (dev->config[i].descconf);

  return err;
}

void holy_usb_device_attach (holy_usb_device_t dev)
{
  int i;
  
  /* XXX: Just check configuration 0 for now.  */
  for (i = 0; i < dev->config[0].descconf->numif; i++)
    {
      struct holy_usb_desc_if *interf;
      struct holy_usb_attach_desc *desc;

      interf = dev->config[0].interf[i].descif;

      holy_dprintf ("usb", "iterate: interf=%d, class=%d, subclass=%d, protocol=%d\n",
		    i, interf->class, interf->subclass, interf->protocol);

      if (dev->config[0].interf[i].attached)
	continue;

      for (desc = attach_hooks; desc; desc = desc->next)
	if (interf->class == desc->class)
	  {
	    holy_boot_time ("Probing USB device driver class %x", desc->class);
	    if (desc->hook (dev, 0, i))
	      dev->config[0].interf[i].attached = 1;
	    holy_boot_time ("Probed USB device driver class %x", desc->class);
	  }

      if (dev->config[0].interf[i].attached)
	continue;

      switch (interf->class)
	{
	case holy_USB_CLASS_MASS_STORAGE:
	  holy_dl_load ("usbms");
	  holy_print_error ();
	  break;
	case holy_USB_CLASS_HID:
	  holy_dl_load ("usb_keyboard");
	  holy_print_error ();
	  break;
	case 0xff:
	  /* FIXME: don't load useless modules.  */
	  holy_dl_load ("usbserial_ftdi");
	  holy_print_error ();
	  holy_dl_load ("usbserial_pl2303");
	  holy_print_error ();
	  holy_dl_load ("usbserial_usbdebug");
	  holy_print_error ();
	  break;
	}
    }
}

/* Helper for holy_usb_register_attach_hook_class.  */
static int
holy_usb_register_attach_hook_class_iter (holy_usb_device_t usbdev, void *data)
{
  struct holy_usb_attach_desc *desc = data;
  struct holy_usb_desc_device *descdev = &usbdev->descdev;
  int i;

  if (descdev->class != 0 || descdev->subclass || descdev->protocol != 0
      || descdev->configcnt == 0)
    return 0;

  /* XXX: Just check configuration 0 for now.  */
  for (i = 0; i < usbdev->config[0].descconf->numif; i++)
    {
      struct holy_usb_desc_if *interf;

      interf = usbdev->config[0].interf[i].descif;

      holy_dprintf ("usb", "iterate: interf=%d, class=%d, subclass=%d, protocol=%d\n",
		    i, interf->class, interf->subclass, interf->protocol);

      if (usbdev->config[0].interf[i].attached)
	continue;

      if (interf->class != desc->class)
	continue;
      if (desc->hook (usbdev, 0, i))
	usbdev->config[0].interf[i].attached = 1;
    }

  return 0;
}

void
holy_usb_register_attach_hook_class (struct holy_usb_attach_desc *desc)
{
  desc->next = attach_hooks;
  attach_hooks = desc;

  holy_usb_iterate (holy_usb_register_attach_hook_class_iter, desc);
}

void
holy_usb_unregister_attach_hook_class (struct holy_usb_attach_desc *desc)
{
  holy_list_remove (holy_AS_LIST (desc));
}


holy_MOD_INIT(usb)
{
  holy_term_poll_usb = holy_usb_poll_devices;
}

holy_MOD_FINI(usb)
{
  holy_term_poll_usb = NULL;
}
