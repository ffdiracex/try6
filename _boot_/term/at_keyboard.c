/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/at_keyboard.h>
#include <holy/cpu/at_keyboard.h>
#include <holy/cpu/io.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/keyboard_layouts.h>
#include <holy/time.h>
#include <holy/loader.h>

holy_MOD_LICENSE ("GPLv2+");

static short at_keyboard_status = 0;
static int e0_received = 0;
static int f0_received = 0;

static holy_uint8_t led_status;

#define KEYBOARD_LED_SCROLL		(1 << 0)
#define KEYBOARD_LED_NUM		(1 << 1)
#define KEYBOARD_LED_CAPS		(1 << 2)

static holy_uint8_t holy_keyboard_controller_orig;
static holy_uint8_t holy_keyboard_orig_set;
static holy_uint8_t current_set;

static void
holy_keyboard_controller_init (void);

static const holy_uint8_t set1_mapping[128] =
  {
    /* 0x00 */ 0 /* Unused  */,               holy_KEYBOARD_KEY_ESCAPE,
    /* 0x02 */ holy_KEYBOARD_KEY_1,           holy_KEYBOARD_KEY_2,
    /* 0x04 */ holy_KEYBOARD_KEY_3,           holy_KEYBOARD_KEY_4,
    /* 0x06 */ holy_KEYBOARD_KEY_5,           holy_KEYBOARD_KEY_6,
    /* 0x08 */ holy_KEYBOARD_KEY_7,           holy_KEYBOARD_KEY_8,
    /* 0x0a */ holy_KEYBOARD_KEY_9,           holy_KEYBOARD_KEY_0,
    /* 0x0c */ holy_KEYBOARD_KEY_DASH,        holy_KEYBOARD_KEY_EQUAL,
    /* 0x0e */ holy_KEYBOARD_KEY_BACKSPACE,   holy_KEYBOARD_KEY_TAB,
    /* 0x10 */ holy_KEYBOARD_KEY_Q,           holy_KEYBOARD_KEY_W,
    /* 0x12 */ holy_KEYBOARD_KEY_E,           holy_KEYBOARD_KEY_R,
    /* 0x14 */ holy_KEYBOARD_KEY_T,           holy_KEYBOARD_KEY_Y,
    /* 0x16 */ holy_KEYBOARD_KEY_U,           holy_KEYBOARD_KEY_I,
    /* 0x18 */ holy_KEYBOARD_KEY_O,           holy_KEYBOARD_KEY_P,
    /* 0x1a */ holy_KEYBOARD_KEY_LBRACKET,    holy_KEYBOARD_KEY_RBRACKET,
    /* 0x1c */ holy_KEYBOARD_KEY_ENTER,       holy_KEYBOARD_KEY_LEFT_CTRL,
    /* 0x1e */ holy_KEYBOARD_KEY_A,           holy_KEYBOARD_KEY_S,
    /* 0x20 */ holy_KEYBOARD_KEY_D,           holy_KEYBOARD_KEY_F,
    /* 0x22 */ holy_KEYBOARD_KEY_G,           holy_KEYBOARD_KEY_H,
    /* 0x24 */ holy_KEYBOARD_KEY_J,           holy_KEYBOARD_KEY_K,
    /* 0x26 */ holy_KEYBOARD_KEY_L,           holy_KEYBOARD_KEY_SEMICOLON,
    /* 0x28 */ holy_KEYBOARD_KEY_DQUOTE,      holy_KEYBOARD_KEY_RQUOTE,
    /* 0x2a */ holy_KEYBOARD_KEY_LEFT_SHIFT,  holy_KEYBOARD_KEY_BACKSLASH,
    /* 0x2c */ holy_KEYBOARD_KEY_Z,           holy_KEYBOARD_KEY_X,
    /* 0x2e */ holy_KEYBOARD_KEY_C,           holy_KEYBOARD_KEY_V,
    /* 0x30 */ holy_KEYBOARD_KEY_B,           holy_KEYBOARD_KEY_N,
    /* 0x32 */ holy_KEYBOARD_KEY_M,           holy_KEYBOARD_KEY_COMMA,
    /* 0x34 */ holy_KEYBOARD_KEY_DOT,         holy_KEYBOARD_KEY_SLASH,
    /* 0x36 */ holy_KEYBOARD_KEY_RIGHT_SHIFT, holy_KEYBOARD_KEY_NUMMUL,
    /* 0x38 */ holy_KEYBOARD_KEY_LEFT_ALT,    holy_KEYBOARD_KEY_SPACE,
    /* 0x3a */ holy_KEYBOARD_KEY_CAPS_LOCK,   holy_KEYBOARD_KEY_F1,
    /* 0x3c */ holy_KEYBOARD_KEY_F2,          holy_KEYBOARD_KEY_F3,
    /* 0x3e */ holy_KEYBOARD_KEY_F4,          holy_KEYBOARD_KEY_F5,
    /* 0x40 */ holy_KEYBOARD_KEY_F6,          holy_KEYBOARD_KEY_F7,
    /* 0x42 */ holy_KEYBOARD_KEY_F8,          holy_KEYBOARD_KEY_F9,
    /* 0x44 */ holy_KEYBOARD_KEY_F10,         holy_KEYBOARD_KEY_NUM_LOCK,
    /* 0x46 */ holy_KEYBOARD_KEY_SCROLL_LOCK, holy_KEYBOARD_KEY_NUM7,
    /* 0x48 */ holy_KEYBOARD_KEY_NUM8,        holy_KEYBOARD_KEY_NUM9,
    /* 0x4a */ holy_KEYBOARD_KEY_NUMMINUS,    holy_KEYBOARD_KEY_NUM4,
    /* 0x4c */ holy_KEYBOARD_KEY_NUM5,        holy_KEYBOARD_KEY_NUM6,
    /* 0x4e */ holy_KEYBOARD_KEY_NUMPLUS,     holy_KEYBOARD_KEY_NUM1,
    /* 0x50 */ holy_KEYBOARD_KEY_NUM2,        holy_KEYBOARD_KEY_NUM3,
    /* 0x52 */ holy_KEYBOARD_KEY_NUM0,        holy_KEYBOARD_KEY_NUMDOT,
    /* 0x54 */ 0,                             0, 
    /* 0x56 */ holy_KEYBOARD_KEY_102ND,       holy_KEYBOARD_KEY_F11,
    /* 0x58 */ holy_KEYBOARD_KEY_F12,         0,
    /* 0x5a */ 0,                             0,
    /* 0x5c */ 0,                             0,
    /* 0x5e */ 0,                             0,
    /* 0x60 */ 0,                             0,
    /* 0x62 */ 0,                             0,
    /* OLPC keys. Just mapped to normal keys.  */
    /* 0x64 */ 0,                             holy_KEYBOARD_KEY_UP,
    /* 0x66 */ holy_KEYBOARD_KEY_DOWN,        holy_KEYBOARD_KEY_LEFT,
    /* 0x68 */ holy_KEYBOARD_KEY_RIGHT,       0,
    /* 0x6a */ 0,                             0,
    /* 0x6c */ 0,                             0,
    /* 0x6e */ 0,                             0,
    /* 0x70 */ 0,                             0,
    /* 0x72 */ 0,                             holy_KEYBOARD_KEY_JP_RO,
    /* 0x74 */ 0,                             0,
    /* 0x76 */ 0,                             0,
    /* 0x78 */ 0,                             0,
    /* 0x7a */ 0,                             0,
    /* 0x7c */ 0,                             holy_KEYBOARD_KEY_JP_YEN,
    /* 0x7e */ holy_KEYBOARD_KEY_KPCOMMA
  };

static const struct
{
  holy_uint8_t from, to;
} set1_e0_mapping[] = 
  {
    {0x1c, holy_KEYBOARD_KEY_NUMENTER},
    {0x1d, holy_KEYBOARD_KEY_RIGHT_CTRL},
    {0x35, holy_KEYBOARD_KEY_NUMSLASH },
    {0x38, holy_KEYBOARD_KEY_RIGHT_ALT},
    {0x47, holy_KEYBOARD_KEY_HOME},
    {0x48, holy_KEYBOARD_KEY_UP},
    {0x49, holy_KEYBOARD_KEY_PPAGE},
    {0x4b, holy_KEYBOARD_KEY_LEFT},
    {0x4d, holy_KEYBOARD_KEY_RIGHT},
    {0x4f, holy_KEYBOARD_KEY_END},
    {0x50, holy_KEYBOARD_KEY_DOWN},
    {0x51, holy_KEYBOARD_KEY_NPAGE},
    {0x52, holy_KEYBOARD_KEY_INSERT},
    {0x53, holy_KEYBOARD_KEY_DELETE},
  };

static const holy_uint8_t set2_mapping[256] =
  {
    /* 0x00 */ 0,                             holy_KEYBOARD_KEY_F9,
    /* 0x02 */ 0,                             holy_KEYBOARD_KEY_F5,
    /* 0x04 */ holy_KEYBOARD_KEY_F3,          holy_KEYBOARD_KEY_F1,
    /* 0x06 */ holy_KEYBOARD_KEY_F2,          holy_KEYBOARD_KEY_F12,
    /* 0x08 */ 0,                             holy_KEYBOARD_KEY_F10,
    /* 0x0a */ holy_KEYBOARD_KEY_F8,          holy_KEYBOARD_KEY_F6,
    /* 0x0c */ holy_KEYBOARD_KEY_F4,          holy_KEYBOARD_KEY_TAB,
    /* 0x0e */ holy_KEYBOARD_KEY_RQUOTE,      0,
    /* 0x10 */ 0,                             holy_KEYBOARD_KEY_LEFT_ALT,
    /* 0x12 */ holy_KEYBOARD_KEY_LEFT_SHIFT,  0,
    /* 0x14 */ holy_KEYBOARD_KEY_LEFT_CTRL,   holy_KEYBOARD_KEY_Q,
    /* 0x16 */ holy_KEYBOARD_KEY_1,           0,
    /* 0x18 */ 0,                             0,
    /* 0x1a */ holy_KEYBOARD_KEY_Z,           holy_KEYBOARD_KEY_S,
    /* 0x1c */ holy_KEYBOARD_KEY_A,           holy_KEYBOARD_KEY_W,
    /* 0x1e */ holy_KEYBOARD_KEY_2,           0,
    /* 0x20 */ 0,                             holy_KEYBOARD_KEY_C,
    /* 0x22 */ holy_KEYBOARD_KEY_X,           holy_KEYBOARD_KEY_D,
    /* 0x24 */ holy_KEYBOARD_KEY_E,           holy_KEYBOARD_KEY_4,
    /* 0x26 */ holy_KEYBOARD_KEY_3,           0,
    /* 0x28 */ 0,                             holy_KEYBOARD_KEY_SPACE,
    /* 0x2a */ holy_KEYBOARD_KEY_V,           holy_KEYBOARD_KEY_F,
    /* 0x2c */ holy_KEYBOARD_KEY_T,           holy_KEYBOARD_KEY_R,
    /* 0x2e */ holy_KEYBOARD_KEY_5,           0,
    /* 0x30 */ 0,                             holy_KEYBOARD_KEY_N,
    /* 0x32 */ holy_KEYBOARD_KEY_B,           holy_KEYBOARD_KEY_H,
    /* 0x34 */ holy_KEYBOARD_KEY_G,           holy_KEYBOARD_KEY_Y,
    /* 0x36 */ holy_KEYBOARD_KEY_6,           0,
    /* 0x38 */ 0,                             0,
    /* 0x3a */ holy_KEYBOARD_KEY_M,           holy_KEYBOARD_KEY_J,
    /* 0x3c */ holy_KEYBOARD_KEY_U,           holy_KEYBOARD_KEY_7,
    /* 0x3e */ holy_KEYBOARD_KEY_8,           0,
    /* 0x40 */ 0,                             holy_KEYBOARD_KEY_COMMA,
    /* 0x42 */ holy_KEYBOARD_KEY_K,           holy_KEYBOARD_KEY_I,
    /* 0x44 */ holy_KEYBOARD_KEY_O,           holy_KEYBOARD_KEY_0,
    /* 0x46 */ holy_KEYBOARD_KEY_9,           0,
    /* 0x48 */ 0,                             holy_KEYBOARD_KEY_DOT,
    /* 0x4a */ holy_KEYBOARD_KEY_SLASH,       holy_KEYBOARD_KEY_L,
    /* 0x4c */ holy_KEYBOARD_KEY_SEMICOLON,   holy_KEYBOARD_KEY_P,
    /* 0x4e */ holy_KEYBOARD_KEY_DASH,        0,
    /* 0x50 */ 0,                             holy_KEYBOARD_KEY_JP_RO,
    /* 0x52 */ holy_KEYBOARD_KEY_DQUOTE,      0,
    /* 0x54 */ holy_KEYBOARD_KEY_LBRACKET,    holy_KEYBOARD_KEY_EQUAL,
    /* 0x56 */ 0,                             0,
    /* 0x58 */ holy_KEYBOARD_KEY_CAPS_LOCK,   holy_KEYBOARD_KEY_RIGHT_SHIFT,
    /* 0x5a */ holy_KEYBOARD_KEY_ENTER,       holy_KEYBOARD_KEY_RBRACKET,
    /* 0x5c */ 0,                             holy_KEYBOARD_KEY_BACKSLASH,
    /* 0x5e */ 0,                             0,
    /* 0x60 */ 0,                             holy_KEYBOARD_KEY_102ND,
    /* 0x62 */ 0,                             0,
    /* 0x64 */ 0,                             0,
    /* 0x66 */ holy_KEYBOARD_KEY_BACKSPACE,   0,
    /* 0x68 */ 0,                             holy_KEYBOARD_KEY_NUM1,
    /* 0x6a */ holy_KEYBOARD_KEY_JP_YEN,      holy_KEYBOARD_KEY_NUM4,
    /* 0x6c */ holy_KEYBOARD_KEY_NUM7,        holy_KEYBOARD_KEY_KPCOMMA,
    /* 0x6e */ 0,                             0,
    /* 0x70 */ holy_KEYBOARD_KEY_NUM0,        holy_KEYBOARD_KEY_NUMDOT,
    /* 0x72 */ holy_KEYBOARD_KEY_NUM2,        holy_KEYBOARD_KEY_NUM5,
    /* 0x74 */ holy_KEYBOARD_KEY_NUM6,        holy_KEYBOARD_KEY_NUM8,
    /* 0x76 */ holy_KEYBOARD_KEY_ESCAPE,      holy_KEYBOARD_KEY_NUM_LOCK,
    /* 0x78 */ holy_KEYBOARD_KEY_F11,         holy_KEYBOARD_KEY_NUMPLUS,
    /* 0x7a */ holy_KEYBOARD_KEY_NUM3,        holy_KEYBOARD_KEY_NUMMINUS,
    /* 0x7c */ holy_KEYBOARD_KEY_NUMMUL,      holy_KEYBOARD_KEY_NUM9,
    /* 0x7e */ holy_KEYBOARD_KEY_SCROLL_LOCK, 0,
    /* 0x80 */ 0,                             0, 
    /* 0x82 */ 0,                             holy_KEYBOARD_KEY_F7,
  };

static const struct
{
  holy_uint8_t from, to;
} set2_e0_mapping[] = 
  {
    {0x11, holy_KEYBOARD_KEY_RIGHT_ALT},
    {0x14, holy_KEYBOARD_KEY_RIGHT_CTRL},
    {0x4a, holy_KEYBOARD_KEY_NUMSLASH},
    {0x5a, holy_KEYBOARD_KEY_NUMENTER},
    {0x69, holy_KEYBOARD_KEY_END},
    {0x6b, holy_KEYBOARD_KEY_LEFT},
    {0x6c, holy_KEYBOARD_KEY_HOME},
    {0x70, holy_KEYBOARD_KEY_INSERT},
    {0x71, holy_KEYBOARD_KEY_DELETE},
    {0x72, holy_KEYBOARD_KEY_DOWN},
    {0x74, holy_KEYBOARD_KEY_RIGHT},
    {0x75, holy_KEYBOARD_KEY_UP},
    {0x7a, holy_KEYBOARD_KEY_NPAGE},
    {0x7d, holy_KEYBOARD_KEY_PPAGE},
  };

static int ping_sent;

static void
keyboard_controller_wait_until_ready (void)
{
  while (! KEYBOARD_COMMAND_ISREADY (holy_inb (KEYBOARD_REG_STATUS)));
}

static holy_uint8_t
wait_ack (void)
{
  holy_uint64_t endtime;
  holy_uint8_t ack;

  endtime = holy_get_time_ms () + 20;
  do
    ack = holy_inb (KEYBOARD_REG_DATA);
  while (ack != holy_AT_ACK && ack != holy_AT_NACK
	 && holy_get_time_ms () < endtime);
  return ack;
}

static int
at_command (holy_uint8_t data)
{
  unsigned i;
  for (i = 0; i < holy_AT_TRIES; i++)
    {
      holy_uint8_t ack;
      keyboard_controller_wait_until_ready ();
      holy_outb (data, KEYBOARD_REG_STATUS);
      ack = wait_ack ();
      if (ack == holy_AT_NACK)
	continue;
      if (ack == holy_AT_ACK)
	break;
      return 0;
    }
  return (i != holy_AT_TRIES);
}

static void
holy_keyboard_controller_write (holy_uint8_t c)
{
  at_command (KEYBOARD_COMMAND_WRITE);
  keyboard_controller_wait_until_ready ();
  holy_outb (c, KEYBOARD_REG_DATA);
}

#if defined (holy_MACHINE_MIPS_LOONGSON) || defined (holy_MACHINE_QEMU) || defined (holy_MACHINE_COREBOOT) || defined (holy_MACHINE_MIPS_QEMU_MIPS)
#define USE_SCANCODE_SET 1
#else
#define USE_SCANCODE_SET 0
#endif

#if !USE_SCANCODE_SET

static holy_uint8_t
holy_keyboard_controller_read (void)
{
  at_command (KEYBOARD_COMMAND_READ);
  keyboard_controller_wait_until_ready ();
  return holy_inb (KEYBOARD_REG_DATA);
}

#endif

static int
write_mode (int mode)
{
  unsigned i;
  for (i = 0; i < holy_AT_TRIES; i++)
    {
      holy_uint8_t ack;
      keyboard_controller_wait_until_ready ();
      holy_outb (0xf0, KEYBOARD_REG_DATA);
      keyboard_controller_wait_until_ready ();
      holy_outb (mode, KEYBOARD_REG_DATA);
      keyboard_controller_wait_until_ready ();
      ack = wait_ack ();
      if (ack == holy_AT_NACK)
	continue;
      if (ack == holy_AT_ACK)
	break;
      return 0;
    }

  return (i != holy_AT_TRIES);
}

static int
query_mode (void)
{
  holy_uint8_t ret;
  int e;

  e = write_mode (0);
  if (!e)
    return 0;

  keyboard_controller_wait_until_ready ();

  do
    ret = holy_inb (KEYBOARD_REG_DATA);
  while (ret == holy_AT_ACK);

  /* QEMU translates the set even in no-translate mode.  */
  if (ret == 0x43 || ret == 1)
    return 1;
  if (ret == 0x41 || ret == 2)
    return 2;
  if (ret == 0x3f || ret == 3)
    return 3;
  return 0;
}

static void
set_scancodes (void)
{
  /* You must have visited computer museum. Keyboard without scancode set
     knowledge. Assume XT. */
  if (!holy_keyboard_orig_set)
    {
      holy_dprintf ("atkeyb", "No sets support assumed\n");
      current_set = 1;
      return;
    }

#if !USE_SCANCODE_SET
  current_set = 1;
  return;
#else

  holy_keyboard_controller_write (holy_keyboard_controller_orig
				  & ~KEYBOARD_AT_TRANSLATE);

  write_mode (2);
  current_set = query_mode ();
  holy_dprintf ("atkeyb", "returned set %d\n", current_set);
  if (current_set == 2)
    return;

  write_mode (1);
  current_set = query_mode ();
  holy_dprintf ("atkeyb", "returned set %d\n", current_set);
  if (current_set == 1)
    return;
  holy_dprintf ("atkeyb", "no supported scancode set found\n");
#endif
}

static void
keyboard_controller_led (holy_uint8_t leds)
{
  keyboard_controller_wait_until_ready ();
  holy_outb (0xed, KEYBOARD_REG_DATA);
  keyboard_controller_wait_until_ready ();
  holy_outb (leds & 0x7, KEYBOARD_REG_DATA);
}

static int
fetch_key (int *is_break)
{
  int was_ext = 0;
  holy_uint8_t at_key;
  int ret = 0;

  if (! KEYBOARD_ISREADY (holy_inb (KEYBOARD_REG_STATUS)))
    return -1;
  at_key = holy_inb (KEYBOARD_REG_DATA);
  /* May happen if no keyboard is connected. Just ignore this.  */
  if (at_key == 0xff)
    return -1;
  if (at_key == 0xe0)
    {
      e0_received = 1;
      return -1;
    }

  if ((current_set == 2 || current_set == 3) && at_key == 0xf0)
    {
      f0_received = 1;
      return -1;
    }

  /* Setting LEDs may generate ACKs.  */
  if (at_key == holy_AT_ACK)
    return -1;

  was_ext = e0_received;
  e0_received = 0;

  switch (current_set)
    {
    case 1:
      *is_break = !!(at_key & 0x80);
      if (!was_ext)
	ret = set1_mapping[at_key & 0x7f];
      else
	{
	  unsigned i;
	  for (i = 0; i < ARRAY_SIZE (set1_e0_mapping); i++)
	    if (set1_e0_mapping[i].from == (at_key & 0x7f))
	      {
		ret = set1_e0_mapping[i].to;
		break;
	      }
	}
      break;
    case 2:
      *is_break = f0_received;
      f0_received = 0;
      if (!was_ext)
	ret = set2_mapping[at_key];
      else
	{
	  unsigned i;
	  for (i = 0; i < ARRAY_SIZE (set2_e0_mapping); i++)
	    if (set2_e0_mapping[i].from == at_key)
	      {
		ret = set2_e0_mapping[i].to;
		break;
	      }
	}	
      break;
    default:
      return -1;
    }
  if (!ret)
    {
      if (was_ext)
	holy_dprintf ("atkeyb", "Unknown key 0xe0+0x%02x from set %d\n",
		      at_key, current_set);
      else
	holy_dprintf ("atkeyb", "Unknown key 0x%02x from set %d\n",
		      at_key, current_set);
      return -1;
    }
  return ret;
}

/* FIXME: This should become an interrupt service routine.  For now
   it's just used to catch events from control keys.  */
static int
holy_keyboard_isr (holy_keyboard_key_t key, int is_break)
{
  if (!is_break)
    switch (key)
      {
      case holy_KEYBOARD_KEY_LEFT_SHIFT:
	at_keyboard_status |= holy_TERM_STATUS_LSHIFT;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_SHIFT:
	at_keyboard_status |= holy_TERM_STATUS_RSHIFT;
	return 1;
      case holy_KEYBOARD_KEY_LEFT_CTRL:
	at_keyboard_status |= holy_TERM_STATUS_LCTRL;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_CTRL:
	at_keyboard_status |= holy_TERM_STATUS_RCTRL;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_ALT:
	at_keyboard_status |= holy_TERM_STATUS_RALT;
	return 1;
      case holy_KEYBOARD_KEY_LEFT_ALT:
	at_keyboard_status |= holy_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
  else
    switch (key)
      {
      case holy_KEYBOARD_KEY_LEFT_SHIFT:
	at_keyboard_status &= ~holy_TERM_STATUS_LSHIFT;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_SHIFT:
	at_keyboard_status &= ~holy_TERM_STATUS_RSHIFT;
	return 1;
      case holy_KEYBOARD_KEY_LEFT_CTRL:
	at_keyboard_status &= ~holy_TERM_STATUS_LCTRL;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_CTRL:
	at_keyboard_status &= ~holy_TERM_STATUS_RCTRL;
	return 1;
      case holy_KEYBOARD_KEY_RIGHT_ALT:
	at_keyboard_status &= ~holy_TERM_STATUS_RALT;
	return 1;
      case holy_KEYBOARD_KEY_LEFT_ALT:
	at_keyboard_status &= ~holy_TERM_STATUS_LALT;
	return 1;
      default:
	return 0;
      }
}

/* If there is a raw key pending, return it; otherwise return -1.  */
static int
holy_keyboard_getkey (void)
{
  int key;
  int is_break = 0;

  key = fetch_key (&is_break);
  if (key == -1)
    return -1;

  if (holy_keyboard_isr (key, is_break))
    return -1;
  if (is_break)
    return -1;
  return key;
}

int
holy_at_keyboard_is_alive (void)
{
  if (current_set != 0)
    return 1;
  if (ping_sent
      && KEYBOARD_COMMAND_ISREADY (holy_inb (KEYBOARD_REG_STATUS))
      && holy_inb (KEYBOARD_REG_DATA) == 0x55)
    {
      holy_keyboard_controller_init ();
      return 1;
    }

  if (KEYBOARD_COMMAND_ISREADY (holy_inb (KEYBOARD_REG_STATUS)))
    {
      holy_outb (0xaa, KEYBOARD_REG_STATUS);
      ping_sent = 1;
    }
  return 0;
}

/* If there is a character pending, return it;
   otherwise return holy_TERM_NO_KEY.  */
static int
holy_at_keyboard_getkey (struct holy_term_input *term __attribute__ ((unused)))
{
  int code;

  if (!holy_at_keyboard_is_alive ())
    return holy_TERM_NO_KEY;

  code = holy_keyboard_getkey ();
  if (code == -1)
    return holy_TERM_NO_KEY;
#ifdef DEBUG_AT_KEYBOARD
  holy_dprintf ("atkeyb", "Detected key 0x%x\n", code);
#endif
  switch (code)
    {
      case holy_KEYBOARD_KEY_CAPS_LOCK:
	at_keyboard_status ^= holy_TERM_STATUS_CAPS;
	led_status ^= KEYBOARD_LED_CAPS;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	holy_dprintf ("atkeyb", "caps_lock = %d\n", !!(at_keyboard_status & holy_KEYBOARD_STATUS_CAPS_LOCK));
#endif
	return holy_TERM_NO_KEY;
      case holy_KEYBOARD_KEY_NUM_LOCK:
	at_keyboard_status ^= holy_TERM_STATUS_NUM;
	led_status ^= KEYBOARD_LED_NUM;
	keyboard_controller_led (led_status);

#ifdef DEBUG_AT_KEYBOARD
	holy_dprintf ("atkeyb", "num_lock = %d\n", !!(at_keyboard_status & holy_KEYBOARD_STATUS_NUM_LOCK));
#endif
	return holy_TERM_NO_KEY;
      case holy_KEYBOARD_KEY_SCROLL_LOCK:
	at_keyboard_status ^= holy_TERM_STATUS_SCROLL;
	led_status ^= KEYBOARD_LED_SCROLL;
	keyboard_controller_led (led_status);
	return holy_TERM_NO_KEY;
      default:
	return holy_term_map_key (code, at_keyboard_status);
    }
}

static void
holy_keyboard_controller_init (void)
{
  at_keyboard_status = 0;
  /* Drain input buffer. */
  while (1)
    {
      keyboard_controller_wait_until_ready ();
      if (! KEYBOARD_ISREADY (holy_inb (KEYBOARD_REG_STATUS)))
	break;
      keyboard_controller_wait_until_ready ();
      holy_inb (KEYBOARD_REG_DATA);
    }
#if defined (holy_MACHINE_MIPS_LOONGSON) || defined (holy_MACHINE_MIPS_QEMU_MIPS)
  holy_keyboard_controller_orig = 0;
  holy_keyboard_orig_set = 2;
#elif defined (holy_MACHINE_QEMU) || defined (holy_MACHINE_COREBOOT)
  /* *BSD relies on those settings.  */
  holy_keyboard_controller_orig = KEYBOARD_AT_TRANSLATE;
  holy_keyboard_orig_set = 2;
#else
  holy_keyboard_controller_orig = holy_keyboard_controller_read ();
  holy_keyboard_orig_set = query_mode ();
#endif
  set_scancodes ();
  keyboard_controller_led (led_status);
}

static holy_err_t
holy_keyboard_controller_fini (struct holy_term_input *term __attribute__ ((unused)))
{
  if (current_set == 0)
    return holy_ERR_NONE;
  if (holy_keyboard_orig_set)
    write_mode (holy_keyboard_orig_set);
  holy_keyboard_controller_write (holy_keyboard_controller_orig);
  return holy_ERR_NONE;
}

static holy_err_t
holy_at_fini_hw (int noreturn __attribute__ ((unused)))
{
  return holy_keyboard_controller_fini (NULL);
}

static holy_err_t
holy_at_restore_hw (void)
{
  if (current_set == 0)
    return holy_ERR_NONE;

  /* Drain input buffer. */
  while (1)
    {
      keyboard_controller_wait_until_ready ();
      if (! KEYBOARD_ISREADY (holy_inb (KEYBOARD_REG_STATUS)))
	break;
      keyboard_controller_wait_until_ready ();
      holy_inb (KEYBOARD_REG_DATA);
    }
  set_scancodes ();
  keyboard_controller_led (led_status);

  return holy_ERR_NONE;
}


static struct holy_term_input holy_at_keyboard_term =
  {
    .name = "at_keyboard",
    .fini = holy_keyboard_controller_fini,
    .getkey = holy_at_keyboard_getkey
  };

holy_MOD_INIT(at_keyboard)
{
  holy_term_register_input ("at_keyboard", &holy_at_keyboard_term);
  holy_loader_register_preboot_hook (holy_at_fini_hw, holy_at_restore_hw,
				     holy_LOADER_PREBOOT_HOOK_PRIO_CONSOLE);
}

holy_MOD_FINI(at_keyboard)
{
  holy_keyboard_controller_fini (NULL);
  holy_term_unregister_input (&holy_at_keyboard_term);
}
