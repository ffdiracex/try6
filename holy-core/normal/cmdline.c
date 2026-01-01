/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/normal.h>
#include <holy/misc.h>
#include <holy/term.h>
#include <holy/err.h>
#include <holy/types.h>
#include <holy/mm.h>
#include <holy/partition.h>
#include <holy/disk.h>
#include <holy/file.h>
#include <holy/env.h>
#include <holy/i18n.h>
#include <holy/charset.h>

static holy_uint32_t *kill_buf;

static int hist_size;
static holy_uint32_t **hist_lines = 0;
static int hist_pos = 0;
static int hist_end = 0;
static int hist_used = 0;

holy_err_t
holy_set_history (int newsize)
{
  holy_uint32_t **old_hist_lines = hist_lines;
  hist_lines = holy_malloc (sizeof (holy_uint32_t *) * newsize);

  /* Copy the old lines into the new buffer.  */
  if (old_hist_lines)
    {
      /* Remove the lines that don't fit in the new buffer.  */
      if (newsize < hist_used)
	{
	  holy_size_t i;
	  holy_size_t delsize = hist_used - newsize;
	  hist_used = newsize;

	  for (i = 1; i < delsize + 1; i++)
	    {
	      holy_ssize_t pos = hist_end - i;
	      if (pos < 0)
		pos += hist_size;
	      holy_free (old_hist_lines[pos]);
	    }

	  hist_end -= delsize;
	  if (hist_end < 0)
	    hist_end += hist_size;
	}

      if (hist_pos < hist_end)
	holy_memmove (hist_lines, old_hist_lines + hist_pos,
		      (hist_end - hist_pos) * sizeof (holy_uint32_t *));
      else if (hist_used)
	{
	  /* Copy the older part.  */
	  holy_memmove (hist_lines, old_hist_lines + hist_pos,
 			(hist_size - hist_pos) * sizeof (holy_uint32_t *));

	  /* Copy the newer part. */
	  holy_memmove (hist_lines + hist_size - hist_pos, old_hist_lines,
			hist_end * sizeof (holy_uint32_t *));
	}
    }

  holy_free (old_hist_lines);

  hist_size = newsize;
  hist_pos = 0;
  hist_end = hist_used;
  return 0;
}

/* Get the entry POS from the history where `0' is the newest
   entry.  */
static holy_uint32_t *
holy_history_get (unsigned pos)
{
  pos = (hist_pos + pos) % hist_size;
  return hist_lines[pos];
}

static holy_size_t
strlen_ucs4 (const holy_uint32_t *s)
{
  const holy_uint32_t *p = s;

  while (*p)
    p++;

  return p - s;
}

/* Replace the history entry on position POS with the string S.  */
static void
holy_history_set (int pos, holy_uint32_t *s, holy_size_t len)
{
  holy_free (hist_lines[pos]);
  hist_lines[pos] = holy_malloc ((len + 1) * sizeof (holy_uint32_t));
  if (!hist_lines[pos])
    {
      holy_print_error ();
      holy_errno = holy_ERR_NONE;
      return ;
    }
  holy_memcpy (hist_lines[pos], s, len * sizeof (holy_uint32_t));
  hist_lines[pos][len] = 0;
}

/* Insert a new history line S on the top of the history.  */
static void
holy_history_add (holy_uint32_t *s, holy_size_t len)
{
  /* Remove the oldest entry in the history to make room for a new
     entry.  */
  if (hist_used + 1 > hist_size)
    {
      hist_end--;
      if (hist_end < 0)
	hist_end = hist_size + hist_end;

      holy_free (hist_lines[hist_end]);
    }
  else
    hist_used++;

  /* Move to the next position.  */
  hist_pos--;
  if (hist_pos < 0)
    hist_pos = hist_size + hist_pos;

  /* Insert into history.  */
  hist_lines[hist_pos] = NULL;
  holy_history_set (hist_pos, s, len);
}

/* Replace the history entry on position POS with the string S.  */
static void
holy_history_replace (unsigned pos, holy_uint32_t *s, holy_size_t len)
{
  holy_history_set ((hist_pos + pos) % hist_size, s, len);
}

/* A completion hook to print items.  */
static void
print_completion (const char *item, holy_completion_type_t type, int count)
{
  if (count == 0)
    {
      /* If this is the first time, print a label.  */
      
      holy_puts ("");
      switch (type)
	{
	case holy_COMPLETION_TYPE_COMMAND:
	  holy_puts_ (N_("Possible commands are:"));
	  break;
	case holy_COMPLETION_TYPE_DEVICE:
	  holy_puts_ (N_("Possible devices are:"));
	  break;
	case holy_COMPLETION_TYPE_FILE:
	  holy_puts_ (N_("Possible files are:"));
	  break;
	case holy_COMPLETION_TYPE_PARTITION:
	  holy_puts_ (N_("Possible partitions are:"));
	  break;
	case holy_COMPLETION_TYPE_ARGUMENT:
	  holy_puts_ (N_("Possible arguments are:"));
	  break;
	default:
	  /* TRANSLATORS: this message is used if none of above matches.
	     This shouldn't happen but please use the general term for
	     "thing" or "object".  */
	  holy_puts_ (N_("Possible things are:"));
	  break;
	}
      holy_puts ("");
    }

  if (type == holy_COMPLETION_TYPE_PARTITION)
    {
      holy_normal_print_device_info (item);
      holy_errno = holy_ERR_NONE;
    }
  else
    holy_printf (" %s", item);
}

struct cmdline_term
{
  struct holy_term_coordinate pos;
  unsigned ystart, width, height;
  unsigned prompt_len;
  struct holy_term_output *term;
};

static inline void
cl_set_pos (struct cmdline_term *cl_term, holy_size_t lpos)
{
  cl_term->pos.x = (cl_term->prompt_len + lpos) % cl_term->width;
  cl_term->pos.y = cl_term->ystart
    + (cl_term->prompt_len + lpos) / cl_term->width;
  holy_term_gotoxy (cl_term->term, cl_term->pos);
}

static void
cl_set_pos_all (struct cmdline_term *cl_terms, unsigned nterms,
		holy_size_t lpos)
{
  unsigned i;
  for (i = 0; i < nterms; i++)
    cl_set_pos (&cl_terms[i], lpos);
}

static inline void __attribute__ ((always_inline))
cl_print (struct cmdline_term *cl_term, holy_uint32_t c,
	  holy_uint32_t *start, holy_uint32_t *end)
{
  holy_uint32_t *p;

  for (p = start; p < end; p++)
    {
      if (c)
	holy_putcode (c, cl_term->term);
      else
	holy_putcode (*p, cl_term->term);
      cl_term->pos.x++;
      if (cl_term->pos.x >= cl_term->width - 1)
	{
	  cl_term->pos.x = 0;
	  if (cl_term->pos.y >= (unsigned) (cl_term->height - 1))
	    cl_term->ystart--;
	  else
	    cl_term->pos.y++;
	  holy_putcode ('\n', cl_term->term);
	}
    }
}

static void
cl_print_all (struct cmdline_term *cl_terms, unsigned nterms,
	      holy_uint32_t c, holy_uint32_t *start, holy_uint32_t *end)
{
  unsigned i;
  for (i = 0; i < nterms; i++)
    cl_print (&cl_terms[i], c, start, end);
}

static void
init_clterm (struct cmdline_term *cl_term_cur)
{
  cl_term_cur->pos.x = cl_term_cur->prompt_len;
  cl_term_cur->pos.y = holy_term_getxy (cl_term_cur->term).y;
  cl_term_cur->ystart = cl_term_cur->pos.y;
  cl_term_cur->width = holy_term_width (cl_term_cur->term);
  cl_term_cur->height = holy_term_height (cl_term_cur->term);
}


static void
cl_delete (struct cmdline_term *cl_terms, unsigned nterms,
	   holy_uint32_t *buf,
	   holy_size_t lpos, holy_size_t *llen, unsigned len)
{
  if (lpos + len <= (*llen))
    {
      cl_set_pos_all (cl_terms, nterms, (*llen) - len);
      cl_print_all (cl_terms, nterms, ' ', buf + (*llen) - len, buf + (*llen));

      cl_set_pos_all (cl_terms, nterms, lpos);

      holy_memmove (buf + lpos, buf + lpos + len,
		    sizeof (holy_uint32_t) * ((*llen) - lpos + 1));
      (*llen) -= len;
      cl_print_all (cl_terms, nterms, 0, buf + lpos, buf + (*llen));
      cl_set_pos_all (cl_terms, nterms, lpos);
    }
}


static void
cl_insert (struct cmdline_term *cl_terms, unsigned nterms,
	   holy_size_t *lpos, holy_size_t *llen,
	   holy_size_t *max_len, holy_uint32_t **buf,
	   const holy_uint32_t *str)
{
  holy_size_t len = strlen_ucs4 (str);

  if (len + (*llen) >= (*max_len))
    {
      holy_uint32_t *nbuf;
      (*max_len) *= 2;
      nbuf = holy_realloc ((*buf), sizeof (holy_uint32_t) * (*max_len));
      if (nbuf)
	(*buf) = nbuf;
      else
	{
	  holy_print_error ();
	  holy_errno = holy_ERR_NONE;
	  (*max_len) /= 2;
	}
    }

  if (len + (*llen) < (*max_len))
    {
      holy_memmove ((*buf) + (*lpos) + len, (*buf) + (*lpos),
		    ((*llen) - (*lpos) + 1) * sizeof (holy_uint32_t));
      holy_memmove ((*buf) + (*lpos), str, len * sizeof (holy_uint32_t));

      (*llen) += len;
      cl_set_pos_all (cl_terms, nterms, (*lpos));
      cl_print_all (cl_terms, nterms, 0, *buf + (*lpos), *buf + (*llen));
      (*lpos) += len;
      cl_set_pos_all (cl_terms, nterms, (*lpos));
    }
}


/* Get a command-line. If ESC is pushed, return zero,
   otherwise return command line.  */
/* FIXME: The dumb interface is not supported yet.  */
char *
holy_cmdline_get (const char *prompt_translated)
{
  holy_size_t lpos, llen;
  holy_uint32_t *buf;
  holy_size_t max_len = 256;
  int key;
  int histpos = 0;
  struct cmdline_term *cl_terms;
  char *ret;
  unsigned nterms;

  buf = holy_malloc (max_len * sizeof (holy_uint32_t));
  if (!buf)
    return 0;

  lpos = llen = 0;
  buf[0] = '\0';

  {
    holy_term_output_t term;

    FOR_ACTIVE_TERM_OUTPUTS(term)
      if ((holy_term_getxy (term).x) != 0)
	holy_putcode ('\n', term);
  }
  holy_xputs (prompt_translated);
  holy_xputs (" ");
  holy_normal_reset_more ();

  {
    struct cmdline_term *cl_term_cur;
    struct holy_term_output *cur;
    holy_uint32_t *unicode_msg;
    holy_size_t msg_len = holy_strlen (prompt_translated) + 3;

    nterms = 0;
    FOR_ACTIVE_TERM_OUTPUTS(cur)
      nterms++;

    cl_terms = holy_malloc (sizeof (cl_terms[0]) * nterms);
    if (!cl_terms)
      {
	holy_free (buf);
	return 0;
      }
    cl_term_cur = cl_terms;

    unicode_msg = holy_malloc (msg_len * sizeof (holy_uint32_t));
    if (!unicode_msg)
      {
	holy_free (buf);
	holy_free (cl_terms);
	return 0;
      }
    msg_len = holy_utf8_to_ucs4 (unicode_msg, msg_len - 1,
				 (holy_uint8_t *) prompt_translated, -1, 0);
    unicode_msg[msg_len++] = ' ';

    FOR_ACTIVE_TERM_OUTPUTS(cur)
    {
      cl_term_cur->term = cur;
      cl_term_cur->prompt_len = holy_getstringwidth (unicode_msg,
						     unicode_msg + msg_len,
						     cur);
      init_clterm (cl_term_cur);
      cl_term_cur++;
    }
    holy_free (unicode_msg);
  }

  if (hist_used == 0)
    holy_history_add (buf, llen);

  holy_refresh ();

  while ((key = holy_getkey ()) != '\n' && key != '\r')
    {
      switch (key)
	{
	case holy_TERM_CTRL | 'a':
	case holy_TERM_KEY_HOME:
	  lpos = 0;
	  cl_set_pos_all (cl_terms, nterms, lpos);
	  break;

	case holy_TERM_CTRL | 'b':
	case holy_TERM_KEY_LEFT:
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos_all (cl_terms, nterms, lpos);
	    }
	  break;

	case holy_TERM_CTRL | 'e':
	case holy_TERM_KEY_END:
	  lpos = llen;
	  cl_set_pos_all (cl_terms, nterms, lpos);
	  break;

	case holy_TERM_CTRL | 'f':
	case holy_TERM_KEY_RIGHT:
	  if (lpos < llen)
	    {
	      lpos++;
	      cl_set_pos_all (cl_terms, nterms, lpos);
	    }
	  break;

	case holy_TERM_CTRL | 'i':
	case '\t':
	  {
	    int restore;
	    char *insertu8;
	    char *bufu8;
	    holy_uint32_t c;

	    c = buf[lpos];
	    buf[lpos] = '\0';

	    bufu8 = holy_ucs4_to_utf8_alloc (buf, lpos);
	    buf[lpos] = c;
	    if (!bufu8)
	      {
		holy_print_error ();
		holy_errno = holy_ERR_NONE;
		break;
	      }

	    insertu8 = holy_normal_do_completion (bufu8, &restore,
						  print_completion);
	    holy_free (bufu8);

	    holy_normal_reset_more ();

	    if (restore)
	      {
		unsigned i;

		/* Restore the prompt.  */
		holy_xputs ("\n");
		holy_xputs (prompt_translated);
		holy_xputs (" ");

		for (i = 0; i < nterms; i++)
		  init_clterm (&cl_terms[i]);

		cl_print_all (cl_terms, nterms, 0, buf, buf + llen);
	      }

	    if (insertu8)
	      {
		holy_size_t insertlen;
		holy_ssize_t t;
		holy_uint32_t *insert;

		insertlen = holy_strlen (insertu8);
		insert = holy_malloc ((insertlen + 1) * sizeof (holy_uint32_t));
		if (!insert)
		  {
		    holy_free (insertu8);
		    holy_print_error ();
		    holy_errno = holy_ERR_NONE;
		    break;
		  }
		t = holy_utf8_to_ucs4 (insert, insertlen,
				       (holy_uint8_t *) insertu8,
				       insertlen, 0);
		if (t > 0)
		  {
		    if (insert[t-1] == ' ' && buf[lpos] == ' ')
		      {
			insert[t-1] = 0;
			if (t != 1)
			  cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, insert);
			lpos++;
		      }
		    else
		      {
			insert[t] = 0;
			cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, insert);
		      }
		  }

		holy_free (insertu8);
		holy_free (insert);
	      }
	    cl_set_pos_all (cl_terms, nterms, lpos);
	  }
	  break;

	case holy_TERM_CTRL | 'k':
	  if (lpos < llen)
	    {
	      holy_free (kill_buf);

	      kill_buf = holy_malloc ((llen - lpos + 1)
				      * sizeof (holy_uint32_t));
	      if (holy_errno)
		{
		  holy_print_error ();
		  holy_errno = holy_ERR_NONE;
		}
	      else
		{
		  holy_memcpy (kill_buf, buf + lpos,
			       (llen - lpos + 1) * sizeof (holy_uint32_t));
		  kill_buf[llen - lpos] = 0;
		}

	      cl_delete (cl_terms, nterms,
			 buf, lpos, &llen, llen - lpos);
	    }
	  break;

	case holy_TERM_CTRL | 'n':
	case holy_TERM_KEY_DOWN:
	  {
	    holy_uint32_t *hist;

	    lpos = 0;

	    if (histpos > 0)
	      {
		holy_history_replace (histpos, buf, llen);
		histpos--;
	      }

	    cl_delete (cl_terms, nterms,
		       buf, lpos, &llen, llen);
	    hist = holy_history_get (histpos);
	    cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, hist);

	    break;
	  }

	case holy_TERM_KEY_UP:
	case holy_TERM_CTRL | 'p':
	  {
	    holy_uint32_t *hist;

	    lpos = 0;

	    if (histpos < hist_used - 1)
	      {
		holy_history_replace (histpos, buf, llen);
		histpos++;
	      }

	    cl_delete (cl_terms, nterms,
		       buf, lpos, &llen, llen);
	    hist = holy_history_get (histpos);

	    cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, hist);
	  }
	  break;

	case holy_TERM_CTRL | 'u':
	  if (lpos > 0)
	    {
	      holy_size_t n = lpos;

	      holy_free (kill_buf);

	      kill_buf = holy_malloc ((n + 1) * sizeof(holy_uint32_t));
	      if (holy_errno)
		{
		  holy_print_error ();
		  holy_errno = holy_ERR_NONE;
		}
	      if (kill_buf)
		{
		  holy_memcpy (kill_buf, buf, n * sizeof(holy_uint32_t));
		  kill_buf[n] = 0;
		}

	      lpos = 0;
	      cl_set_pos_all (cl_terms, nterms, lpos);
	      cl_delete (cl_terms, nterms,
			 buf, lpos, &llen, n);
	    }
	  break;

	case holy_TERM_CTRL | 'y':
	  if (kill_buf)
	    cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, kill_buf);
	  break;

	case '\e':
	  holy_free (cl_terms);
	  holy_free (buf);
	  return 0;

	case '\b':
	  if (lpos > 0)
	    {
	      lpos--;
	      cl_set_pos_all (cl_terms, nterms, lpos);
	    }
          else
            break;
	  /* fall through */

	case holy_TERM_CTRL | 'd':
	case holy_TERM_KEY_DC:
	  if (lpos < llen)
	    cl_delete (cl_terms, nterms,
		       buf, lpos, &llen, 1);
	  break;

	default:
	  if (holy_isprint (key))
	    {
	      holy_uint32_t str[2];

	      str[0] = key;
	      str[1] = '\0';
	      cl_insert (cl_terms, nterms, &lpos, &llen, &max_len, &buf, str);
	    }
	  break;
	}

      holy_refresh ();
    }

  holy_xputs ("\n");
  holy_refresh ();

  histpos = 0;
  if (strlen_ucs4 (buf) > 0)
    {
      holy_uint32_t empty[] = { 0 };
      holy_history_replace (histpos, buf, llen);
      holy_history_add (empty, 0);
    }

  ret = holy_ucs4_to_utf8_alloc (buf, llen + 1);
  holy_free (buf);
  holy_free (cl_terms);
  return ret;
}
