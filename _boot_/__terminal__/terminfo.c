/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/err.h>
#include <holy/dl.h>
#include <holy/term.h>
#include <holy/terminfo.h>
#include <holy/tparm.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/time.h>
#if defined(__powerpc__) && defined(holy_MACHINE_IEEE1275)
#include <holy/ieee1275/ieee1275.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

#define ANSI_CSI 0x9b
#define ANSI_CSI_STR "\x9b"

static struct holy_term_output *terminfo_outputs;

/* Get current terminfo name.  */
char *
holy_terminfo_get_current (struct holy_term_output *term)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;
  return data->name;
}

/* Free *PTR and set *PTR to NULL, to prevent double-free.  */
static void
holy_terminfo_free (char **ptr)
{
  holy_free (*ptr);
  *ptr = 0;
}

static void
holy_terminfo_all_free (struct holy_term_output *term)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  /* Free previously allocated memory.  */
  holy_terminfo_free (&data->name);
  holy_terminfo_free (&data->gotoxy);
  holy_terminfo_free (&data->cls);
  holy_terminfo_free (&data->reverse_video_on);
  holy_terminfo_free (&data->reverse_video_off);
  holy_terminfo_free (&data->cursor_on);
  holy_terminfo_free (&data->cursor_off);
}

/* Set current terminfo type.  */
holy_err_t
holy_terminfo_set_current (struct holy_term_output *term,
			   const char *str)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;
  /* TODO
   * Lookup user specified terminfo type. If found, set term variables
   * as appropriate. Otherwise return an error.
   *
   * How should this be done?
   *  a. A static table included in this module.
   *     - I do not like this idea.
   *  b. A table stored in the configuration directory.
   *     - Users must convert their terminfo settings if we have not already.
   *  c. Look for terminfo files in the configuration directory.
   *     - /usr/share/terminfo is 6.3M on my system.
   *     - /usr/share/terminfo is not on most users boot partition.
   *     + Copying the terminfo files you want to use to the holy
   *       configuration directory is easier then (b).
   *  d. Your idea here.
   */

  holy_terminfo_all_free (term);

  if (holy_strcmp ("vt100", str) == 0)
    {
      data->name              = holy_strdup ("vt100");
      data->gotoxy            = holy_strdup ("\e[%i%p1%d;%p2%dH");
      data->cls               = holy_strdup ("\e[H\e[J");
      data->reverse_video_on  = holy_strdup ("\e[7m");
      data->reverse_video_off = holy_strdup ("\e[m");
      data->cursor_on         = holy_strdup ("\e[?25h");
      data->cursor_off        = holy_strdup ("\e[?25l");
      data->setcolor          = NULL;
      return holy_errno;
    }

  if (holy_strcmp ("vt100-color", str) == 0)
    {
      data->name              = holy_strdup ("vt100-color");
      data->gotoxy            = holy_strdup ("\e[%i%p1%d;%p2%dH");
      data->cls               = holy_strdup ("\e[H\e[J");
      data->reverse_video_on  = holy_strdup ("\e[7m");
      data->reverse_video_off = holy_strdup ("\e[m");
      data->cursor_on         = holy_strdup ("\e[?25h");
      data->cursor_off        = holy_strdup ("\e[?25l");
      data->setcolor          = holy_strdup ("\e[3%p1%dm\e[4%p2%dm");
      return holy_errno;
    }

  if (holy_strcmp ("arc", str) == 0)
    {
      data->name              = holy_strdup ("arc");
      data->gotoxy            = holy_strdup (ANSI_CSI_STR "%i%p1%d;%p2%dH");
      data->cls               = holy_strdup (ANSI_CSI_STR "2J");
      data->reverse_video_on  = holy_strdup (ANSI_CSI_STR "7m");
      data->reverse_video_off = holy_strdup (ANSI_CSI_STR "0m");
      data->cursor_on         = 0;
      data->cursor_off        = 0;
      data->setcolor          = holy_strdup (ANSI_CSI_STR "3%p1%dm"
					     ANSI_CSI_STR "4%p2%dm");
      return holy_errno;
    }

  if (holy_strcmp ("ieee1275", str) == 0
      || holy_strcmp ("ieee1275-nocursor", str) == 0)
    {
      data->name              = holy_strdup ("ieee1275");
      data->gotoxy            = holy_strdup ("\e[%i%p1%d;%p2%dH");
      /* Clear the screen.  Using serial console, screen(1) only recognizes the
       * ANSI escape sequence.  Using video console, Apple Open Firmware
       * (version 3.1.1) only recognizes the literal ^L.  So use both.  */
      data->cls               = holy_strdup ("\e[2J");
      data->reverse_video_on  = holy_strdup ("\e[7m");
      data->reverse_video_off = holy_strdup ("\e[m");
      if (holy_strcmp ("ieee1275", str) == 0)
	{
	  data->cursor_on         = holy_strdup ("\e[?25h");
	  data->cursor_off        = holy_strdup ("\e[?25l");
	}
      else
	{
	  data->cursor_on         = 0;
	  data->cursor_off        = 0;
	}
      data->setcolor          = holy_strdup ("\e[3%p1%dm\e[4%p2%dm");
      return holy_errno;
    }

  if (holy_strcmp ("dumb", str) == 0)
    {
      data->name              = holy_strdup ("dumb");
      data->gotoxy            = NULL;
      data->cls               = NULL;
      data->reverse_video_on  = NULL;
      data->reverse_video_off = NULL;
      data->cursor_on         = NULL;
      data->cursor_off        = NULL;
      data->setcolor          = NULL;
      return holy_errno;
    }

  return holy_error (holy_ERR_BAD_ARGUMENT, N_("unknown terminfo type `%s'"),
		     str);
}

holy_err_t
holy_terminfo_output_register (struct holy_term_output *term,
			       const char *type)
{
  holy_err_t err;
  struct holy_terminfo_output_state *data;

  err = holy_terminfo_set_current (term, type);

  if (err)
    return err;

  data = (struct holy_terminfo_output_state *) term->data;
  data->next = terminfo_outputs;
  terminfo_outputs = term;

  return holy_ERR_NONE;
}

holy_err_t
holy_terminfo_output_unregister (struct holy_term_output *term)
{
  struct holy_term_output **ptr;

  for (ptr = &terminfo_outputs; *ptr;
       ptr = &((struct holy_terminfo_output_state *) (*ptr)->data)->next)
    if (*ptr == term)
      {
	holy_terminfo_all_free (term);
	*ptr = ((struct holy_terminfo_output_state *) (*ptr)->data)->next;
	return holy_ERR_NONE;
      }
  return holy_error (holy_ERR_BUG, "terminal not found");
}

/* Wrapper for holy_putchar to write strings.  */
static void
putstr (struct holy_term_output *term, const char *str)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;
  while (*str)
    data->put (term, *str++);
}

struct holy_term_coordinate
holy_terminfo_getxy (struct holy_term_output *term)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  return data->pos;
}

void
holy_terminfo_gotoxy (struct holy_term_output *term,
		      struct holy_term_coordinate pos)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  if (pos.x > holy_term_width (term) || pos.y > holy_term_height (term))
    {
      holy_error (holy_ERR_BUG, "invalid point (%u,%u)", pos.x, pos.y);
      return;
    }

  if (data->gotoxy)
    putstr (term, holy_terminfo_tparm (data->gotoxy, pos.y, pos.x));
  else
    {
      if ((pos.y == data->pos.y) && (pos.x == data->pos.x - 1))
	data->put (term, '\b');
    }

  data->pos = pos;
}

/* Clear the screen.  */
void
holy_terminfo_cls (struct holy_term_output *term)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  putstr (term, holy_terminfo_tparm (data->cls));
  holy_terminfo_gotoxy (term, (struct holy_term_coordinate) { 0, 0 });
}

void
holy_terminfo_setcolorstate (struct holy_term_output *term,
			     const holy_term_color_state state)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  if (data->setcolor)
    {
      int fg;
      int bg;
      /* Map from VGA to terminal colors.  */
      const int colormap[8] 
	= { 0, /* Black. */
	    4, /* Blue. */
	    2, /* Green. */
	    6, /* Cyan. */
	    1, /* Red.  */
	    5, /* Magenta.  */
	    3, /* Yellow.  */
	    7, /* White.  */
      };

      switch (state)
	{
	case holy_TERM_COLOR_STANDARD:
	case holy_TERM_COLOR_NORMAL:
	  fg = holy_term_normal_color & 0x0f;
	  bg = holy_term_normal_color >> 4;
	  break;
	case holy_TERM_COLOR_HIGHLIGHT:
	  fg = holy_term_highlight_color & 0x0f;
	  bg = holy_term_highlight_color >> 4;
	  break;
	default:
	  return;
	}

      putstr (term, holy_terminfo_tparm (data->setcolor, colormap[fg & 7],
					 colormap[bg & 7]));
      return;
    }

  switch (state)
    {
    case holy_TERM_COLOR_STANDARD:
    case holy_TERM_COLOR_NORMAL:
      putstr (term, holy_terminfo_tparm (data->reverse_video_off));
      break;
    case holy_TERM_COLOR_HIGHLIGHT:
      putstr (term, holy_terminfo_tparm (data->reverse_video_on));
      break;
    default:
      break;
    }
}

void
holy_terminfo_setcursor (struct holy_term_output *term, const int on)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  if (on)
    putstr (term, holy_terminfo_tparm (data->cursor_on));
  else
    putstr (term, holy_terminfo_tparm (data->cursor_off));
}

/* The terminfo version of putchar.  */
void
holy_terminfo_putchar (struct holy_term_output *term,
		       const struct holy_unicode_glyph *c)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  /* Keep track of the cursor.  */
  switch (c->base)
    {
    case '\a':
      break;

    case '\b':
    case 127:
      if (data->pos.x > 0)
	data->pos.x--;
    break;

    case '\n':
      if (data->pos.y < holy_term_height (term) - 1)
	data->pos.y++;
      break;

    case '\r':
      data->pos.x = 0;
      break;

    default:
      if ((int) data->pos.x + c->estimated_width >= (int) holy_term_width (term) + 1)
	{
	  data->pos.x = 0;
	  if (data->pos.y < holy_term_height (term) - 1)
	    data->pos.y++;
	  data->put (term, '\r');
	  data->put (term, '\n');
	}
      data->pos.x += c->estimated_width;
      break;
    }

  data->put (term, c->base);
}

struct holy_term_coordinate
holy_terminfo_getwh (struct holy_term_output *term)
{
  struct holy_terminfo_output_state *data
    = (struct holy_terminfo_output_state *) term->data;

  return data->size;
}

static void
holy_terminfo_readkey (struct holy_term_input *term, int *keys, int *len,
		       int (*readkey) (struct holy_term_input *term))
{
  int c;

#define CONTINUE_READ						\
  {								\
    holy_uint64_t start;					\
    /* On 9600 we have to wait up to 12 milliseconds.  */	\
    start = holy_get_time_ms ();				\
    do								\
      c = readkey (term);					\
    while (c == -1 && holy_get_time_ms () - start < 100);	\
    if (c == -1)						\
      return;							\
								\
    keys[*len] = c;						\
    (*len)++;							\
  }

  c = readkey (term);
  if (c < 0)
    {
      *len = 0;
      return;
    }
  *len = 1;
  keys[0] = c;
  if (c != ANSI_CSI && c != '\e')
    {
      /* Backspace: Ctrl-h.  */
      if (c == 0x7f)
	c = '\b'; 
      if (c < 0x20 && c != '\t' && c!= '\b' && c != '\n' && c != '\r')
	c = holy_TERM_CTRL | (c - 1 + 'a');
      *len = 1;
      keys[0] = c;
      return;
    }

  {
    static struct
    {
      char key;
      unsigned ascii;
    }
    three_code_table[] =
      {
	{'4', holy_TERM_KEY_DC},
	{'A', holy_TERM_KEY_UP},
	{'B', holy_TERM_KEY_DOWN},
	{'C', holy_TERM_KEY_RIGHT},
	{'D', holy_TERM_KEY_LEFT},
	{'F', holy_TERM_KEY_END},
	{'H', holy_TERM_KEY_HOME},
	{'K', holy_TERM_KEY_END},
	{'P', holy_TERM_KEY_DC},
	{'?', holy_TERM_KEY_PPAGE},
	{'/', holy_TERM_KEY_NPAGE},
	{'@', holy_TERM_KEY_INSERT},
      };

    static unsigned four_code_table[] =
      {
	[1] = holy_TERM_KEY_HOME,
	[3] = holy_TERM_KEY_DC,
	[5] = holy_TERM_KEY_PPAGE,
	[6] = holy_TERM_KEY_NPAGE,
	[7] = holy_TERM_KEY_HOME,
	[8] = holy_TERM_KEY_END,
	[17] = holy_TERM_KEY_F6,
	[18] = holy_TERM_KEY_F7,
	[19] = holy_TERM_KEY_F8,
	[20] = holy_TERM_KEY_F9,
	[21] = holy_TERM_KEY_F10,
	[23] = holy_TERM_KEY_F11,
	[24] = holy_TERM_KEY_F12,
      };
    char fx_key[] = 
      { 'P', 'Q', 'w', 'x', 't', 'u',
        'q', 'r', 'p', 'M', 'A', 'B', 'H', 'F' };
    unsigned fx_code[] = 
	{ holy_TERM_KEY_F1, holy_TERM_KEY_F2, holy_TERM_KEY_F3,
	  holy_TERM_KEY_F4, holy_TERM_KEY_F5, holy_TERM_KEY_F6,
	  holy_TERM_KEY_F7, holy_TERM_KEY_F8, holy_TERM_KEY_F9,
	  holy_TERM_KEY_F10, holy_TERM_KEY_F11, holy_TERM_KEY_F12,
	  holy_TERM_KEY_HOME, holy_TERM_KEY_END };
    unsigned i;

    if (c == '\e')
      {
	CONTINUE_READ;

	if (c == 'O')
	  {
	    CONTINUE_READ;

	    for (i = 0; i < ARRAY_SIZE (fx_key); i++)
	      if (fx_key[i] == c)
		{
		  keys[0] = fx_code[i];
		  *len = 1;
		  return;
		}
	  }

	if (c != '[')
	  return;
      }

    CONTINUE_READ;
	
    for (i = 0; i < ARRAY_SIZE (three_code_table); i++)
      if (three_code_table[i].key == c)
	{
	  keys[0] = three_code_table[i].ascii;
	  *len = 1;
	  return;
	}

    switch (c)
      {
      case '[':
	CONTINUE_READ;
	if (c >= 'A' && c <= 'E')
	  {
	    keys[0] = holy_TERM_KEY_F1 + c - 'A';
	    *len = 1;
	    return;
	  }
	return;
      case 'O':
	CONTINUE_READ;
	for (i = 0; i < ARRAY_SIZE (fx_key); i++)
	  if (fx_key[i] == c)
	    {
	      keys[0] = fx_code[i];
	      *len = 1;
	      return;
	    }
	return;

      case '0':
	{
	  int num = 0;
	  CONTINUE_READ;
	  if (c != '0' && c != '1')
	    return;
	  num = (c - '0') * 10;
	  CONTINUE_READ;
	  if (c < '0' || c > '9')
	    return;
	  num += (c - '0');
	  if (num == 0 || num > 12)
	    return;
	  CONTINUE_READ;
	  if (c != 'q')
	    return;
	  keys[0] = fx_code[num - 1];
	  *len = 1;
	  return;
	}	  

      case '1' ... '9':
	{
	  unsigned val = c - '0';
	  CONTINUE_READ;
	  if (c >= '0' && c <= '9')
	    {
	      val = val * 10 + (c - '0');
	      CONTINUE_READ;
	    }
	  if (c != '~')
	    return;
	  if (val >= ARRAY_SIZE (four_code_table)
	      || four_code_table[val] == 0)
	    return;
	  keys[0] = four_code_table[val];
	  *len = 1;
	  return;
	}
	default:
	  return;
      }
  }
#undef CONTINUE_READ
}

/* The terminfo version of getkey.  */
int
holy_terminfo_getkey (struct holy_term_input *termi)
{
  struct holy_terminfo_input_state *data
    = (struct holy_terminfo_input_state *) (termi->data);
  if (data->npending)
    {
      int ret;
      data->npending--;
      ret = data->input_buf[0];
      holy_memmove (data->input_buf, data->input_buf + 1, data->npending
		    * sizeof (data->input_buf[0]));
      return ret;
    }

  holy_terminfo_readkey (termi, data->input_buf,
			 &data->npending, data->readkey);

#if defined(__powerpc__) && defined(holy_MACHINE_IEEE1275)
  if (data->npending == 1 && data->input_buf[0] == '\e'
      && holy_ieee1275_test_flag (holy_IEEE1275_FLAG_BROKEN_REPEAT)
      && holy_get_time_ms () - data->last_key_time < 1000
      && (data->last_key & holy_TERM_EXTENDED))
    {
      data->npending = 0;
      data->last_key_time = holy_get_time_ms ();
      return data->last_key;
    }
#endif

  if (data->npending)
    {
      int ret;
      data->npending--;
      ret = data->input_buf[0];
#if defined(__powerpc__) && defined(holy_MACHINE_IEEE1275)
      if (holy_ieee1275_test_flag (holy_IEEE1275_FLAG_BROKEN_REPEAT))
	{
	  data->last_key = ret;
	  data->last_key_time = holy_get_time_ms ();
	}
#endif
      holy_memmove (data->input_buf, data->input_buf + 1, data->npending
		    * sizeof (data->input_buf[0]));
      return ret;
    }

  return holy_TERM_NO_KEY;
}

holy_err_t
holy_terminfo_input_init (struct holy_term_input *termi)
{
  struct holy_terminfo_input_state *data
    = (struct holy_terminfo_input_state *) (termi->data);
  data->npending = 0;

  return holy_ERR_NONE;
}

holy_err_t
holy_terminfo_output_init (struct holy_term_output *term)
{
  holy_terminfo_cls (term);
  return holy_ERR_NONE;
}

/* holy Command.  */

static holy_err_t
print_terminfo (void)
{
  const char *encoding_names[(holy_TERM_CODE_TYPE_MASK
			      >> holy_TERM_CODE_TYPE_SHIFT) + 1]
    = {
    /* VGA and glyph descriptor types are just for completeness,
       they are not used on terminfo terminals.
    */
    [holy_TERM_CODE_TYPE_ASCII >> holy_TERM_CODE_TYPE_SHIFT] = _("ASCII"),
    [holy_TERM_CODE_TYPE_CP437 >> holy_TERM_CODE_TYPE_SHIFT] = "CP-437",
    [holy_TERM_CODE_TYPE_UTF8_LOGICAL >> holy_TERM_CODE_TYPE_SHIFT]
    = _("UTF-8"),
    [holy_TERM_CODE_TYPE_UTF8_VISUAL >> holy_TERM_CODE_TYPE_SHIFT]
    /* TRANSLATORS: visually ordered UTF-8 is a non-compliant encoding
       based on UTF-8 with right-to-left languages written in reverse.
       Used on some terminals. Normal UTF-8 is refered as
       "logically-ordered UTF-8" by opposition.  */
    = _("visually-ordered UTF-8"),
    [holy_TERM_CODE_TYPE_VISUAL_GLYPHS >> holy_TERM_CODE_TYPE_SHIFT]
    = "Glyph descriptors",
    _("Unknown encoding"), _("Unknown encoding"), _("Unknown encoding")
  };
  struct holy_term_output *cur;

  holy_puts_ (N_("Current terminfo types:"));
  for (cur = terminfo_outputs; cur;
       cur = ((struct holy_terminfo_output_state *) cur->data)->next)
    holy_printf ("%s: %s\t%s\t%dx%d\n", cur->name,
		 holy_terminfo_get_current(cur),
		 encoding_names[(cur->flags & holy_TERM_CODE_TYPE_MASK)
				>> holy_TERM_CODE_TYPE_SHIFT],
		 ((struct holy_terminfo_output_state *) cur->data)->pos.x,
	         ((struct holy_terminfo_output_state *) cur->data)->pos.y);

  return holy_ERR_NONE;
}

static const struct holy_arg_option options[] =
{
  {"ascii", 'a', 0, N_("Terminal is ASCII-only [default]."),  0, ARG_TYPE_NONE},
  {"utf8",  'u', 0, N_("Terminal is logical-ordered UTF-8."), 0, ARG_TYPE_NONE},
  {"visual-utf8", 'v', 0, N_("Terminal is visually-ordered UTF-8."), 0,
   ARG_TYPE_NONE},
  {"geometry", 'g', 0, N_("Terminal has specified geometry."),
   /* TRANSLATORS: "x" has to be entered in, like an identifier, so please don't
      use better Unicode codepoints.  */
   N_("WIDTHxHEIGHT."), ARG_TYPE_STRING},
  {0, 0, 0, 0, 0, 0}
};

enum
  {
    OPTION_ASCII,
    OPTION_UTF8,
    OPTION_VISUAL_UTF8,
    OPTION_GEOMETRY
  };

static holy_err_t
holy_cmd_terminfo (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_term_output *cur;
  int encoding = holy_TERM_CODE_TYPE_ASCII;
  struct holy_arg_list *state = ctxt->state;
  int w = 0, h = 0;

  if (argc == 0)
    return print_terminfo ();

  if (state[OPTION_ASCII].set)
    encoding = holy_TERM_CODE_TYPE_ASCII;

  if (state[OPTION_UTF8].set)
    encoding = holy_TERM_CODE_TYPE_UTF8_LOGICAL;

  if (state[OPTION_VISUAL_UTF8].set)
    encoding = holy_TERM_CODE_TYPE_UTF8_VISUAL;

  if (state[OPTION_GEOMETRY].set)
    {
      char *ptr = state[OPTION_GEOMETRY].arg;
      w = holy_strtoul (ptr, &ptr, 0);
      if (holy_errno)
	return holy_errno;
      if (*ptr != 'x')
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("incorrect terminal dimensions specification"));
      ptr++;
      h = holy_strtoul (ptr, &ptr, 0);
      if (holy_errno)
	return holy_errno;
    }

  for (cur = terminfo_outputs; cur;
       cur = ((struct holy_terminfo_output_state *) cur->data)->next)
    if (holy_strcmp (args[0], cur->name) == 0
	|| (holy_strcmp (args[0], "ofconsole") == 0
	    && holy_strcmp ("console", cur->name) == 0))
      {
	cur->flags = (cur->flags & ~holy_TERM_CODE_TYPE_MASK) | encoding;

	if (w && h)
	  {
	    struct holy_terminfo_output_state *data
	      = (struct holy_terminfo_output_state *) cur->data;
	    data->size.x = w;
	    data->size.y = h;
	  }

	if (argc == 1)
	  return holy_ERR_NONE;

	return holy_terminfo_set_current (cur, args[1]);
      }

  return holy_error (holy_ERR_BAD_ARGUMENT,
		     N_("terminal %s isn't found or it's not handled by terminfo"),
		     args[0]);
}

static holy_extcmd_t cmd;

holy_MOD_INIT(terminfo)
{
  cmd = holy_register_extcmd ("terminfo", holy_cmd_terminfo, 0,
			      N_("[[-a|-u|-v] [-g WxH] TERM [TYPE]]"),
			      N_("Set terminfo type of TERM  to TYPE.\n"),
			      options);
}

holy_MOD_FINI(terminfo)
{
  holy_unregister_extcmd (cmd);
}
