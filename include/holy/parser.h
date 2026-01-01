/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_PARSER_HEADER
#define holy_PARSER_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/reader.h>

/* All the states for the command line.  */
typedef enum
  {
    holy_PARSER_STATE_TEXT = 1,
    holy_PARSER_STATE_ESC,
    holy_PARSER_STATE_QUOTE,
    holy_PARSER_STATE_DQUOTE,
    holy_PARSER_STATE_VAR,
    holy_PARSER_STATE_VARNAME,
    holy_PARSER_STATE_VARNAME2,
    holy_PARSER_STATE_QVAR,
    holy_PARSER_STATE_QVARNAME,
    holy_PARSER_STATE_QVARNAME2
  } holy_parser_state_t;

/* A single state transition.  */
struct holy_parser_state_transition
{
  /* The state that is looked up.  */
  holy_parser_state_t from_state;

  /* The next state, determined by FROM_STATE and INPUT.  */
  holy_parser_state_t to_state;

  /* The input that will determine the next state from FROM_STATE.  */
  char input;

  /* If set to 1, the input is valid and should be used.  */
  int keep_value;
};

/* Determines the state following STATE, determined by C.  */
holy_parser_state_t
EXPORT_FUNC (holy_parser_cmdline_state) (holy_parser_state_t state,
					 char c, char *result);

holy_err_t
EXPORT_FUNC (holy_parser_split_cmdline) (const char *cmdline,
					 holy_reader_getline_t getline_func,
					 void *getline_func_data,
					 int *argc, char ***argv);

struct holy_parser
{
  /* The next parser.  */
  struct holy_parser *next;

  /* The parser name.  */
  const char *name;

  /* Initialize the parser.  */
  holy_err_t (*init) (void);

  /* Clean up the parser.  */
  holy_err_t (*fini) (void);

  holy_err_t (*parse_line) (char *line,
			    holy_reader_getline_t getline_func,
			    void *getline_func_data);
};
typedef struct holy_parser *holy_parser_t;

holy_err_t holy_parser_execute (char *source);

holy_err_t
holy_rescue_parse_line (char *line,
			holy_reader_getline_t getline_func,
			void *getline_func_data);

#endif /* ! holy_PARSER_HEADER */
