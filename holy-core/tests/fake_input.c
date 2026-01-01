/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/mm.h>
#include <holy/dl.h>
#include <holy/test.h>

holy_MOD_LICENSE ("GPLv2+");

static int *seq;
static int seqptr, nseq;
static struct holy_term_input *saved;
static int fake_input;

static int
fake_getkey (struct holy_term_input *term __attribute__ ((unused)))
{
  if (seq && seqptr < nseq)
    return seq[seqptr++];
  return -1;
}

static struct holy_term_input fake_input_term =
  {
    .name = "fake",
    .getkey = fake_getkey
  };

void
holy_terminal_input_fake_sequence (int *seq_in, int nseq_in)
{
  if (!fake_input)
    saved = holy_term_inputs;
  if (seq)
    holy_free (seq);
  seq = holy_malloc (nseq_in * sizeof (seq[0]));
  if (!seq)
    return;

  holy_term_inputs = &fake_input_term;
  holy_memcpy (seq, seq_in, nseq_in * sizeof (seq[0]));

  nseq = nseq_in;
  seqptr = 0;
  fake_input = 1;
}

void
holy_terminal_input_fake_sequence_end (void)
{
  if (!fake_input)
    return;
  fake_input = 0;
  holy_term_inputs = saved;
  holy_free (seq);
  seq = 0;
  nseq = 0;
  seqptr = 0;
}
