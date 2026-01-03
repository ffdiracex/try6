/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/file.h>
#include <holy/dl.h>
#include <holy/env.h>
#include <holy/normal.h>
#include <holy/charset.h>
#include <holy/i18n.h>

struct term_state
{
  struct term_state *next;
  struct holy_unicode_glyph *backlog_glyphs;
  const holy_uint32_t *backlog_ucs4;
  int backlog_fixed_tab;
  holy_size_t backlog_len;

  int bidi_stack_depth;
  holy_uint8_t bidi_stack[holy_BIDI_MAX_EXPLICIT_LEVEL];
  int invalid_pushes;

  void *free;
  int num_lines;
  char *term_name;
};

static int
print_ucs4_real (const holy_uint32_t * str,
		 const holy_uint32_t * last_position,
		 int margin_left, int margin_right,
		 struct holy_term_output *term, int backlog,
		 int dry_run, int fixed_tab, unsigned skip_lines,
		 unsigned max_lines,
		 holy_uint32_t contchar, int fill_right,
		 struct holy_term_pos *pos);

static struct term_state *term_states = NULL;

/* If the more pager is active.  */
static int holy_more;

static void
putcode_real (holy_uint32_t code, struct holy_term_output *term, int fixed_tab);

void
holy_normal_reset_more (void)
{
  static struct term_state *state;
  for (state = term_states; state; state = state->next)
    state->num_lines = 0;
}

static void
print_more (void)
{
  char key;
  struct holy_term_coordinate *pos;
  holy_term_output_t term;
  holy_uint32_t *unicode_str, *unicode_last_position;

  /* TRANSLATORS: This has to fit on one line.  It's ok to include few
     words but don't write poems.  */
  holy_utf8_to_ucs4_alloc (_("--MORE--"), &unicode_str,
			   &unicode_last_position);

  if (!unicode_str)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }

  pos = holy_term_save_pos ();

  holy_setcolorstate (holy_TERM_COLOR_HIGHLIGHT);

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    holy_print_ucs4 (unicode_str, unicode_last_position, 0, 0, term);
  }
  holy_setcolorstate (holy_TERM_COLOR_NORMAL);

  holy_free (unicode_str);
  
  key = holy_getkey ();

  /* Remove the message.  */
  holy_term_restore_pos (pos);
  FOR_ACTIVE_TERM_OUTPUTS(term)
    holy_print_spaces (term, 8);
  holy_term_restore_pos (pos);
  holy_free (pos);

  /* Scroll one line or an entire page, depending on the key.  */

  if (key == '\r' || key =='\n')
    {
      static struct term_state *state;
      for (state = term_states; state; state = state->next)
	state->num_lines--;
    }
  else
    holy_normal_reset_more ();
}

void
holy_set_more (int onoff)
{
  holy_more = !!onoff;
  holy_normal_reset_more ();
}

enum
  {
    holy_CP437_UPDOWNARROW     = 0x12,
    holy_CP437_UPARROW         = 0x18,
    holy_CP437_DOWNARROW       = 0x19,
    holy_CP437_RIGHTARROW      = 0x1a,
    holy_CP437_LEFTARROW       = 0x1b,
    holy_CP437_VLINE           = 0xb3,
    holy_CP437_CORNER_UR       = 0xbf,
    holy_CP437_CORNER_LL       = 0xc0,
    holy_CP437_HLINE           = 0xc4,
    holy_CP437_CORNER_LR       = 0xd9,
    holy_CP437_CORNER_UL       = 0xda,
  };

static holy_uint32_t
map_code (holy_uint32_t in, struct holy_term_output *term)
{
  if (in <= 0x7f)
    return in;

  switch (term->flags & holy_TERM_CODE_TYPE_MASK)
    {
    case holy_TERM_CODE_TYPE_CP437:
      switch (in)
	{
	case holy_UNICODE_LEFTARROW:
	  return holy_CP437_LEFTARROW;
	case holy_UNICODE_UPARROW:
	  return holy_CP437_UPARROW;
	case holy_UNICODE_UPDOWNARROW:
	  return holy_CP437_UPDOWNARROW;
	case holy_UNICODE_RIGHTARROW:
	  return holy_CP437_RIGHTARROW;
	case holy_UNICODE_DOWNARROW:
	  return holy_CP437_DOWNARROW;
	case holy_UNICODE_HLINE:
	  return holy_CP437_HLINE;
	case holy_UNICODE_VLINE:
	  return holy_CP437_VLINE;
	case holy_UNICODE_CORNER_UL:
	  return holy_CP437_CORNER_UL;
	case holy_UNICODE_CORNER_UR:
	  return holy_CP437_CORNER_UR;
	case holy_UNICODE_CORNER_LL:
	  return holy_CP437_CORNER_LL;
	case holy_UNICODE_CORNER_LR:
	  return holy_CP437_CORNER_LR;
	}
      return '?';
    case holy_TERM_CODE_TYPE_ASCII:
      /* Better than nothing.  */
      switch (in)
	{
	case holy_UNICODE_LEFTARROW:
	  return '<';
		
	case holy_UNICODE_UPARROW:
	  return '^';
	  
	case holy_UNICODE_RIGHTARROW:
	  return '>';
		
	case holy_UNICODE_DOWNARROW:
	  return 'v';
		  
	case holy_UNICODE_HLINE:
	  return '-';
		  
	case holy_UNICODE_VLINE:
	  return '|';
		  
	case holy_UNICODE_CORNER_UL:
	case holy_UNICODE_CORNER_UR:
	case holy_UNICODE_CORNER_LL:
	case holy_UNICODE_CORNER_LR:
	  return '+';
		
	}
      return '?';
    }
  return in;
}

void
holy_puts_terminal (const char *str, struct holy_term_output *term)
{
  holy_uint32_t *unicode_str, *unicode_last_position;
  holy_error_push ();
  holy_utf8_to_ucs4_alloc (str, &unicode_str,
			   &unicode_last_position);
  holy_error_pop ();
  if (!unicode_str)
    {
      for (; *str; str++)
	{
	  struct holy_unicode_glyph c =
	    {
	      .variant = 0,
	      .attributes = 0,
	      .ncomb = 0,
	      .estimated_width = 1,
	      .base = *str
	    };

	  FOR_ACTIVE_TERM_OUTPUTS(term)
	  {
	    (term->putchar) (term, &c);
	  }
	  if (*str == '\n')
	    {
	      c.base = '\r';
	      FOR_ACTIVE_TERM_OUTPUTS(term)
	      {
		(term->putchar) (term, &c);
	      }
	    }
	}
      return;
    }

  print_ucs4_real (unicode_str, unicode_last_position, 0, 0, term,
		   0, 0, 0, 0, -1, 0, 0, 0);
  holy_free (unicode_str);
}

struct holy_term_coordinate *
holy_term_save_pos (void)
{
  struct holy_term_output *cur;
  unsigned cnt = 0;
  struct holy_term_coordinate *ret, *ptr;
  
  FOR_ACTIVE_TERM_OUTPUTS(cur)
    cnt++;

  ret = holy_malloc (cnt * sizeof (ret[0]));
  if (!ret)
    return NULL;

  ptr = ret;
  FOR_ACTIVE_TERM_OUTPUTS(cur)
    *ptr++ = holy_term_getxy (cur);

  return ret;
}

void
holy_term_restore_pos (struct holy_term_coordinate *pos)
{
  struct holy_term_output *cur;
  struct holy_term_coordinate *ptr = pos;

  if (!pos)
    return;

  FOR_ACTIVE_TERM_OUTPUTS(cur)
  {
    holy_term_gotoxy (cur, *ptr);
    ptr++;
  }
}

static void 
holy_terminal_autoload_free (void)
{
  struct holy_term_autoload *cur, *next;
  unsigned i;
  for (i = 0; i < 2; i++)
    for (cur = i ? holy_term_input_autoload : holy_term_output_autoload;
	 cur; cur = next)
      {
	next = cur->next;
	holy_free (cur->name);
	holy_free (cur->modname);
	holy_free (cur);
      }
  holy_term_input_autoload = NULL;
  holy_term_output_autoload = NULL;
}

/* Read the file terminal.lst for auto-loading.  */
void
read_terminal_list (const char *prefix)
{
  char *filename;
  holy_file_t file;
  char *buf = NULL;

  if (!prefix)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }
  
  filename = holy_xasprintf ("%s/" holy_TARGET_CPU "-" holy_PLATFORM
			     "/terminal.lst", prefix);
  if (!filename)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }

  file = holy_file_open (filename);
  holy_free (filename);
  if (!file)
    {
      holy_errno = holy_ERR_NONE;
      return;
    }

  /* Override previous terminal.lst.  */
  holy_terminal_autoload_free ();

  for (;; holy_free (buf))
    {
      char *p, *name;
      struct holy_term_autoload *cur;
      struct holy_term_autoload **target = NULL;
      
      buf = holy_file_getline (file);
	
      if (! buf)
	break;

      p = buf;
      while (holy_isspace (p[0]))
	p++;

      switch (p[0])
	{
	case 'i':
	  target = &holy_term_input_autoload;
	  break;

	case 'o':
	  target = &holy_term_output_autoload;
	  break;
	}
      if (!target)
	continue;
      
      name = p + 1;
            
      p = holy_strchr (name, ':');
      if (! p)
	continue;
      *p = 0;

      p++;
      while (*p == ' ' || *p == '\t')
	p++;

      cur = holy_malloc (sizeof (*cur));
      if (!cur)
	{
	  holy_errno = holy_ERR_NONE;
	  continue;
	}
      
      cur->name = holy_strdup (name);
      if (! cur->name)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_free (cur);
	  continue;
	}
	
      cur->modname = holy_strdup (p);
      if (! cur->modname)
	{
	  holy_errno = holy_ERR_NONE;
	  holy_free (cur->name);
	  holy_free (cur);
	  continue;
	}
      cur->next = *target;
      *target = cur;
    }
  
  holy_file_close (file);

  holy_errno = holy_ERR_NONE;
}

static void
putglyph (const struct holy_unicode_glyph *c, struct holy_term_output *term,
	  int fixed_tab)
{
  struct holy_unicode_glyph c2 =
    {
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .estimated_width = 1
    };

  if (c->base == '\t' && fixed_tab)
    {
      int n;

      n = holy_TERM_TAB_WIDTH;
      c2.base = ' ';
      while (n--)
	(term->putchar) (term, &c2);

      return;
    }

  if (c->base == '\t' && term->getxy)
    {
      int n;

      n = holy_TERM_TAB_WIDTH - ((term->getxy (term).x)
				 % holy_TERM_TAB_WIDTH);
      c2.base = ' ';
      while (n--)
	(term->putchar) (term, &c2);

      return;
    }

  if ((term->flags & holy_TERM_CODE_TYPE_MASK)
      == holy_TERM_CODE_TYPE_UTF8_LOGICAL
      || (term->flags & holy_TERM_CODE_TYPE_MASK)
      == holy_TERM_CODE_TYPE_UTF8_VISUAL)
    {
      int i;
      c2.estimated_width = holy_term_getcharwidth (term, c);
      for (i = -1; i < (int) c->ncomb; i++)
	{
	  holy_uint8_t u8[20], *ptr;
	  holy_uint32_t code;
	      
	  if (i == -1)
	    {
	      code = c->base;
	      if ((term->flags & holy_TERM_CODE_TYPE_MASK)
		  == holy_TERM_CODE_TYPE_UTF8_VISUAL)
		{
		  if ((c->attributes & holy_UNICODE_GLYPH_ATTRIBUTE_MIRROR))
		    code = holy_unicode_mirror_code (code);
		  code = holy_unicode_shape_code (code, c->attributes);
		}
	    }
	  else
	    code = holy_unicode_get_comb (c) [i].code;

	  holy_ucs4_to_utf8 (&code, 1, u8, sizeof (u8));

	  for (ptr = u8; *ptr; ptr++)
	    {
	      c2.base = *ptr;
	      (term->putchar) (term, &c2);
	      c2.estimated_width = 0;
	    }
	}
      c2.estimated_width = 1;
    }
  else
    (term->putchar) (term, c);

  if (c->base == '\n')
    {
      c2.base = '\r';
      (term->putchar) (term, &c2);
    }
}

static void
putcode_real (holy_uint32_t code, struct holy_term_output *term, int fixed_tab)
{
  struct holy_unicode_glyph c =
    {
      .variant = 0,
      .attributes = 0,
      .ncomb = 0,
      .estimated_width = 1
    };

  c.base = map_code (code, term);
  putglyph (&c, term, fixed_tab);
}

/* Put a Unicode character.  */
void
holy_putcode (holy_uint32_t code, struct holy_term_output *term)
{
  /* Combining character by itself?  */
  if (holy_unicode_get_comb_type (code) != holy_UNICODE_COMB_NONE)
    return;

  putcode_real (code, term, 0);
}

static holy_ssize_t
get_maxwidth (struct holy_term_output *term,
	      int margin_left, int margin_right)
{
  struct holy_unicode_glyph space_glyph = {
    .base = ' ',
    .variant = 0,
    .attributes = 0,
    .ncomb = 0,
  };
  return (holy_term_width (term)
	  - holy_term_getcharwidth (term, &space_glyph)
	  * (margin_left + margin_right) - 1);
}

static holy_ssize_t
get_startwidth (struct holy_term_output *term,
		int margin_left)
{
  return (term->getxy (term).x) - margin_left;
}

static void
fill_margin (struct holy_term_output *term, int r)
{
  int sp = (term->getwh (term).x)
    - (term->getxy (term).x) - r;
  if (sp > 0)
    holy_print_spaces (term, sp);
}

static int
print_ucs4_terminal (const holy_uint32_t * str,
		     const holy_uint32_t * last_position,
		     int margin_left, int margin_right,
		     struct holy_term_output *term,
		     struct term_state *state,
		     int dry_run, int fixed_tab, unsigned skip_lines,
		     unsigned max_lines,
		     holy_uint32_t contchar,
		     int primitive_wrap, int fill_right, struct holy_term_pos *pos)
{
  const holy_uint32_t *ptr;
  holy_ssize_t startwidth = dry_run ? 0 : get_startwidth (term, margin_left);
  holy_ssize_t line_width = startwidth;
  holy_ssize_t lastspacewidth = 0;
  holy_ssize_t max_width = get_maxwidth (term, margin_left, margin_right);
  const holy_uint32_t *line_start = str, *last_space = str - 1;
  int lines = 0;
  int i;
  struct term_state local_state;

  if (!state)
    {
      holy_memset (&local_state, 0, sizeof (local_state));
      state = &local_state;
    }

  for (i = 0; i < state->bidi_stack_depth; i++)
    putcode_real (state->bidi_stack[i] | (holy_UNICODE_LRE & ~0xff),
		  term, fixed_tab);

  for (ptr = str; ptr < last_position; ptr++)
    {
      holy_ssize_t last_width = 0;

      switch (*ptr)
	{
	case holy_UNICODE_LRE:
	case holy_UNICODE_RLE:
	case holy_UNICODE_LRO:
	case holy_UNICODE_RLO:
	  if (state->bidi_stack_depth >= (int) ARRAY_SIZE (state->bidi_stack))
	    state->invalid_pushes++;
	  else
	    state->bidi_stack[state->bidi_stack_depth++] = *ptr;
	  break;
	case holy_UNICODE_PDF:
	  if (state->invalid_pushes)
	    state->invalid_pushes--;
	  else if (state->bidi_stack_depth)
	    state->bidi_stack_depth--;
	  break;
	}
      if (holy_unicode_get_comb_type (*ptr) == holy_UNICODE_COMB_NONE)
	{
	  struct holy_unicode_glyph c = {
	    .variant = 0,
	    .attributes = 0,
	    .ncomb = 0,
	  };
	  c.base = *ptr;
	  if (pos)
	    {
	      pos[ptr - str].x = line_width;
	      pos[ptr - str].y = lines;
	      pos[ptr - str].valid = 1;
	    }
	  line_width += last_width = holy_term_getcharwidth (term, &c);
	}

      if (*ptr == ' ' && !primitive_wrap)
	{
	  lastspacewidth = line_width;
	  last_space = ptr;
	}

      if (line_width > max_width || *ptr == '\n')
	{
	  const holy_uint32_t *ptr2;
	  int wasn = (*ptr == '\n');

	  if (wasn)
	    {
	      state->bidi_stack_depth = 0;
	      state->invalid_pushes = 0;
	    }

	  if (line_width > max_width && last_space > line_start)
	    ptr = last_space;
	  else if (line_width > max_width 
		   && line_start == str && line_width - lastspacewidth < max_width - 5)
	    {
	      ptr = str;
	      lastspacewidth = startwidth;
	    }
	  else
	    lastspacewidth = line_width - last_width;

	  lines++;

	  if (!skip_lines && !dry_run)
	    {
	      for (ptr2 = line_start; ptr2 < ptr; ptr2++)
		{
		  /* Skip combining characters on non-UTF8 terminals.  */
		  if ((term->flags & holy_TERM_CODE_TYPE_MASK)
		      != holy_TERM_CODE_TYPE_UTF8_LOGICAL
		      && holy_unicode_get_comb_type (*ptr2)
		      != holy_UNICODE_COMB_NONE)
		    continue;
		  putcode_real (*ptr2, term, fixed_tab);
		}

	      if (!wasn && contchar)
		putcode_real (contchar, term, fixed_tab);
	      if (fill_right)
		fill_margin (term, margin_right);

	      if (!contchar || max_lines != 1)
		holy_putcode ('\n', term);
	      if (state != &local_state && ++state->num_lines
		  >= (holy_ssize_t) holy_term_height (term) - 2)
		{
		  state->backlog_ucs4 = (ptr == last_space || *ptr == '\n') 
		    ? ptr + 1 : ptr;
		  state->backlog_len = last_position - state->backlog_ucs4;
		  state->backlog_fixed_tab = fixed_tab;
		  return 1;
		}
	    }

	  line_width -= lastspacewidth;
	  if (ptr == last_space || *ptr == '\n')
	    ptr++;
	  else if (pos)
	      {
		pos[ptr - str].x = line_width - last_width;
		pos[ptr - str].y = lines;
		pos[ptr - str].valid = 1;
	      }

	  line_start = ptr;

	  if (skip_lines)
	    skip_lines--;
	  else if (max_lines != (unsigned) -1)
	    {
	      max_lines--;
	      if (!max_lines)
		break;
	    }
	  if (!skip_lines && !dry_run)
	    {
	      if (!contchar)
		holy_print_spaces (term, margin_left);
	      else
		holy_term_gotoxy (term, (struct holy_term_coordinate)
				  { margin_left, holy_term_getxy (term).y });
	      for (i = 0; i < state->bidi_stack_depth; i++)
		putcode_real (state->bidi_stack[i] | (holy_UNICODE_LRE & ~0xff),
			      term, fixed_tab);
	    }
	}
    }

  if (pos)
    {
      pos[ptr - str].x = line_width;
      pos[ptr - str].y = lines;
      pos[ptr - str].valid = 1;
    }

  if (line_start < last_position)
      lines++;
  if (!dry_run && !skip_lines && max_lines)
    {
      const holy_uint32_t *ptr2;

      for (ptr2 = line_start; ptr2 < last_position; ptr2++)
	{
	  /* Skip combining characters on non-UTF8 terminals.  */
	  if ((term->flags & holy_TERM_CODE_TYPE_MASK)
	      != holy_TERM_CODE_TYPE_UTF8_LOGICAL
	      && holy_unicode_get_comb_type (*ptr2)
	      != holy_UNICODE_COMB_NONE)
	    continue;
	  putcode_real (*ptr2, term, fixed_tab);
	}

      if (fill_right)
	fill_margin (term, margin_right);
    }
  return dry_run ? lines : 0;
}

static struct term_state *
find_term_state (struct holy_term_output *term)
{
  struct term_state *state;
  for (state = term_states; state; state = state->next)
    if (holy_strcmp (state->term_name, term->name) == 0)
      return state;

  state = holy_zalloc (sizeof (*state));
  if (!state)
    {
      holy_errno = holy_ERR_NONE;
      return NULL;
    }

  state->term_name = holy_strdup (term->name);
  state->next = term_states;
  term_states = state;

  return state;
}

static int
put_glyphs_terminal (struct holy_unicode_glyph *visual,
		     holy_ssize_t visual_len,
		     int margin_left, int margin_right,
		     struct holy_term_output *term,
		     struct term_state *state, int fixed_tab,
		     holy_uint32_t contchar,
		     int fill_right)
{
  struct holy_unicode_glyph *visual_ptr;
  int since_last_nl = 1;
  for (visual_ptr = visual; visual_ptr < visual + visual_len; visual_ptr++)
    {
      if (visual_ptr->base == '\n' && fill_right)
	fill_margin (term, margin_right);

      putglyph (visual_ptr, term, fixed_tab);
      since_last_nl++;
      if (visual_ptr->base == '\n')
	{
	  since_last_nl = 0;
	  if (state && ++state->num_lines
	      >= (holy_ssize_t) holy_term_height (term) - 2)
	    {
	      state->backlog_glyphs = visual_ptr + 1;
	      state->backlog_len = visual_len - (visual_ptr - visual) - 1;
	      state->backlog_fixed_tab = fixed_tab;
	      return 1;
	    }

	  if (!contchar)
	    holy_print_spaces (term, margin_left);
	  else
	    holy_term_gotoxy (term,
			      (struct holy_term_coordinate)
			      { margin_left, holy_term_getxy (term).y });
	}
      holy_unicode_destroy_glyph (visual_ptr);
    }
  if (fill_right && since_last_nl)
	fill_margin (term, margin_right);

  return 0;
}

static int
print_backlog (struct holy_term_output *term,
	       int margin_left, int margin_right)
{
  struct term_state *state = find_term_state (term);

  if (!state)
    return 0;

  if (state->backlog_ucs4)
    {
      int ret;
      ret = print_ucs4_terminal (state->backlog_ucs4,
				 state->backlog_ucs4 + state->backlog_len,
				 margin_left, margin_right, term, state, 0,
				 state->backlog_fixed_tab, 0, -1, 0, 0,
				 0, 0);
      if (!ret)
	{
	  holy_free (state->free);
	  state->free = NULL;
	  state->backlog_len = 0;
	  state->backlog_ucs4 = 0;
	}
      return ret;
    }

  if (state->backlog_glyphs)
    {
      int ret;
      ret = put_glyphs_terminal (state->backlog_glyphs,
				 state->backlog_len,
				 margin_left, margin_right, term, state,
				 state->backlog_fixed_tab, 0, 0);
      if (!ret)
	{
	  holy_free (state->free);
	  state->free = NULL;
	  state->backlog_len = 0;
	  state->backlog_glyphs = 0;
	}
      return ret;
    }

  return 0;
}

static holy_size_t
getcharwidth (const struct holy_unicode_glyph *c, void *term)
{
  return holy_term_getcharwidth (term, c);
}

static int
print_ucs4_real (const holy_uint32_t * str,
		 const holy_uint32_t * last_position,
		 int margin_left, int margin_right,
		 struct holy_term_output *term, int backlog,
		 int dry_run, int fixed_tab, unsigned skip_lines,
		 unsigned max_lines,
		 holy_uint32_t contchar, int fill_right,
		 struct holy_term_pos *pos)
{
  struct term_state *state = NULL;

  if (!dry_run)
    {
      struct holy_term_coordinate xy;
      if (backlog)
	state = find_term_state (term);

      xy = term->getxy (term);
      
      if (xy.x < margin_left)
	{
	  if (!contchar)
	    holy_print_spaces (term, margin_left - xy.x);
	  else
	    holy_term_gotoxy (term, (struct holy_term_coordinate) {margin_left,
		  xy.y});
	}
    }

  if ((term->flags & holy_TERM_CODE_TYPE_MASK)
      == holy_TERM_CODE_TYPE_VISUAL_GLYPHS
      || (term->flags & holy_TERM_CODE_TYPE_MASK)
      == holy_TERM_CODE_TYPE_UTF8_VISUAL)
    {
      holy_ssize_t visual_len;
      struct holy_unicode_glyph *visual;
      holy_ssize_t visual_len_show;
      struct holy_unicode_glyph *visual_show;
      int ret;
      struct holy_unicode_glyph *vptr;

      visual_len = holy_bidi_logical_to_visual (str, last_position - str,
						&visual, getcharwidth, term,
						get_maxwidth (term, 
							      margin_left,
							      margin_right),
						dry_run ? 0 : get_startwidth (term, 
									      margin_left),
						contchar, pos, !!contchar);
      if (visual_len < 0)
	{
	  holy_print_error ();
	  return 0;
	}
      visual_show = visual;
      for (; skip_lines && visual_show < visual + visual_len; visual_show++)
	if (visual_show->base == '\n')
	  skip_lines--;
      if (max_lines != (unsigned) -1)
	{
	  for (vptr = visual_show;
	       max_lines && vptr < visual + visual_len; vptr++)
	    if (vptr->base == '\n')
	      max_lines--;

	  visual_len_show = vptr - visual_show;	  
	}
      else
	visual_len_show = visual + visual_len - visual_show;

      if (dry_run)
	{
	  ret = 0;
	  for (vptr = visual_show; vptr < visual_show + visual_len_show; vptr++)
	    if (vptr->base == '\n')
	      ret++;
	  if (visual_len_show && visual[visual_len_show - 1].base != '\n')
	    ret++;
	  for (vptr = visual; vptr < visual + visual_len; vptr++)
	    holy_unicode_destroy_glyph (vptr);
	  holy_free (visual);
	}
      else
	{
	  ret = put_glyphs_terminal (visual_show, visual_len_show, margin_left,
				     margin_right,
				     term, state, fixed_tab, contchar, fill_right);

	  if (!ret)
	    holy_free (visual);
	  else
	    state->free = visual;
	}
      return ret;
    }
  return print_ucs4_terminal (str, last_position, margin_left, margin_right,
			      term, state, dry_run, fixed_tab, skip_lines,
			      max_lines, contchar, !!contchar, fill_right, pos);
}

void
holy_print_ucs4_menu (const holy_uint32_t * str,
		      const holy_uint32_t * last_position,
		      int margin_left, int margin_right,
		      struct holy_term_output *term,
		      int skip_lines, int max_lines,
		      holy_uint32_t contchar,
		      struct holy_term_pos *pos)
{
  print_ucs4_real (str, last_position, margin_left, margin_right,
		   term, 0, 0, 1, skip_lines, max_lines,
		   contchar, 1, pos);
}

void
holy_print_ucs4 (const holy_uint32_t * str,
		 const holy_uint32_t * last_position,
		 int margin_left, int margin_right,
		 struct holy_term_output *term)
{
  print_ucs4_real (str, last_position, margin_left, margin_right,
		   term, 0, 0, 1, 0, -1, 0, 0, 0);
}

int
holy_ucs4_count_lines (const holy_uint32_t * str,
		       const holy_uint32_t * last_position,
		       int margin_left, int margin_right,
		       struct holy_term_output *term)
{
  return print_ucs4_real (str, last_position, margin_left, margin_right,
			  term, 0, 1, 1, 0, -1, 0, 0, 0);
}

void
holy_xnputs (const char *str, holy_size_t msg_len)
{
  holy_uint32_t *unicode_str = NULL, *unicode_last_position;
  int backlog = 0;
  holy_term_output_t term;

  holy_error_push ();

  unicode_str = holy_malloc (msg_len * sizeof (holy_uint32_t));
 
  holy_error_pop ();

  if (!unicode_str)
    {
      for (; msg_len--; str++, msg_len++)
	{
	  struct holy_unicode_glyph c =
	    {
	      .variant = 0,
	      .attributes = 0,
	      .ncomb = 0,
	      .estimated_width = 1,
	      .base = *str
	    };

	  FOR_ACTIVE_TERM_OUTPUTS(term)
	  {
	    (term->putchar) (term, &c);
	  }
	  if (*str == '\n')
	    {
	      c.base = '\r';
	      FOR_ACTIVE_TERM_OUTPUTS(term)
	      {
		(term->putchar) (term, &c);
	      }
	    }
	}

      return;
    }

  msg_len = holy_utf8_to_ucs4 (unicode_str, msg_len,
			       (holy_uint8_t *) str, -1, 0);
  unicode_last_position = unicode_str + msg_len;

  FOR_ACTIVE_TERM_OUTPUTS(term)
  {
    int cur;
    cur = print_ucs4_real (unicode_str, unicode_last_position, 0, 0,
			   term, holy_more, 0, 0, 0, -1, 0, 0, 0);
    if (cur)
      backlog = 1;
  }
  while (backlog)
    {
      print_more ();
      backlog = 0;
      FOR_ACTIVE_TERM_OUTPUTS(term)
      {
	int cur;
	cur = print_backlog (term, 0, 0);
	if (cur)
	  backlog = 1;
      }
    }
  holy_free (unicode_str);
}

void
holy_xputs_normal (const char *str)
{
  holy_xnputs (str, holy_strlen (str));
}

void
holy_cls (void)
{
  struct holy_term_output *term;

  FOR_ACTIVE_TERM_OUTPUTS(term)  
  {
    if ((term->flags & holy_TERM_DUMB) || (holy_env_get ("debug")))
      {
	holy_putcode ('\n', term);
	holy_term_refresh (term);
      }
    else
      (term->cls) (term);
  }
}
