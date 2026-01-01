/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/term.h>
#include <holy/types.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/terminfo.h>
#include <holy/xen.h>

static int
readkey (struct holy_term_input *term __attribute__ ((unused)))
{
  holy_size_t prod, cons;
  int r;
  mb ();
  prod = holy_xen_xcons->in_prod;
  cons = holy_xen_xcons->in_cons;
  if (prod <= cons)
    return -1;
  r = holy_xen_xcons->in[cons];
  cons++;
  mb ();
  holy_xen_xcons->in_cons = cons;
  return r;
}

static int signal_sent = 1;

static void
refresh (struct holy_term_output *term __attribute__ ((unused)))
{
  struct evtchn_send send;
  send.port = holy_xen_start_page_addr->console.domU.evtchn;
  holy_xen_event_channel_op (EVTCHNOP_send, &send);
  signal_sent = 1;
  while (holy_xen_xcons->out_prod != holy_xen_xcons->out_cons)
    {
      holy_xen_sched_op (SCHEDOP_yield, 0);
    }
}

static void
put (struct holy_term_output *term __attribute__ ((unused)), const int c)
{
  holy_size_t prod, cons;

  while (1)
    {
      mb ();
      prod = holy_xen_xcons->out_prod;
      cons = holy_xen_xcons->out_cons;
      if (prod < cons + sizeof (holy_xen_xcons->out))
	break;
      if (!signal_sent)
	refresh (term);
      holy_xen_sched_op (SCHEDOP_yield, 0);
    }
  holy_xen_xcons->out[prod++ & (sizeof (holy_xen_xcons->out) - 1)] = c;
  mb ();
  holy_xen_xcons->out_prod = prod;
  signal_sent = 0;
}


struct holy_terminfo_input_state holy_console_terminfo_input = {
  .readkey = readkey
};

struct holy_terminfo_output_state holy_console_terminfo_output = {
  .put = put,
  .size = {80, 24}
};

static struct holy_term_input holy_console_term_input = {
  .name = "console",
  .init = 0,
  .getkey = holy_terminfo_getkey,
  .data = &holy_console_terminfo_input
};

static struct holy_term_output holy_console_term_output = {
  .name = "console",
  .init = 0,
  .putchar = holy_terminfo_putchar,
  .getxy = holy_terminfo_getxy,
  .getwh = holy_terminfo_getwh,
  .gotoxy = holy_terminfo_gotoxy,
  .cls = holy_terminfo_cls,
  .refresh = refresh,
  .setcolorstate = holy_terminfo_setcolorstate,
  .setcursor = holy_terminfo_setcursor,
  .flags = holy_TERM_CODE_TYPE_ASCII,
  .data = &holy_console_terminfo_output,
};


void
holy_console_init (void)
{
  holy_term_register_input ("console", &holy_console_term_input);
  holy_term_register_output ("console", &holy_console_term_output);

  holy_terminfo_init ();
  holy_terminfo_output_register (&holy_console_term_output, "vt100-color");
}
