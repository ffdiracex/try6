/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_TERM_HEADER
#define holy_TERM_HEADER	1

#define holy_TERM_NO_KEY        0

/* Internal codes used by holy to represent terminal input.  */
/* Only for keys otherwise not having shifted modification.  */
#define holy_TERM_SHIFT         0x01000000
#define holy_TERM_CTRL          0x02000000
#define holy_TERM_ALT           0x04000000

/* Keys without associated character.  */
#define holy_TERM_EXTENDED      0x00800000
#define holy_TERM_KEY_MASK      0x00ffffff

#define holy_TERM_KEY_LEFT      (holy_TERM_EXTENDED | 0x4b)
#define holy_TERM_KEY_RIGHT     (holy_TERM_EXTENDED | 0x4d)
#define holy_TERM_KEY_UP        (holy_TERM_EXTENDED | 0x48)
#define holy_TERM_KEY_DOWN      (holy_TERM_EXTENDED | 0x50)
#define holy_TERM_KEY_HOME      (holy_TERM_EXTENDED | 0x47)
#define holy_TERM_KEY_END       (holy_TERM_EXTENDED | 0x4f)
#define holy_TERM_KEY_DC        (holy_TERM_EXTENDED | 0x53)
#define holy_TERM_KEY_PPAGE     (holy_TERM_EXTENDED | 0x49)
#define holy_TERM_KEY_NPAGE     (holy_TERM_EXTENDED | 0x51)
#define holy_TERM_KEY_F1        (holy_TERM_EXTENDED | 0x3b)
#define holy_TERM_KEY_F2        (holy_TERM_EXTENDED | 0x3c)
#define holy_TERM_KEY_F3        (holy_TERM_EXTENDED | 0x3d)
#define holy_TERM_KEY_F4        (holy_TERM_EXTENDED | 0x3e)
#define holy_TERM_KEY_F5        (holy_TERM_EXTENDED | 0x3f)
#define holy_TERM_KEY_F6        (holy_TERM_EXTENDED | 0x40)
#define holy_TERM_KEY_F7        (holy_TERM_EXTENDED | 0x41)
#define holy_TERM_KEY_F8        (holy_TERM_EXTENDED | 0x42)
#define holy_TERM_KEY_F9        (holy_TERM_EXTENDED | 0x43)
#define holy_TERM_KEY_F10       (holy_TERM_EXTENDED | 0x44)
#define holy_TERM_KEY_F11       (holy_TERM_EXTENDED | 0x57)
#define holy_TERM_KEY_F12       (holy_TERM_EXTENDED | 0x58)
#define holy_TERM_KEY_INSERT    (holy_TERM_EXTENDED | 0x52)
#define holy_TERM_KEY_CENTER    (holy_TERM_EXTENDED | 0x4c)

#define holy_TERM_ESC		'\e'
#define holy_TERM_TAB		'\t'
#define holy_TERM_BACKSPACE	'\b'

#define holy_PROGRESS_NO_UPDATE -1
#define holy_PROGRESS_FAST      0
#define holy_PROGRESS_SLOW      2

#ifndef ASM_FILE

#include <holy/err.h>
#include <holy/symbol.h>
#include <holy/types.h>
#include <holy/unicode.h>
#include <holy/list.h>

/* These are used to represent the various color states we use.  */
typedef enum
  {
    /* The color used to display all text that does not use the
       user defined colors below.  */
    holy_TERM_COLOR_STANDARD,
    /* The user defined colors for normal text.  */
    holy_TERM_COLOR_NORMAL,
    /* The user defined colors for highlighted text.  */
    holy_TERM_COLOR_HIGHLIGHT
  }
holy_term_color_state;

/* Flags for representing the capabilities of a terminal.  */
/* Some notes about the flags:
   - These flags are used by higher-level functions but not terminals
   themselves.
   - If a terminal is dumb, you may assume that only putchar, getkey and
   checkkey are called.
   - Some fancy features (setcolorstate, setcolor and setcursor) can be set
   to NULL.  */

/* Set when input characters shouldn't be echoed back.  */
#define holy_TERM_NO_ECHO	        (1 << 0)
/* Set when the editing feature should be disabled.  */
#define holy_TERM_NO_EDIT	        (1 << 1)
/* Set when the terminal cannot do fancy things.  */
#define holy_TERM_DUMB		        (1 << 2)
/* Which encoding does terminal expect stream to be.  */
#define holy_TERM_CODE_TYPE_SHIFT       3
#define holy_TERM_CODE_TYPE_MASK	        (7 << holy_TERM_CODE_TYPE_SHIFT)
/* Only ASCII characters accepted.  */
#define holy_TERM_CODE_TYPE_ASCII	        (0 << holy_TERM_CODE_TYPE_SHIFT)
/* Expects CP-437 characters (ASCII + pseudographics).  */
#define holy_TERM_CODE_TYPE_CP437	                (1 << holy_TERM_CODE_TYPE_SHIFT)
/* UTF-8 stream in logical order. Usually used for terminals
   which just forward the stream to another computer.  */
#define holy_TERM_CODE_TYPE_UTF8_LOGICAL       	(2 << holy_TERM_CODE_TYPE_SHIFT)
/* UTF-8 in visual order. Like UTF-8 logical but for buggy endpoints.  */
#define holy_TERM_CODE_TYPE_UTF8_VISUAL	        (3 << holy_TERM_CODE_TYPE_SHIFT)
/* Glyph description in visual order.  */
#define holy_TERM_CODE_TYPE_VISUAL_GLYPHS       (4 << holy_TERM_CODE_TYPE_SHIFT)


/* Bitmasks for modifier keys returned by holy_getkeystatus.  */
#define holy_TERM_STATUS_RSHIFT	(1 << 0)
#define holy_TERM_STATUS_LSHIFT	(1 << 1)
#define holy_TERM_STATUS_RCTRL	(1 << 2)
#define holy_TERM_STATUS_RALT	(1 << 3)
#define holy_TERM_STATUS_SCROLL	(1 << 4)
#define holy_TERM_STATUS_NUM	(1 << 5)
#define holy_TERM_STATUS_CAPS	(1 << 6)
#define holy_TERM_STATUS_LCTRL	(1 << 8)
#define holy_TERM_STATUS_LALT	(1 << 9)

/* Menu-related geometrical constants.  */

/* The number of columns/lines between messages/borders/etc.  */
#define holy_TERM_MARGIN	1

/* The number of columns of scroll information.  */
#define holy_TERM_SCROLL_WIDTH	1

struct holy_term_input
{
  /* The next terminal.  */
  struct holy_term_input *next;
  struct holy_term_input **prev;

  /* The terminal name.  */
  const char *name;

  /* Initialize the terminal.  */
  holy_err_t (*init) (struct holy_term_input *term);

  /* Clean up the terminal.  */
  holy_err_t (*fini) (struct holy_term_input *term);

  /* Get a character if any input character is available. Otherwise return -1  */
  int (*getkey) (struct holy_term_input *term);

  /* Get keyboard modifier status.  */
  int (*getkeystatus) (struct holy_term_input *term);

  void *data;
};
typedef struct holy_term_input *holy_term_input_t;

/* Made in a way to fit into uint32_t and so be passed in a register.  */
struct holy_term_coordinate
{
  holy_uint16_t x;
  holy_uint16_t y;
};

struct holy_term_output
{
  /* The next terminal.  */
  struct holy_term_output *next;
  struct holy_term_output **prev;

  /* The terminal name.  */
  const char *name;

  /* Initialize the terminal.  */
  holy_err_t (*init) (struct holy_term_output *term);

  /* Clean up the terminal.  */
  holy_err_t (*fini) (struct holy_term_output *term);

  /* Put a character. C is encoded in Unicode.  */
  void (*putchar) (struct holy_term_output *term,
		   const struct holy_unicode_glyph *c);

  /* Get the number of columns occupied by a given character C. C is
     encoded in Unicode.  */
  holy_size_t (*getcharwidth) (struct holy_term_output *term,
			       const struct holy_unicode_glyph *c);

  /* Get the screen size.  */
  struct holy_term_coordinate (*getwh) (struct holy_term_output *term);

  /* Get the cursor position. The return value is ((X << 8) | Y).  */
  struct holy_term_coordinate (*getxy) (struct holy_term_output *term);

  /* Go to the position (X, Y).  */
  void (*gotoxy) (struct holy_term_output *term,
		  struct holy_term_coordinate pos);

  /* Clear the screen.  */
  void (*cls) (struct holy_term_output *term);

  /* Set the current color to be used */
  void (*setcolorstate) (struct holy_term_output *term,
			 holy_term_color_state state);

  /* Turn on/off the cursor.  */
  void (*setcursor) (struct holy_term_output *term, int on);

  /* Update the screen.  */
  void (*refresh) (struct holy_term_output *term);

  /* gfxterm only: put in fullscreen mode.  */
  holy_err_t (*fullscreen) (void);

  /* The feature flags defined above.  */
  holy_uint32_t flags;

  /* Progress data. */
  holy_uint32_t progress_update_divisor;
  holy_uint32_t progress_update_counter;

  void *data;
};
typedef struct holy_term_output *holy_term_output_t;

#define holy_TERM_DEFAULT_NORMAL_COLOR 0x07
#define holy_TERM_DEFAULT_HIGHLIGHT_COLOR 0x70
#define holy_TERM_DEFAULT_STANDARD_COLOR 0x07

/* Current color state.  */
extern holy_uint8_t EXPORT_VAR(holy_term_normal_color);
extern holy_uint8_t EXPORT_VAR(holy_term_highlight_color);

extern struct holy_term_output *EXPORT_VAR(holy_term_outputs_disabled);
extern struct holy_term_input *EXPORT_VAR(holy_term_inputs_disabled);
extern struct holy_term_output *EXPORT_VAR(holy_term_outputs);
extern struct holy_term_input *EXPORT_VAR(holy_term_inputs);

static inline void
holy_term_register_input (const char *name __attribute__ ((unused)),
			  holy_term_input_t term)
{
  if (holy_term_inputs)
    holy_list_push (holy_AS_LIST_P (&holy_term_inputs_disabled),
		    holy_AS_LIST (term));
  else
    {
      /* If this is the first terminal, enable automatically.  */
      if (! term->init || term->init (term) == holy_ERR_NONE)
	holy_list_push (holy_AS_LIST_P (&holy_term_inputs), holy_AS_LIST (term));
    }
}

static inline void
holy_term_register_input_inactive (const char *name __attribute__ ((unused)),
				   holy_term_input_t term)
{
  holy_list_push (holy_AS_LIST_P (&holy_term_inputs_disabled),
		  holy_AS_LIST (term));
}

static inline void
holy_term_register_input_active (const char *name __attribute__ ((unused)),
				 holy_term_input_t term)
{
  if (! term->init || term->init (term) == holy_ERR_NONE)
    holy_list_push (holy_AS_LIST_P (&holy_term_inputs), holy_AS_LIST (term));
}

static inline void
holy_term_register_output (const char *name __attribute__ ((unused)),
			   holy_term_output_t term)
{
  if (holy_term_outputs)
    holy_list_push (holy_AS_LIST_P (&holy_term_outputs_disabled),
		    holy_AS_LIST (term));
  else
    {
      /* If this is the first terminal, enable automatically.  */
      if (! term->init || term->init (term) == holy_ERR_NONE)
	holy_list_push (holy_AS_LIST_P (&holy_term_outputs),
			holy_AS_LIST (term));
    }
}

static inline void
holy_term_register_output_inactive (const char *name __attribute__ ((unused)),
				    holy_term_output_t term)
{
  holy_list_push (holy_AS_LIST_P (&holy_term_outputs_disabled),
		  holy_AS_LIST (term));
}

static inline void
holy_term_register_output_active (const char *name __attribute__ ((unused)),
				  holy_term_output_t term)
{
  if (! term->init || term->init (term) == holy_ERR_NONE)
    holy_list_push (holy_AS_LIST_P (&holy_term_outputs),
		    holy_AS_LIST (term));
}

static inline void
holy_term_unregister_input (holy_term_input_t term)
{
  holy_list_remove (holy_AS_LIST (term));
  holy_list_remove (holy_AS_LIST (term));
}

static inline void
holy_term_unregister_output (holy_term_output_t term)
{
  holy_list_remove (holy_AS_LIST (term));
  holy_list_remove (holy_AS_LIST (term));
}

#define FOR_ACTIVE_TERM_INPUTS(var) FOR_LIST_ELEMENTS((var), (holy_term_inputs))
#define FOR_DISABLED_TERM_INPUTS(var) FOR_LIST_ELEMENTS((var), (holy_term_inputs_disabled))
#define FOR_ACTIVE_TERM_OUTPUTS(var) FOR_LIST_ELEMENTS((var), (holy_term_outputs))
#define FOR_DISABLED_TERM_OUTPUTS(var) FOR_LIST_ELEMENTS((var), (holy_term_outputs_disabled))

void holy_putcode (holy_uint32_t code, struct holy_term_output *term);
int EXPORT_FUNC(holy_getkey) (void);
int EXPORT_FUNC(holy_getkey_noblock) (void);
void holy_cls (void);
void EXPORT_FUNC(holy_refresh) (void);
void holy_puts_terminal (const char *str, struct holy_term_output *term);
struct holy_term_coordinate *holy_term_save_pos (void);
void holy_term_restore_pos (struct holy_term_coordinate *pos);

static inline unsigned holy_term_width (struct holy_term_output *term)
{
  return term->getwh(term).x ? : 80;
}

static inline unsigned holy_term_height (struct holy_term_output *term)
{
  return term->getwh(term).y ? : 24;
}

static inline struct holy_term_coordinate
holy_term_getxy (struct holy_term_output *term)
{
  return term->getxy (term);
}

static inline void
holy_term_refresh (struct holy_term_output *term)
{
  if (term->refresh)
    term->refresh (term);
}

static inline void
holy_term_gotoxy (struct holy_term_output *term, struct holy_term_coordinate pos)
{
  term->gotoxy (term, pos);
}

static inline void 
holy_term_setcolorstate (struct holy_term_output *term,
			 holy_term_color_state state)
{
  if (term->setcolorstate)
    term->setcolorstate (term, state);
}

static inline void
holy_setcolorstate (holy_term_color_state state)
{
  struct holy_term_output *term;
  
  FOR_ACTIVE_TERM_OUTPUTS(term)
    holy_term_setcolorstate (term, state);
}

/* Turn on/off the cursor.  */
static inline void 
holy_term_setcursor (struct holy_term_output *term, int on)
{
  if (term->setcursor)
    term->setcursor (term, on);
}

static inline void 
holy_term_cls (struct holy_term_output *term)
{
  if (term->cls)
    (term->cls) (term);
  else
    {
      holy_putcode ('\n', term);
      holy_term_refresh (term);
    }
}

#if HAVE_FONT_SOURCE

holy_size_t
holy_unicode_estimate_width (const struct holy_unicode_glyph *c);

#else

static inline holy_size_t
holy_unicode_estimate_width (const struct holy_unicode_glyph *c __attribute__ ((unused)))
{
  if (holy_unicode_get_comb_type (c->base))
    return 0;
  return 1;
}

#endif

#define holy_TERM_TAB_WIDTH 8

static inline holy_size_t
holy_term_getcharwidth (struct holy_term_output *term,
			const struct holy_unicode_glyph *c)
{
  if (c->base == '\t')
    return holy_TERM_TAB_WIDTH;

  if (term->getcharwidth)
    return term->getcharwidth (term, c);
  else if (((term->flags & holy_TERM_CODE_TYPE_MASK)
	    == holy_TERM_CODE_TYPE_UTF8_LOGICAL)
	   || ((term->flags & holy_TERM_CODE_TYPE_MASK)
	       == holy_TERM_CODE_TYPE_UTF8_VISUAL)
	   || ((term->flags & holy_TERM_CODE_TYPE_MASK)
	       == holy_TERM_CODE_TYPE_VISUAL_GLYPHS))
    return holy_unicode_estimate_width (c);
  else
    return 1;
}

struct holy_term_autoload
{
  struct holy_term_autoload *next;
  char *name;
  char *modname;
};

extern struct holy_term_autoload *holy_term_input_autoload;
extern struct holy_term_autoload *holy_term_output_autoload;

static inline void
holy_print_spaces (struct holy_term_output *term, int number_spaces)
{
  while (--number_spaces >= 0)
    holy_putcode (' ', term);
}

extern void (*EXPORT_VAR (holy_term_poll_usb)) (int wait_for_completion);

#define holy_TERM_REPEAT_PRE_INTERVAL 400
#define holy_TERM_REPEAT_INTERVAL 50

#endif /* ! ASM_FILE */

#endif /* ! holy_TERM_HEADER */
