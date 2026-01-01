/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/serial.h>
#include <holy/ns8250.h>
#include <holy/types.h>
#include <holy/dl.h>
#include <holy/misc.h>
#include <holy/cpu/io.h>
#include <holy/mm.h>
#include <holy/time.h>
#include <holy/i18n.h>

#ifdef holy_MACHINE_PCBIOS
#include <holy/machine/memory.h>
static const unsigned short *serial_hw_io_addr = (const unsigned short *) holy_MEMORY_MACHINE_BIOS_DATA_AREA_ADDR;
#define holy_SERIAL_PORT_NUM 4
#else
#include <holy/machine/serial.h>
static const holy_port_t serial_hw_io_addr[] = holy_MACHINE_SERIAL_PORTS;
#define holy_SERIAL_PORT_NUM (ARRAY_SIZE(serial_hw_io_addr))
#endif

static int dead_ports = 0;

#ifdef holy_MACHINE_MIPS_LOONGSON
#define DEFAULT_BASE_CLOCK (2 * 115200)
#else
#define DEFAULT_BASE_CLOCK 115200
#endif


/* Convert speed to divisor.  */
static unsigned short
serial_get_divisor (const struct holy_serial_port *port __attribute__ ((unused)),
		    const struct holy_serial_config *config)
{
  holy_uint32_t base_clock;
  holy_uint32_t divisor;
  holy_uint32_t actual_speed, error;

  base_clock = config->base_clock ? (config->base_clock >> 4) : DEFAULT_BASE_CLOCK;

  divisor = (base_clock + (config->speed / 2)) / config->speed;
  if (config->speed == 0)
    return 0;
  if (divisor > 0xffff || divisor == 0)
    return 0;
  actual_speed = base_clock / divisor;
  error = actual_speed > config->speed ? (actual_speed - config->speed)
    : (config->speed - actual_speed);
  if (error > (config->speed / 30 + 1))
    return 0;
  return divisor;
}

static void
do_real_config (struct holy_serial_port *port)
{
  int divisor;
  unsigned char status = 0;
  holy_uint64_t endtime;

  const unsigned char parities[] = {
    [holy_SERIAL_PARITY_NONE] = UART_NO_PARITY,
    [holy_SERIAL_PARITY_ODD] = UART_ODD_PARITY,
    [holy_SERIAL_PARITY_EVEN] = UART_EVEN_PARITY
  };
  const unsigned char stop_bits[] = {
    [holy_SERIAL_STOP_BITS_1] = UART_1_STOP_BIT,
    [holy_SERIAL_STOP_BITS_2] = UART_2_STOP_BITS,
  };

  if (port->configured)
    return;

  port->broken = 0;

  divisor = serial_get_divisor (port, &port->config);

  /* Turn off the interrupt.  */
  holy_outb (0, port->port + UART_IER);

  /* Set DLAB.  */
  holy_outb (UART_DLAB, port->port + UART_LCR);

  /* Set the baud rate.  */
  holy_outb (divisor & 0xFF, port->port + UART_DLL);
  holy_outb (divisor >> 8, port->port + UART_DLH);

  /* Set the line status.  */
  status |= (parities[port->config.parity]
	     | (port->config.word_len - 5)
	     | stop_bits[port->config.stop_bits]);
  holy_outb (status, port->port + UART_LCR);

  if (port->config.rtscts)
    {
      /* Enable the FIFO.  */
      holy_outb (UART_ENABLE_FIFO_TRIGGER1, port->port + UART_FCR);

      /* Turn on DTR and RTS.  */
      holy_outb (UART_ENABLE_DTRRTS, port->port + UART_MCR);
    }
  else
    {
      /* Enable the FIFO.  */
      holy_outb (UART_ENABLE_FIFO_TRIGGER14, port->port + UART_FCR);

      /* Turn on DTR, RTS, and OUT2.  */
      holy_outb (UART_ENABLE_DTRRTS | UART_ENABLE_OUT2, port->port + UART_MCR);
    }

  /* Drain the input buffer.  */
  endtime = holy_get_time_ms () + 1000;
  while (holy_inb (port->port + UART_LSR) & UART_DATA_READY)
    {
      holy_inb (port->port + UART_RX);
      if (holy_get_time_ms () > endtime)
	{
	  port->broken = 1;
	  break;
	}
    }

  port->configured = 1;
}

/* Fetch a key.  */
static int
serial_hw_fetch (struct holy_serial_port *port)
{
  do_real_config (port);
  if (holy_inb (port->port + UART_LSR) & UART_DATA_READY)
    return holy_inb (port->port + UART_RX);

  return -1;
}

/* Put a character.  */
static void
serial_hw_put (struct holy_serial_port *port, const int c)
{
  holy_uint64_t endtime;

  do_real_config (port);

  if (port->broken > 5)
    endtime = holy_get_time_ms ();
  else if (port->broken > 1)
    endtime = holy_get_time_ms () + 50;
  else
    endtime = holy_get_time_ms () + 200;
  /* Wait until the transmitter holding register is empty.  */
  while ((holy_inb (port->port + UART_LSR) & UART_EMPTY_TRANSMITTER) == 0)
    {
      if (holy_get_time_ms () > endtime)
	{
	  port->broken++;
	  /* There is something wrong. But what can I do?  */
	  return;
	}
    }

  if (port->broken)
    port->broken--;

  holy_outb (c, port->port + UART_TX);
}

/* Initialize a serial device. PORT is the port number for a serial device.
   SPEED is a DTE-DTE speed which must be one of these: 2400, 4800, 9600,
   19200, 38400, 57600 and 115200. WORD_LEN is the word length to be used
   for the device. Likewise, PARITY is the type of the parity and
   STOP_BIT_LEN is the length of the stop bit. The possible values for
   WORD_LEN, PARITY and STOP_BIT_LEN are defined in the header file as
   macros.  */
static holy_err_t
serial_hw_configure (struct holy_serial_port *port,
		     struct holy_serial_config *config)
{
  unsigned short divisor;

  divisor = serial_get_divisor (port, config);
  if (divisor == 0)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port speed"));

  if (config->parity != holy_SERIAL_PARITY_NONE
      && config->parity != holy_SERIAL_PARITY_ODD
      && config->parity != holy_SERIAL_PARITY_EVEN)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port parity"));

  if (config->stop_bits != holy_SERIAL_STOP_BITS_1
      && config->stop_bits != holy_SERIAL_STOP_BITS_2)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port stop bits number"));

  if (config->word_len < 5 || config->word_len > 8)
    return holy_error (holy_ERR_BAD_ARGUMENT,
		       N_("unsupported serial port word length"));

  port->config = *config;
  port->configured = 0;

  /*  FIXME: should check if the serial terminal was found.  */

  return holy_ERR_NONE;
}

struct holy_serial_driver holy_ns8250_driver =
  {
    .configure = serial_hw_configure,
    .fetch = serial_hw_fetch,
    .put = serial_hw_put
  };

static char com_names[holy_SERIAL_PORT_NUM][20];
static struct holy_serial_port com_ports[holy_SERIAL_PORT_NUM];

void
holy_ns8250_init (void)
{
  unsigned i;
  for (i = 0; i < holy_SERIAL_PORT_NUM; i++)
    if (serial_hw_io_addr[i])
      {
	holy_err_t err;

	holy_outb (0x5a, serial_hw_io_addr[i] + UART_SR);
	if (holy_inb (serial_hw_io_addr[i] + UART_SR) != 0x5a)
	  {
	    dead_ports |= (1 << i);
	    continue;
	  }
	holy_outb (0xa5, serial_hw_io_addr[i] + UART_SR);
	if (holy_inb (serial_hw_io_addr[i] + UART_SR) != 0xa5)
	  {
	    dead_ports |= (1 << i);
	    continue;
	  }

	holy_snprintf (com_names[i], sizeof (com_names[i]), "com%d", i);
	com_ports[i].name = com_names[i];
	com_ports[i].driver = &holy_ns8250_driver;
	com_ports[i].port = serial_hw_io_addr[i];
	err = holy_serial_config_defaults (&com_ports[i]);
	if (err)
	  holy_print_error ();

	holy_serial_register (&com_ports[i]);
      }
}

/* Return the port number for the UNITth serial device.  */
holy_port_t
holy_ns8250_hw_get_port (const unsigned int unit)
{
  if (unit < holy_SERIAL_PORT_NUM
      && !(dead_ports & (1 << unit)))
    return serial_hw_io_addr[unit];
  else
    return 0;
}

char *
holy_serial_ns8250_add_port (holy_port_t port)
{
  struct holy_serial_port *p;
  unsigned i;
  for (i = 0; i < holy_SERIAL_PORT_NUM; i++)
    if (com_ports[i].port == port)
      {
	if (dead_ports & (1 << i))
	  return NULL;
	return com_names[i];
      }

  holy_outb (0x5a, port + UART_SR);
  if (holy_inb (port + UART_SR) != 0x5a)
    return NULL;

  holy_outb (0xa5, port + UART_SR);
  if (holy_inb (port + UART_SR) != 0xa5)
    return NULL;

  p = holy_malloc (sizeof (*p));
  if (!p)
    return NULL;
  p->name = holy_xasprintf ("port%lx", (unsigned long) port);
  if (!p->name)
    {
      holy_free (p);
      return NULL;
    }
  p->driver = &holy_ns8250_driver;
  holy_serial_config_defaults (p);
  p->port = port;
  holy_serial_register (p);

  return p->name;
}
