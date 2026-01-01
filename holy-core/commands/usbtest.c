/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/charset.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/usb.h>
#include <holy/command.h>
#include <holy/i18n.h>

holy_MOD_LICENSE ("GPLv2+");

static const char *usb_classes[] =
  {
    "Unknown",
    "Audio",
    "Communication Interface",
    "HID",
    "Unknown",
    "Physical",
    "Image",
    "Printer",
    "Mass Storage",
    "Hub",
    "Data Interface",
    "Smart Card",
    "Content Security",
    "Video"
  };

static const char *usb_endp_type[] =
  {
    "Control",
    "Isochronous",
    "Bulk",
    "Interrupt"
  };

static const char *usb_devspeed[] =
  {
    "",
    "Low",
    "Full",
    "High"
  };

static holy_usb_err_t
holy_usb_get_string (holy_usb_device_t dev, holy_uint8_t index, int langid,
		     char **string)
{
  struct holy_usb_desc_str descstr;
  struct holy_usb_desc_str *descstrp;
  holy_usb_err_t err;

  /* Only get the length.  */
  err = holy_usb_control_msg (dev, 1 << 7,
			      0x06, (3 << 8) | index,
			      langid, 1, (char *) &descstr);
  if (err)
    return err;

  descstrp = holy_malloc (descstr.length);
  if (! descstrp)
    return holy_USB_ERR_INTERNAL;
  err = holy_usb_control_msg (dev, 1 << 7,
			      0x06, (3 << 8) | index,
			      langid, descstr.length, (char *) descstrp);

  if (descstrp->length == 0)
    {
      holy_free (descstrp);
      *string = holy_strdup ("");
      if (! *string)
	return holy_USB_ERR_INTERNAL;
      return holy_USB_ERR_NONE;
    }

  *string = holy_malloc (descstr.length * 2 + 1);
  if (! *string)
    {
      holy_free (descstrp);
      return holy_USB_ERR_INTERNAL;
    }

  *holy_utf16_to_utf8 ((holy_uint8_t *) *string, descstrp->str,
		       descstrp->length / 2 - 1) = 0;
  holy_free (descstrp);

  return holy_USB_ERR_NONE;
}

static void
usb_print_str (const char *description, holy_usb_device_t dev, int idx)
{
  char *name = NULL;
  holy_usb_err_t err;
  /* XXX: LANGID  */

  if (! idx)
    return;

  err = holy_usb_get_string (dev, idx, 0x0409, &name);
  if (err)
    holy_printf ("Error %d retrieving %s\n", err, description);
  else
    {
      holy_printf ("%s: `%s'\n", description, name);
      holy_free (name);
    }
}

static int
usb_iterate (holy_usb_device_t dev, void *data __attribute__ ((unused)))
{
  struct holy_usb_desc_device *descdev;
  int i;

  descdev = &dev->descdev;

  usb_print_str ("Product", dev, descdev->strprod);
  usb_print_str ("Vendor", dev, descdev->strvendor);
  usb_print_str ("Serial", dev, descdev->strserial);

  holy_printf ("Class: (0x%02x) %s, Subclass: 0x%02x, Protocol: 0x%02x\n",
	       descdev->class, descdev->class < ARRAY_SIZE (usb_classes)
	       ? usb_classes[descdev->class] : "Unknown",
	       descdev->subclass, descdev->protocol);
  holy_printf ("USB version %d.%d, VendorID: 0x%02x, ProductID: 0x%02x, #conf: %d\n",
	       descdev->usbrel >> 8, (descdev->usbrel >> 4) & 0x0F,
	       descdev->vendorid, descdev->prodid, descdev->configcnt);

  holy_printf ("%s speed device\n", usb_devspeed[dev->speed]);

  for (i = 0; i < descdev->configcnt; i++)
    {
      struct holy_usb_desc_config *config;

      config = dev->config[i].descconf;
      usb_print_str ("Configuration:", dev, config->strconfig);
    }

  for (i = 0; i < dev->config[0].descconf->numif; i++)
    {
      int j;
      struct holy_usb_desc_if *interf;
      interf = dev->config[0].interf[i].descif;

      holy_printf ("Interface #%d: #Endpoints: %d   ",
		   i, interf->endpointcnt);
      holy_printf ("Class: (0x%02x) %s, Subclass: 0x%02x, Protocol: 0x%02x\n",
		   interf->class, interf->class < ARRAY_SIZE (usb_classes)
		   ? usb_classes[interf->class] : "Unknown",
		   interf->subclass, interf->protocol);

      usb_print_str ("Interface", dev, interf->strif);

      for (j = 0; j < interf->endpointcnt; j++)
	{
	  struct holy_usb_desc_endp *endp;
	  endp = &dev->config[0].interf[i].descendp[j];

	  holy_printf ("Endpoint #%d: %s, max packed size: %d, transfer type: %s, latency: %d\n",
		       endp->endp_addr & 15,
		       (endp->endp_addr & 128) ? "IN" : "OUT",
		       endp->maxpacket, usb_endp_type[endp->attrib & 3],
		       endp->interval);
	}
    }

  holy_printf("\n");

  return 0;
}

static holy_err_t
holy_cmd_usbtest (holy_command_t cmd __attribute__ ((unused)),
		  int argc __attribute__ ((unused)),
		  char **args __attribute__ ((unused)))
{
  holy_usb_poll_devices (1);

  holy_printf ("USB devices:\n\n");
  holy_usb_iterate (usb_iterate, NULL);

  return 0;
}

static holy_command_t cmd;

holy_MOD_INIT(usbtest)
{
  cmd = holy_register_command ("usb", holy_cmd_usbtest,
			       0, N_("Test USB support."));
}

holy_MOD_FINI(usbtest)
{
  holy_unregister_command (cmd);
}
