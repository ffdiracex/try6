/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/misc.h>
#include <holy/cpu/gdb.h>
#include <holy/gdb.h>
#include <holy/serial.h>
#include <holy/backtrace.h>

static const char hexchars[] = "0123456789abcdef";
int holy_gdb_regs[holy_MACHINE_NR_REGS];

#define holy_GDB_COMBUF_SIZE 400	/* At least sizeof(holy_gdb_regs)*2 are needed for
					   register packets.  */
static char holy_gdb_inbuf[holy_GDB_COMBUF_SIZE + 1];
static char holy_gdb_outbuf[holy_GDB_COMBUF_SIZE + 1];

struct holy_serial_port *holy_gdb_port;

static int
hex (char ch)
{
  if ((ch >= 'a') && (ch <= 'f'))
    return (ch - 'a' + 10);
  if ((ch >= '0') && (ch <= '9'))
    return (ch - '0');
  if ((ch >= 'A') && (ch <= 'F'))
    return (ch - 'A' + 10);
  return (-1);
}

/* Scan for the sequence $<data>#<checksum>.  */
static char *
holy_gdb_getpacket (void)
{
  char *buffer = &holy_gdb_inbuf[0];
  unsigned char checksum;
  unsigned char xmitcsum;
  int count;
  int ch;

  while (1)
    {
      /* Wait around for the start character, ignore all other
         characters.  */
      while ((ch = holy_serial_port_fetch (holy_gdb_port)) != '$');

    retry:
      checksum = 0;
      xmitcsum = -1;
      count = 0;

      /* Now read until a # or end of buffer is found.  */
      while (count < holy_GDB_COMBUF_SIZE)
	{
	  do
	    ch = holy_serial_port_fetch (holy_gdb_port);
	  while (ch < 0);
	  if (ch == '$')
	    goto retry;
	  if (ch == '#')
	    break;
	  checksum += ch;
	  buffer[count] = ch;
	  count = count + 1;
	}
      buffer[count] = 0;
      if (ch == '#')
	{
	  do
	    ch = holy_serial_port_fetch (holy_gdb_port);
	  while (ch < 0);
	  xmitcsum = hex (ch) << 4;
	  do
	    ch = holy_serial_port_fetch (holy_gdb_port);
	  while (ch < 0);
	  xmitcsum += hex (ch);

	  if (checksum != xmitcsum)
	    holy_serial_port_put (holy_gdb_port, '-');	/* Failed checksum.  */
	  else
	    {
	      holy_serial_port_put (holy_gdb_port, '+');	/* Successful transfer.  */

	      /* If a sequence char is present, reply the sequence ID.  */
	      if (buffer[2] == ':')
		{
		  holy_serial_port_put (holy_gdb_port, buffer[0]);
		  holy_serial_port_put (holy_gdb_port, buffer[1]);

		  return &buffer[3];
		}
	      return &buffer[0];
	    }
	}
    }
}

/* Send the packet in buffer.  */
static void
holy_gdb_putpacket (char *buffer)
{
  holy_uint8_t checksum;

  /* $<packet info>#<checksum>.  */
  do
    {
      char *ptr;
      holy_serial_port_put (holy_gdb_port, '$');
      checksum = 0;

      for (ptr = buffer; *ptr; ptr++)
	{
	  holy_serial_port_put (holy_gdb_port, *ptr);
	  checksum += *ptr;
	}

      holy_serial_port_put (holy_gdb_port, '#');
      holy_serial_port_put (holy_gdb_port, hexchars[checksum >> 4]);
      holy_serial_port_put (holy_gdb_port, hexchars[checksum & 0xf]);
    }
  while (holy_serial_port_fetch (holy_gdb_port) != '+');
}

/* Convert the memory pointed to by mem into hex, placing result in buf.
   Return a pointer to the last char put in buf (NULL).  */
static char *
holy_gdb_mem2hex (char *mem, char *buf, holy_size_t count)
{
  holy_size_t i;
  unsigned char ch;

  for (i = 0; i < count; i++)
    {
      ch = *mem++;
      *buf++ = hexchars[ch >> 4];
      *buf++ = hexchars[ch % 16];
    }
  *buf = 0;
  return (buf);
}

/* Convert the hex array pointed to by buf into binary to be placed in mem.
   Return a pointer to the character after the last byte written.  */
static char *
holy_gdb_hex2mem (char *buf, char *mem, int count)
{
  int i;
  unsigned char ch;

  for (i = 0; i < count; i++)
    {
      ch = hex (*buf++) << 4;
      ch = ch + hex (*buf++);
      *mem++ = ch;
    }
  return (mem);
}

/* Convert hex characters to int and return the number of characters
   processed.  */
static int
holy_gdb_hex2int (char **ptr, holy_uint64_t *int_value)
{
  int num_chars = 0;
  int hex_value;

  *int_value = 0;

  while (**ptr)
    {
      hex_value = hex (**ptr);
      if (hex_value >= 0)
	{
	  *int_value = (*int_value << 4) | hex_value;
	  num_chars++;
	}
      else
	break;

      (*ptr)++;
    }

  return (num_chars);
}

/* This function does all command procesing for interfacing to gdb.  */
void __attribute__ ((regparm(3)))
holy_gdb_trap (int trap_no)
{
  unsigned int sig_no;
  int stepping;
  holy_uint64_t addr;
  holy_uint64_t length;
  char *ptr;

  if (!holy_gdb_port)
    {
      holy_printf ("Unhandled exception 0x%x at ", trap_no);
      holy_backtrace_print_address ((void *) holy_gdb_regs[PC]);
      holy_printf ("\n");
      holy_backtrace_pointer ((void *) holy_gdb_regs[EBP]);
      holy_fatal ("Unhandled exception");
    }

  sig_no = holy_gdb_trap2sig (trap_no);

  ptr = holy_gdb_outbuf;

  /* Reply to host that an exception has occurred.  */

  *ptr++ = 'T';	/* Notify gdb with signo, PC, FP and SP.  */

  *ptr++ = hexchars[sig_no >> 4];
  *ptr++ = hexchars[sig_no & 0xf];

  /* Stack pointer.  */
  *ptr++ = hexchars[SP];
  *ptr++ = ':';
  ptr = holy_gdb_mem2hex ((char *) &holy_gdb_regs[ESP], ptr, 4);
  *ptr++ = ';';

  /* Frame pointer.  */
  *ptr++ = hexchars[FP];
  *ptr++ = ':';
  ptr = holy_gdb_mem2hex ((char *) &holy_gdb_regs[EBP], ptr, 4);
  *ptr++ = ';';

  /* Program counter.  */
  *ptr++ = hexchars[PC];
  *ptr++ = ':';
  ptr = holy_gdb_mem2hex ((char *) &holy_gdb_regs[PC], ptr, 4);
  *ptr++ = ';';

  *ptr = '\0';

  holy_gdb_putpacket (holy_gdb_outbuf);

  stepping = 0;

  while (1)
    {
      holy_gdb_outbuf[0] = 0;
      ptr = holy_gdb_getpacket ();

      switch (*ptr++)
	{
	case '?':
	  holy_gdb_outbuf[0] = 'S';
	  holy_gdb_outbuf[1] = hexchars[sig_no >> 4];
	  holy_gdb_outbuf[2] = hexchars[sig_no & 0xf];
	  holy_gdb_outbuf[3] = 0;
	  break;

	/* Return values of the CPU registers.  */
	case 'g':
	  holy_gdb_mem2hex ((char *) holy_gdb_regs, holy_gdb_outbuf,
			    sizeof (holy_gdb_regs));
	  break;

	/* Set values of the CPU registers -- return OK.  */
	case 'G':
	  holy_gdb_hex2mem (ptr, (char *) holy_gdb_regs,
			    sizeof (holy_gdb_regs));
	  holy_strcpy (holy_gdb_outbuf, "OK");
	  break;

	/* Set the value of a single CPU register -- return OK.  */
	case 'P':
	  {
	    holy_uint64_t regno;

	    if (holy_gdb_hex2int (&ptr, &regno) && *ptr++ == '=')
	      if (regno < holy_MACHINE_NR_REGS)
		{
		  holy_gdb_hex2mem (ptr, (char *) &holy_gdb_regs[regno], 4);
		  holy_strcpy (holy_gdb_outbuf, "OK");
		  break;
		}
	    /* FIXME: GDB requires setting orig_eax. I don't know what's
	       this register is about. For now just simulate setting any
	       registers.  */
	    holy_strcpy (holy_gdb_outbuf, /*"E01"*/ "OK");
	    break;
	  }

	/* mAA..AA,LLLL: Read LLLL bytes at address AA..AA.  */
	case 'm':
	  /* Try to read %x,%x.  Set ptr = 0 if successful.  */
	  if (holy_gdb_hex2int (&ptr, &addr))
	    if (*(ptr++) == ',')
	      if (holy_gdb_hex2int (&ptr, &length))
		{
		  ptr = 0;
		  holy_gdb_mem2hex ((char *) (holy_addr_t) addr,
				    holy_gdb_outbuf, length);
		}
	  if (ptr)
	    holy_strcpy (holy_gdb_outbuf, "E01");
	  break;

	/* MAA..AA,LLLL: Write LLLL bytes at address AA.AA -- return OK.  */
	case 'M':
	  /* Try to read %x,%x.  Set ptr = 0 if successful.  */
	  if (holy_gdb_hex2int (&ptr, &addr))
	    if (*(ptr++) == ',')
	      if (holy_gdb_hex2int (&ptr, &length))
		if (*(ptr++) == ':')
		  {
		    holy_gdb_hex2mem (ptr, (char *) (holy_addr_t) addr, length);
		    holy_strcpy (holy_gdb_outbuf, "OK");
		    ptr = 0;
		  }
	  if (ptr)
	    {
	      holy_strcpy (holy_gdb_outbuf, "E02");
	    }
	  break;

	/* sAA..AA: Step one instruction from AA..AA(optional).  */
	case 's':
	  stepping = 1;
	  /* FALLTHROUGH */

	/* cAA..AA: Continue at address AA..AA(optional).  */
	case 'c':
	  /* try to read optional parameter, pc unchanged if no parm */
	  if (holy_gdb_hex2int (&ptr, &addr))
	    holy_gdb_regs[PC] = addr;

	  /* Clear the trace bit.  */
	  holy_gdb_regs[PS] &= 0xfffffeff;

	  /* Set the trace bit if we're stepping.  */
	  if (stepping)
	    holy_gdb_regs[PS] |= 0x100;

	  return;

	/* Kill the program.  */
	case 'k':
	  /* Do nothing.  */
	  return;
	}

      /* Reply to the request.  */
      holy_gdb_putpacket (holy_gdb_outbuf);
    }
}

