/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/err.h>
#include <holy/i18n.h>
#include <holy/mm.h>
#include <holy/tpm.h>
#include <holy/misc.h>
#include <holy/i386/pc/int.h>

#define TCPA_MAGIC 0x41504354

static int tpm_presence = -1;

int tpm_present(void);

int tpm_present(void)
{
  struct holy_bios_int_registers regs;

  if (tpm_presence != -1)
    return tpm_presence;

  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xbb00;
  regs.ebx = TCPA_MAGIC;
  holy_bios_interrupt (0x1a, &regs);

  if (regs.eax == 0)
    tpm_presence = 1;
  else
    tpm_presence = 0;

  return tpm_presence;
}

holy_err_t
holy_tpm_execute(PassThroughToTPM_InputParamBlock *inbuf,
		 PassThroughToTPM_OutputParamBlock *outbuf)
{
  struct holy_bios_int_registers regs;
  holy_addr_t inaddr, outaddr;

  if (!tpm_present())
    return 0;

  inaddr = (holy_addr_t) inbuf;
  outaddr = (holy_addr_t) outbuf;
  regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
  regs.eax = 0xbb02;
  regs.ebx = TCPA_MAGIC;
  regs.ecx = 0;
  regs.edx = 0;
  regs.es = (inaddr & 0xffff0000) >> 4;
  regs.edi = inaddr & 0xffff;
  regs.ds = outaddr >> 4;
  regs.esi = outaddr & 0xf;

  holy_bios_interrupt (0x1a, &regs);

  if (regs.eax)
    {
	tpm_presence = 0;
	return holy_error (holy_ERR_IO, N_("TPM error %x, disabling TPM"), regs.eax);
    }

  return 0;
}

typedef struct {
	holy_uint32_t pcrindex;
	holy_uint32_t eventtype;
	holy_uint8_t digest[20];
	holy_uint32_t eventdatasize;
	holy_uint8_t event[0];
} holy_PACKED Event;

typedef struct {
	holy_uint16_t ipblength;
	holy_uint16_t reserved;
	holy_uint32_t hashdataptr;
	holy_uint32_t hashdatalen;
	holy_uint32_t pcr;
	holy_uint32_t reserved2;
	holy_uint32_t logdataptr;
	holy_uint32_t logdatalen;
} holy_PACKED EventIncoming;

typedef struct {
	holy_uint16_t opblength;
	holy_uint16_t reserved;
	holy_uint32_t eventnum;
	holy_uint8_t  hashvalue[20];
} holy_PACKED EventOutgoing;

holy_err_t
holy_tpm_log_event(unsigned char *buf, holy_size_t size, holy_uint8_t pcr,
		   const char *description)
{
	struct holy_bios_int_registers regs;
	EventIncoming incoming;
	EventOutgoing outgoing;
	Event *event;
	holy_uint32_t datalength;

	if (!tpm_present())
		return 0;

	datalength = holy_strlen(description);
	event = holy_zalloc(datalength + sizeof(Event));
	if (!event)
		return holy_error (holy_ERR_OUT_OF_MEMORY,
				   N_("cannot allocate TPM event buffer"));

	event->pcrindex = pcr;
	event->eventtype = 0x0d;
	event->eventdatasize = holy_strlen(description);
	holy_memcpy(event->event, description, datalength);

	incoming.ipblength = sizeof(incoming);
	incoming.hashdataptr = (holy_uint32_t)buf;
	incoming.hashdatalen = size;
	incoming.pcr = pcr;
	incoming.logdataptr = (holy_uint32_t)event;
	incoming.logdatalen = datalength + sizeof(Event);

	regs.flags = holy_CPU_INT_FLAGS_DEFAULT;
	regs.eax = 0xbb01;
	regs.ebx = TCPA_MAGIC;
	regs.ecx = 0;
	regs.edx = 0;
	regs.es = (((holy_addr_t) &incoming) & 0xffff0000) >> 4;
	regs.edi = ((holy_addr_t) &incoming) & 0xffff;
	regs.ds = (((holy_addr_t) &outgoing) & 0xffff0000) >> 4;
	regs.esi = ((holy_addr_t) &outgoing) & 0xffff;

	holy_bios_interrupt (0x1a, &regs);

	holy_free(event);

	if (regs.eax)
	  {
		tpm_presence = 0;
		return holy_error (holy_ERR_IO, N_("TPM error %x, disabling TPM"), regs.eax);
	  }

	return 0;
}
