/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_SERIAL_HEADER
#define holy_SERIAL_HEADER	1

#include <holy/types.h>
#if defined(__mips__) || defined (__i386__) || defined (__x86_64__)
#include <holy/cpu/io.h>
#endif
#include <holy/usb.h>
#include <holy/list.h>
#include <holy/term.h>
#ifdef holy_MACHINE_IEEE1275
#include <holy/ieee1275/ieee1275.h>
#endif
#ifdef holy_MACHINE_ARC
#include <holy/arc/arc.h>
#endif

struct holy_serial_port;
struct holy_serial_config;

struct holy_serial_driver
{
  holy_err_t (*configure) (struct holy_serial_port *port,
			   struct holy_serial_config *config);
  int (*fetch) (struct holy_serial_port *port);
  void (*put) (struct holy_serial_port *port, const int c);
  void (*fini) (struct holy_serial_port *port);
};

/* The type of parity.  */
typedef enum
  {
    holy_SERIAL_PARITY_NONE,
    holy_SERIAL_PARITY_ODD,
    holy_SERIAL_PARITY_EVEN,
  } holy_serial_parity_t;

typedef enum
  {
    holy_SERIAL_STOP_BITS_1,
    holy_SERIAL_STOP_BITS_1_5,
    holy_SERIAL_STOP_BITS_2,
  } holy_serial_stop_bits_t;

struct holy_serial_config
{
  unsigned speed;
  int word_len;
  holy_serial_parity_t parity;
  holy_serial_stop_bits_t stop_bits;
  holy_uint64_t base_clock;
  int rtscts;
};

struct holy_serial_port
{
  struct holy_serial_port *next;
  struct holy_serial_port **prev;
  char *name;
  struct holy_serial_driver *driver;
  struct holy_serial_config config;
  int configured;
  int broken;

  /* This should be void *data but since serial is useful as an early console
     when malloc isn't available it's a union.
   */
  union
  {
#if defined(__mips__) || defined (__i386__) || defined (__x86_64__)
    holy_port_t port;
#endif
    struct
    {
      holy_usb_device_t usbdev;
      int configno;
      int interfno;
      char buf[64];
      int bufstart, bufend;
      struct holy_usb_desc_endp *in_endp;
      struct holy_usb_desc_endp *out_endp;
    };
    struct holy_escc_descriptor *escc_desc;
#ifdef holy_MACHINE_IEEE1275
    struct
    {
      holy_ieee1275_ihandle_t handle;
      struct ofserial_hash_ent *elem;
    };
#endif
#ifdef holy_MACHINE_EFI
    struct holy_efi_serial_io_interface *interface;
#endif
#ifdef holy_MACHINE_ARC
    struct
    {
      holy_arc_fileno_t handle;
      int handle_valid;
    };
#endif
  };
  holy_term_output_t term_out;
  holy_term_input_t term_in;
};

holy_err_t EXPORT_FUNC(holy_serial_register) (struct holy_serial_port *port);

void EXPORT_FUNC(holy_serial_unregister) (struct holy_serial_port *port);

  /* Convenience functions to perform primitive operations on a port.  */
static inline holy_err_t
holy_serial_port_configure (struct holy_serial_port *port,
			    struct holy_serial_config *config)
{
  return port->driver->configure (port, config);
}

static inline int
holy_serial_port_fetch (struct holy_serial_port *port)
{
  return port->driver->fetch (port);
}

static inline void
holy_serial_port_put (struct holy_serial_port *port, const int c)
{
  port->driver->put (port, c);
}

static inline void
holy_serial_port_fini (struct holy_serial_port *port)
{
  port->driver->fini (port);
}

  /* Set default settings.  */
static inline holy_err_t
holy_serial_config_defaults (struct holy_serial_port *port)
{
  struct holy_serial_config config =
    {
#ifdef holy_MACHINE_MIPS_LOONGSON
      .speed = 115200,
      /* On Loongson machines serial port has only 3 wires.  */
      .rtscts = 0,
#else
      .speed = 9600,
      .rtscts = 1,
#endif
      .word_len = 8,
      .parity = holy_SERIAL_PARITY_NONE,
      .stop_bits = holy_SERIAL_STOP_BITS_1,
      .base_clock = 0
    };

  return port->driver->configure (port, &config);
}

#if defined(__mips__) || defined (__i386__) || defined (__x86_64__)
void holy_ns8250_init (void);
char *holy_serial_ns8250_add_port (holy_port_t port);
#endif
#ifdef holy_MACHINE_IEEE1275
void holy_ofserial_init (void);
#endif
#ifdef holy_MACHINE_EFI
void
holy_efiserial_init (void);
#endif
#ifdef holy_MACHINE_ARC
void
holy_arcserial_init (void);
const char *
holy_arcserial_add_port (const char *path);
#endif

struct holy_serial_port *holy_serial_find (const char *name);
extern struct holy_serial_driver holy_ns8250_driver;
void EXPORT_FUNC(holy_serial_unregister_driver) (struct holy_serial_driver *driver);

#ifndef holy_MACHINE_EMU
extern void holy_serial_init (void);
extern void holy_serial_fini (void);
#endif

#endif
