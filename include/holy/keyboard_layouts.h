/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_KEYBOARD_LAYOUTS_H
#define holy_KEYBOARD_LAYOUTS_H 1

#define holy_KEYBOARD_LAYOUTS_FILEMAGIC "holyLAYO"
#define holy_KEYBOARD_LAYOUTS_FILEMAGIC_SIZE (sizeof(holy_KEYBOARD_LAYOUTS_FILEMAGIC) - 1)
#define holy_KEYBOARD_LAYOUTS_VERSION 10

#define holy_KEYBOARD_LAYOUTS_ARRAY_SIZE 160

struct holy_keyboard_layout
{
  holy_uint32_t keyboard_map[holy_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  holy_uint32_t keyboard_map_shift[holy_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  holy_uint32_t keyboard_map_l3[holy_KEYBOARD_LAYOUTS_ARRAY_SIZE];
  holy_uint32_t keyboard_map_shift_l3[holy_KEYBOARD_LAYOUTS_ARRAY_SIZE];
};

typedef enum holy_keyboard_key
  {
    holy_KEYBOARD_KEY_A = 0x04,
    holy_KEYBOARD_KEY_B = 0x05,
    holy_KEYBOARD_KEY_C = 0x06,
    holy_KEYBOARD_KEY_D = 0x07,
    holy_KEYBOARD_KEY_E = 0x08,
    holy_KEYBOARD_KEY_F = 0x09,
    holy_KEYBOARD_KEY_G = 0x0a,
    holy_KEYBOARD_KEY_H = 0x0b,
    holy_KEYBOARD_KEY_I = 0x0c,
    holy_KEYBOARD_KEY_J = 0x0d,
    holy_KEYBOARD_KEY_K = 0x0e,
    holy_KEYBOARD_KEY_L = 0x0f,
    holy_KEYBOARD_KEY_M = 0x10,
    holy_KEYBOARD_KEY_N = 0x11,
    holy_KEYBOARD_KEY_O = 0x12,
    holy_KEYBOARD_KEY_P = 0x13,
    holy_KEYBOARD_KEY_Q = 0x14,
    holy_KEYBOARD_KEY_R = 0x15,
    holy_KEYBOARD_KEY_S = 0x16,
    holy_KEYBOARD_KEY_T = 0x17,
    holy_KEYBOARD_KEY_U = 0x18,
    holy_KEYBOARD_KEY_V = 0x19,
    holy_KEYBOARD_KEY_W = 0x1a,
    holy_KEYBOARD_KEY_X = 0x1b,
    holy_KEYBOARD_KEY_Y = 0x1c,
    holy_KEYBOARD_KEY_Z = 0x1d,
    holy_KEYBOARD_KEY_1 = 0x1e,
    holy_KEYBOARD_KEY_2 = 0x1f,
    holy_KEYBOARD_KEY_3 = 0x20,
    holy_KEYBOARD_KEY_4 = 0x21,
    holy_KEYBOARD_KEY_5 = 0x22,
    holy_KEYBOARD_KEY_6 = 0x23,
    holy_KEYBOARD_KEY_7 = 0x24,
    holy_KEYBOARD_KEY_8 = 0x25,
    holy_KEYBOARD_KEY_9 = 0x26,
    holy_KEYBOARD_KEY_0 = 0x27,
    holy_KEYBOARD_KEY_ENTER = 0x28,
    holy_KEYBOARD_KEY_ESCAPE = 0x29,
    holy_KEYBOARD_KEY_BACKSPACE = 0x2a,
    holy_KEYBOARD_KEY_TAB = 0x2b,
    holy_KEYBOARD_KEY_SPACE = 0x2c,
    holy_KEYBOARD_KEY_DASH = 0x2d,
    holy_KEYBOARD_KEY_EQUAL = 0x2e,
    holy_KEYBOARD_KEY_LBRACKET = 0x2f,
    holy_KEYBOARD_KEY_RBRACKET = 0x30,
    holy_KEYBOARD_KEY_BACKSLASH = 0x32,
    holy_KEYBOARD_KEY_SEMICOLON = 0x33,
    holy_KEYBOARD_KEY_DQUOTE = 0x34,
    holy_KEYBOARD_KEY_RQUOTE = 0x35,
    holy_KEYBOARD_KEY_COMMA = 0x36,
    holy_KEYBOARD_KEY_DOT = 0x37,
    holy_KEYBOARD_KEY_SLASH = 0x38,
    holy_KEYBOARD_KEY_CAPS_LOCK  = 0x39,
    holy_KEYBOARD_KEY_F1 = 0x3a,
    holy_KEYBOARD_KEY_F2 = 0x3b,
    holy_KEYBOARD_KEY_F3 = 0x3c,
    holy_KEYBOARD_KEY_F4 = 0x3d,
    holy_KEYBOARD_KEY_F5 = 0x3e,
    holy_KEYBOARD_KEY_F6 = 0x3f,
    holy_KEYBOARD_KEY_F7 = 0x40,
    holy_KEYBOARD_KEY_F8 = 0x41,
    holy_KEYBOARD_KEY_F9 = 0x42,
    holy_KEYBOARD_KEY_F10 = 0x43,
    holy_KEYBOARD_KEY_F11 = 0x44,
    holy_KEYBOARD_KEY_F12 = 0x45,
    holy_KEYBOARD_KEY_SCROLL_LOCK  = 0x47,
    holy_KEYBOARD_KEY_INSERT = 0x49,
    holy_KEYBOARD_KEY_HOME = 0x4a,
    holy_KEYBOARD_KEY_PPAGE = 0x4b,
    holy_KEYBOARD_KEY_DELETE = 0x4c,
    holy_KEYBOARD_KEY_END = 0x4d,
    holy_KEYBOARD_KEY_NPAGE = 0x4e,
    holy_KEYBOARD_KEY_RIGHT = 0x4f,
    holy_KEYBOARD_KEY_LEFT = 0x50,
    holy_KEYBOARD_KEY_DOWN = 0x51,
    holy_KEYBOARD_KEY_UP = 0x52,
    holy_KEYBOARD_KEY_NUM_LOCK = 0x53,
    holy_KEYBOARD_KEY_NUMSLASH = 0x54,
    holy_KEYBOARD_KEY_NUMMUL = 0x55,
    holy_KEYBOARD_KEY_NUMMINUS = 0x56,
    holy_KEYBOARD_KEY_NUMPLUS = 0x57,
    holy_KEYBOARD_KEY_NUMENTER = 0x58,
    holy_KEYBOARD_KEY_NUM1 = 0x59,
    holy_KEYBOARD_KEY_NUM2 = 0x5a,
    holy_KEYBOARD_KEY_NUM3 = 0x5b,
    holy_KEYBOARD_KEY_NUM4 = 0x5c,
    holy_KEYBOARD_KEY_NUM5 = 0x5d,
    holy_KEYBOARD_KEY_NUM6 = 0x5e,
    holy_KEYBOARD_KEY_NUM7 = 0x5f,
    holy_KEYBOARD_KEY_NUM8 = 0x60,
    holy_KEYBOARD_KEY_NUM9 = 0x61,
    holy_KEYBOARD_KEY_NUM0 = 0x62,
    holy_KEYBOARD_KEY_NUMDOT = 0x63,
    holy_KEYBOARD_KEY_102ND = 0x64,
    holy_KEYBOARD_KEY_KPCOMMA = 0x85,
    holy_KEYBOARD_KEY_JP_RO = 0x87,
    holy_KEYBOARD_KEY_JP_YEN = 0x89,
    holy_KEYBOARD_KEY_LEFT_CTRL = 0xe0,
    holy_KEYBOARD_KEY_LEFT_SHIFT = 0xe1,
    holy_KEYBOARD_KEY_LEFT_ALT = 0xe2,
    holy_KEYBOARD_KEY_RIGHT_CTRL = 0xe4,
    holy_KEYBOARD_KEY_RIGHT_SHIFT = 0xe5,
    holy_KEYBOARD_KEY_RIGHT_ALT = 0xe6,
  } holy_keyboard_key_t;

unsigned EXPORT_FUNC(holy_term_map_key) (holy_keyboard_key_t code, int status);

#ifndef holy_MACHINE_EMU
extern void holy_keylayouts_init (void);
extern void holy_keylayouts_fini (void);
#endif

#endif /* holy_KEYBOARD_LAYOUTS  */
