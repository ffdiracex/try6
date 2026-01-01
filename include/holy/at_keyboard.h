/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_AT_KEYBOARD_HEADER
#define holy_AT_KEYBOARD_HEADER	1

/* Used for sending commands to the controller.  */
#define KEYBOARD_COMMAND_ISREADY(x)	!((x) & 0x02)
#define KEYBOARD_COMMAND_READ		0x20
#define KEYBOARD_COMMAND_WRITE		0x60
#define KEYBOARD_COMMAND_REBOOT		0xfe

#define KEYBOARD_AT_TRANSLATE		0x40

#define holy_AT_ACK                     0xfa
#define holy_AT_NACK                    0xfe
#define holy_AT_TRIES                   5

#define KEYBOARD_ISMAKE(x)	!((x) & 0x80)
#define KEYBOARD_ISREADY(x)	((x) & 0x01)
#define KEYBOARD_SCANCODE(x)	((x) & 0x7f)

extern void holy_at_keyboard_init (void);
extern void holy_at_keyboard_fini (void);
int holy_at_keyboard_is_alive (void);

#endif
