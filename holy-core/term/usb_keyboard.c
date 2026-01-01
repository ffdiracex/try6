/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/time.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/usb.h>
#include <holy/dl.h>
#include <holy/time.h>
#include <holy/keyboard_layouts.h>

holy_MOD_LICENSE ("GPLv2+");



enum
  {
    KEY_NO_KEY = 0x00,
    KEY_ERR_BUFFER = 0x01,
    KEY_ERR_POST  = 0x02,
    KEY_ERR_UNDEF = 0x03,
    KEY_CAPS_LOCK = 0x39,
    KEY_NUM_LOCK  = 0x53,
  };

enum
  {
    LED_NUM_LOCK = 0x01,
    LED_CAPS_LOCK = 0x02
  };

/* Valid values for bRequest.  See HID definition version 1.11 section 7.2. */
#define USB_HID_GET_REPORT	0x01
#define USB_HID_GET_IDLE	0x02
#define USB_HID_GET_PROTOCOL	0x03
#define USB_HID_SET_REPORT	0x09
#define USB_HID_SET_IDLE	0x0A
#define USB_HID_SET_PROTOCOL	0x0B

#define USB_HID_BOOT_SUBCLASS	0x01
#define USB_HID_KBD_PROTOCOL	0x01

#define holy_USB_KEYBOARD_LEFT_CTRL   0x01
#define holy_USB_KEYBOARD_LEFT_SHIFT  0x02
#define holy_USB_KEYBOARD_LEFT_ALT    0x04
#define holy_USB_KEYBOARD_RIGHT_CTRL  0x10
#define holy_USB_KEYBOARD_RIGHT_SHIFT 0x20
#define holy_USB_KEYBOARD_RIGHT_ALT   0x40

struct holy_usb_keyboard_data
{
  holy_usb_device_t usbdev;
  holy_uint8_t status;
  holy_uint16_t mods;
  int interfno;
  struct holy_usb_desc_endp *endp;
  holy_usb_transfer_t transfer;
  holy_uint8_t report[8];
  int dead;
  int last_key;
  holy_uint64_t repeat_time;
  holy_uint8_t current_report[8];
  holy_uint8_t last_report[8];
  int index;
  int max_index;
};

static int holy_usb_keyboard_getkey (struct holy_term_input *term);
static int holy_usb_keyboard_getkeystatus (struct holy_term_input *term);

static struct holy_term_input holy_usb_keyboard_term =
  {
    .getkey = holy_usb_keyboard_getkey,
    .getkeystatus = holy_usb_keyboard_getkeystatus,
    .next = 0
  };

static struct holy_term_input holy_usb_keyboards[16];

static int
interpret_status (holy_uint8_t data0)
{
  int mods = 0;

  /* Check Shift, Control, and Alt status.  */
  if (data0 & holy_USB_KEYBOARD_LEFT_SHIFT)
    mods |= holy_TERM_STATUS_LSHIFT;
  if (data0 & holy_USB_KEYBOARD_RIGHT_SHIFT)
    mods |= holy_TERM_STATUS_RSHIFT;
  if (data0 & holy_USB_KEYBOARD_LEFT_CTRL)
    mods |= holy_TERM_STATUS_LCTRL;
  if (data0 & holy_USB_KEYBOARD_RIGHT_CTRL)
    mods |= holy_TERM_STATUS_RCTRL;
  if (data0 & holy_USB_KEYBOARD_LEFT_ALT)
    mods |= holy_TERM_STATUS_LALT;
  if (data0 & holy_USB_KEYBOARD_RIGHT_ALT)
    mods |= holy_TERM_STATUS_RALT;

  return mods;
}

static void
holy_usb_keyboard_detach (holy_usb_device_t usbdev,
			  int config __attribute__ ((unused)),
			  int interface __attribute__ ((unused)))
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (holy_usb_keyboards); i++)
    {
      struct holy_usb_keyboard_data *data = holy_usb_keyboards[i].data;

      if (!data)
	continue;

      if (data->usbdev != usbdev)
	continue;

      if (data->transfer)
	holy_usb_cancel_transfer (data->transfer);

      holy_term_unregister_input (&holy_usb_keyboards[i]);
      holy_free ((char *) holy_usb_keyboards[i].name);
      holy_usb_keyboards[i].name = NULL;
      holy_free (holy_usb_keyboards[i].data);
      holy_usb_keyboards[i].data = 0;
    }
}

static int
holy_usb_keyboard_attach (holy_usb_device_t usbdev, int configno, int interfno)
{
  unsigned curnum;
  struct holy_usb_keyboard_data *data;
  struct holy_usb_desc_endp *endp = NULL;
  int j;

  holy_dprintf ("usb_keyboard", "%x %x %x %d %d\n",
		usbdev->descdev.class, usbdev->descdev.subclass,
		usbdev->descdev.protocol, configno, interfno);

  for (curnum = 0; curnum < ARRAY_SIZE (holy_usb_keyboards); curnum++)
    if (!holy_usb_keyboards[curnum].data)
      break;

  if (curnum == ARRAY_SIZE (holy_usb_keyboards))
    return 0;

  if (usbdev->descdev.class != 0 
      || usbdev->descdev.subclass != 0 || usbdev->descdev.protocol != 0)
    return 0;

  if (usbdev->config[configno].interf[interfno].descif->subclass
      != USB_HID_BOOT_SUBCLASS
      || usbdev->config[configno].interf[interfno].descif->protocol
      != USB_HID_KBD_PROTOCOL)
    return 0;

  for (j = 0; j < usbdev->config[configno].interf[interfno].descif->endpointcnt;
       j++)
    {
      endp = &usbdev->config[configno].interf[interfno].descendp[j];

      if ((endp->endp_addr & 128) && holy_usb_get_ep_type(endp)
	  == holy_USB_EP_INTERRUPT)
	break;
    }
  if (j == usbdev->config[configno].interf[interfno].descif->endpointcnt)
    return 0;

  holy_dprintf ("usb_keyboard", "HID found!\n");

  data = holy_malloc (sizeof (*data));
  if (!data)
    {
      holy_print_error ();
      return 0;
    }

  data->usbdev = usbdev;
  data->interfno = interfno;
  data->endp = endp;

  /* Configure device */
  holy_usb_set_configuration (usbdev, configno + 1);
  
  /* Place the device in boot mode.  */
  holy_usb_control_msg (usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_PROTOCOL, 0, interfno, 0, 0);

  /* Reports every time an event occurs and not more often than that.  */
  holy_usb_control_msg (usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
  			USB_HID_SET_IDLE, 0<<8, interfno, 0, 0);

  holy_memcpy (&holy_usb_keyboards[curnum], &holy_usb_keyboard_term,
	       sizeof (holy_usb_keyboards[curnum]));
  holy_usb_keyboards[curnum].data = data;
  usbdev->config[configno].interf[interfno].detach_hook
    = holy_usb_keyboard_detach;
  holy_usb_keyboards[curnum].name = holy_xasprintf ("usb_keyboard%d", curnum);
  if (!holy_usb_keyboards[curnum].name)
    {
      holy_print_error ();
      return 0;
    }

  /* Test showed that getting report may make the keyboard go nuts.
     Moreover since we're reattaching keyboard it usually sends
     an initial message on interrupt pipe and so we retrieve
     the same keystatus.
   */
#if 0
  {
    holy_uint8_t report[8];
    holy_usb_err_t err;
    holy_memset (report, 0, sizeof (report));
    err = holy_usb_control_msg (usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_IN,
    				USB_HID_GET_REPORT, 0x0100, interfno,
				sizeof (report), (char *) report);
    if (err)
      data->status = 0;
    else
      data->status = report[0];
  }
#else
  data->status = 0;
#endif

  data->transfer = holy_usb_bulk_read_background (usbdev,
						  data->endp,
						  sizeof (data->report),
						  (char *) data->report);
  if (!data->transfer)
    {
      holy_print_error ();
      return 0;
    }

  data->last_key = -1;
  data->mods = 0;
  data->dead = 0;

  holy_term_register_input_active ("usb_keyboard", &holy_usb_keyboards[curnum]);

  return 1;
}



static void
send_leds (struct holy_usb_keyboard_data *termdata)
{
  char report[1];
  report[0] = 0;
  if (termdata->mods & holy_TERM_STATUS_CAPS)
    report[0] |= LED_CAPS_LOCK;
  if (termdata->mods & holy_TERM_STATUS_NUM)
    report[0] |= LED_NUM_LOCK;
  holy_usb_control_msg (termdata->usbdev, holy_USB_REQTYPE_CLASS_INTERFACE_OUT,
			USB_HID_SET_REPORT, 0x0200, termdata->interfno,
			sizeof (report), (char *) report);
  holy_errno = holy_ERR_NONE;
}

static int
parse_keycode (struct holy_usb_keyboard_data *termdata)
{
  int index = termdata->index;
  int i, keycode;

  /* Sanity check */
  if (index < 2)
    index = 2;

  for ( ; index < termdata->max_index; index++)
    {
      keycode = termdata->current_report[index];
      
      if (keycode == KEY_NO_KEY
          || keycode == KEY_ERR_BUFFER
          || keycode == KEY_ERR_POST
          || keycode == KEY_ERR_UNDEF)
        {
          /* Don't parse (rest of) this report */
          termdata->index = 0;
          if (keycode != KEY_NO_KEY)
          /* Don't replace last report with current faulty report
           * in future ! */
            holy_memcpy (termdata->current_report,
                         termdata->last_report,
                         sizeof (termdata->report));
          return holy_TERM_NO_KEY;
        }

      /* Try to find current keycode in last report. */
      for (i = 2; i < 8; i++)
        if (keycode == termdata->last_report[i])
          break;
      if (i < 8)
        /* Keycode is in last report, it means it was not released,
         * ignore it. */
        continue;
        
      if (keycode == KEY_CAPS_LOCK)
        {
          termdata->mods ^= holy_TERM_STATUS_CAPS;
          send_leds (termdata);
          continue;
        }

      if (keycode == KEY_NUM_LOCK)
        {
          termdata->mods ^= holy_TERM_STATUS_NUM;
          send_leds (termdata);
          continue;
        }

      termdata->last_key = holy_term_map_key (keycode,
                                              interpret_status (termdata->current_report[0])
					        | termdata->mods);
      termdata->repeat_time = holy_get_time_ms () + holy_TERM_REPEAT_PRE_INTERVAL;

      holy_errno = holy_ERR_NONE;

      index++;
      if (index >= termdata->max_index)
        termdata->index = 0;
      else
        termdata->index = index;

      return termdata->last_key;
    }

  /* All keycodes parsed */
  termdata->index = 0;
  return holy_TERM_NO_KEY;
}

static int
holy_usb_keyboard_getkey (struct holy_term_input *term)
{
  holy_usb_err_t err;
  struct holy_usb_keyboard_data *termdata = term->data;
  holy_size_t actual;
  int keycode = holy_TERM_NO_KEY;

  if (termdata->dead)
    return holy_TERM_NO_KEY;

  if (termdata->index)
    keycode = parse_keycode (termdata);
  if (keycode != holy_TERM_NO_KEY)
    return keycode;
    
  /* Poll interrupt pipe.  */
  err = holy_usb_check_transfer (termdata->transfer, &actual);

  if (err == holy_USB_ERR_WAIT)
    {
      if (termdata->last_key != -1
	  && holy_get_time_ms () > termdata->repeat_time)
	{
	  termdata->repeat_time = holy_get_time_ms ()
	    + holy_TERM_REPEAT_INTERVAL;
	  return termdata->last_key;
	}
      return holy_TERM_NO_KEY;
    }

  if (!err && (actual >= 3))
    holy_memcpy (termdata->last_report,
                 termdata->current_report,
                 sizeof (termdata->report));
                 
  holy_memcpy (termdata->current_report,
               termdata->report,
               sizeof (termdata->report));

  termdata->transfer = holy_usb_bulk_read_background (termdata->usbdev,
						      termdata->endp,
						      sizeof (termdata->report),
						      (char *) termdata->report);
  if (!termdata->transfer)
    {
      holy_printf ("%s failed. Stopped\n", term->name);
      termdata->dead = 1;
    }

  termdata->last_key = -1;

  holy_dprintf ("usb_keyboard",
		"err = %d, actual = %" PRIuholy_SIZE
		" report: 0x%02x 0x%02x 0x%02x 0x%02x"
		" 0x%02x 0x%02x 0x%02x 0x%02x\n",
		err, actual,
		termdata->current_report[0], termdata->current_report[1],
		termdata->current_report[2], termdata->current_report[3],
		termdata->current_report[4], termdata->current_report[5],
		termdata->current_report[6], termdata->current_report[7]);

  if (err || actual < 1)
    return holy_TERM_NO_KEY;

  termdata->status = termdata->current_report[0];

  if (actual < 3)
    return holy_TERM_NO_KEY;

  termdata->index = 2; /* New data received. */
  termdata->max_index = actual;
  
  return parse_keycode (termdata);
}

static int
holy_usb_keyboard_getkeystatus (struct holy_term_input *term)
{
  struct holy_usb_keyboard_data *termdata = term->data;

  return interpret_status (termdata->status) | termdata->mods;
}

static struct holy_usb_attach_desc attach_hook =
{
  .class = holy_USB_CLASS_HID,
  .hook = holy_usb_keyboard_attach
};

holy_MOD_INIT(usb_keyboard)
{
  holy_usb_register_attach_hook_class (&attach_hook);
}

holy_MOD_FINI(usb_keyboard)
{
  unsigned i;
  for (i = 0; i < ARRAY_SIZE (holy_usb_keyboards); i++)
    if (holy_usb_keyboards[i].data)
      {
	struct holy_usb_keyboard_data *data = holy_usb_keyboards[i].data;

	if (!data)
	  continue;
	
	if (data->transfer)
	  holy_usb_cancel_transfer (data->transfer);

	holy_term_unregister_input (&holy_usb_keyboards[i]);
	holy_free ((char *) holy_usb_keyboards[i].name);
	holy_usb_keyboards[i].name = NULL;
	holy_free (holy_usb_keyboards[i].data);
	holy_usb_keyboards[i].data = 0;
      }
  holy_usb_unregister_attach_hook_class (&attach_hook);
}
