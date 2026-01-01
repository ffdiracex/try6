/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#include <config.h>

#include <holy/parser.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/script_sh.h>
#include <holy/i18n.h>

#define yytext_ptr char *
#include "holy_script.tab.h"
#include "holy_script.yy.h"

void
holy_script_lexer_ref (struct holy_lexer_param *state)
{
  state->refs++;
}

void
holy_script_lexer_deref (struct holy_lexer_param *state)
{
  state->refs--;
}

/* Start recording all characters passing through the lexer.  */
unsigned
holy_script_lexer_record_start (struct holy_parser_param *parser)
{
  struct holy_lexer_param *lexer = parser->lexerstate;

  lexer->record++;
  if (lexer->recording)
    return lexer->recordpos;

  lexer->recordpos = 0;
  lexer->recordlen = holy_LEXER_INITIAL_RECORD_SIZE;
  lexer->recording = holy_malloc (lexer->recordlen);
  if (!lexer->recording)
    {
      holy_script_yyerror (parser, 0);
      lexer->recordlen = 0;
    }
  return lexer->recordpos;
}

char *
holy_script_lexer_record_stop (struct holy_parser_param *parser, unsigned offset)
{
  int count;
  char *result;
  struct holy_lexer_param *lexer = parser->lexerstate;

  if (!lexer->record)
    return 0;

  lexer->record--;
  if (!lexer->recording)
    return 0;

  count = lexer->recordpos - offset;
  result = holy_script_malloc (parser, count + 1);
  if (result) {
    holy_strncpy (result, lexer->recording + offset, count);
    result[count] = '\0';
  }

  if (lexer->record == 0)
    {
      holy_free (lexer->recording);
      lexer->recording = 0;
      lexer->recordlen = 0;
      lexer->recordpos = 0;
    }
  return result;
}

/* Record STR if input recording is enabled.  */
void
holy_script_lexer_record (struct holy_parser_param *parser, char *str)
{
  int len;
  char *old;
  struct holy_lexer_param *lexer = parser->lexerstate;

  if (!lexer->record || !lexer->recording)
    return;

  len = holy_strlen (str);
  if (lexer->recordpos + len + 1 > lexer->recordlen)
    {
      old = lexer->recording;
      if (lexer->recordlen < len)
	lexer->recordlen = len;
      lexer->recordlen *= 2;
      lexer->recording = holy_realloc (lexer->recording, lexer->recordlen);
      if (!lexer->recording)
	{
	  holy_free (old);
	  lexer->recordpos = 0;
	  lexer->recordlen = 0;
	  holy_script_yyerror (parser, 0);
	  return;
	}
    }
  holy_strcpy (lexer->recording + lexer->recordpos, str);
  lexer->recordpos += len;
}

/* Read next line of input if necessary, and set yyscanner buffers.  */
int
holy_script_lexer_yywrap (struct holy_parser_param *parserstate,
			  const char *input)
{
  holy_size_t len = 0;
  char *p = 0;
  char *line = 0;
  YY_BUFFER_STATE buffer;
  struct holy_lexer_param *lexerstate = parserstate->lexerstate;

  if (! lexerstate->refs && ! lexerstate->prefix && ! input)
    return 1;

  if (! lexerstate->getline && ! input)
    {
      holy_script_yyerror (parserstate, N_("unexpected end of file"));
      return 1;
    }

  line = 0;
  if (! input)
    lexerstate->getline (&line, 1, lexerstate->getline_data);
  else
    line = holy_strdup (input);

  if (! line)
    {
      holy_script_yyerror (parserstate, N_("out of memory"));
      return 1;
    }

  len = holy_strlen (line);

  /* Ensure '\n' at the end.  */
  if (line[0] == '\0')
    {
      holy_free (line);
      line = holy_strdup ("\n");
      len = 1;
    }
  else if (len && line[len - 1] != '\n')
    {
      p = holy_realloc (line, len + 2);
      if (p)
	{
	  p[len++] = '\n';
	  p[len] = '\0';
	}
      line = p;
    }

  if (! line)
    {
      holy_script_yyerror (parserstate, N_("out of memory"));
      return 1;
    }

  /* Prepend any left over unput-text.  */
  if (lexerstate->prefix)
    {
      int plen = holy_strlen (lexerstate->prefix);

      p = holy_malloc (len + plen + 1);
      if (! p)
	{
	  holy_free (line);
	  return 1;
	}
      holy_strcpy (p, lexerstate->prefix);
      lexerstate->prefix = 0;

      holy_strcpy (p + plen, line);
      holy_free (line);

      line = p;
      len = len + plen;
    }

  buffer = yy_scan_string (line, lexerstate->yyscanner);
  holy_free (line);

  if (! buffer)
    {
      holy_script_yyerror (parserstate, 0);
      return 1;
    }
  return 0;
}

struct holy_lexer_param *
holy_script_lexer_init (struct holy_parser_param *parser, char *script,
			holy_reader_getline_t arg_getline, void *getline_data)
{
  struct holy_lexer_param *lexerstate;

  lexerstate = holy_zalloc (sizeof (*lexerstate));
  if (!lexerstate)
    return 0;

  lexerstate->size = holy_LEXER_INITIAL_TEXT_SIZE;
  lexerstate->text = holy_malloc (lexerstate->size);
  if (!lexerstate->text)
    {
      holy_free (lexerstate);
      return 0;
    }

  lexerstate->getline = arg_getline;
  lexerstate->getline_data = getline_data;
  /* The other elements of lexerstate are all zeros already.  */

  if (yylex_init (&lexerstate->yyscanner))
    {
      holy_free (lexerstate->text);
      holy_free (lexerstate);
      return 0;
    }

  yyset_extra (parser, lexerstate->yyscanner);
  parser->lexerstate = lexerstate;

  if (holy_script_lexer_yywrap (parser, script ?: "\n"))
    {
      parser->lexerstate = 0;
      yylex_destroy (lexerstate->yyscanner);
      holy_free (lexerstate->text);
      holy_free (lexerstate);
      return 0;
    }

  return lexerstate;
}

void
holy_script_lexer_fini (struct holy_lexer_param *lexerstate)
{
  if (!lexerstate)
    return;

  yylex_destroy (lexerstate->yyscanner);

  holy_free (lexerstate->recording);
  holy_free (lexerstate->text);
  holy_free (lexerstate);
}

int
holy_script_yylex (union YYSTYPE *value,
		   struct holy_parser_param *parserstate)
{
  char *str;
  int token;
  holy_script_arg_type_t type;
  struct holy_lexer_param *lexerstate = parserstate->lexerstate;

  value->arg = 0;
  if (parserstate->err)
    return holy_PARSER_TOKEN_BAD;

  if (lexerstate->eof)
    return holy_PARSER_TOKEN_EOF;

  /* 
   * Words with environment variables, like foo${bar}baz needs
   * multiple tokens to be merged into a single holy_script_arg.  We
   * use two variables to achieve this: lexerstate->merge_start and
   * lexerstate->merge_end
   */

  lexerstate->merge_start = 0;
  lexerstate->merge_end = 0;
  do
    {
      /* Empty lexerstate->text.  */
      lexerstate->used = 1;
      lexerstate->text[0] = '\0';

      token = yylex (value, lexerstate->yyscanner);
      if (token == holy_PARSER_TOKEN_BAD)
	break;

      /* Merging feature uses lexerstate->text instead of yytext.  */
      if (lexerstate->merge_start)
	{
	  str = lexerstate->text;
	  type = lexerstate->type;
	}
      else
	{
	  str = yyget_text (lexerstate->yyscanner);
	  type = holy_SCRIPT_ARG_TYPE_TEXT;
	}
      holy_dprintf("lexer", "token %u text [%s]\n", token, str);

      value->arg = holy_script_arg_add (parserstate, value->arg, type, str);
    }
  while (lexerstate->merge_start && !lexerstate->merge_end);

  if (!value->arg || parserstate->err)
    return holy_PARSER_TOKEN_BAD;

  return token;
}

void
holy_script_yyerror (struct holy_parser_param *state, char const *err)
{
  if (err)
    holy_error (holy_ERR_INVALID_COMMAND, err);

  holy_print_error ();
  state->err++;
}
