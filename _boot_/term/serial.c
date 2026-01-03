/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/serial.h>
#include <holy/term.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/terminfo.h>
#if !defined (holy_MACHINE_EMU) && (defined(__mips__) || defined (__i386__) || defined (__x86_64__))
#include <holy/cpu/io.h>
#endif
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/list.h>
#ifdef holy_MACHINE_MIPS_LOONGSON
#include <holy/machine/kernel.h>
#endif
#ifdef holy_MACHINE_IEEE1275
#include <holy/ieee1275/console.h>
#endif

holy_MOD_LICENSE ("GPLv2+");

#define FOR_SERIAL_PORTS(var) FOR_LIST_ELEMENTS((var), (holy_serial_ports))

enum
  {
    OPTION_UNIT,
    OPTION_PORT,
    OPTION_SPEED,
    OPTION_WORD,
    OPTION_PARITY,
    OPTION_STOP,
    OPTION_BASE_CLOCK,
    OPTION_RTSCTS
  };

/* Argument options.  */
static const struct holy_arg_option options[] =
{
  {"unit",   'u', 0, N_("Set the serial unit."),             0, ARG_TYPE_INT},
  {"port",   'p', 0, N_("Set the serial port address."),     0, ARG_TYPE_STRING},
  {"speed",  's', 0, N_("Set the serial port speed."),       0, ARG_TYPE_INT},
  {"word",   'w', 0, N_("Set the serial port word length."), 0, ARG_TYPE_INT},
  {"parity", 'r', 0, N_("Set the serial port parity."),      0, ARG_TYPE_STRING},
  {"stop",   't', 0, N_("Set the serial port stop bits."),   0, ARG_TYPE_INT},
  {"base-clock",   'b', 0, N_("Set the base frequency."),   0, ARG_TYPE_STRING},
  {"rtscts",   'f', 0, N_("Enable/disable RTS/CTS."),   "on|off", ARG_TYPE_STRING},
  {0, 0, 0, 0, 0, 0}
};

static struct holy_serial_port *holy_serial_ports;

struct holy_serial_output_state
{
  struct holy_terminfo_output_state tinfo;
  struct holy_serial_port *port;
};

struct holy_serial_input_state
{
  struct holy_terminfo_input_state tinfo;
  struct holy_serial_port *port;
};

static void 
serial_put (holy_term_output_t term, const int c)
{
  struct holy_serial_output_state *data = term->data;
  data->port->driver->put (data->port, c);
}

static int
serial_fetch (holy_term_input_t term)
{
  struct holy_serial_input_state *data = term->data;
  return data->port->driver->fetch (data->port);
}

static const struct holy_serial_input_state holy_serial_terminfo_input_template =
  {
    .tinfo =
    {
      .readkey = serial_fetch
    }
  };

static const struct holy_serial_output_state holy_serial_terminfo_output_template =
  {
    .tinfo =
    {
      .put = serial_put,
      .size = { 80, 24 }
    }
  };

static struct holy_serial_input_state holy_serial_terminfo_input;

static struct holy_serial_output_state holy_serial_terminfo_output;

static int registered = 0;

static struct holy_term_input holy_serial_term_input =
{
  .name = "serial",
  .init = holy_terminfo_input_init,
  .getkey = holy_terminfo_getkey,
  .data = &holy_serial_terminfo_input
};

static struct holy_term_output holy_serial_term_output =
{
  .name = "serial",
  .init = holy_terminfo_output_init,
  .putchar = holy_terminfo_putchar,
  .getwh = holy_terminfo_getwh,
  .getxy = holy_terminfo_getxy,
  .gotoxy = holy_terminfo_gotoxy,
  .cls = holy_terminfo_cls,
  .setcolorstate = holy_terminfo_setcolorstate,
  .setcursor = holy_terminfo_setcursor,
  .flags = holy_TERM_CODE_TYPE_ASCII,
  .data = &holy_serial_terminfo_output,
  .progress_update_divisor = holy_PROGRESS_SLOW
};



struct holy_serial_port *
holy_serial_find (const char *name)
{
  struct holy_serial_port *port;

  FOR_SERIAL_PORTS (port)
    if (holy_strcmp (port->name, name) == 0)
      break;

#if (defined(__mips__) || defined (__i386__) || defined (__x86_64__)) && !defined(holy_MACHINE_EMU) && !defined(holy_MACHINE_ARC)
  if (!port && holy_memcmp (name, "port", sizeof ("port") - 1) == 0
      && holy_isxdigit (name [sizeof ("port") - 1]))
    {
      name = holy_serial_ns8250_add_port (holy_strtoul (&name[sizeof ("port") - 1],
							0, 16));
      if (!name)
	return NULL;

      FOR_SERIAL_PORTS (port)
	if (holy_strcmp (port->name, name) == 0)
	  break;
    }
#endif

#ifdef holy_MACHINE_IEEE1275
  if (!port && holy_memcmp (name, "ieee1275/", sizeof ("ieee1275/") - 1) == 0)
    {
      name = holy_ofserial_add_port (&name[sizeof ("ieee1275/") - 1]);
      if (!name)
	return NULL;

      FOR_SERIAL_PORTS (port)
	if (holy_strcmp (port->name, name) == 0)
	  break;
    }
#endif

  return port;
}

static holy_err_t
holy_cmd_serial (holy_extcmd_context_t ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  char pname[40];
  const char *name = NULL;
  struct holy_serial_port *port;
  struct holy_serial_config config;
  holy_err_t err;

  if (state[OPTION_UNIT].set)
    {
      holy_snprintf (pname, sizeof (pname), "com%ld",
		     holy_strtoul (state[0].arg, 0, 0));
      name = pname;
    }

  if (state[OPTION_PORT].set)
    {
      holy_snprintf (pname, sizeof (pname), "port%lx",
		     holy_strtoul (state[1].arg, 0, 0));
      name = pname;
    }

  if (argc >= 1)
    name = args[0];

  if (!name)
    name = "com0";

  port = holy_serial_find (name);
  if (!port)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("serial port `%s' isn't found"),
		       name);

  config = port->config;

  if (state[OPTION_SPEED].set) {
    config.speed = holy_strtoul (state[OPTION_SPEED].arg, 0, 0);
    if (config.speed == 0)
      return holy_error (holy_ERR_BAD_ARGUMENT,
			 N_("unsupported serial port parity"));
  }

  if (state[OPTION_WORD].set)
    config.word_len = holy_strtoul (state[OPTION_WORD].arg, 0, 0);

  if (state[OPTION_PARITY].set)
    {
      if (! holy_strcmp (state[OPTION_PARITY].arg, "no"))
	config.parity = holy_SERIAL_PARITY_NONE;
      else if (! holy_strcmp (state[OPTION_PARITY].arg, "odd"))
	config.parity = holy_SERIAL_PARITY_ODD;
      else if (! holy_strcmp (state[OPTION_PARITY].arg, "even"))
	config.parity = holy_SERIAL_PARITY_EVEN;
      else
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("unsupported serial port parity"));
    }

  if (state[OPTION_RTSCTS].set)
    {
      if (holy_strcmp (state[OPTION_RTSCTS].arg, "on") == 0)
	config.rtscts = 1;
      else if (holy_strcmp (state[OPTION_RTSCTS].arg, "off") == 0)
	config.rtscts = 0;
      else
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("unsupported serial port flow control"));
    }

  if (state[OPTION_STOP].set)
    {
      if (! holy_strcmp (state[OPTION_STOP].arg, "1"))
	config.stop_bits = holy_SERIAL_STOP_BITS_1;
      else if (! holy_strcmp (state[OPTION_STOP].arg, "2"))
	config.stop_bits = holy_SERIAL_STOP_BITS_2;
      else if (! holy_strcmp (state[OPTION_STOP].arg, "1.5"))
	config.stop_bits = holy_SERIAL_STOP_BITS_1_5;
      else
	return holy_error (holy_ERR_BAD_ARGUMENT,
			   N_("unsupported serial port stop bits number"));
    }

  if (state[OPTION_BASE_CLOCK].set)
    {
      char *ptr;
      config.base_clock = holy_strtoull (state[OPTION_BASE_CLOCK].arg, &ptr, 0);
      if (holy_errno)
	return holy_errno;
      if (ptr && *ptr == 'M')
	config.base_clock *= 1000000;
      if (ptr && (*ptr == 'k' || *ptr == 'K'))
	config.base_clock *= 1000;
    }

  if (config.speed == 0)
    config.speed = 9600;

  /* Initialize with new settings.  */
  err = port->driver->configure (port, &config);
  if (err)
    return err;
#if !defined (holy_MACHINE_EMU) && !defined(holy_MACHINE_ARC) && (defined(__mips__) || defined (__i386__) || defined (__x86_64__))

  /* Compatibility kludge.  */
  if (port->driver == &holy_ns8250_driver)
    {
      if (!registered)
	{
	  holy_terminfo_output_register (&holy_serial_term_output, "vt100");

	  holy_term_register_input ("serial", &holy_serial_term_input);
	  holy_term_register_output ("serial", &holy_serial_term_output);
	}
      holy_serial_terminfo_output.port = port;
      holy_serial_terminfo_input.port = port;
      registered = 1;
    }
#endif
  return holy_ERR_NONE;
}

#ifdef holy_MACHINE_MIPS_LOONGSON
const char loongson_defserial[][6] =
  {
    [holy_ARCH_MACHINE_YEELOONG] = "com0",
    [holy_ARCH_MACHINE_FULOONG2F]  = "com2",
    [holy_ARCH_MACHINE_FULOONG2E]  = "com1"
  };
#endif

holy_err_t
holy_serial_register (struct holy_serial_port *port)
{
  struct holy_term_input *in;
  struct holy_term_output *out;
  struct holy_serial_input_state *indata;
  struct holy_serial_output_state *outdata;

  in = holy_malloc (sizeof (*in));
  if (!in)
    return holy_errno;

  indata = holy_malloc (sizeof (*indata));
  if (!indata)
    {
      holy_free (in);
      return holy_errno;
    }

  holy_memcpy (in, &holy_serial_term_input, sizeof (*in));
  in->data = indata;
  in->name = holy_xasprintf ("serial_%s", port->name);
  holy_memcpy (indata, &holy_serial_terminfo_input, sizeof (*indata));

  if (!in->name)
    {
      holy_free (in);
      holy_free (indata);
      return holy_errno;
    }

  out = holy_zalloc (sizeof (*out));
  if (!out)
    {
      holy_free (indata);
      holy_free ((char *) in->name);
      holy_free (in);
      return holy_errno;
    }

  outdata = holy_malloc (sizeof (*outdata));
  if (!outdata)
    {
      holy_free (indata);
      holy_free ((char *) in->name);
      holy_free (out);
      holy_free (in);
      return holy_errno;
    }

  holy_memcpy (out, &holy_serial_term_output, sizeof (*out));
  out->data = outdata;
  out->name = in->name;
  holy_memcpy (outdata, &holy_serial_terminfo_output, sizeof (*outdata));

  holy_list_push (holy_AS_LIST_P (&holy_serial_ports), holy_AS_LIST (port));
  ((struct holy_serial_input_state *) in->data)->port = port;
  ((struct holy_serial_output_state *) out->data)->port = port;
  port->term_in = in;
  port->term_out = out;
  holy_terminfo_output_register (out, "vt100");
#ifdef holy_MACHINE_MIPS_LOONGSON
  if (holy_strcmp (port->name, loongson_defserial[holy_arch_machine]) == 0)
    {
      holy_term_register_input_active ("serial_*", in);
      holy_term_register_output_active ("serial_*", out);
    }
  else
    {
      holy_term_register_input_inactive ("serial_*", in);
      holy_term_register_output_inactive ("serial_*", out);
    }
#else
  holy_term_register_input ("serial_*", in);
  holy_term_register_output ("serial_*", out);
#endif

  return holy_ERR_NONE;
}

void
holy_serial_unregister (struct holy_serial_port *port)
{
  if (port->driver->fini)
    port->driver->fini (port);
  
  if (port->term_in)
    holy_term_unregister_input (port->term_in);
  if (port->term_out)
    holy_term_unregister_output (port->term_out);

  holy_list_remove (holy_AS_LIST (port));
}

void
holy_serial_unregister_driver (struct holy_serial_driver *driver)
{
  struct holy_serial_port *port, *next;
  for (port = holy_serial_ports; port; port = next)
    {
      next = port->next;
      if (port->driver == driver)
	holy_serial_unregister (port);
    }
}

static holy_extcmd_t cmd;

holy_MOD_INIT(serial)
{
  cmd = holy_register_extcmd ("serial", holy_cmd_serial, 0,
			      N_("[OPTIONS...]"),
			      N_("Configure serial port."), options);
  holy_memcpy (&holy_serial_terminfo_output,
	       &holy_serial_terminfo_output_template,
	       sizeof (holy_serial_terminfo_output));

  holy_memcpy (&holy_serial_terminfo_input,
	       &holy_serial_terminfo_input_template,
	       sizeof (holy_serial_terminfo_input));

#if !defined (holy_MACHINE_EMU) && !defined(holy_MACHINE_ARC) && (defined(__mips__) || defined (__i386__) || defined (__x86_64__))
  holy_ns8250_init ();
#endif
#ifdef holy_MACHINE_IEEE1275
  holy_ofserial_init ();
#endif
#ifdef holy_MACHINE_EFI
  holy_efiserial_init ();
#endif
#ifdef holy_MACHINE_ARC
  holy_arcserial_init ();
#endif
}

holy_MOD_FINI(serial)
{
  while (holy_serial_ports)
    holy_serial_unregister (holy_serial_ports);
  if (registered)
    {
      holy_term_unregister_input (&holy_serial_term_input);
      holy_term_unregister_output (&holy_serial_term_output);
    }
  holy_unregister_extcmd (cmd);
}
