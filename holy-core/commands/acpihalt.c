/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifdef holy_DSDT_TEST
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#define holy_dprintf(cond, args...) printf ( args )
#define holy_printf printf
#define holy_util_fopen fopen
#define holy_memcmp memcmp
typedef uint64_t holy_uint64_t;
typedef uint32_t holy_uint32_t;
typedef uint16_t holy_uint16_t;
typedef uint8_t holy_uint8_t;

#endif

#include <holy/acpi.h>
#ifndef holy_DSDT_TEST
#include <holy/i18n.h>
#else
#define _(x) x
#define N_(x) x
#endif

#ifndef holy_DSDT_TEST
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/time.h>
#include <holy/cpu/io.h>
#endif

static inline holy_uint32_t
decode_length (const holy_uint8_t *ptr, int *numlen)
{
  int num_bytes, i;
  holy_uint32_t ret;
  if (*ptr < 64)
    {
      if (numlen)
	*numlen = 1;
      return *ptr;
    }
  num_bytes = *ptr >> 6;
  if (numlen)
    *numlen = num_bytes + 1;
  ret = *ptr & 0xf;
  ptr++;
  for (i = 0; i < num_bytes; i++)
    {
      ret |= *ptr << (8 * i + 4);
      ptr++;
    }
  return ret;
}

static inline holy_uint32_t
skip_name_string (const holy_uint8_t *ptr, const holy_uint8_t *end)
{
  const holy_uint8_t *ptr0 = ptr;
  
  while (ptr < end && (*ptr == '^' || *ptr == '\\'))
    ptr++;
  switch (*ptr)
    {
    case '.':
      ptr++;
      ptr += 8;
      break;
    case '/':
      ptr++;
      ptr += 1 + (*ptr) * 4;
      break;
    case 0:
      ptr++;
      break;
    default:
      ptr += 4;
      break;
    }
  return ptr - ptr0;
}

static inline holy_uint32_t
skip_data_ref_object (const holy_uint8_t *ptr, const holy_uint8_t *end)
{
  holy_dprintf ("acpi", "data type = 0x%x\n", *ptr);
  switch (*ptr)
    {
    case holy_ACPI_OPCODE_PACKAGE:
    case holy_ACPI_OPCODE_BUFFER:
      return 1 + decode_length (ptr + 1, 0);
    case holy_ACPI_OPCODE_ZERO:
    case holy_ACPI_OPCODE_ONES:
    case holy_ACPI_OPCODE_ONE:
      return 1;
    case holy_ACPI_OPCODE_BYTE_CONST:
      return 2;
    case holy_ACPI_OPCODE_WORD_CONST:
      return 3;
    case holy_ACPI_OPCODE_DWORD_CONST:
      return 5;
    case holy_ACPI_OPCODE_STRING_CONST:
      {
	const holy_uint8_t *ptr0 = ptr;
	for (ptr++; ptr < end && *ptr; ptr++);
	if (ptr == end)
	  return 0;
	return ptr - ptr0 + 1;
      }
    default:
      if (*ptr == '^' || *ptr == '\\' || *ptr == '_'
	  || (*ptr >= 'A' && *ptr <= 'Z'))
	return skip_name_string (ptr, end);
      holy_printf ("Unknown opcode 0x%x\n", *ptr);
      return 0;
    }
}

static inline holy_uint32_t
skip_term (const holy_uint8_t *ptr, const holy_uint8_t *end)
{
  holy_uint32_t add;
  const holy_uint8_t *ptr0 = ptr;

  switch(*ptr)
  {
    case holy_ACPI_OPCODE_ADD:
    case holy_ACPI_OPCODE_AND:
    case holy_ACPI_OPCODE_CONCAT:
    case holy_ACPI_OPCODE_CONCATRES:
    case holy_ACPI_OPCODE_DIVIDE:
    case holy_ACPI_OPCODE_INDEX:
    case holy_ACPI_OPCODE_LSHIFT:
    case holy_ACPI_OPCODE_MOD:
    case holy_ACPI_OPCODE_MULTIPLY:
    case holy_ACPI_OPCODE_NAND:
    case holy_ACPI_OPCODE_NOR:
    case holy_ACPI_OPCODE_OR:
    case holy_ACPI_OPCODE_RSHIFT:
    case holy_ACPI_OPCODE_SUBTRACT:
    case holy_ACPI_OPCODE_TOSTRING:
    case holy_ACPI_OPCODE_XOR:
      /*
       * Parameters for these opcodes: TermArg, TermArg Target, see ACPI
       * spec r5.0, page 828f.
       */
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
        return 0;
      ptr += skip_name_string (ptr, end);
      break;
    default:
      return skip_data_ref_object (ptr, end);
  }
  return ptr - ptr0;
}

static inline holy_uint32_t
skip_ext_op (const holy_uint8_t *ptr, const holy_uint8_t *end)
{
  const holy_uint8_t *ptr0 = ptr;
  int add;
  holy_dprintf ("acpi", "Extended opcode: 0x%x\n", *ptr);
  switch (*ptr)
    {
    case holy_ACPI_EXTOPCODE_MUTEX:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      break;
    case holy_ACPI_EXTOPCODE_EVENT_OP:
      ptr++;
      ptr += skip_name_string (ptr, end);
      break;
    case holy_ACPI_EXTOPCODE_OPERATION_REGION:
      ptr++;
      ptr += skip_name_string (ptr, end);
      ptr++;
      ptr += add = skip_term (ptr, end);
      if (!add)
	return 0;
      ptr += add = skip_term (ptr, end);
      if (!add)
	return 0;
      break;
    case holy_ACPI_EXTOPCODE_FIELD_OP:
    case holy_ACPI_EXTOPCODE_DEVICE_OP:
    case holy_ACPI_EXTOPCODE_PROCESSOR_OP:
    case holy_ACPI_EXTOPCODE_POWER_RES_OP:
    case holy_ACPI_EXTOPCODE_THERMAL_ZONE_OP:
    case holy_ACPI_EXTOPCODE_INDEX_FIELD_OP:
    case holy_ACPI_EXTOPCODE_BANK_FIELD_OP:
      ptr++;
      ptr += decode_length (ptr, 0);
      break;
    default:
      holy_printf ("Unexpected extended opcode: 0x%x\n", *ptr);
      return 0;
    }
  return ptr - ptr0;
}


static int
get_sleep_type (holy_uint8_t *table, holy_uint8_t *ptr, holy_uint8_t *end,
		holy_uint8_t *scope, int scope_len)
{
  holy_uint8_t *prev = table;
  
  if (!ptr)
    ptr = table + sizeof (struct holy_acpi_table_header);
  while (ptr < end && prev < ptr)
    {
      int add;
      prev = ptr;
      holy_dprintf ("acpi", "Opcode 0x%x\n", *ptr);
      holy_dprintf ("acpi", "Tell %x\n", (unsigned) (ptr - table));
      switch (*ptr)
	{
	case holy_ACPI_OPCODE_EXTOP:
	  ptr++;
	  ptr += add = skip_ext_op (ptr, end);
	  if (!add)
	    return -1;
	  break;
	case holy_ACPI_OPCODE_CREATE_DWORD_FIELD:
	case holy_ACPI_OPCODE_CREATE_WORD_FIELD:
	case holy_ACPI_OPCODE_CREATE_BYTE_FIELD:
	  {
	    ptr += 5;
	    ptr += add = skip_data_ref_object (ptr, end);
	    if (!add)
	      return -1;
	    ptr += 4;
	    break;
	  }
	case holy_ACPI_OPCODE_NAME:
	  ptr++;
	  if ((!scope || holy_memcmp (scope, "\\", scope_len) == 0) &&
	      (holy_memcmp (ptr, "_S5_", 4) == 0 || holy_memcmp (ptr, "\\_S5_", 4) == 0))
	    {
	      int ll;
	      holy_uint8_t *ptr2 = ptr;
	      holy_dprintf ("acpi", "S5 found\n");
	      ptr2 += skip_name_string (ptr, end);
	      if (*ptr2 != 0x12)
		{
		  holy_printf ("Unknown opcode in _S5: 0x%x\n", *ptr2);
		  return -1;
		}
	      ptr2++;
	      decode_length (ptr2, &ll);
	      ptr2 += ll;
	      ptr2++;
	      switch (*ptr2)
		{
		case holy_ACPI_OPCODE_ZERO:
		  return 0;
		case holy_ACPI_OPCODE_ONE:
		  return 1;
		case holy_ACPI_OPCODE_BYTE_CONST:
		  return ptr2[1];
		default:
		  holy_printf ("Unknown data type in _S5: 0x%x\n", *ptr2);
		  return -1;
		}
	    }
	  ptr += add = skip_name_string (ptr, end);
	  if (!add)
	    return -1;
	  ptr += add = skip_data_ref_object (ptr, end);
	  if (!add)
	    return -1;
	  break;
	case holy_ACPI_OPCODE_ALIAS:
	  ptr++;
	  /* We need to skip two name strings */
	  ptr += add = skip_name_string (ptr, end);
	  if (!add)
	    return -1;
	  ptr += add = skip_name_string (ptr, end);
	  if (!add)
	    return -1;
	  break;

	case holy_ACPI_OPCODE_SCOPE:
	  {
	    int scope_sleep_type;
	    int ll;
	    holy_uint8_t *name;
	    int name_len;

	    ptr++;
	    add = decode_length (ptr, &ll);
	    name = ptr + ll;
	    name_len = skip_name_string (name, ptr + add);
	    if (!name_len)
	      return -1;
	    scope_sleep_type = get_sleep_type (table, name + name_len,
					       ptr + add, name, name_len);
	    if (scope_sleep_type != -2)
	      return scope_sleep_type;
	    ptr += add;
	    break;
	  }
	case holy_ACPI_OPCODE_IF:
	case holy_ACPI_OPCODE_METHOD:
	  {
	    ptr++;
	    ptr += decode_length (ptr, 0);
	    break;
	  }
	default:
	  holy_printf ("Unknown opcode 0x%x\n", *ptr);
	  return -1;	  
	}
    }

  return -2;
}

#ifdef holy_DSDT_TEST
int
main (int argc, char **argv)
{
  FILE *f;
  size_t len;
  unsigned char *buf;
  if (argc < 2)
    printf ("Usage: %s FILE\n", argv[0]);
  f = holy_util_fopen (argv[1], "rb");
  if (!f)
    {
      printf ("Couldn't open file\n");
      return 1;
    }
  fseek (f, 0, SEEK_END);
  len = ftell (f);
  fseek (f, 0, SEEK_SET);
  buf = malloc (len);
  if (!buf)
    {
      printf (_("error: %s.\n"), _("out of memory"));
      fclose (f);
      return 2;
    }
  if (fread (buf, 1, len, f) != len)
    {
      printf (_("cannot read `%s': %s"), argv[1], strerror (errno));
      free (buf);
      fclose (f);
      return 2;
    }

  printf ("Sleep type = %d\n", get_sleep_type (buf, NULL, buf + len, NULL, 0));
  free (buf);
  fclose (f);
  return 0;
}

#else

void
holy_acpi_halt (void)
{
  struct holy_acpi_rsdp_v20 *rsdp2;
  struct holy_acpi_rsdp_v10 *rsdp1;
  struct holy_acpi_table_header *rsdt;
  holy_uint32_t *entry_ptr;
  holy_uint32_t port = 0;
  int sleep_type = -1;

  rsdp2 = holy_acpi_get_rsdpv2 ();
  if (rsdp2)
    rsdp1 = &(rsdp2->rsdpv1);
  else
    rsdp1 = holy_acpi_get_rsdpv1 ();
  holy_dprintf ("acpi", "rsdp1=%p\n", rsdp1);
  if (!rsdp1)
    return;

  rsdt = (struct holy_acpi_table_header *) (holy_addr_t) rsdp1->rsdt_addr;
  for (entry_ptr = (holy_uint32_t *) (rsdt + 1);
       entry_ptr < (holy_uint32_t *) (((holy_uint8_t *) rsdt)
				      + rsdt->length);
       entry_ptr++)
    {
      if (holy_memcmp ((void *) (holy_addr_t) *entry_ptr, "FACP", 4) == 0)
	{
	  struct holy_acpi_fadt *fadt
	    = ((struct holy_acpi_fadt *) (holy_addr_t) *entry_ptr);
	  struct holy_acpi_table_header *dsdt
	    = (struct holy_acpi_table_header *) (holy_addr_t) fadt->dsdt_addr;
	  holy_uint8_t *buf = (holy_uint8_t *) dsdt;

	  port = fadt->pm1a;

	  holy_dprintf ("acpi", "PM1a port=%x\n", port);

	  if (holy_memcmp (dsdt->signature, "DSDT",
			   sizeof (dsdt->signature)) == 0
	      && sleep_type < 0)
	    sleep_type = get_sleep_type (buf, NULL, buf + dsdt->length,
					 NULL, 0);
	}
      else if (holy_memcmp ((void *) (holy_addr_t) *entry_ptr, "SSDT", 4) == 0
	       && sleep_type < 0)
	{
	  struct holy_acpi_table_header *ssdt
	    = (struct holy_acpi_table_header *) (holy_addr_t) *entry_ptr;
	  holy_uint8_t *buf = (holy_uint8_t *) ssdt;

	  holy_dprintf ("acpi", "SSDT = %p\n", ssdt);

	  sleep_type = get_sleep_type (buf, NULL, buf + ssdt->length, NULL, 0);
	}
    }

  holy_dprintf ("acpi", "SLP_TYP = %d, port = 0x%x\n", sleep_type, port);
  if (port && sleep_type >= 0 && sleep_type < 8)
    holy_outw (holy_ACPI_SLP_EN | (sleep_type << holy_ACPI_SLP_TYP_OFFSET),
	       port & 0xffff);

  holy_millisleep (1500);

  /* TRANSLATORS: It's computer shutdown using ACPI, not disabling ACPI.  */
  holy_puts_ (N_("ACPI shutdown failed"));
}
#endif
