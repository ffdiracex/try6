/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/mm.h>
#include <holy/usb.h>
#include <holy/misc.h>
#include <holy/time.h>

#define holy_USBHUB_MAX_DEVICES 128

/* USB Supports 127 devices, with device 0 as special case.  */
static struct holy_usb_device *holy_usb_devs[holy_USBHUB_MAX_DEVICES];

static int rescan = 0;
static int npending = 0;

struct holy_usb_hub
{
  struct holy_usb_hub *next;
  holy_usb_controller_t controller;
  int nports;
  struct holy_usb_device **devices;
  struct holy_usb_hub_port *ports;
  holy_usb_device_t dev;
};

static struct holy_usb_hub *hubs;
static holy_usb_controller_dev_t holy_usb_list;

/* Add a device that currently has device number 0 and resides on
   CONTROLLER, the Hub reported that the device speed is SPEED.  */
static holy_usb_device_t
holy_usb_hub_add_dev (holy_usb_controller_t controller,
                      holy_usb_speed_t speed,
                      int split_hubport, int split_hubaddr)
{
  holy_usb_device_t dev;
  int i;
  holy_usb_err_t err;

  holy_boot_time ("Attaching USB device");

  dev = holy_zalloc (sizeof (struct holy_usb_device));
  if (! dev)
    return NULL;

  dev->controller = *controller;
  dev->speed = speed;
  dev->split_hubport = split_hubport;
  dev->split_hubaddr = split_hubaddr;

  err = holy_usb_device_initialize (dev);
  if (err)
    {
      holy_free (dev);
      return NULL;
    }

  /* Assign a new address to the device.  */
  for (i = 1; i < holy_USBHUB_MAX_DEVICES; i++)
    {
      if (! holy_usb_devs[i])
	break;
    }
  if (i == holy_USBHUB_MAX_DEVICES)
    {
      holy_error (holy_ERR_IO, "can't assign address to USB device");
      for (i = 0; i < 8; i++)
        holy_free (dev->config[i].descconf);
      holy_free (dev);
      return NULL;
    }

  err = holy_usb_control_msg (dev,
			      (holy_USB_REQTYPE_OUT
			       | holy_USB_REQTYPE_STANDARD
			       | holy_USB_REQTYPE_TARGET_DEV),
			      holy_USB_REQ_SET_ADDRESS,
			      i, 0, 0, NULL);
  if (err)
    {
      for (i = 0; i < 8; i++)
        holy_free (dev->config[i].descconf);
      holy_free (dev);
      return NULL;
    }

  dev->addr = i;
  dev->initialized = 1;
  holy_usb_devs[i] = dev;

  holy_dprintf ("usb", "Added new usb device: %p, addr=%d\n",
		dev, i);
  holy_dprintf ("usb", "speed=%d, split_hubport=%d, split_hubaddr=%d\n",
		speed, split_hubport, split_hubaddr);

  /* Wait "recovery interval", spec. says 2ms */
  holy_millisleep (2);

  holy_boot_time ("Probing USB device driver");
  
  holy_usb_device_attach (dev);

  holy_boot_time ("Attached USB device");
  
  return dev;
}


static holy_usb_err_t
holy_usb_add_hub (holy_usb_device_t dev)
{
  struct holy_usb_usb_hubdesc hubdesc;
  holy_usb_err_t err;
  int i;
  
  err = holy_usb_control_msg (dev, (holy_USB_REQTYPE_IN
	  		            | holy_USB_REQTYPE_CLASS
			            | holy_USB_REQTYPE_TARGET_DEV),
                              holy_USB_REQ_GET_DESCRIPTOR,
			      (holy_USB_DESCRIPTOR_HUB << 8) | 0,
			      0, sizeof (hubdesc), (char *) &hubdesc);
  if (err)
    return err;
  holy_dprintf ("usb", "Hub descriptor:\n\t\t len:%d, typ:0x%02x, cnt:%d, char:0x%02x, pwg:%d, curr:%d\n",
                hubdesc.length, hubdesc.type, hubdesc.portcnt,
                hubdesc.characteristics, hubdesc.pwdgood,
                hubdesc.current);

  /* Activate the first configuration. Hubs should have only one conf. */
  holy_dprintf ("usb", "Hub set configuration\n");
  holy_usb_set_configuration (dev, 1);

  dev->nports = hubdesc.portcnt;
  dev->children = holy_zalloc (hubdesc.portcnt * sizeof (dev->children[0]));
  dev->ports = holy_zalloc (dev->nports * sizeof (dev->ports[0]));
  if (!dev->children || !dev->ports)
    {
      holy_free (dev->children);
      holy_free (dev->ports);
      return holy_USB_ERR_INTERNAL;
    }

  /* Power on all Hub ports.  */
  for (i = 1; i <= hubdesc.portcnt; i++)
    {
      holy_dprintf ("usb", "Power on - port %d\n", i);
      /* Power on the port and wait for possible device connect */
      holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				  | holy_USB_REQTYPE_CLASS
				  | holy_USB_REQTYPE_TARGET_OTHER),
			    holy_USB_REQ_SET_FEATURE,
			    holy_USB_HUB_FEATURE_PORT_POWER,
			    i, 0, NULL);
    }

  /* Rest will be done on next usb poll.  */
  for (i = 0; i < dev->config[0].interf[0].descif->endpointcnt;
       i++)
    {
      struct holy_usb_desc_endp *endp = NULL;
      endp = &dev->config[0].interf[0].descendp[i];

      if ((endp->endp_addr & 128) && holy_usb_get_ep_type(endp)
	  == holy_USB_EP_INTERRUPT)
	{
	  holy_size_t len;
	  dev->hub_endpoint = endp;
	  len = endp->maxpacket;
	  if (len > sizeof (dev->statuschange))
	    len = sizeof (dev->statuschange);
	  dev->hub_transfer
	    = holy_usb_bulk_read_background (dev, endp, len,
					     (char *) &dev->statuschange);
	  break;
	}
    }

  rescan = 1;

  return holy_USB_ERR_NONE;
}

static void
attach_root_port (struct holy_usb_hub *hub, int portno,
		  holy_usb_speed_t speed)
{
  holy_usb_device_t dev;
  holy_usb_err_t err;

  holy_boot_time ("After detect_dev");

  /* Enable the port.  */
  err = hub->controller->dev->portstatus (hub->controller, portno, 1);
  if (err)
    return;
  hub->controller->dev->pending_reset = holy_get_time_ms () + 5000;
  npending++;

  holy_millisleep (10);

  holy_boot_time ("Port enabled");

  /* Enable the port and create a device.  */
  /* High speed device needs not transaction translation
     and full/low speed device cannot be connected to EHCI root hub
     and full/low speed device connected to OHCI/UHCI needs not
     transaction translation - e.g. hubport and hubaddr should be
     always none (zero) for any device connected to any root hub. */
  dev = holy_usb_hub_add_dev (hub->controller, speed, 0, 0);
  hub->controller->dev->pending_reset = 0;
  npending--;
  if (! dev)
    return;

  hub->devices[portno] = dev;

  /* If the device is a Hub, scan it for more devices.  */
  if (dev->descdev.class == 0x09)
    holy_usb_add_hub (dev);

  holy_boot_time ("Attached root port");
}

/* Iterate over all controllers found by the driver.  */
static int
holy_usb_controller_dev_register_iter (holy_usb_controller_t controller, void *data)
{
  holy_usb_controller_dev_t usb = data;
  struct holy_usb_hub *hub;

  controller->dev = usb;

  holy_boot_time ("Registering USB root hub");

  hub = holy_malloc (sizeof (*hub));
  if (!hub)
    return holy_USB_ERR_INTERNAL;

  hub->next = hubs;
  hubs = hub;
  hub->controller = holy_malloc (sizeof (*controller));
  if (!hub->controller)
    {
      holy_free (hub);
      return holy_USB_ERR_INTERNAL;
    }

  holy_memcpy (hub->controller, controller, sizeof (*controller));
  hub->dev = 0;

  /* Query the number of ports the root Hub has.  */
  hub->nports = controller->dev->hubports (controller);
  hub->devices = holy_zalloc (sizeof (hub->devices[0]) * hub->nports);
  hub->ports = holy_zalloc (sizeof (hub->ports[0]) * hub->nports);
  if (!hub->devices || !hub->ports)
    {
      holy_free (hub->devices);
      holy_free (hub->ports);
      holy_free (hub->controller);
      holy_free (hub);
      holy_print_error ();
      return 0;
    }

  return 0;
}

void
holy_usb_controller_dev_unregister (holy_usb_controller_dev_t usb)
{
  holy_usb_controller_dev_t *p, q;

  for (p = &holy_usb_list, q = *p; q; p = &(q->next), q = q->next)
    if (q == usb)
      {
	*p = q->next;
	break;
      }
}

void
holy_usb_controller_dev_register (holy_usb_controller_dev_t usb)
{
  int portno;
  int continue_waiting = 0;
  struct holy_usb_hub *hub;

  usb->next = holy_usb_list;
  holy_usb_list = usb;

  if (usb->iterate)
    usb->iterate (holy_usb_controller_dev_register_iter, usb);

  holy_boot_time ("waiting for stable power on USB root\n");

  while (1)
    {
      for (hub = hubs; hub; hub = hub->next)
	if (hub->controller->dev == usb)
	  {
	    /* Wait for completion of insertion and stable power (USB spec.)
	     * Should be at least 100ms, some devices requires more...
	     * There is also another thing - some devices have worse contacts
	     * and connected signal is unstable for some time - we should handle
	     * it - but prevent deadlock in case when device is too faulty... */
	    for (portno = 0; portno < hub->nports; portno++)
	      {
		holy_usb_speed_t speed;
		int changed = 0;

		speed = hub->controller->dev->detect_dev (hub->controller, portno,
							  &changed);
      
		if (hub->ports[portno].state == PORT_STATE_NORMAL
		    && speed != holy_USB_SPEED_NONE)
		  {
		    hub->ports[portno].soft_limit_time = holy_get_time_ms () + 250;
		    hub->ports[portno].hard_limit_time = hub->ports[portno].soft_limit_time + 1750;
		    hub->ports[portno].state = PORT_STATE_WAITING_FOR_STABLE_POWER;
		    holy_boot_time ("Scheduling stable power wait for port %p:%d",
				    usb, portno);
		    continue_waiting++;
		    continue;
		  }

		if (hub->ports[portno].state == PORT_STATE_WAITING_FOR_STABLE_POWER
		    && speed == holy_USB_SPEED_NONE)
		  {
		    hub->ports[portno].soft_limit_time = holy_get_time_ms () + 250;
		    continue;
		  }
		if (hub->ports[portno].state == PORT_STATE_WAITING_FOR_STABLE_POWER
		    && holy_get_time_ms () > hub->ports[portno].soft_limit_time)
		  {
		    hub->ports[portno].state = PORT_STATE_STABLE_POWER;
		    holy_boot_time ("Got stable power wait for port %p:%d",
				    usb, portno);
		    continue_waiting--;
		    continue;
		  }
		if (hub->ports[portno].state == PORT_STATE_WAITING_FOR_STABLE_POWER
		    && holy_get_time_ms () > hub->ports[portno].hard_limit_time)
		  {
		    hub->ports[portno].state = PORT_STATE_FAILED_DEVICE;
		    continue_waiting--;
		    continue;
		  }
	      }
	  }
      if (!continue_waiting)
	break;
      holy_millisleep (1);
    }

  holy_boot_time ("After the stable power wait on USB root");

  for (hub = hubs; hub; hub = hub->next)
    if (hub->controller->dev == usb)
      for (portno = 0; portno < hub->nports; portno++)
	if (hub->ports[portno].state == PORT_STATE_STABLE_POWER)
	  {
	    holy_usb_speed_t speed;
	    int changed = 0;
	    hub->ports[portno].state = PORT_STATE_NORMAL;
	    speed = hub->controller->dev->detect_dev (hub->controller, portno, &changed);
	    attach_root_port (hub, portno, speed);
	  }

  holy_boot_time ("USB root hub registered");
}

static void detach_device (holy_usb_device_t dev);

static void
detach_device (holy_usb_device_t dev)
{
  unsigned i;
  int k;
  if (!dev)
    return;
  if (dev->descdev.class == holy_USB_CLASS_HUB)
    {
      if (dev->hub_transfer)
	holy_usb_cancel_transfer (dev->hub_transfer);

      for (i = 0; i < dev->nports; i++)
	detach_device (dev->children[i]);
      holy_free (dev->children);
    }
  for (i = 0; i < ARRAY_SIZE (dev->config); i++)
    if (dev->config[i].descconf)
      for (k = 0; k < dev->config[i].descconf->numif; k++)
	{
	  struct holy_usb_interface *inter = &dev->config[i].interf[k];
	  if (inter && inter->detach_hook)
	    inter->detach_hook (dev, i, k);
	}
  holy_usb_devs[dev->addr] = 0;
}

static int
wait_power_nonroot_hub (holy_usb_device_t dev)
{
  holy_usb_err_t err;
  int continue_waiting = 0;
  unsigned i;
  
  for (i = 1; i <= dev->nports; i++)
    if (dev->ports[i - 1].state == PORT_STATE_WAITING_FOR_STABLE_POWER)
      {
	holy_uint64_t tm;
	holy_uint32_t current_status = 0;

	/* Get the port status.  */
	err = holy_usb_control_msg (dev, (holy_USB_REQTYPE_IN
					  | holy_USB_REQTYPE_CLASS
					  | holy_USB_REQTYPE_TARGET_OTHER),
				    holy_USB_REQ_GET_STATUS,
				    0, i,
				    sizeof (current_status),
				    (char *) &current_status);
	if (err)
	  {
	    dev->ports[i - 1].state = PORT_STATE_FAILED_DEVICE;
	    continue;
	  }
	tm = holy_get_time_ms ();
	if (!(current_status & holy_USB_HUB_STATUS_PORT_CONNECTED))
	  dev->ports[i - 1].soft_limit_time = tm + 250;
	if (tm >= dev->ports[i - 1].soft_limit_time)
	  {
	    if (dev->controller.dev->pending_reset)
	      continue;
	    /* Now do reset of port. */
	    holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
					| holy_USB_REQTYPE_CLASS
					| holy_USB_REQTYPE_TARGET_OTHER),
				  holy_USB_REQ_SET_FEATURE,
				  holy_USB_HUB_FEATURE_PORT_RESET,
				  i, 0, 0);
	    dev->ports[i - 1].state = PORT_STATE_NORMAL;
	    holy_boot_time ("Resetting port %p:%d", dev, i - 1);

	    rescan = 1;
	    /* We cannot reset more than one device at the same time !
	     * Resetting more devices together results in very bad
	     * situation: more than one device has default address 0
	     * at the same time !!!
	     * Additionaly, we cannot perform another reset
	     * anywhere on the same OHCI controller until
	     * we will finish addressing of reseted device ! */
	    dev->controller.dev->pending_reset = holy_get_time_ms () + 5000;
	    npending++;
	    continue;
	  }
	if (tm >= dev->ports[i - 1].hard_limit_time)
	  {
	    dev->ports[i - 1].state = PORT_STATE_FAILED_DEVICE;
	    continue;
	  }
	continue_waiting = 1;
      }
  return continue_waiting && dev->controller.dev->pending_reset == 0;
}

static void
poll_nonroot_hub (holy_usb_device_t dev)
{
  holy_usb_err_t err;
  unsigned i;
  holy_uint32_t changed;
  holy_size_t actual, len;

  if (!dev->hub_transfer)
    return;

  err = holy_usb_check_transfer (dev->hub_transfer, &actual);

  if (err == holy_USB_ERR_WAIT)
    return;

  changed = dev->statuschange;

  len = dev->hub_endpoint->maxpacket;
  if (len > sizeof (dev->statuschange))
    len = sizeof (dev->statuschange);
  dev->hub_transfer
    = holy_usb_bulk_read_background (dev, dev->hub_endpoint, len,
				     (char *) &dev->statuschange);

  if (err || actual == 0 || changed == 0)
    return;

  /* Iterate over the Hub ports.  */
  for (i = 1; i <= dev->nports; i++)
    {
      holy_uint32_t status;

      if (!(changed & (1 << i))
	  || dev->ports[i - 1].state == PORT_STATE_WAITING_FOR_STABLE_POWER)
	continue;

      /* Get the port status.  */
      err = holy_usb_control_msg (dev, (holy_USB_REQTYPE_IN
					| holy_USB_REQTYPE_CLASS
					| holy_USB_REQTYPE_TARGET_OTHER),
				  holy_USB_REQ_GET_STATUS,
				  0, i, sizeof (status), (char *) &status);

      holy_dprintf ("usb", "dev = %p, i = %d, status = %08x\n",
                   dev, i, status);

      if (err)
	continue;

      /* FIXME: properly handle these conditions.  */
      if (status & holy_USB_HUB_STATUS_C_PORT_ENABLED)
	holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				    | holy_USB_REQTYPE_CLASS
				    | holy_USB_REQTYPE_TARGET_OTHER),
			      holy_USB_REQ_CLEAR_FEATURE,
			      holy_USB_HUB_FEATURE_C_PORT_ENABLED, i, 0, 0);

      if (status & holy_USB_HUB_STATUS_C_PORT_SUSPEND)
	holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				    | holy_USB_REQTYPE_CLASS
				    | holy_USB_REQTYPE_TARGET_OTHER),
			      holy_USB_REQ_CLEAR_FEATURE,
			      holy_USB_HUB_FEATURE_C_PORT_SUSPEND, i, 0, 0);

      if (status & holy_USB_HUB_STATUS_C_PORT_OVERCURRENT)
	holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				    | holy_USB_REQTYPE_CLASS
				    | holy_USB_REQTYPE_TARGET_OTHER),
			      holy_USB_REQ_CLEAR_FEATURE,
			      holy_USB_HUB_FEATURE_C_PORT_OVERCURRENT, i, 0, 0);

      if (!dev->controller.dev->pending_reset &&
          (status & holy_USB_HUB_STATUS_C_PORT_CONNECTED))
	{
	  holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				      | holy_USB_REQTYPE_CLASS
				      | holy_USB_REQTYPE_TARGET_OTHER),
				holy_USB_REQ_CLEAR_FEATURE,
				holy_USB_HUB_FEATURE_C_PORT_CONNECTED, i, 0, 0);

	  detach_device (dev->children[i - 1]);
	  dev->children[i - 1] = NULL;
      	    
	  /* Connected and status of connection changed ? */
	  if (status & holy_USB_HUB_STATUS_PORT_CONNECTED)
	    {
	      holy_boot_time ("Before the stable power wait portno=%d", i);
	      /* A device is actually connected to this port. */
	      /* Wait for completion of insertion and stable power (USB spec.)
	       * Should be at least 100ms, some devices requires more...
	       * There is also another thing - some devices have worse contacts
	       * and connected signal is unstable for some time - we should handle
	       * it - but prevent deadlock in case when device is too faulty... */
	      dev->ports[i - 1].soft_limit_time = holy_get_time_ms () + 250;
	      dev->ports[i - 1].hard_limit_time = dev->ports[i - 1].soft_limit_time + 1750;
	      dev->ports[i - 1].state = PORT_STATE_WAITING_FOR_STABLE_POWER;
	      holy_boot_time ("Scheduling stable power wait for port %p:%d",
			      dev, i - 1);
	      continue;
	    }
	}

      if (status & holy_USB_HUB_STATUS_C_PORT_RESET)
	{
	  holy_usb_control_msg (dev, (holy_USB_REQTYPE_OUT
				      | holy_USB_REQTYPE_CLASS
				      | holy_USB_REQTYPE_TARGET_OTHER),
				holy_USB_REQ_CLEAR_FEATURE,
				holy_USB_HUB_FEATURE_C_PORT_RESET, i, 0, 0);

	  holy_boot_time ("Port %d reset", i);

	  if (status & holy_USB_HUB_STATUS_PORT_CONNECTED)
	    {
	      holy_usb_speed_t speed;
	      holy_usb_device_t next_dev;
	      int split_hubport = 0;
	      int split_hubaddr = 0;

	      /* Determine the device speed.  */
	      if (status & holy_USB_HUB_STATUS_PORT_LOWSPEED)
		speed = holy_USB_SPEED_LOW;
	      else
		{
		  if (status & holy_USB_HUB_STATUS_PORT_HIGHSPEED)
		    speed = holy_USB_SPEED_HIGH;
		  else
		    speed = holy_USB_SPEED_FULL;
		}

	      /* Wait a recovery time after reset, spec. says 10ms */
	      holy_millisleep (10);

              /* Find correct values for SPLIT hubport and hubaddr */
	      if (speed == holy_USB_SPEED_HIGH)
	        {
		  /* HIGH speed device needs not transaction translation */
		  split_hubport = 0;
		  split_hubaddr = 0;
		}
	      else
	        /* FULL/LOW device needs hub port and hub address
		   for transaction translation (if connected to EHCI) */
	        if (dev->speed == holy_USB_SPEED_HIGH)
	          {
		    /* This port is the first FULL/LOW speed port
		       in the chain from root hub. Attached device
		       should use its port number and hub address */
		    split_hubport = i;
		    split_hubaddr = dev->addr;
		  }
	        else
	          {
		    /* This port is NOT the first FULL/LOW speed port
		       in the chain from root hub. Attached device
		       should use values inherited from some parent
		       HIGH speed hub - if any. */
		    split_hubport = dev->split_hubport;
		    split_hubaddr = dev->split_hubaddr;
		  }
		
	      /* Add the device and assign a device address to it.  */
	      next_dev = holy_usb_hub_add_dev (&dev->controller, speed,
					       split_hubport, split_hubaddr);
	      if (dev->controller.dev->pending_reset)
		{
		  dev->controller.dev->pending_reset = 0;
		  npending--;
		}
	      if (! next_dev)
		continue;

	      dev->children[i - 1] = next_dev;

	      /* If the device is a Hub, scan it for more devices.  */
	      if (next_dev->descdev.class == 0x09)
		holy_usb_add_hub (next_dev);
	    }
	}
    }
}

void
holy_usb_poll_devices (int wait_for_completion)
{
  struct holy_usb_hub *hub;
  int i;

  for (hub = hubs; hub; hub = hub->next)
    {
      /* Do we have to recheck number of ports?  */
      /* No, it should be never changed, it should be constant. */
      for (i = 0; i < hub->nports; i++)
	{
	  holy_usb_speed_t speed = holy_USB_SPEED_NONE;
	  int changed = 0;

          if (hub->controller->dev->pending_reset)
            {
              /* Check for possible timeout */
              if (holy_get_time_ms () > hub->controller->dev->pending_reset)
                {
                  /* Something went wrong, reset device was not
                   * addressed properly, timeout happened */
	          hub->controller->dev->pending_reset = 0;
		  npending--;
                }
            }
          if (!hub->controller->dev->pending_reset)
	    speed = hub->controller->dev->detect_dev (hub->controller,
						      i, &changed);

	  if (changed)
	    {
	      detach_device (hub->devices[i]);
	      hub->devices[i] = NULL;
	      if (speed != holy_USB_SPEED_NONE)
                attach_root_port (hub, i, speed);
	    }
	}
    }

  while (1)
    {
      rescan = 0;
      
      /* We should check changes of non-root hubs too. */
      for (i = 0; i < holy_USBHUB_MAX_DEVICES; i++)
	{
	  holy_usb_device_t dev = holy_usb_devs[i];
	  
	  if (dev && dev->descdev.class == 0x09)
	    poll_nonroot_hub (dev);
	}

      while (1)
	{
	  int continue_waiting = 0;
	  for (i = 0; i < holy_USBHUB_MAX_DEVICES; i++)
	    {
	      holy_usb_device_t dev = holy_usb_devs[i];
	    
	      if (dev && dev->descdev.class == 0x09)
		continue_waiting = continue_waiting || wait_power_nonroot_hub (dev);
	    }
	  if (!continue_waiting)
	    break;
	  holy_millisleep (1);
	}

      if (!(rescan || (npending && wait_for_completion)))
	break;
      holy_millisleep (25);
    }
}

int
holy_usb_iterate (holy_usb_iterate_hook_t hook, void *hook_data)
{
  int i;

  for (i = 0; i < holy_USBHUB_MAX_DEVICES; i++)
    {
      if (holy_usb_devs[i])
	{
	  if (hook (holy_usb_devs[i], hook_data))
	      return 1;
	}
    }

  return 0;
}
