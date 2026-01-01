/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/extcmd.h>
#include <holy/file.h>
#include <holy/disk.h>
#include <holy/term.h>
#include <holy/misc.h>
#include <holy/acpi.h>
#include <holy/mm.h>
#include <holy/memory.h>
#include <holy/i18n.h>

#ifdef holy_MACHINE_EFI
#include <holy/efi/efi.h>
#include <holy/efi/api.h>
#endif

#pragma GCC diagnostic ignored "-Wcast-align"

holy_MOD_LICENSE ("GPLv2+");

static const struct holy_arg_option options[] = {
  {"exclude", 'x', 0,
   N_("Don't load host tables specified by comma-separated list."),
   0, ARG_TYPE_STRING},
  {"load-only", 'n', 0,
   N_("Load only tables specified by comma-separated list."), 0, ARG_TYPE_STRING},
  {"v1", '1', 0, N_("Export version 1 tables to the OS."), 0, ARG_TYPE_NONE},
  {"v2", '2', 0, N_("Export version 2 and version 3 tables to the OS."), 0, ARG_TYPE_NONE},
  {"oemid", 'o', 0, N_("Set OEMID of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtable", 't', 0,
   N_("Set OEMTABLE ID of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtablerev", 'r', 0,
   N_("Set OEMTABLE revision of RSDP, XSDT and RSDT."), 0, ARG_TYPE_INT},
  {"oemtablecreator", 'c', 0,
   N_("Set creator field of RSDP, XSDT and RSDT."), 0, ARG_TYPE_STRING},
  {"oemtablecreatorrev", 'd', 0,
   N_("Set creator revision of RSDP, XSDT and RSDT."), 0, ARG_TYPE_INT},
  /* TRANSLATORS: "hangs" here is a noun, not a verb.  */
  {"no-ebda", 'e', 0, N_("Don't update EBDA. May fix failures or hangs on some "
   "BIOSes but makes it ineffective with OS not receiving RSDP from holy."),
   0, ARG_TYPE_NONE},
  {0, 0, 0, 0, 0, 0}
};

/* rev1 is 1 if ACPIv1 is to be generated, 0 otherwise.
   rev2 contains the revision of ACPIv2+ to generate or 0 if none. */
static int rev1, rev2;
/* OEMID of RSDP, RSDT and XSDT. */
static char root_oemid[6];
/* OEMTABLE of the same tables. */
static char root_oemtable[8];
/* OEMREVISION of the same tables. */
static holy_uint32_t root_oemrev;
/* CreatorID of the same tables. */
static char root_creator_id[4];
/* CreatorRevision of the same tables. */
static holy_uint32_t root_creator_rev;
static struct holy_acpi_rsdp_v10 *rsdpv1_new = 0;
static struct holy_acpi_rsdp_v20 *rsdpv2_new = 0;
static char *playground = 0, *playground_ptr = 0;
static int playground_size = 0;

/* Linked list of ACPI tables. */
struct efiemu_acpi_table
{
  void *addr;
  holy_size_t size;
  struct efiemu_acpi_table *next;
};
static struct efiemu_acpi_table *acpi_tables = 0;

/* DSDT isn't in RSDT. So treat it specially. */
static void *table_dsdt = 0;
/* Pointer to recreated RSDT. */
static void *rsdt_addr = 0;

/* Allocation handles for different tables. */
static holy_size_t dsdt_size = 0;

/* Address of original FACS. */
static holy_uint32_t facs_addr = 0;

struct holy_acpi_rsdp_v20 *
holy_acpi_get_rsdpv2 (void)
{
  if (rsdpv2_new)
    return rsdpv2_new;
  if (rsdpv1_new)
    return 0;
  return holy_machine_acpi_get_rsdpv2 ();
}

struct holy_acpi_rsdp_v10 *
holy_acpi_get_rsdpv1 (void)
{
  if (rsdpv1_new)
    return rsdpv1_new;
  if (rsdpv2_new)
    return 0;
  return holy_machine_acpi_get_rsdpv1 ();
}

#if defined (__i386__) || defined (__x86_64__)

static inline int
iszero (holy_uint8_t *reg, int size)
{
  int i;
  for (i = 0; i < size; i++)
    if (reg[i])
      return 0;
  return 1;
}

/* Context for holy_acpi_create_ebda.  */
struct holy_acpi_create_ebda_ctx {
  int ebda_len;
  holy_uint64_t highestlow;
};

/* Helper for holy_acpi_create_ebda.  */
static int
find_hook (holy_uint64_t start, holy_uint64_t size, holy_memory_type_t type,
	   void *data)
{
  struct holy_acpi_create_ebda_ctx *ctx = data;
  holy_uint64_t end = start + size;
  if (type != holy_MEMORY_AVAILABLE)
    return 0;
  if (end > 0x100000)
    end = 0x100000;
  if (end > start + ctx->ebda_len
      && ctx->highestlow < ((end - ctx->ebda_len) & (~0xf)) )
    ctx->highestlow = (end - ctx->ebda_len) & (~0xf);
  return 0;
}

holy_err_t
holy_acpi_create_ebda (void)
{
  struct holy_acpi_create_ebda_ctx ctx = {
    .highestlow = 0
  };
  int ebda_kb_len = 0;
  int mmapregion = 0;
  holy_uint8_t *ebda, *v1inebda = 0, *v2inebda = 0;
  holy_uint8_t *targetebda, *target;
  struct holy_acpi_rsdp_v10 *v1;
  struct holy_acpi_rsdp_v20 *v2;

  ebda = (holy_uint8_t *) (holy_addr_t) ((*((holy_uint16_t *)0x40e)) << 4);
  holy_dprintf ("acpi", "EBDA @%p\n", ebda);
  if (ebda)
    ebda_kb_len = *(holy_uint16_t *) ebda;
  holy_dprintf ("acpi", "EBDA length 0x%x\n", ebda_kb_len);
  if (ebda_kb_len > 16)
    ebda_kb_len = 0;
  ctx.ebda_len = (ebda_kb_len + 1) << 10;

  /* FIXME: use low-memory mm allocation once it's available. */
  holy_mmap_iterate (find_hook, &ctx);
  targetebda = (holy_uint8_t *) (holy_addr_t) ctx.highestlow;
  holy_dprintf ("acpi", "creating ebda @%llx\n",
		(unsigned long long) ctx.highestlow);
  if (! ctx.highestlow)
    return holy_error (holy_ERR_OUT_OF_MEMORY,
		       "couldn't find space for the new EBDA");

  mmapregion = holy_mmap_register ((holy_addr_t) targetebda, ctx.ebda_len,
				   holy_MEMORY_RESERVED);
  if (! mmapregion)
    return holy_errno;

  /* XXX: EBDA is unstandardized, so this implementation is heuristical. */
  if (ebda_kb_len)
    holy_memcpy (targetebda, ebda, 0x400);
  else
    holy_memset (targetebda, 0, 0x400);
  *((holy_uint16_t *) targetebda) = ebda_kb_len + 1;
  target = targetebda;

  v1 = holy_acpi_get_rsdpv1 ();
  v2 = holy_acpi_get_rsdpv2 ();
  if (v2 && v2->length > 40)
    v2 = 0;

  /* First try to replace already existing rsdp. */
  if (v2)
    {
      holy_dprintf ("acpi", "Scanning EBDA for old rsdpv2\n");
      for (; target < targetebda + 0x400 - v2->length; target += 0x10)
	if (holy_memcmp (target, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	    && holy_byte_checksum (target,
				   sizeof (struct holy_acpi_rsdp_v10)) == 0
	    && ((struct holy_acpi_rsdp_v10 *) target)->revision != 0
	    && ((struct holy_acpi_rsdp_v20 *) target)->length <= v2->length)
	  {
	    holy_memcpy (target, v2, v2->length);
	    holy_dprintf ("acpi", "Copying rsdpv2 to %p\n", target);
	    v2inebda = target;
	    target += v2->length;
	    target = (holy_uint8_t *) ALIGN_UP((holy_addr_t) target, 16);
	    v2 = 0;
	    break;
	  }
    }

  if (v1)
    {
      holy_dprintf ("acpi", "Scanning EBDA for old rsdpv1\n");
      for (; target < targetebda + 0x400 - sizeof (struct holy_acpi_rsdp_v10);
	   target += 0x10)
	if (holy_memcmp (target, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	    && holy_byte_checksum (target,
				   sizeof (struct holy_acpi_rsdp_v10)) == 0)
	  {
	    holy_memcpy (target, v1, sizeof (struct holy_acpi_rsdp_v10));
	    holy_dprintf ("acpi", "Copying rsdpv1 to %p\n", target);
	    v1inebda = target;
	    target += sizeof (struct holy_acpi_rsdp_v10);
	    target = (holy_uint8_t *) ALIGN_UP((holy_addr_t) target, 16);
	    v1 = 0;
	    break;
	  }
    }

  target = targetebda + 0x100;

  /* Try contiguous zeros. */
  if (v2)
    {
      holy_dprintf ("acpi", "Scanning EBDA for block of zeros\n");
      for (; target < targetebda + 0x400 - v2->length; target += 0x10)
	if (iszero (target, v2->length))
	  {
	    holy_dprintf ("acpi", "Copying rsdpv2 to %p\n", target);
	    holy_memcpy (target, v2, v2->length);
	    v2inebda = target;
	    target += v2->length;
	    target = (holy_uint8_t *) ALIGN_UP((holy_addr_t) target, 16);
	    v2 = 0;
	    break;
	  }
    }

  if (v1)
    {
      holy_dprintf ("acpi", "Scanning EBDA for block of zeros\n");
      for (; target < targetebda + 0x400 - sizeof (struct holy_acpi_rsdp_v10);
	   target += 0x10)
	if (iszero (target, sizeof (struct holy_acpi_rsdp_v10)))
	  {
	    holy_dprintf ("acpi", "Copying rsdpv1 to %p\n", target);
	    holy_memcpy (target, v1, sizeof (struct holy_acpi_rsdp_v10));
	    v1inebda = target;
	    target += sizeof (struct holy_acpi_rsdp_v10);
	    target = (holy_uint8_t *) ALIGN_UP((holy_addr_t) target, 16);
	    v1 = 0;
	    break;
	  }
    }

  if (v1 || v2)
    {
      holy_mmap_unregister (mmapregion);
      return holy_error (holy_ERR_OUT_OF_MEMORY,
			 "couldn't find suitable spot in EBDA");
    }

  /* Remove any other RSDT. */
  for (target = targetebda;
       target < targetebda + 0x400 - sizeof (struct holy_acpi_rsdp_v10);
       target += 0x10)
    if (holy_memcmp (target, holy_RSDP_SIGNATURE, holy_RSDP_SIGNATURE_SIZE) == 0
	&& holy_byte_checksum (target,
			       sizeof (struct holy_acpi_rsdp_v10)) == 0
	&& target != v1inebda && target != v2inebda)
      *target = 0;

  holy_dprintf ("acpi", "Switching EBDA\n");
  (*((holy_uint16_t *) 0x40e)) = ((holy_addr_t) targetebda) >> 4;
  holy_dprintf ("acpi", "EBDA switched\n");

  return holy_ERR_NONE;
}
#endif

/* Create tables common to ACPIv1 and ACPIv2+ */
static void
setup_common_tables (void)
{
  struct efiemu_acpi_table *cur;
  struct holy_acpi_table_header *rsdt;
  holy_uint32_t *rsdt_entry;
  int numoftables;

  /* Treat DSDT. */
  holy_memcpy (playground_ptr, table_dsdt, dsdt_size);
  holy_free (table_dsdt);
  table_dsdt = playground_ptr;
  playground_ptr += dsdt_size;

  /* Treat other tables. */
  for (cur = acpi_tables; cur; cur = cur->next)
    {
      struct holy_acpi_fadt *fadt;

      holy_memcpy (playground_ptr, cur->addr, cur->size);
      holy_free (cur->addr);
      cur->addr = playground_ptr;
      playground_ptr += cur->size;

      /* If it's FADT correct DSDT and FACS addresses. */
      fadt = (struct holy_acpi_fadt *) cur->addr;
      if (holy_memcmp (fadt->hdr.signature, holy_ACPI_FADT_SIGNATURE,
		       sizeof (fadt->hdr.signature)) == 0)
	{
	  fadt->dsdt_addr = (holy_addr_t) table_dsdt;
	  fadt->facs_addr = facs_addr;

	  /* Does a revision 2 exist at all? */
	  if (fadt->hdr.revision >= 3)
	    {
	      fadt->dsdt_xaddr = (holy_addr_t) table_dsdt;
	      fadt->facs_xaddr = facs_addr;
	    }

	  /* Recompute checksum. */
	  fadt->hdr.checksum = 0;
	  fadt->hdr.checksum = 1 + ~holy_byte_checksum (fadt, fadt->hdr.length);
	}
    }

  /* Fill RSDT entries. */
  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  rsdt_addr = rsdt = (struct holy_acpi_table_header *) playground_ptr;
  playground_ptr += sizeof (struct holy_acpi_table_header) + sizeof (holy_uint32_t) * numoftables;

  rsdt_entry = (holy_uint32_t *) (rsdt + 1);

  /* Fill RSDT header. */
  holy_memcpy (&(rsdt->signature), "RSDT", 4);
  rsdt->length = sizeof (struct holy_acpi_table_header) + sizeof (holy_uint32_t) * numoftables;
  rsdt->revision = 1;
  holy_memcpy (&(rsdt->oemid), root_oemid, sizeof (rsdt->oemid));
  holy_memcpy (&(rsdt->oemtable), root_oemtable, sizeof (rsdt->oemtable));
  rsdt->oemrev = root_oemrev;
  holy_memcpy (&(rsdt->creator_id), root_creator_id, sizeof (rsdt->creator_id));
  rsdt->creator_rev = root_creator_rev;

  for (cur = acpi_tables; cur; cur = cur->next)
    *(rsdt_entry++) = (holy_addr_t) cur->addr;

  /* Recompute checksum. */
  rsdt->checksum = 0;
  rsdt->checksum = 1 + ~holy_byte_checksum (rsdt, rsdt->length);
}

/* Regenerate ACPIv1 RSDP */
static void
setv1table (void)
{
  /* Create RSDP. */
  rsdpv1_new = (struct holy_acpi_rsdp_v10 *) playground_ptr;
  playground_ptr += sizeof (struct holy_acpi_rsdp_v10);
  holy_memcpy (&(rsdpv1_new->signature), holy_RSDP_SIGNATURE,
	       sizeof (rsdpv1_new->signature));
  holy_memcpy (&(rsdpv1_new->oemid), root_oemid, sizeof  (rsdpv1_new->oemid));
  rsdpv1_new->revision = 0;
  rsdpv1_new->rsdt_addr = (holy_addr_t) rsdt_addr;
  rsdpv1_new->checksum = 0;
  rsdpv1_new->checksum = 1 + ~holy_byte_checksum (rsdpv1_new,
						  sizeof (*rsdpv1_new));
  holy_dprintf ("acpi", "Generated ACPIv1 tables\n");
}

static void
setv2table (void)
{
  struct holy_acpi_table_header *xsdt;
  struct efiemu_acpi_table *cur;
  holy_uint64_t *xsdt_entry;
  int numoftables;

  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  /* Create XSDT. */
  xsdt = (struct holy_acpi_table_header *) playground_ptr;
  playground_ptr += sizeof (struct holy_acpi_table_header) + sizeof (holy_uint64_t) * numoftables;

  xsdt_entry = (holy_uint64_t *)(xsdt + 1);
  for (cur = acpi_tables; cur; cur = cur->next)
    *(xsdt_entry++) = (holy_addr_t) cur->addr;
  holy_memcpy (&(xsdt->signature), "XSDT", 4);
  xsdt->length = sizeof (struct holy_acpi_table_header) + sizeof (holy_uint64_t) * numoftables;
  xsdt->revision = 1;
  holy_memcpy (&(xsdt->oemid), root_oemid, sizeof (xsdt->oemid));
  holy_memcpy (&(xsdt->oemtable), root_oemtable, sizeof (xsdt->oemtable));
  xsdt->oemrev = root_oemrev;
  holy_memcpy (&(xsdt->creator_id), root_creator_id, sizeof (xsdt->creator_id));
  xsdt->creator_rev = root_creator_rev;
  xsdt->checksum = 0;
  xsdt->checksum = 1 + ~holy_byte_checksum (xsdt, xsdt->length);

  /* Create RSDPv2. */
  rsdpv2_new = (struct holy_acpi_rsdp_v20 *) playground_ptr;
  playground_ptr += sizeof (struct holy_acpi_rsdp_v20);
  holy_memcpy (&(rsdpv2_new->rsdpv1.signature), holy_RSDP_SIGNATURE,
	       sizeof (rsdpv2_new->rsdpv1.signature));
  holy_memcpy (&(rsdpv2_new->rsdpv1.oemid), root_oemid,
	       sizeof (rsdpv2_new->rsdpv1.oemid));
  rsdpv2_new->rsdpv1.revision = rev2;
  rsdpv2_new->rsdpv1.rsdt_addr = (holy_addr_t) rsdt_addr;
  rsdpv2_new->rsdpv1.checksum = 0;
  rsdpv2_new->rsdpv1.checksum = 1 + ~holy_byte_checksum
    (&(rsdpv2_new->rsdpv1), sizeof (rsdpv2_new->rsdpv1));
  rsdpv2_new->length = sizeof (*rsdpv2_new);
  rsdpv2_new->xsdt_addr = (holy_addr_t) xsdt;
  rsdpv2_new->checksum = 0;
  rsdpv2_new->checksum = 1 + ~holy_byte_checksum (rsdpv2_new,
						  rsdpv2_new->length);
  holy_dprintf ("acpi", "Generated ACPIv2 tables\n");
}

static void
free_tables (void)
{
  struct efiemu_acpi_table *cur, *t;
  if (table_dsdt)
    holy_free (table_dsdt);
  for (cur = acpi_tables; cur;)
    {
      t = cur;
      holy_free (cur->addr);
      cur = cur->next;
      holy_free (t);
    }
  acpi_tables = 0;
  table_dsdt = 0;
}

static holy_err_t
holy_cmd_acpi (struct holy_extcmd_context *ctxt, int argc, char **args)
{
  struct holy_arg_list *state = ctxt->state;
  struct holy_acpi_rsdp_v10 *rsdp;
  struct efiemu_acpi_table *cur, *t;
  int i, mmapregion;
  int numoftables;

  /* Default values if no RSDP is found. */
  rev1 = 1;
  rev2 = 3;

  facs_addr = 0;
  playground = playground_ptr = 0;
  playground_size = 0;

  rsdp = (struct holy_acpi_rsdp_v10 *) holy_machine_acpi_get_rsdpv2 ();

  if (! rsdp)
    rsdp = holy_machine_acpi_get_rsdpv1 ();

  holy_dprintf ("acpi", "RSDP @%p\n", rsdp);

  if (rsdp)
    {
      holy_uint32_t *entry_ptr;
      char *exclude = 0;
      char *load_only = 0;
      char *ptr;
      /* RSDT consists of header and an array of 32-bit pointers. */
      struct holy_acpi_table_header *rsdt;

      exclude = state[0].set ? holy_strdup (state[0].arg) : 0;
      if (exclude)
	{
	  for (ptr = exclude; *ptr; ptr++)
	    *ptr = holy_tolower (*ptr);
	}

      load_only = state[1].set ? holy_strdup (state[1].arg) : 0;
      if (load_only)
	{
	  for (ptr = load_only; *ptr; ptr++)
	    *ptr = holy_tolower (*ptr);
	}

      /* Set revision variables to replicate the same version as host. */
      rev1 = ! rsdp->revision;
      rev2 = rsdp->revision;
      rsdt = (struct holy_acpi_table_header *) (holy_addr_t) rsdp->rsdt_addr;
      /* Load host tables. */
      for (entry_ptr = (holy_uint32_t *) (rsdt + 1);
	   entry_ptr < (holy_uint32_t *) (((holy_uint8_t *) rsdt)
					  + rsdt->length);
	   entry_ptr++)
	{
	  char signature[5];
	  struct efiemu_acpi_table *table;
	  struct holy_acpi_table_header *curtable
	    = (struct holy_acpi_table_header *) (holy_addr_t) *entry_ptr;
	  signature[4] = 0;
	  for (i = 0; i < 4;i++)
	    signature[i] = holy_tolower (curtable->signature[i]);

	  /* If it's FADT it contains addresses of DSDT and FACS. */
	  if (holy_strcmp (signature, "facp") == 0)
	    {
	      struct holy_acpi_table_header *dsdt;
	      struct holy_acpi_fadt *fadt = (struct holy_acpi_fadt *) curtable;

	      /* Set root header variables to the same values
		 as FADT by default. */
	      holy_memcpy (&root_oemid, &(fadt->hdr.oemid),
			   sizeof (root_oemid));
	      holy_memcpy (&root_oemtable, &(fadt->hdr.oemtable),
			   sizeof (root_oemtable));
	      root_oemrev = fadt->hdr.oemrev;
	      holy_memcpy (&root_creator_id, &(fadt->hdr.creator_id),
			   sizeof (root_creator_id));
	      root_creator_rev = fadt->hdr.creator_rev;

	      /* Load DSDT if not excluded. */
	      dsdt = (struct holy_acpi_table_header *)
		(holy_addr_t) fadt->dsdt_addr;
	      if (dsdt && (! exclude || ! holy_strword (exclude, "dsdt"))
		  && (! load_only || holy_strword (load_only, "dsdt"))
		  && dsdt->length >= sizeof (*dsdt))
		{
		  dsdt_size = dsdt->length;
		  table_dsdt = holy_malloc (dsdt->length);
		  if (! table_dsdt)
		    {
		      free_tables ();
		      holy_free (exclude);
		      holy_free (load_only);
		      return holy_errno;
		    }
		  holy_memcpy (table_dsdt, dsdt, dsdt->length);
		}

	      /* Save FACS address. FACS shouldn't be overridden. */
	      facs_addr = fadt->facs_addr;
	    }

	  /* Skip excluded tables. */
	  if (exclude && holy_strword (exclude, signature))
	    continue;
	  if (load_only && ! holy_strword (load_only, signature))
	    continue;

	  /* Sanity check. */
	  if (curtable->length < sizeof (*curtable))
	    continue;

	  table = (struct efiemu_acpi_table *) holy_malloc
	    (sizeof (struct efiemu_acpi_table));
	  if (! table)
	    {
	      free_tables ();
	      holy_free (exclude);
	      holy_free (load_only);
	      return holy_errno;
	    }
	  table->size = curtable->length;
	  table->addr = holy_malloc (table->size);
	  playground_size += table->size;
	  if (! table->addr)
	    {
	      free_tables ();
	      holy_free (exclude);
	      holy_free (load_only);
	      holy_free (table);
	      return holy_errno;
	    }
	  table->next = acpi_tables;
	  acpi_tables = table;
	  holy_memcpy (table->addr, curtable, table->size);
	}
      holy_free (exclude);
      holy_free (load_only);
    }

  /* Does user specify versions to generate? */
  if (state[2].set || state[3].set)
    {
      rev1 = state[2].set;
      if (state[3].set)
	rev2 = rev2 ? : 2;
      else
	rev2 = 0;
    }

  /* Does user override root header information? */
  if (state[4].set)
    holy_strncpy (root_oemid, state[4].arg, sizeof (root_oemid));
  if (state[5].set)
    holy_strncpy (root_oemtable, state[5].arg, sizeof (root_oemtable));
  if (state[6].set)
    root_oemrev = holy_strtoul (state[6].arg, 0, 0);
  if (state[7].set)
    holy_strncpy (root_creator_id, state[7].arg, sizeof (root_creator_id));
  if (state[8].set)
    root_creator_rev = holy_strtoul (state[8].arg, 0, 0);

  /* Load user tables */
  for (i = 0; i < argc; i++)
    {
      holy_file_t file;
      holy_size_t size;
      char *buf;

      file = holy_file_open (args[i]);
      if (! file)
	{
	  free_tables ();
	  return holy_errno;
	}

      size = holy_file_size (file);
      if (size < sizeof (struct holy_acpi_table_header))
	{
	  holy_file_close (file);
	  free_tables ();
	  return holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			     args[i]);
	}

      buf = (char *) holy_malloc (size);
      if (! buf)
	{
	  holy_file_close (file);
	  free_tables ();
	  return holy_errno;
	}

      if (holy_file_read (file, buf, size) != (int) size)
	{
	  holy_file_close (file);
	  free_tables ();
	  if (!holy_errno)
	    holy_error (holy_ERR_BAD_OS, N_("premature end of file %s"),
			args[i]);
	  return holy_errno;
	}
      holy_file_close (file);

      if (holy_memcmp (((struct holy_acpi_table_header *) buf)->signature,
		       "DSDT", 4) == 0)
	{
	  holy_free (table_dsdt);
	  table_dsdt = buf;
	  dsdt_size = size;
	}
      else
	{
	  struct efiemu_acpi_table *table;
	  table = (struct efiemu_acpi_table *) holy_malloc
	    (sizeof (struct efiemu_acpi_table));
	  if (! table)
	    {
	      free_tables ();
	      return holy_errno;
	    }

	  table->size = size;
	  table->addr = buf;
	  playground_size += table->size;

	  table->next = acpi_tables;
	  acpi_tables = table;
	}
    }

  numoftables = 0;
  for (cur = acpi_tables; cur; cur = cur->next)
    numoftables++;

  /* DSDT. */
  playground_size += dsdt_size;
  /* RSDT. */
  playground_size += sizeof (struct holy_acpi_table_header) + sizeof (holy_uint32_t) * numoftables;
  /* RSDPv1. */
  playground_size += sizeof (struct holy_acpi_rsdp_v10);
  /* XSDT. */
  playground_size += sizeof (struct holy_acpi_table_header) + sizeof (holy_uint64_t) * numoftables;
  /* RSDPv2. */
  playground_size += sizeof (struct holy_acpi_rsdp_v20);

  playground = playground_ptr
    = holy_mmap_malign_and_register (1, playground_size, &mmapregion,
				     holy_MEMORY_ACPI, 0);

  if (! playground)
    {
      free_tables ();
      return holy_error (holy_ERR_OUT_OF_MEMORY,
			 "couldn't allocate space for ACPI tables");
    }

  setup_common_tables ();

  /* Request space for RSDPv1. */
  if (rev1)
    setv1table ();

  /* Request space for RSDPv2+ and XSDT. */
  if (rev2)
    setv2table ();

  for (cur = acpi_tables; cur;)
    {
      t = cur;
      cur = cur->next;
      holy_free (t);
    }
  acpi_tables = 0;

#if defined (__i386__) || defined (__x86_64__)
  if (! state[9].set)
    {
      holy_err_t err;
      err = holy_acpi_create_ebda ();
      if (err)
	{
	  rsdpv1_new = 0;
	  rsdpv2_new = 0;
	  holy_mmap_free_and_unregister (mmapregion);
	  return err;
	}
    }
#endif

#ifdef holy_MACHINE_EFI
  {
    struct holy_efi_guid acpi = holy_EFI_ACPI_TABLE_GUID;
    struct holy_efi_guid acpi20 = holy_EFI_ACPI_20_TABLE_GUID;

    efi_call_2 (holy_efi_system_table->boot_services->install_configuration_table,
      &acpi20, holy_acpi_get_rsdpv2 ());
    efi_call_2 (holy_efi_system_table->boot_services->install_configuration_table,
      &acpi, holy_acpi_get_rsdpv1 ());
  }
#endif

  return holy_ERR_NONE;
}

static holy_extcmd_t cmd;

holy_MOD_INIT(acpi)
{
  cmd = holy_register_extcmd ("acpi", holy_cmd_acpi, 0,
			      N_("[-1|-2] [--exclude=TABLE1,TABLE2|"
			      "--load-only=TABLE1,TABLE2] FILE1"
			      " [FILE2] [...]"),
			      N_("Load host ACPI tables and tables "
			      "specified by arguments."),
			      options);
}

holy_MOD_FINI(acpi)
{
  holy_unregister_extcmd (cmd);
}
