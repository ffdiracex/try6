%{

#include <holy/parser.h>
#include <holy/misc.h>
#include <holy/mm.h>
#include <holy/script_sh.h>
#include <holy/i18n.h>
#include "holy_script.tab.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"

#define yyalloc(size, scanner)   (holy_malloc((size)))
#define yyfree(ptr, scanner)   (holy_free((ptr)))
#define yyrealloc(ptr, size, scanner) (holy_realloc((ptr), (size)))

/* 
 * As we don't have access to yyscanner, we cannot do much except to
 * print the fatal error.
 */
#define YY_FATAL_ERROR(msg)                     \
  do {                                          \
    holy_printf (_("fatal error: %s\n"), _(msg));     \
  } while (0)

#define COPY(str, hint)                         \
  do {                                          \
    copy_string (yyextra, str, hint);           \
  } while (0)


#define RECORD                                  \
  do {                                          \
    holy_script_lexer_record (yyextra, yytext); \
  } while (0)

#define ARG(t)                        \
  do {                                \
    yyextra->lexerstate->type = t;    \
    return holy_PARSER_TOKEN_WORD;    \
  } while (0)

/* We don't need YY_INPUT, as we rely on yy_scan_strings */
#define YY_INPUT(buf,res,max) do { res = 0; } while (0)

/* forward declarations */
static int holy_lexer_unput (const char *input, yyscan_t yyscanner);
static int holy_lexer_resplit (const char *input, yyscan_t yyscanner);

static void  copy_string (struct holy_parser_param *, const char *,
                          unsigned hint);

%}

%top{

#include <config.h>

#include <sys/types.h>

typedef size_t yy_size_t;
#define YY_TYPEDEF_YY_SIZE_T 1

/* 
 * Some flex hacks for -nostdinc; XXX We need to fix these when libc
 * support becomes availble in holy.
 */

#ifndef holy_UTIL
#define stdin  0
#define stdout 0

#define fprintf(...) 0
#define exit(...) holy_fatal("fatal error in lexer")
#endif

}

%option ecs
%option meta-ecs

%option warn
%option array
%option stack
%option reentrant
%option bison-bridge
%option never-interactive

%option noyyfree noyyalloc noyyrealloc
%option nounistd nostdinit nodefault noyylineno

/* Reduce lexer size, by not defining these.  */
%option noyy_top_state
%option noinput nounput
%option noyyget_in noyyset_in
%option noyyget_out noyyset_out
%option noyyget_debug noyyset_debug
%option noyyget_lineno noyyset_lineno

%option extra-type="struct holy_parser_param*"

BLANK           [ \t]
COMMENT         #.*$

CHAR            [^{}|&$;<> \t\n\'\"\\]
DIGITS          [[:digit:]]+
NAME            [[:alpha:]_][[:alnum:]_]*

ESC             \\(.|\n)
SQCHR           [^\']
DQCHR           {ESC}|[^\\\"]
DQSTR           \"{DQCHR}*\"
I18NSTR         \$\"{DQCHR}*\"
SQSTR           \'{SQCHR}*\'
SPECIAL         \?|\#|\*|\@
VARIABLE        ${NAME}|$\{{NAME}\}|${DIGITS}|$\{{DIGITS}\}|${SPECIAL}|$\{{SPECIAL}\}
WORD            ({CHAR}|{DQSTR}|{SQSTR}|{ESC}|{VARIABLE}|{I18NSTR})+

MULTILINE       {WORD}?((\"{DQCHR}*)|(\$\"{DQCHR}*)|(\'{SQCHR}*))
POS_MULTILINE   {WORD}?\\\n

%x              SPLIT
%x              DQUOTE
%x              I18NQUOTE
%x              SQUOTE
%x              VAR

%%

 /* White spaces */
{BLANK}+        { RECORD; }
{COMMENT}       { RECORD; }

 /* Special symbols */
"\n"            { RECORD; return holy_PARSER_TOKEN_NEWLINE; }
"||"            { RECORD; return holy_PARSER_TOKEN_OR;      }
"&&"            { RECORD; return holy_PARSER_TOKEN_AND;     }
";;"            { RECORD; return holy_PARSER_TOKEN_SEMI2;   }
"|"             { RECORD; return holy_PARSER_TOKEN_PIPE;    }
"&"             { RECORD; return holy_PARSER_TOKEN_AMP;     }
";"             { RECORD; return holy_PARSER_TOKEN_SEMI;    }
"<"             { RECORD; return holy_PARSER_TOKEN_LT;      }
">"             { RECORD; return holy_PARSER_TOKEN_GT;      }

 /* Reserved words */
"{"             { RECORD; return holy_PARSER_TOKEN_LBR;       }
"}"             { RECORD; return holy_PARSER_TOKEN_RBR;       }
"[["            { RECORD; return holy_PARSER_TOKEN_LSQBR2;    }
"]]"            { RECORD; return holy_PARSER_TOKEN_RSQBR2;    }
"case"          { RECORD; return holy_PARSER_TOKEN_CASE;      }
"do"            { RECORD; return holy_PARSER_TOKEN_DO;        }
"done"          { RECORD; return holy_PARSER_TOKEN_DONE;      }
"elif"          { RECORD; return holy_PARSER_TOKEN_ELIF;      }
"else"          { RECORD; return holy_PARSER_TOKEN_ELSE;      }
"esac"          { RECORD; return holy_PARSER_TOKEN_ESAC;      }
"fi"            { RECORD; return holy_PARSER_TOKEN_FI;        }
"for"           { RECORD; return holy_PARSER_TOKEN_FOR;       }
"if"            { RECORD; return holy_PARSER_TOKEN_IF;        }
"in"            { RECORD; return holy_PARSER_TOKEN_IN;        }
"select"        { RECORD; return holy_PARSER_TOKEN_SELECT;    }
"then"          { RECORD; return holy_PARSER_TOKEN_THEN;      }
"until"         { RECORD; return holy_PARSER_TOKEN_UNTIL;     }
"while"         { RECORD; return holy_PARSER_TOKEN_WHILE;     }
"function"      { RECORD; return holy_PARSER_TOKEN_FUNCTION;  }

{MULTILINE}     {
                  if (holy_lexer_unput (yytext, yyscanner))
		    return holy_PARSER_TOKEN_BAD;
		}

{POS_MULTILINE} {
                  if (yyg->yy_c_buf_p + 1 == &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[yyg->yy_n_chars + 1] )
		    {
		      if (holy_lexer_unput (yytext, yyscanner))
			return holy_PARSER_TOKEN_BAD;
		    }
		  else
		    {
		      RECORD;
		      yypush_buffer_state (YY_CURRENT_BUFFER, yyscanner);
		      if (holy_lexer_resplit (yytext, yyscanner))
			{
			  yypop_buffer_state (yyscanner);
			  return holy_PARSER_TOKEN_WORD;
			}
		      yyextra->lexerstate->resplit = 1;
		    }
		}


{NAME}          { RECORD; return holy_PARSER_TOKEN_NAME; }
{WORD}          {
                  RECORD;
		  yypush_buffer_state (YY_CURRENT_BUFFER, yyscanner);
		  if (holy_lexer_resplit (yytext, yyscanner))
		    {
		      yypop_buffer_state (yyscanner);
		      return holy_PARSER_TOKEN_WORD;
		    }
		  yyextra->lexerstate->resplit = 1;
                }
.               {
                  holy_script_yyerror (yyextra, yytext);
		  return holy_PARSER_TOKEN_BAD;
                }

 /* Split word into multiple args */

<SPLIT>{
  \\.           { COPY (yytext, yyleng); }
  \\\n          { /* ignore */ }
  \"            {
                  yy_push_state (DQUOTE, yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_TEXT);
                }
  \'            {
                  yy_push_state (SQUOTE, yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_TEXT);
                }
  "\$\""        {
                  yy_push_state (I18NQUOTE, yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_GETTEXT);
                }
  \$            {
                  yy_push_state (VAR, yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_TEXT);
                }
  \\            |
  [^\"\'\$\\]+  { COPY (yytext, yyleng); }
  <<EOF>>       {
                  yy_pop_state (yyscanner);
                  yypop_buffer_state (yyscanner);
		  yyextra->lexerstate->resplit = 0;
                  yyextra->lexerstate->merge_end = 1;
                  ARG (holy_SCRIPT_ARG_TYPE_TEXT);
                }
}

<VAR>{
  {SPECIAL}     |
  {DIGITS}      |
  {NAME}        {
                  COPY (yytext, yyleng);
                  yy_pop_state (yyscanner);
                  if (YY_START == SPLIT)
                    ARG (holy_SCRIPT_ARG_TYPE_VAR);
                  else
                    ARG (holy_SCRIPT_ARG_TYPE_DQVAR);
                }
  \{{SPECIAL}\} |
  \{{DIGITS}\}  |
  \{{NAME}\}    {
                  yytext[yyleng - 1] = '\0';
                  COPY (yytext + 1, yyleng - 2);
                  yy_pop_state (yyscanner);
                  if (YY_START == SPLIT)
                    ARG (holy_SCRIPT_ARG_TYPE_VAR);
                  else
                    ARG (holy_SCRIPT_ARG_TYPE_DQVAR);
                }
  .|\n          { return holy_PARSER_TOKEN_BAD; }
}

<SQUOTE>{
  \'            {
                  yy_pop_state (yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_SQSTR);
                }
  [^\']+        { COPY (yytext, yyleng); }
}

<DQUOTE>{
  \\\$          { COPY ("$", 1); }
  \\\\          { COPY ("\\", 1); }
  \\\"          { COPY ("\"", 1); }
  \\\n          { /* ignore */ }
  [^\"\$\\\n]+  { COPY (yytext, yyleng); }
  \"            {
                  yy_pop_state (yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_DQSTR);
                }
  \$            {
                  yy_push_state (VAR, yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_DQSTR);
                }
  (.|\n)        { COPY (yytext, yyleng); }
}

<I18NQUOTE>{
  \\\\          { COPY ("\\\\", 2); }
  \\\"          { COPY ("\"", 1); }
  \\\n          { /* ignore */ }
  [^\"\\\n]+    { COPY (yytext, yyleng); }
  \"            {
                  yy_pop_state (yyscanner);
                  ARG (holy_SCRIPT_ARG_TYPE_GETTEXT);
                }
  \\            { COPY ("\\", 1); }
  (.|\n)        { COPY (yytext, yyleng); }
}

<<EOF>>         {
                  yypop_buffer_state (yyscanner);
		  yyextra->lexerstate->eof = 1;
		  return holy_PARSER_TOKEN_EOF;
                }
%%

int
yywrap (yyscan_t yyscanner)
{
  if (yyget_extra (yyscanner)->lexerstate->resplit)
    return 1;

  return holy_script_lexer_yywrap (yyget_extra (yyscanner), 0);
}

static void copy_string (struct holy_parser_param *parser, const char *str, unsigned hint)
{
  holy_size_t size;
  char *ptr;
  unsigned len;

  len = hint ? hint : holy_strlen (str);
  if (parser->lexerstate->used + len >= parser->lexerstate->size)
    {
      size = len * 2;
      if (size < parser->lexerstate->size * 2)
        size = parser->lexerstate->size * 2;
      ptr = holy_realloc (parser->lexerstate->text, size);
      if (!ptr)
        {
          holy_script_yyerror (parser, 0);
          return;
        }

      parser->lexerstate->text = ptr;
      parser->lexerstate->size = size;
    }
  holy_strcpy (parser->lexerstate->text + parser->lexerstate->used - 1, str);
  parser->lexerstate->used += len;
}

static int
holy_lexer_resplit (const char *text, yyscan_t yyscanner)
{
  /* resplit text */
  if (yy_scan_string (text, yyscanner))
    {
      yyget_extra (yyscanner)->lexerstate->merge_start = 1;
      yy_push_state (SPLIT, yyscanner);
      return 0;
    }
  holy_script_yyerror (yyget_extra (yyscanner), 0);
  return 1;
}

static int
holy_lexer_unput (const char *text, yyscan_t yyscanner)
{
  struct holy_lexer_param *lexerstate = yyget_extra (yyscanner)->lexerstate;

  holy_free (lexerstate->prefix);

  lexerstate->prefix = holy_strdup (text);
  if (! lexerstate->prefix)
    {
      holy_script_yyerror (yyget_extra (yyscanner), N_("out of memory"));
      return 1;
    }
  return 0;
}
