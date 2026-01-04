/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <holy/dl.h>
#include <holy/env.h>
#include <holy/extcmd.h>
#include <holy/i18n.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/smbios.h>

holy_MOD_LICENSE ("GPLv2+");


/* Locate the SMBIOS entry point structure depending on the hardware. */
struct holy_smbios_eps *
holy_smbios_get_eps (void)
{
  static struct holy_smbios_eps *eps = NULL;
  if (eps != NULL)
    return eps;
  eps = holy_machine_smbios_get_eps ();
  return eps;
}

/* Locate the SMBIOS3 entry point structure depending on the hardware. */
struct holy_smbios_eps3 *
holy_smbios_get_eps3 (void)
{
  static struct holy_smbios_eps3 *eps = NULL;
  if (eps != NULL)
    return eps;
  eps = holy_machine_smbios_get_eps3 ();
  return eps;
}

/* Abstract useful values found in either the SMBIOS3 or SMBIOS EPS. */
static struct {
  holy_addr_t start;
  holy_addr_t end;
  holy_uint16_t structures;
} table_desc = {0, 0, 0};


/*
 * These functions convert values from the various SMBIOS structure field types
 * into a string formatted to be returned to the user.  They expect that the
 * structure and offset were already validated.  The given buffer stores the
 * newly formatted string if needed.  When the requested data is successfully
 * retrieved and formatted, the pointer to the string is returned; otherwise,
 * NULL is returned on failure.
 */

static const char *
holy_smbios_format_byte (char *buffer, holy_size_t size,
                         const holy_uint8_t *structure, holy_uint8_t offset)
{
  holy_snprintf (buffer, size, "%u", structure[offset]);
  return (const char *)buffer;
}

static const char *
holy_smbios_format_word (char *buffer, holy_size_t size,
                         const holy_uint8_t *structure, holy_uint8_t offset)
{
  holy_uint16_t value = holy_get_unaligned16 (structure + offset);
  holy_snprintf (buffer, size, "%u", value);
  return (const char *)buffer;
}

static const char *
holy_smbios_format_dword (char *buffer, holy_size_t size,
                          const holy_uint8_t *structure, holy_uint8_t offset)
{
  holy_uint32_t value = holy_get_unaligned32 (structure + offset);
  holy_snprintf (buffer, size, "%" PRIuholy_UINT32_T, value);
  return (const char *)buffer;
}

static const char *
holy_smbios_format_qword (char *buffer, holy_size_t size,
                          const holy_uint8_t *structure, holy_uint8_t offset)
{
  holy_uint64_t value = holy_get_unaligned64 (structure + offset);
  holy_snprintf (buffer, size, "%" PRIuholy_UINT64_T, value);
  return (const char *)buffer;
}

/* The matching string pointer is returned directly to avoid extra copying. */
static const char *
holy_smbios_get_string (char *buffer __attribute__ ((unused)),
                        holy_size_t size __attribute__ ((unused)),
                        const holy_uint8_t *structure, holy_uint8_t offset)
{
  const holy_uint8_t *ptr = structure + structure[1];
  const holy_uint8_t *table_end = (const holy_uint8_t *)table_desc.end;
  const holy_uint8_t referenced_string_number = structure[offset];
  holy_uint8_t i;

  /* A string referenced with zero is interpreted as unset. */
  if (referenced_string_number == 0)
    return NULL;

  /* Search the string set. */
  for (i = 1; *ptr != 0 && ptr < table_end; i++)
    if (i == referenced_string_number)
      {
        const char *str = (const char *)ptr;
        while (*ptr++ != 0)
          if (ptr >= table_end)
            return NULL; /* The string isn't terminated. */
        return str;
      }
    else
      while (*ptr++ != 0 && ptr < table_end);

  /* The string number is greater than the number of strings in the set. */
  return NULL;
}

static const char *
holy_smbios_format_uuid (char *buffer, holy_size_t size,
                         const holy_uint8_t *structure, holy_uint8_t offset)
{
  const holy_uint8_t *f = structure + offset; /* little-endian fields */
  const holy_uint8_t *g = f + 8; /* byte-by-byte fields */
  holy_snprintf (buffer, size,
                 "%02x%02x%02x%02x-%02x%02x-%02x%02x-"
                 "%02x%02x-%02x%02x%02x%02x%02x%02x",
                 f[3], f[2], f[1], f[0], f[5], f[4], f[7], f[6],
                 g[0], g[1], g[2], g[3], g[4], g[5], g[6], g[7]);
  return (const char *)buffer;
}


/* List the field formatting functions and the number of bytes they need. */
#define MAXIMUM_FORMAT_LENGTH (sizeof ("ffffffff-ffff-ffff-ffff-ffffffffffff"))
static const struct {
  const char *(*format) (char *buffer, holy_size_t size,
                         const holy_uint8_t *structure, holy_uint8_t offset);
  holy_uint8_t field_length;
} field_extractors[] = {
  {holy_smbios_format_byte, 1},
  {holy_smbios_format_word, 2},
  {holy_smbios_format_dword, 4},
  {holy_smbios_format_qword, 8},
  {holy_smbios_get_string, 1},
  {holy_smbios_format_uuid, 16}
};

/* List command options, with structure field getters ordered as above. */
#define FIRST_GETTER_OPT (3)
#define SETTER_OPT (FIRST_GETTER_OPT + ARRAY_SIZE(field_extractors))
static const struct holy_arg_option options[] = {
  {"type",       't', 0, N_("Match entries with the given type."),
                         N_("type"), ARG_TYPE_INT},
  {"handle",     'h', 0, N_("Match entries with the given handle."),
                         N_("handle"), ARG_TYPE_INT},
  {"match",      'm', 0, N_("Select a structure when several match."),
                         N_("match"), ARG_TYPE_INT},
  {"get-byte",   'b', 0, N_("Get the byte's value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-word",   'w', 0, N_("Get two bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-dword",  'd', 0, N_("Get four bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-qword",  'q', 0, N_("Get eight bytes' value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-string", 's', 0, N_("Get the string specified at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"get-uuid",   'u', 0, N_("Get the UUID's value at the given offset."),
                         N_("offset"), ARG_TYPE_INT},
  {"set",       '\0', 0, N_("Store the value in the given variable name."),
                         N_("variable"), ARG_TYPE_STRING},
  {0, 0, 0, 0, 0, 0}
};


/*
 * Return a matching SMBIOS structure.
 *
 * This method can use up to three criteria for selecting a structure:
 *   - The "type" field                  (use -1 to ignore)
 *   - The "handle" field                (use -1 to ignore)
 *   - Which to return if several match  (use  0 to ignore)
 *
 * The return value is a pointer to the first matching structure.  If no
 * structures match the given parameters, NULL is returned.
 */
static const holy_uint8_t *
holy_smbios_match_structure (const holy_int16_t type,
                             const holy_int32_t handle,
                             const holy_uint16_t match)
{
  const holy_uint8_t *ptr = (const holy_uint8_t *)table_desc.start;
  const holy_uint8_t *table_end = (const holy_uint8_t *)table_desc.end;
  holy_uint16_t structures = table_desc.structures;
  holy_uint16_t structure_count = 0;
  holy_uint16_t matches = 0;

  while (ptr < table_end
         && ptr[1] >= 4 /* Valid structures include the 4-byte header. */
         && (structure_count++ < structures || structures == 0))
    {
      holy_uint16_t structure_handle = holy_get_unaligned16 (ptr + 2);
      holy_uint8_t structure_type = ptr[0];

      if ((handle < 0 || handle == structure_handle)
          && (type < 0 || type == structure_type)
          && (match == 0 || match == ++matches))
        return ptr;

      else
        {
          ptr += ptr[1];
          while ((*ptr++ != 0 || *ptr++ != 0) && ptr < table_end);
        }

      if (structure_type == holy_SMBIOS_TYPE_END_OF_TABLE)
        break;
    }

  return NULL;
}


static holy_err_t
holy_cmd_smbios (holy_extcmd_context_t ctxt,
                 int argc __attribute__ ((unused)),
                 char **argv __attribute__ ((unused)))
{
  struct holy_arg_list *state = ctxt->state;

  holy_int16_t type = -1;
  holy_int32_t handle = -1;
  holy_uint16_t match = 0;
  holy_uint8_t offset = 0;

  const holy_uint8_t *structure;
  const char *value;
  char buffer[MAXIMUM_FORMAT_LENGTH];
  holy_int32_t option;
  holy_int8_t field_type = -1;
  holy_uint8_t i;

  if (table_desc.start == 0)
    return holy_error (holy_ERR_IO,
                       N_("the SMBIOS entry point structure was not found"));

  /* Read the given filtering options. */
  if (state[0].set)
    {
      option = holy_strtol (state[0].arg, NULL, 0);
      if (option < 0 || option > 255)
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("the type must be between 0 and 255"));
      type = (holy_int16_t)option;
    }
  if (state[1].set)
    {
      option = holy_strtol (state[1].arg, NULL, 0);
      if (option < 0 || option > 65535)
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("the handle must be between 0 and 65535"));
      handle = (holy_int32_t)option;
    }
  if (state[2].set)
    {
      option = holy_strtol (state[2].arg, NULL, 0);
      if (option <= 0)
        return holy_error (holy_ERR_BAD_ARGUMENT,
                           N_("the match must be a positive integer"));
      match = (holy_uint16_t)option;
    }

  /* Determine the data type of the structure field to retrieve. */
  for (i = 0; i < ARRAY_SIZE(field_extractors); i++)
    if (state[FIRST_GETTER_OPT + i].set)
      {
        if (field_type >= 0)
          return holy_error (holy_ERR_BAD_ARGUMENT,
                             N_("only one --get option is usable at a time"));
        field_type = i;
      }

  /* Require a choice of a structure field to return. */
  if (field_type < 0)
    return holy_error (holy_ERR_BAD_ARGUMENT,
                       N_("one of the --get options is required"));

  /* Locate a matching SMBIOS structure. */
  structure = holy_smbios_match_structure (type, handle, match);
  if (structure == NULL)
    return holy_error (holy_ERR_IO,
                       N_("no structure matched the given options"));

  /* Ensure the requested byte offset is inside the structure. */
  option = holy_strtol (state[FIRST_GETTER_OPT + field_type].arg, NULL, 0);
  if (option < 0 || option >= structure[1])
    return holy_error (holy_ERR_OUT_OF_RANGE,
                       N_("the given offset is outside the structure"));

  /* Ensure the requested data type at the offset is inside the structure. */
  offset = (holy_uint8_t)option;
  if (offset + field_extractors[field_type].field_length > structure[1])
    return holy_error (holy_ERR_OUT_OF_RANGE,
                       N_("the field ends outside the structure"));

  /* Format the requested structure field into a readable string. */
  value = field_extractors[field_type].format (buffer, sizeof (buffer),
                                               structure, offset);
  if (value == NULL)
    return holy_error (holy_ERR_IO,
                       N_("failed to retrieve the structure field"));

  /* Store or print the formatted value. */
  if (state[SETTER_OPT].set)
    holy_env_set (state[SETTER_OPT].arg, value);
  else
    holy_printf ("%s\n", value);

  return holy_ERR_NONE;
}


static holy_extcmd_t cmd;

holy_MOD_INIT(smbios)
{
  struct holy_smbios_eps3 *eps3;
  struct holy_smbios_eps *eps;

  if ((eps3 = holy_smbios_get_eps3 ()))
    {
      table_desc.start = (holy_addr_t)eps3->table_address;
      table_desc.end = table_desc.start + eps3->maximum_table_length;
      table_desc.structures = 0; /* SMBIOS3 drops the structure count. */
    }
  else if ((eps = holy_smbios_get_eps ()))
    {
      table_desc.start = (holy_addr_t)eps->intermediate.table_address;
      table_desc.end = table_desc.start + eps->intermediate.table_length;
      table_desc.structures = eps->intermediate.structures;
    }

  cmd = holy_register_extcmd ("smbios", holy_cmd_smbios, 0,
                              N_("[-t type] [-h handle] [-m match] "
                                 "(-b|-w|-d|-q|-s|-u) offset "
                                 "[--set variable]"),
                              N_("Retrieve SMBIOS information."), options);
}

holy_MOD_FINI(smbios)
{
  holy_unregister_extcmd (cmd);
}
