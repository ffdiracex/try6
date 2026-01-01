/*
 * Copyright 2025 Felix P. A. Gillberg HolyBooter
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef holy_NORMAL_PARSER_HEADER
#define holy_NORMAL_PARSER_HEADER	1

#include <holy/types.h>
#include <holy/err.h>
#include <holy/parser.h>
#include <holy/command.h>

struct holy_script_mem;

/* The generic header for each scripting command or structure.  */
struct holy_script_cmd
{
  /* This function is called to execute the command.  */
  holy_err_t (*exec) (struct holy_script_cmd *cmd);

  /* The next command.  This can be used by the parent to form a chain
     of commands.  */
  struct holy_script_cmd *next;
};

struct holy_script
{
  unsigned refcnt;
  struct holy_script_mem *mem;
  struct holy_script_cmd *cmd;

  /* holy_scripts from block arguments.  */
  struct holy_script *next_siblings;
  struct holy_script *children;
};

typedef enum
{
  holy_SCRIPT_ARG_TYPE_VAR,
  holy_SCRIPT_ARG_TYPE_TEXT,
  holy_SCRIPT_ARG_TYPE_GETTEXT,
  holy_SCRIPT_ARG_TYPE_DQVAR,
  holy_SCRIPT_ARG_TYPE_DQSTR,
  holy_SCRIPT_ARG_TYPE_SQSTR,
  holy_SCRIPT_ARG_TYPE_BLOCK
} holy_script_arg_type_t;

/* A part of an argument.  */
struct holy_script_arg
{
  holy_script_arg_type_t type;

  char *str;

  /* Parsed block argument.  */
  struct holy_script *script;

  /* Next argument part.  */
  struct holy_script_arg *next;
};

/* An argument vector.  */
struct holy_script_argv
{
  unsigned argc;
  char **args;
  struct holy_script *script;
};

/* Pluggable wildcard translator.  */
struct holy_script_wildcard_translator
{
  holy_err_t (*expand) (const char *str, char ***expansions);
};
extern struct holy_script_wildcard_translator *holy_wildcard_translator;
extern struct holy_script_wildcard_translator holy_filename_translator;

/* A complete argument.  It consists of a list of one or more `struct
   holy_script_arg's.  */
struct holy_script_arglist
{
  struct holy_script_arglist *next;
  struct holy_script_arg *arg;
  /* Only stored in the first link.  */
  int argcount;
};

/* A single command line.  */
struct holy_script_cmdline
{
  struct holy_script_cmd cmd;

  /* The arguments for this command.  */
  struct holy_script_arglist *arglist;
};

/* An if statement.  */
struct holy_script_cmdif
{
  struct holy_script_cmd cmd;

  /* The command used to check if the 'if' is true or false.  */
  struct holy_script_cmd *exec_to_evaluate;

  /* The code executed in case the result of 'if' was true.  */
  struct holy_script_cmd *exec_on_true;

  /* The code executed in case the result of 'if' was false.  */
  struct holy_script_cmd *exec_on_false;
};

/* A for statement.  */
struct holy_script_cmdfor
{
  struct holy_script_cmd cmd;

  /* The name used as looping variable.  */
  struct holy_script_arg *name;

  /* The words loop iterates over.  */
  struct holy_script_arglist *words;

  /* The command list executed in each loop.  */
  struct holy_script_cmd *list;
};

/* A while/until command.  */
struct holy_script_cmdwhile
{
  struct holy_script_cmd cmd;

  /* The command list used as condition.  */
  struct holy_script_cmd *cond;

  /* The command list executed in each loop.  */
  struct holy_script_cmd *list;

  /* The flag to indicate this as "until" loop.  */
  int until;
};

/* State of the lexer as passed to the lexer.  */
struct holy_lexer_param
{
  /* Function used by the lexer to get a new line when more input is
     expected, but not available.  */
  holy_reader_getline_t getline;

  /* Caller-supplied data passed to `getline'.  */
  void *getline_data;

  /* A reference counter.  If this is >0 it means that the parser
     expects more tokens and `getline' should be called to fetch more.
     Otherwise the lexer can stop processing if the current buffer is
     depleted.  */
  int refs;

  /* While walking through the databuffer, `record' the characters to
     this other buffer.  It can be used to edit the menu entry at a
     later moment.  */

  /* If true, recording is enabled.  */
  int record;

  /* Points to the recording.  */
  char *recording;

  /* index in the RECORDING.  */
  int recordpos;

  /* Size of RECORDING.  */
  int recordlen;

  /* End of file reached.  */
  int eof;

  /* Merge multiple word tokens.  */
  int merge_start;
  int merge_end;

  /* Part of a multi-part token.  */
  char *text;
  unsigned used;
  unsigned size;

  /* Type of text.  */
  holy_script_arg_type_t type;

  /* Flag to indicate resplit in progres.  */
  unsigned resplit;

  /* Text that is unput.  */
  char *prefix;

  /* Flex scanner.  */
  void *yyscanner;

  /* Flex scanner buffer.  */
  void *buffer;
};

#define holy_LEXER_INITIAL_TEXT_SIZE   32
#define holy_LEXER_INITIAL_RECORD_SIZE 256

/* State of the parser as passes to the parser.  */
struct holy_parser_param
{
  /* Keep track of the memory allocated for this specific
     function.  */
  struct holy_script_mem *func_mem;

  /* When set to 0, no errors have occurred during parsing.  */
  int err;

  /* The memory that was used while parsing and scanning.  */
  struct holy_script_mem *memused;

  /* The block argument scripts.  */
  struct holy_script *scripts;

  /* The result of the parser.  */
  struct holy_script_cmd *parsed;

  struct holy_lexer_param *lexerstate;
};

void holy_script_init (void);
void holy_script_fini (void);

void holy_script_mem_free (struct holy_script_mem *mem);

void holy_script_argv_free    (struct holy_script_argv *argv);
int holy_script_argv_make     (struct holy_script_argv *argv, int argc, char **args);
int holy_script_argv_next     (struct holy_script_argv *argv);
int holy_script_argv_append   (struct holy_script_argv *argv, const char *s,
			       holy_size_t slen);
int holy_script_argv_split_append (struct holy_script_argv *argv, const char *s);

struct holy_script_arglist *
holy_script_create_arglist (struct holy_parser_param *state);

struct holy_script_arglist *
holy_script_add_arglist (struct holy_parser_param *state,
			 struct holy_script_arglist *list,
			 struct holy_script_arg *arg);
struct holy_script_cmd *
holy_script_create_cmdline (struct holy_parser_param *state,
			    struct holy_script_arglist *arglist);

struct holy_script_cmd *
holy_script_create_cmdif (struct holy_parser_param *state,
			  struct holy_script_cmd *exec_to_evaluate,
			  struct holy_script_cmd *exec_on_true,
			  struct holy_script_cmd *exec_on_false);

struct holy_script_cmd *
holy_script_create_cmdfor (struct holy_parser_param *state,
			   struct holy_script_arg *name,
			   struct holy_script_arglist *words,
			   struct holy_script_cmd *list);

struct holy_script_cmd *
holy_script_create_cmdwhile (struct holy_parser_param *state,
			     struct holy_script_cmd *cond,
			     struct holy_script_cmd *list,
			     int is_an_until_loop);

struct holy_script_cmd *
holy_script_append_cmd (struct holy_parser_param *state,
			struct holy_script_cmd *list,
			struct holy_script_cmd *last);
struct holy_script_arg *
holy_script_arg_add (struct holy_parser_param *state,
		     struct holy_script_arg *arg,
		     holy_script_arg_type_t type, char *str);

struct holy_script *holy_script_parse (char *script,
				       holy_reader_getline_t getline_func,
				       void *getline_func_data);
void holy_script_free (struct holy_script *script);
struct holy_script *holy_script_create (struct holy_script_cmd *cmd,
					struct holy_script_mem *mem);

struct holy_lexer_param *holy_script_lexer_init (struct holy_parser_param *parser,
						 char *script,
						 holy_reader_getline_t getline_func,
						 void *getline_func_data);
void holy_script_lexer_fini (struct holy_lexer_param *);
void holy_script_lexer_ref (struct holy_lexer_param *);
void holy_script_lexer_deref (struct holy_lexer_param *);
unsigned holy_script_lexer_record_start (struct holy_parser_param *);
char *holy_script_lexer_record_stop (struct holy_parser_param *, unsigned);
int  holy_script_lexer_yywrap (struct holy_parser_param *, const char *input);
void holy_script_lexer_record (struct holy_parser_param *, char *);

/* Functions to track allocated memory.  */
struct holy_script_mem *holy_script_mem_record (struct holy_parser_param *state);
struct holy_script_mem *holy_script_mem_record_stop (struct holy_parser_param *state,
						     struct holy_script_mem *restore);
void *holy_script_malloc (struct holy_parser_param *state, holy_size_t size);

/* Functions used by bison.  */
union YYSTYPE;
int holy_script_yylex (union YYSTYPE *, struct holy_parser_param *);
int holy_script_yyparse (struct holy_parser_param *);
void holy_script_yyerror (struct holy_parser_param *, char const *);

/* Commands to execute, don't use these directly.  */
holy_err_t holy_script_execute_cmdline (struct holy_script_cmd *cmd);
holy_err_t holy_script_execute_cmdlist (struct holy_script_cmd *cmd);
holy_err_t holy_script_execute_cmdif (struct holy_script_cmd *cmd);
holy_err_t holy_script_execute_cmdfor (struct holy_script_cmd *cmd);
holy_err_t holy_script_execute_cmdwhile (struct holy_script_cmd *cmd);

/* Execute any holy pre-parsed command or script.  */
holy_err_t holy_script_execute (struct holy_script *script);
holy_err_t holy_script_execute_sourcecode (const char *source);
holy_err_t holy_script_execute_new_scope (const char *source, int argc, char **args);

/* Break command for loops.  */
holy_err_t holy_script_break (holy_command_t cmd, int argc, char *argv[]);

/* SHIFT command for holy script.  */
holy_err_t holy_script_shift (holy_command_t cmd, int argc, char *argv[]);

/* SETPARAMS command for holy script functions.  */
holy_err_t holy_script_setparams (holy_command_t cmd, int argc, char *argv[]);

/* RETURN command for functions.  */
holy_err_t holy_script_return (holy_command_t cmd, int argc, char *argv[]);

/* This variable points to the parsed command.  This is used to
   communicate with the bison code.  */
extern struct holy_script_cmd *holy_script_parsed;



/* The function description.  */
struct holy_script_function
{
  /* The name.  */
  char *name;

  /* The script function.  */
  struct holy_script *func;

  /* The flags.  */
  unsigned flags;

  /* The next element.  */
  struct holy_script_function *next;

  int references;
};
typedef struct holy_script_function *holy_script_function_t;

extern holy_script_function_t holy_script_function_list;

#define FOR_SCRIPT_FUNCTIONS(var) for((var) = holy_script_function_list; \
				      (var); (var) = (var)->next)

holy_script_function_t holy_script_function_create (struct holy_script_arg *functionname,
						    struct holy_script *cmd);
void holy_script_function_remove (const char *name);
holy_script_function_t holy_script_function_find (char *functionname);

holy_err_t holy_script_function_call (holy_script_function_t func,
				      int argc, char **args);

char **
holy_script_execute_arglist_to_argv (struct holy_script_arglist *arglist, int *count);

holy_err_t
holy_normal_parse_line (char *line,
			holy_reader_getline_t getline_func,
			void *getline_func_data);

static inline struct holy_script *
holy_script_ref (struct holy_script *script)
{
  if (script)
    script->refcnt++;
  return script;
}

static inline void
holy_script_unref (struct holy_script *script)
{
  if (! script)
    return;

  if (script->refcnt == 0)
    holy_script_free (script);
  else
    script->refcnt--;
}

#endif /* ! holy_NORMAL_PARSER_HEADER */
