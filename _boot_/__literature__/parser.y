%{
#include <holy/script_sh.h>
#include <holy/mm.h>
#include <holy/misc.h>
#include <holy/i18n.h>

#define YYFREE          holy_free
#define YYMALLOC        holy_malloc
#define YYLTYPE_IS_TRIVIAL      0
#define YYENABLE_NLS    0

#include "holy_script.tab.h"

#pragma GCC diagnostic ignored "-Wmissing-declarations"

%}

%union {
  struct holy_script_cmd *cmd;
  struct holy_script_arglist *arglist;
  struct holy_script_arg *arg;
  char *string;
  struct {
    unsigned offset;
    struct holy_script_mem *memory;
    struct holy_script *scripts;
  };
}

%token holy_PARSER_TOKEN_BAD
%token holy_PARSER_TOKEN_EOF 0 "end-of-input"

%token holy_PARSER_TOKEN_NEWLINE "\n"
%token holy_PARSER_TOKEN_AND     "&&"
%token holy_PARSER_TOKEN_OR      "||"
%token holy_PARSER_TOKEN_SEMI2   ";;"
%token holy_PARSER_TOKEN_PIPE    "|"
%token holy_PARSER_TOKEN_AMP     "&"
%token holy_PARSER_TOKEN_SEMI    ";"
%token holy_PARSER_TOKEN_LBR     "{"
%token holy_PARSER_TOKEN_RBR     "}"
%token holy_PARSER_TOKEN_NOT     "!"
%token holy_PARSER_TOKEN_LSQBR2  "["
%token holy_PARSER_TOKEN_RSQBR2  "]"
%token holy_PARSER_TOKEN_LT      "<"
%token holy_PARSER_TOKEN_GT      ">"

%token <arg> holy_PARSER_TOKEN_CASE      "case"
%token <arg> holy_PARSER_TOKEN_DO        "do"
%token <arg> holy_PARSER_TOKEN_DONE      "done"
%token <arg> holy_PARSER_TOKEN_ELIF      "elif"
%token <arg> holy_PARSER_TOKEN_ELSE      "else"
%token <arg> holy_PARSER_TOKEN_ESAC      "esac"
%token <arg> holy_PARSER_TOKEN_FI        "fi"
%token <arg> holy_PARSER_TOKEN_FOR       "for"
%token <arg> holy_PARSER_TOKEN_IF        "if"
%token <arg> holy_PARSER_TOKEN_IN        "in"
%token <arg> holy_PARSER_TOKEN_SELECT    "select"
%token <arg> holy_PARSER_TOKEN_THEN      "then"
%token <arg> holy_PARSER_TOKEN_UNTIL     "until"
%token <arg> holy_PARSER_TOKEN_WHILE     "while"
%token <arg> holy_PARSER_TOKEN_FUNCTION  "function"
%token <arg> holy_PARSER_TOKEN_NAME      "name"
%token <arg> holy_PARSER_TOKEN_WORD      "word"

%type <arg> block block0
%type <arglist> word argument arguments0 arguments1

%type <cmd> script_init script
%type <cmd> holycmd ifclause ifcmd forcmd whilecmd untilcmd
%type <cmd> command commands1 statement

%pure-parser
%lex-param   { struct holy_parser_param *state };
%parse-param { struct holy_parser_param *state };

%start script_init

%%
/* It should be possible to do this in a clean way...  */
script_init: { state->err = 0; } script { state->parsed = $2; state->err = 0; }
;

script: newlines0
        {
          $$ = 0;
        }
      | script statement delimiter newlines0
        {
          $$ = holy_script_append_cmd (state, $1, $2);
        }
      | error
        {
          $$ = 0;
          yyerror (state, N_("Incorrect command"));
          yyerrok;
        }
;

newlines0: /* Empty */ | newlines1 ;
newlines1: newlines0 "\n" ;

delimiter: ";"
         | "\n"
;
delimiters0: /* Empty */ | delimiters1 ;
delimiters1: delimiter
          | delimiters1 "\n"
;

word: holy_PARSER_TOKEN_NAME { $$ = holy_script_add_arglist (state, 0, $1); }
    | holy_PARSER_TOKEN_WORD { $$ = holy_script_add_arglist (state, 0, $1); }
;

statement: command   { $$ = $1; }
         | function  { $$ = 0;  }
;

argument : "case"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "do"        { $$ = holy_script_add_arglist (state, 0, $1); }
         | "done"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "elif"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "else"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "esac"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "fi"        { $$ = holy_script_add_arglist (state, 0, $1); }
         | "for"       { $$ = holy_script_add_arglist (state, 0, $1); }
         | "if"        { $$ = holy_script_add_arglist (state, 0, $1); }
         | "in"        { $$ = holy_script_add_arglist (state, 0, $1); }
         | "select"    { $$ = holy_script_add_arglist (state, 0, $1); }
         | "then"      { $$ = holy_script_add_arglist (state, 0, $1); }
         | "until"     { $$ = holy_script_add_arglist (state, 0, $1); }
         | "while"     { $$ = holy_script_add_arglist (state, 0, $1); }
         | "function"  { $$ = holy_script_add_arglist (state, 0, $1); }
         | word { $$ = $1; }
;

/*
  Block parameter is passed to commands in two forms: as unparsed
  string and as pre-parsed holy_script object.  Passing as holy_script
  object makes memory management difficult, because:

  (1) Command may want to keep a reference to holy_script objects for
      later use, so script framework may not free the holy_script
      object after command completes.

  (2) Command may get called multiple times with same holy_script
      object under loops, so we should not let command implementation
      to free the holy_script object.

  To solve above problems, we rely on reference counting for
  holy_script objects.  Commands that want to keep the holy_script
  object must take a reference to it.

  Other complexity comes with arbitrary nesting of holy_script
  objects: a holy_script object may have commands with several block
  parameters, and each block parameter may further contain multiple
  block parameters nested.  We use temporary variable, state->scripts
  to collect nested child scripts (that are linked by siblings and
  children members), and will build holy_scripts tree from bottom.
 */
block: "{"
       {
         holy_script_lexer_ref (state->lexerstate);
         $<offset>$ = holy_script_lexer_record_start (state);
	 $<memory>$ = holy_script_mem_record (state);

	 /* save currently known scripts.  */
	 $<scripts>$ = state->scripts;
	 state->scripts = 0;
       }
       commands1 delimiters0 "}"
       {
         char *p;
	 struct holy_script_mem *memory;
	 struct holy_script *s = $<scripts>2;

	 memory = holy_script_mem_record_stop (state, $<memory>2);
         if ((p = holy_script_lexer_record_stop (state, $<offset>2)))
	   *holy_strrchr (p, '}') = '\0';

	 $$ = holy_script_arg_add (state, 0, holy_SCRIPT_ARG_TYPE_BLOCK, p);
	 if (! $$ || ! ($$->script = holy_script_create ($3, memory)))
	   holy_script_mem_free (memory);

	 else {
	   /* attach nested scripts to $$->script as children */
	   $$->script->children = state->scripts;

	   /* restore old scripts; append $$->script to siblings. */
	   state->scripts = $<scripts>2 ?: $$->script;
	   if (s) {
	     while (s->next_siblings)
	       s = s->next_siblings;
	     s->next_siblings = $$->script;
	   }
	 }

         holy_script_lexer_deref (state->lexerstate);
       }
;
block0: /* Empty */ { $$ = 0; }
      | block { $$ = $1; }
;

arguments0: /* Empty */ { $$ = 0; }
          | arguments1  { $$ = $1; }
;
arguments1: argument arguments0
            {
	      if ($1 && $2)
		{
		  $1->next = $2;
		  $1->argcount += $2->argcount;
		  $2->argcount = 0;
		}
	      $$ = $1;
            }
;

holycmd: word arguments0 block0
         {
	   struct holy_script_arglist *x = $2;

	   if ($3)
	     x = holy_script_add_arglist (state, $2, $3);

           if ($1 && x) {
             $1->next = x;
             $1->argcount += x->argcount;
             x->argcount = 0;
           }
           $$ = holy_script_create_cmdline (state, $1);
         }
;

/* A single command.  */
command: holycmd  { $$ = $1; }
       | ifcmd    { $$ = $1; }
       | forcmd   { $$ = $1; }
       | whilecmd { $$ = $1; }
       | untilcmd { $$ = $1; }
;

/* A list of commands. */
commands1: newlines0 command
           {
             $$ = holy_script_append_cmd (state, 0, $2);
           }
         | commands1 delimiters1 command
           {
	     $$ = holy_script_append_cmd (state, $1, $3);
           }
;

function: "function" "name"
          {
            holy_script_lexer_ref (state->lexerstate);
            state->func_mem = holy_script_mem_record (state);

	    $<scripts>$ = state->scripts;
	    state->scripts = 0;
          }
          delimiters0 "{" commands1 delimiters1 "}"
          {
            struct holy_script *script;
            state->func_mem = holy_script_mem_record_stop (state,
                                                           state->func_mem);
            script = holy_script_create ($6, state->func_mem);
            if (! script)
	      holy_script_mem_free (state->func_mem);
	    else {
	      script->children = state->scripts;
	      holy_script_function_create ($2, script);
	    }

	    state->scripts = $<scripts>3;
            holy_script_lexer_deref (state->lexerstate);
          }
;

ifcmd: "if"
	{
	  holy_script_lexer_ref (state->lexerstate);
	}
	ifclause "fi"
	{
	  $$ = $3;
	  holy_script_lexer_deref (state->lexerstate);
	}
;
ifclause: commands1 delimiters1 "then" commands1 delimiters1
	  {
	    $$ = holy_script_create_cmdif (state, $1, $4, 0);
	  }
	| commands1 delimiters1 "then" commands1 delimiters1 "else" commands1 delimiters1
	  {
	    $$ = holy_script_create_cmdif (state, $1, $4, $7);
	  }
	| commands1 delimiters1 "then" commands1 delimiters1 "elif" ifclause
	  {
	    $$ = holy_script_create_cmdif (state, $1, $4, $7);
	  }
;

forcmd: "for" "name"
        {
	  holy_script_lexer_ref (state->lexerstate);
        }
        "in" arguments0 delimiters1 "do" commands1 delimiters1 "done"
	{
	  $$ = holy_script_create_cmdfor (state, $2, $5, $8);
	  holy_script_lexer_deref (state->lexerstate);
	}
;

whilecmd: "while"
          {
	    holy_script_lexer_ref (state->lexerstate);
          }
          commands1 delimiters1 "do" commands1 delimiters1 "done"
	  {
	    $$ = holy_script_create_cmdwhile (state, $3, $6, 0);
	    holy_script_lexer_deref (state->lexerstate);
	  }
;

untilcmd: "until"
          {
	    holy_script_lexer_ref (state->lexerstate);
          }
          commands1 delimiters1 "do" commands1 delimiters1 "done"
	  {
	    $$ = holy_script_create_cmdwhile (state, $3, $6, 1);
	    holy_script_lexer_deref (state->lexerstate);
	  }
;
