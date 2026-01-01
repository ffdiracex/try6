/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/types.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/normal.h>
#include <holy/acpi.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/dl.h>

#pragma GCC diagnostic ignored "-Wcast-align"

holy_MOD_LICENSE ("GPLv2+");

static void
print_strn (holy_uint8_t *str, holy_size_t len)
{
  for (; *str && len; str++, len--)
    holy_printf ("%c", *str);
  for (len++; len; len--)
    holy_printf (" ");
}

#define print_field(x) print_strn(x, sizeof (x))

static void
disp_acpi_table (struct holy_acpi_table_header *t)
{
  print_field (t->signature);
  holy_printf ("%4" PRIuholy_UINT32_T "B rev=%u chksum=0x%02x (%s) OEM=", t->length, t->revision, t->checksum,
	       holy_byte_checksum (t, t->length) == 0 ? "valid" : "invalid");
  print_field (t->oemid);
  print_field (t->oemtable);
  holy_printf ("OEMrev=%08" PRIxholy_UINT32_T " ", t->oemrev);
  print_field (t->creator_id);
  holy_printf (" %08" PRIxholy_UINT32_T "\n", t->creator_rev);
}

static void
disp_madt_table (struct holy_acpi_madt *t)
{
  struct holy_acpi_madt_entry_header *d;
  holy_uint32_t len;

  disp_acpi_table (&t->hdr);
  holy_printf ("Local APIC=%08" PRIxholy_UINT32_T "  Flags=%08"
	       PRIxholy_UINT32_T "\n",
	       t->lapic_addr, t->flags);
  len = t->hdr.length - sizeof (struct holy_acpi_madt);
  d = t->entries;
  for (;len > 0; len -= d->len, d = (void *) ((holy_uint8_t *) d + d->len))
    {
      switch (d->type)
	{
	case holy_ACPI_MADT_ENTRY_TYPE_LAPIC:
	  {
	    struct holy_acpi_madt_entry_lapic *dt = (void *) d;
	    holy_printf ("  LAPIC ACPI_ID=%02x APIC_ID=%02x Flags=%08x\n",
			 dt->acpiid, dt->apicid, dt->flags);
	    if (dt->hdr.len != sizeof (*dt))
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    break;
	  }

	case holy_ACPI_MADT_ENTRY_TYPE_IOAPIC:
	  {
	    struct holy_acpi_madt_entry_ioapic *dt = (void *) d;
	    holy_printf ("  IOAPIC ID=%02x address=%08x GSI=%08x\n",
			 dt->id, dt->address, dt->global_sys_interrupt);
	    if (dt->hdr.len != sizeof (*dt))
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad)
	      holy_printf ("   non-zero pad: %02x\n", dt->pad);
	    break;
	  }

	case holy_ACPI_MADT_ENTRY_TYPE_INTERRUPT_OVERRIDE:
	  {
	    struct holy_acpi_madt_entry_interrupt_override *dt = (void *) d;
	    holy_printf ("  Int Override bus=%x src=%x GSI=%08x Flags=%04x\n",
			 dt->bus, dt->source, dt->global_sys_interrupt,
			 dt->flags);
	    if (dt->hdr.len != sizeof (*dt))
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	  }
	  break;

	case holy_ACPI_MADT_ENTRY_TYPE_LAPIC_NMI:
	  {
	    struct holy_acpi_madt_entry_lapic_nmi *dt = (void *) d;
	    holy_printf ("  LAPIC_NMI ACPI_ID=%02x Flags=%04x lint=%02x\n",
			 dt->acpiid, dt->flags, dt->lint);
	    if (dt->hdr.len != sizeof (*dt))
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    break;
	  }

	case holy_ACPI_MADT_ENTRY_TYPE_SAPIC:
	  {
	    struct holy_acpi_madt_entry_sapic *dt = (void *) d;
	    holy_printf ("  IOSAPIC Id=%02x GSI=%08x Addr=%016" PRIxholy_UINT64_T
			 "\n",
			 dt->id, dt->global_sys_interrupt_base,
			 dt->addr);
	    if (dt->hdr.len != sizeof (*dt))
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad)
	      holy_printf ("   non-zero pad: %02x\n", dt->pad);

	  }
	  break;
	case holy_ACPI_MADT_ENTRY_TYPE_LSAPIC:
	  {
	    struct holy_acpi_madt_entry_lsapic *dt = (void *) d;
	    holy_printf ("  LSAPIC ProcId=%02x ID=%02x EID=%02x Flags=%x",
			 dt->cpu_id, dt->id, dt->eid, dt->flags);
	    if (dt->flags & holy_ACPI_MADT_ENTRY_SAPIC_FLAGS_ENABLED)
	      holy_printf (" Enabled\n");
	    else
	      holy_printf (" Disabled\n");
	    if (d->len > sizeof (struct holy_acpi_madt_entry_sapic))
	      holy_printf ("  UID val=%08x, Str=%s\n", dt->cpu_uid,
			   dt->cpu_uid_str);
	    if (dt->hdr.len != sizeof (*dt) + holy_strlen ((char *) dt->cpu_uid_str) + 1)
	      holy_printf ("   table size mismatch %d != %d\n", dt->hdr.len,
			   (int) sizeof (*dt));
	    if (dt->pad[0] || dt->pad[1] || dt->pad[2])
	      holy_printf ("   non-zero pad: %02x%02x%02x\n", dt->pad[0], dt->pad[1], dt->pad[2]);
	  }
	  break;
	case holy_ACPI_MADT_ENTRY_TYPE_PLATFORM_INT_SOURCE:
	  {
	    struct holy_acpi_madt_entry_platform_int_source *dt = (void *) d;
	    static const char * const platint_type[] =
	      {"Nul", "PMI", "INIT", "CPEI"};

	    holy_printf ("  Platform INT flags=%04x type=%02x (%s)"
			 " ID=%02x EID=%02x\n",
			 dt->flags, dt->inttype,
			 (dt->inttype < ARRAY_SIZE (platint_type))
			 ? platint_type[dt->inttype] : "??", dt->cpu_id,
			 dt->cpu_eid);
	    holy_printf ("  IOSAPIC Vec=%02x GSI=%08x source flags=%08x\n",
			 dt->sapic_vector, dt->global_sys_int, dt->src_flags);
	  }
	  break;
	default:
	  holy_printf ("  type=%x l=%u ", d->type, d->len);
	  holy_printf (" ??\n");
	}
    }
}

static void
disp_acpi_xsdt_table (struct holy_acpi_table_header *t)
{
  holy_uint32_t len;
  holy_uint64_t *desc;

  disp_acpi_table (t);
  len = t->length - sizeof (*t);
  desc = (holy_uint64_t *) (t + 1);
  for (; len >= sizeof (*desc); desc++, len -= sizeof (*desc))
    {
#if holy_CPU_SIZEOF_VOID_P == 4
      if (*desc >= (1ULL << 32))
	{
	  holy_printf ("Unreachable table\n");
	  continue;
	}
#endif
      t = (struct holy_acpi_table_header *) (holy_addr_t) *desc;

      if (t == NULL)
	continue;

      if (holy_memcmp (t->signature, holy_ACPI_MADT_SIGNATURE,
		       sizeof (t->signature)) == 0)
	disp_madt_table ((struct holy_acpi_madt *) t);
      else
	disp_acpi_table (t);
    }
}

static void
disp_acpi_rsdt_table (struct holy_acpi_table_header *t)
{
  holy_uint32_t len;
  holy_uint32_t *desc;

  disp_acpi_table (t);
  len = t->length - sizeof (*t);
  desc = (holy_uint32_t *) (t + 1);
  for (; len >= sizeof (*desc); desc++, len -= sizeof (*desc))
    {
      t = (struct holy_acpi_table_header *) (holy_addr_t) *desc;

      if (t == NULL)
	continue;

      if (holy_memcmp (t->signature, holy_ACPI_MADT_SIGNATURE,
		       sizeof (t->signature)) == 0)
	disp_madt_table ((struct holy_acpi_madt *) t);
      else
	disp_acpi_table (t);
    }
}

static void
disp_acpi_rsdpv1 (struct holy_acpi_rsdp_v10 *rsdp)
{
  print_field (rsdp->signature);
  holy_printf ("chksum:%02x (%s), OEM-ID: ", rsdp->checksum, holy_byte_checksum (rsdp, sizeof (*rsdp)) == 0 ? "valid" : "invalid");
  print_field (rsdp->oemid);
  holy_printf ("rev=%d\n", rsdp->revision);
  holy_printf ("RSDT=%08" PRIxholy_UINT32_T "\n", rsdp->rsdt_addr);
}

static void
disp_acpi_rsdpv2 (struct holy_acpi_rsdp_v20 *rsdp)
{
  disp_acpi_rsdpv1 (&rsdp->rsdpv1);
  holy_printf ("len=%d chksum=%02x (%s) XSDT=%016" PRIxholy_UINT64_T "\n", rsdp->length, rsdp->checksum, holy_byte_checksum (rsdp, rsdp->length) == 0 ? "valid" : "invalid",
	       rsdp->xsdt_addr);
  if (rsdp->length != sizeof (*rsdp))
    holy_printf (" length mismatch %d != %d\n", rsdp->length,
		 (int) sizeof (*rsdp));
  if (rsdp->reserved[0] || rsdp->reserved[1] || rsdp->reserved[2])
    holy_printf (" non-zero reserved %02x%02x%02x\n", rsdp->reserved[0], rsdp->reserved[1], rsdp->reserved[2]);
}

static const struct holy_arg_option options[] = {
  {"v1", '1', 0, N_("Show version 1 tables only."), 0, ARG_TYPE_NONE},
  {"v2", '2', 0, N_("Show version 2 and version 3 tables only."), 0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

static holy_err_t
holy_cmd_lsacpi (struct holy_extcmd_context *ctxt,
		 int argc __attribute__ ((unused)),
		 char **args __attribute__ ((unused)))
{
  if (!ctxt->state[1].set)
    {
      struct holy_acpi_rsdp_v10 *rsdp1 = holy_acpi_get_rsdpv1 ();
      if (!rsdp1)
	holy_printf ("No RSDPv1\n");
      else
	{
	  holy_printf ("RSDPv1 signature:");
	  disp_acpi_rsdpv1 (rsdp1);
	  disp_acpi_rsdt_table ((void *) (holy_addr_t) rsdp1->rsdt_addr);
	}
    }

  if (!ctxt->state[0].set)
    {
      struct holy_acpi_rsdp_v20 *rsdp2 = holy_acpi_get_rsdpv2 ();
      if (!rsdp2)
	holy_printf ("No RSDPv2\n");
      else
	{
#if holy_CPU_SIZEOF_VOID_P == 4
	  if (rsdp2->xsdt_addr >= (1ULL << 32))
	    holy_printf ("Unreachable RSDPv2\n");
	  else
#endif
	    {
	      holy_printf ("RSDPv2 signature:");
	      disp_acpi_rsdpv2 (rsdp2);
	      disp_acpi_xsdt_table ((void *) (holy_addr_t) rsdp2->xsdt_addr);
	      holy_printf ("\n");
	    }
	}
    }
  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(lsapi)
{
  cmd = holy_register_extcmd ("lsacpi", holy_cmd_lsacpi, 0, "[-1|-2]",
			      N_("Show ACPI information."), options);
}

holy_MOD_FINI(lsacpi)
{
  holy_unregister_extcmd (cmd);
}


