/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/loader.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/env.h>
#include <holy/menu_viewer.h>
#include <holy/i18n.h>
#include <holy/charset.h>

static holy_uint8_t holy_color_menu_normal;
static holy_uint8_t holy_color_menu_highlight;

struct menu_viewer_data
{
  int first, offset;
  struct holy_term_screen_geometry geo;
  enum { 
    TIMEOUT_UNKNOWN, 
    TIMEOUT_NORMAL,
    TIMEOUT_TERSE,
    TIMEOUT_TERSE_NO_MARGIN
  } timeout_msg;
  holy_menu_t menu;
  struct holy_term_output *term;
};

static inline int
holy_term_cursor_x (const struct holy_term_screen_geometry *geo)
{
  return (geo->first_entry_x + geo->entry_width);
}

holy_size_t
holy_getstringwidth (holy_uint32_t * str, const holy_uint32_t * last_position,
		     struct holy_term_output *term)
{
  holy_ssize_t width = 0;

  while (str < last_position)
    {
      struct holy_unicode_glyph glyph;
      glyph.ncomb = 0;
      str += holy_unicode_aglomerate_comb (str, last_position - str, &glyph);
      width += holy_term_getcharwidth (term, &glyph);
      holy_unicode_destroy_glyph (&glyph);
    }
  return width;
}

static int
holy_print_message_indented_real (const char *msg, int margin_left,
				  int margin_right,
				  struct holy_term_output *term, int dry_run)
{
  holy_uint32_t *unicode_msg;
  holy_uint32_t *last_position;
  holy_size_t msg_len = holy_strlen (msg) + 2;
  int ret = 0;

  unicode_msg = holy_malloc (msg_len * sizeof (holy_uint32_t));
 
  if (!unicode_msg)
    return 0;

  msg_len = holy_utf8_to_ucs4 (unicode_msg, msg_len,
			       (holy_uint8_t *) msg, -1, 0);
  
  last_position = unicode_msg + msg_len;
  *last_position = 0;

  if (dry_run)
    ret = holy_ucs4_count_lines (unicode_msg, last_position, margin_left,
				 margin_right, term);
  else
    holy_print_ucs4_menu (unicode_msg, last_position, margin_left,
			  margin_right, term, 0, -1, 0, 0);

  holy_free (unicode_msg);

  return ret;
}

void
holy_print_message_indented (const char *msg, int margin_left, int margin_right,
			     struct holy_term_output *term)
{
  holy_print_message_indented_real (msg, margin_left, margin_right, term, 0);
}

static void
draw_border (struct holy_term_output *term, const struct holy_term_screen_geometry *geo)
{
  int i;

  holy_term_setcolorstate (term, holy_TERM_COLOR_NORMAL);

  holy_term_gotoxy (term, (struct holy_term_coordinate) { geo->first_entry_x - 1,
	geo->first_entry_y - 1 });
  holy_putcode (holy_UNICODE_CORNER_UL, term);
  for (i = 0; i < geo->entry_width + 1; i++)
    holy_putcode (holy_UNICODE_HLINE, term);
  holy_putcode (holy_UNICODE_CORNER_UR, term);

  for (i = 0; i < geo->num_entries; i++)
    {
      holy_term_gotoxy (term, (struct holy_term_coordinate) { geo->first_entry_x - 1,
	    geo->first_entry_y + i });
      holy_putcode (holy_UNICODE_VLINE, term);
      holy_term_gotoxy (term,
			(struct holy_term_coordinate) { geo->first_entry_x + geo->entry_width + 1,
			    geo->first_entry_y + i });
      holy_putcode (holy_UNICODE_VLINE, term);
    }

  holy_term_gotoxy (term,
		    (struct holy_term_coordinate) { geo->first_entry_x - 1,
			geo->first_entry_y - 1 + geo->num_entries + 1 });
  holy_putcode (holy_UNICODE_CORNER_LL, term);
  for (i = 0; i < geo->entry_width + 1; i++)
    holy_putcode (holy_UNICODE_HLINE, term);
  holy_putcode (holy_UNICODE_CORNER_LR, term);

  holy_term_setcolorstate (term, holy_TERM_COLOR_NORMAL);

  holy_term_gotoxy (term,
		    (struct holy_term_coordinate) { geo->first_entry_x - 1,
			(geo->first_entry_y - 1 + geo->num_entries
			 + holy_TERM_MARGIN + 1) });
}

static int
print_message (int nested, int edit, struct holy_term_output *term, int dry_run)
{
  int ret = 0;
  holy_term_setcolorstate (term, holy_TERM_COLOR_NORMAL);

  if (edit)
    {
      ret += holy_print_message_indented_real (_("Minimum Emacs-like screen editing is \
supported. TAB lists completions. Press Ctrl-x or F10 to boot, Ctrl-c or F2 for a \
command-line or ESC to discard edits and return to the holy menu."),
					       STANDARD_MARGIN, STANDARD_MARGIN,
					       term, dry_run);
    }
  else
    {
      char *msg_translated;

      msg_translated = holy_xasprintf (_("Use the %C and %C keys to select which "
					 "entry is highlighted."),
				       holy_UNICODE_UPARROW,
				       holy_UNICODE_DOWNARROW);
      if (!msg_translated)
	return 0;
      ret += holy_print_message_indented_real (msg_translated, STANDARD_MARGIN,
					       STANDARD_MARGIN, term, dry_run);

      holy_free (msg_translated);

      if (nested)
	{
	  ret += holy_print_message_indented_real
	    (_("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line. ESC to return previous menu."),
	     STANDARD_MARGIN, STANDARD_MARGIN, term, dry_run);
	}
      else
	{
	  ret += holy_print_message_indented_real
	    (_("Press enter to boot the selected OS, "
	       "`e' to edit the commands before booting "
	       "or `c' for a command-line."),
	     STANDARD_MARGIN, STANDARD_MARGIN, term, dry_run);
	}	
    }
  return ret;
}

static void
print_entry (int y, int highlight, holy_menu_entry_t entry,
	     const struct menu_viewer_data *data)
{
  const char *title;
  holy_size_t title_len;
  holy_ssize_t len;
  holy_uint32_t *unicode_title;
  holy_ssize_t i;
  holy_uint8_t old_color_normal, old_color_highlight;

  title = entry ? entry->title : "";
  title_len = holy_strlen (title);
  unicode_title = holy_malloc (title_len * sizeof (*unicode_title));
  if (! unicode_title)
    /* XXX How to show this error?  */
    return;

  len = holy_utf8_to_ucs4 (unicode_title, title_len,
                           (holy_uint8_t *) title, -1, 0);
  if (len < 0)
    {
      /* It is an invalid sequence.  */
      holy_free (unicode_title);
      return;
    }

  old_color_normal = holy_term_normal_color;
  old_color_highlight = holy_term_highlight_color;
  holy_term_normal_color = holy_color_menu_normal;
  holy_term_highlight_color = holy_color_menu_highlight;
  holy_term_setcolorstate (data->term, highlight
			   ? holy_TERM_COLOR_HIGHLIGHT
			   : holy_TERM_COLOR_NORMAL);

  holy_term_gotoxy (data->term, (struct holy_term_coordinate) {
      data->geo.first_entry_x, y });

  for (i = 0; i < len; i++)
    if (unicode_title[i] == '\n' || unicode_title[i] == '\b'
	|| unicode_title[i] == '\r' || unicode_title[i] == '\e')
      unicode_title[i] = ' ';

  if (data->geo.num_entries > 1)
    holy_putcode (highlight ? '*' : ' ', data->term);

  holy_print_ucs4_menu (unicode_title,
			unicode_title + len,
			0,
			data->geo.right_margin,
			data->term, 0, 1,
			holy_UNICODE_RIGHTARROW, 0);

  holy_term_setcolorstate (data->term, holy_TERM_COLOR_NORMAL);
  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) {
		      holy_term_cursor_x (&data->geo), y });

  holy_term_normal_color = old_color_normal;
  holy_term_highlight_color = old_color_highlight;

  holy_term_setcolorstate (data->term, holy_TERM_COLOR_NORMAL);
  holy_free (unicode_title);
}

static void
print_entries (holy_menu_t menu, const struct menu_viewer_data *data)
{
  holy_menu_entry_t e;
  int i;

  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) {
		      data->geo.first_entry_x + data->geo.entry_width
			+ data->geo.border + 1,
			data->geo.first_entry_y });

  if (data->geo.num_entries != 1)
    {
      if (data->first)
	holy_putcode (holy_UNICODE_UPARROW, data->term);
      else
	holy_putcode (' ', data->term);
    }
  e = holy_menu_get_entry (menu, data->first);

  for (i = 0; i < data->geo.num_entries; i++)
    {
      print_entry (data->geo.first_entry_y + i, data->offset == i,
		   e, data);
      if (e)
	e = e->next;
    }

  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) { data->geo.first_entry_x + data->geo.entry_width
			+ data->geo.border + 1,
			data->geo.first_entry_y + data->geo.num_entries - 1 });
  if (data->geo.num_entries == 1)
    {
      if (data->first && e)
	holy_putcode (holy_UNICODE_UPDOWNARROW, data->term);
      else if (data->first)
	holy_putcode (holy_UNICODE_UPARROW, data->term);
      else if (e)
	holy_putcode (holy_UNICODE_DOWNARROW, data->term);
      else
	holy_putcode (' ', data->term);
    }
  else
    {
      if (e)
	holy_putcode (holy_UNICODE_DOWNARROW, data->term);
      else
	holy_putcode (' ', data->term);
    }

  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) { holy_term_cursor_x (&data->geo),
			data->geo.first_entry_y + data->offset });
}

/* Initialize the screen.  If NESTED is non-zero, assume that this menu
   is run from another menu or a command-line. If EDIT is non-zero, show
   a message for the menu entry editor.  */
void
holy_menu_init_page (int nested, int edit,
		     struct holy_term_screen_geometry *geo,
		     struct holy_term_output *term)
{
  holy_uint8_t old_color_normal, old_color_highlight;
  int msg_num_lines;
  int bottom_message = 1;
  int empty_lines = 1;
  int version_msg = 1;

  geo->border = 1;
  geo->first_entry_x = 1 /* margin */ + 1 /* border */;
  geo->entry_width = holy_term_width (term) - 5;

  geo->first_entry_y = 2 /* two empty lines*/
    + 1 /* GNU holy version text  */ + 1 /* top border */;

  geo->timeout_lines = 2;

  /* 3 lines for timeout message and bottom margin.  2 lines for the border.  */
  geo->num_entries = holy_term_height (term) - geo->first_entry_y
    - 1 /* bottom border */
    - 1 /* empty line before info message*/
    - geo->timeout_lines /* timeout */
    - 1 /* empty final line  */;
  msg_num_lines = print_message (nested, edit, term, 1);
  if (geo->num_entries - msg_num_lines < 3
      || geo->entry_width < 10)
    {
      geo->num_entries += 4;
      geo->first_entry_y -= 2;
      empty_lines = 0;
      geo->first_entry_x -= 1;
      geo->entry_width += 1;
    }
  if (geo->num_entries - msg_num_lines < 3
      || geo->entry_width < 10)
    {
      geo->num_entries += 2;
      geo->first_entry_y -= 1;
      geo->first_entry_x -= 1;
      geo->entry_width += 2;
      geo->border = 0;
    }

  if (geo->entry_width <= 0)
    geo->entry_width = 1;

  if (geo->num_entries - msg_num_lines < 3
      && geo->timeout_lines == 2)
    {
      geo->timeout_lines = 1;
      geo->num_entries++;
    }

  if (geo->num_entries - msg_num_lines < 3)
    {
      geo->num_entries += 1;
      geo->first_entry_y -= 1;
      version_msg = 0;
    }

  if (geo->num_entries - msg_num_lines >= 2)
    geo->num_entries -= msg_num_lines;
  else
    bottom_message = 0;

  /* By default, use the same colors for the menu.  */
  old_color_normal = holy_term_normal_color;
  old_color_highlight = holy_term_highlight_color;
  holy_color_menu_normal = holy_term_normal_color;
  holy_color_menu_highlight = holy_term_highlight_color;

  /* Then give user a chance to replace them.  */
  holy_parse_color_name_pair (&holy_color_menu_normal,
			      holy_env_get ("menu_color_normal"));
  holy_parse_color_name_pair (&holy_color_menu_highlight,
			      holy_env_get ("menu_color_highlight"));

  if (version_msg)
    holy_normal_init_page (term, empty_lines);
  else
    holy_term_cls (term);

  holy_term_normal_color = holy_color_menu_normal;
  holy_term_highlight_color = holy_color_menu_highlight;
  if (geo->border)
    draw_border (term, geo);
  holy_term_normal_color = old_color_normal;
  holy_term_highlight_color = old_color_highlight;
  geo->timeout_y = geo->first_entry_y + geo->num_entries
    + geo->border + empty_lines;
  if (bottom_message)
    {
      holy_term_gotoxy (term,
			(struct holy_term_coordinate) { holy_TERM_MARGIN,
			    geo->timeout_y });

      print_message (nested, edit, term, 0);
      geo->timeout_y += msg_num_lines;
    }
  geo->right_margin = holy_term_width (term)
    - geo->first_entry_x
    - geo->entry_width - 1;
}

static void
menu_text_print_timeout (int timeout, void *dataptr)
{
  struct menu_viewer_data *data = dataptr;
  char *msg_translated = 0;

  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) { 0, data->geo.timeout_y });

  if (data->timeout_msg == TIMEOUT_TERSE
      || data->timeout_msg == TIMEOUT_TERSE_NO_MARGIN)
    msg_translated = holy_xasprintf (_("%ds"), timeout);
  else
    msg_translated = holy_xasprintf (_("The highlighted entry will be executed automatically in %ds."), timeout);
  if (!msg_translated)
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      return;
    }

  if (data->timeout_msg == TIMEOUT_UNKNOWN)
    {
      data->timeout_msg = holy_print_message_indented_real (msg_translated,
							    3, 1, data->term, 1)
	<= data->geo.timeout_lines ? TIMEOUT_NORMAL : TIMEOUT_TERSE;
      if (data->timeout_msg == TIMEOUT_TERSE)
	{
	  holy_free (msg_translated);
	  msg_translated = holy_xasprintf (_("%ds"), timeout);
	  if (holy_term_width (data->term) < 10)
	    data->timeout_msg = TIMEOUT_TERSE_NO_MARGIN;
	}
    }

  holy_print_message_indented (msg_translated,
			       data->timeout_msg == TIMEOUT_TERSE_NO_MARGIN ? 0 : 3,
			       data->timeout_msg == TIMEOUT_TERSE_NO_MARGIN ? 0 : 1,
			       data->term);
  holy_free (msg_translated);

  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) {
		      holy_term_cursor_x (&data->geo),
			data->geo.first_entry_y + data->offset });
  holy_term_refresh (data->term);
}

static void
menu_text_set_chosen_entry (int entry, void *dataptr)
{
  struct menu_viewer_data *data = dataptr;
  int oldoffset = data->offset;
  int complete_redraw = 0;

  data->offset = entry - data->first;
  if (data->offset > data->geo.num_entries - 1)
    {
      data->first = entry - (data->geo.num_entries - 1);
      data->offset = data->geo.num_entries - 1;
      complete_redraw = 1;
    }
  if (data->offset < 0)
    {
      data->offset = 0;
      data->first = entry;
      complete_redraw = 1;
    }
  if (complete_redraw)
    print_entries (data->menu, data);
  else
    {
      print_entry (data->geo.first_entry_y + oldoffset, 0,
		   holy_menu_get_entry (data->menu, data->first + oldoffset),
		   data);
      print_entry (data->geo.first_entry_y + data->offset, 1,
		   holy_menu_get_entry (data->menu, data->first + data->offset),
		   data);
    }
  holy_term_refresh (data->term);
}

static void
menu_text_fini (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;

  holy_term_setcursor (data->term, 1);
  holy_term_cls (data->term);
  holy_free (data);
}

static void
menu_text_clear_timeout (void *dataptr)
{
  struct menu_viewer_data *data = dataptr;
  int i;

  for (i = 0; i < data->geo.timeout_lines;i++)
    {
      holy_term_gotoxy (data->term, (struct holy_term_coordinate) {
	  0, data->geo.timeout_y + i });
      holy_print_spaces (data->term, holy_term_width (data->term) - 1);
    }
  if (data->geo.num_entries <= 5 && !data->geo.border)
    {
      holy_term_gotoxy (data->term,
			(struct holy_term_coordinate) {
			  data->geo.first_entry_x + data->geo.entry_width
			    + data->geo.border + 1,
			    data->geo.first_entry_y + data->geo.num_entries - 1
			    });
      holy_putcode (' ', data->term);

      data->geo.timeout_lines = 0;
      data->geo.num_entries++;
      print_entries (data->menu, data);
    }
  holy_term_gotoxy (data->term,
		    (struct holy_term_coordinate) {
		      holy_term_cursor_x (&data->geo),
			data->geo.first_entry_y + data->offset });
  holy_term_refresh (data->term);
}

holy_err_t
holy_menu_try_text (struct holy_term_output *term,
		    int entry, holy_menu_t menu, int nested)
{
  struct menu_viewer_data *data;
  struct holy_menu_viewer *instance;

  instance = holy_zalloc (sizeof (*instance));
  if (!instance)
    return holy_errno;

  data = holy_zalloc (sizeof (*data));
  if (!data)
    {
      holy_free (instance);
      return holy_errno;
    }

  data->term = term;
  instance->data = data;
  instance->set_chosen_entry = menu_text_set_chosen_entry;
  instance->print_timeout = menu_text_print_timeout;
  instance->clear_timeout = menu_text_clear_timeout;
  instance->fini = menu_text_fini;

  data->menu = menu;

  data->offset = entry;
  data->first = 0;

  holy_term_setcursor (data->term, 0);
  holy_menu_init_page (nested, 0, &data->geo, data->term);

  if (data->offset > data->geo.num_entries - 1)
    {
      data->first = data->offset - (data->geo.num_entries - 1);
      data->offset = data->geo.num_entries - 1;
    }

  print_entries (menu, data);
  holy_term_refresh (data->term);
  holy_menu_register_viewer (instance);

  return holy_ERR_NONE;
}
