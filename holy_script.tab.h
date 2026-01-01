/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_HOLY_SCRIPT_YY_HOLY_SCRIPT_TAB_H_INCLUDED
# define YY_HOLY_SCRIPT_YY_HOLY_SCRIPT_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int holy_script_yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    holy_PARSER_TOKEN_EOF = 0,     /* "end-of-input"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    holy_PARSER_TOKEN_BAD = 258,   /* holy_PARSER_TOKEN_BAD  */
    holy_PARSER_TOKEN_NEWLINE = 259, /* "\n"  */
    holy_PARSER_TOKEN_AND = 260,   /* "&&"  */
    holy_PARSER_TOKEN_OR = 261,    /* "||"  */
    holy_PARSER_TOKEN_SEMI2 = 262, /* ";;"  */
    holy_PARSER_TOKEN_PIPE = 263,  /* "|"  */
    holy_PARSER_TOKEN_AMP = 264,   /* "&"  */
    holy_PARSER_TOKEN_SEMI = 265,  /* ";"  */
    holy_PARSER_TOKEN_LBR = 266,   /* "{"  */
    holy_PARSER_TOKEN_RBR = 267,   /* "}"  */
    holy_PARSER_TOKEN_NOT = 268,   /* "!"  */
    holy_PARSER_TOKEN_LSQBR2 = 269, /* "["  */
    holy_PARSER_TOKEN_RSQBR2 = 270, /* "]"  */
    holy_PARSER_TOKEN_LT = 271,    /* "<"  */
    holy_PARSER_TOKEN_GT = 272,    /* ">"  */
    holy_PARSER_TOKEN_CASE = 273,  /* "case"  */
    holy_PARSER_TOKEN_DO = 274,    /* "do"  */
    holy_PARSER_TOKEN_DONE = 275,  /* "done"  */
    holy_PARSER_TOKEN_ELIF = 276,  /* "elif"  */
    holy_PARSER_TOKEN_ELSE = 277,  /* "else"  */
    holy_PARSER_TOKEN_ESAC = 278,  /* "esac"  */
    holy_PARSER_TOKEN_FI = 279,    /* "fi"  */
    holy_PARSER_TOKEN_FOR = 280,   /* "for"  */
    holy_PARSER_TOKEN_IF = 281,    /* "if"  */
    holy_PARSER_TOKEN_IN = 282,    /* "in"  */
    holy_PARSER_TOKEN_SELECT = 283, /* "select"  */
    holy_PARSER_TOKEN_THEN = 284,  /* "then"  */
    holy_PARSER_TOKEN_UNTIL = 285, /* "until"  */
    holy_PARSER_TOKEN_WHILE = 286, /* "while"  */
    holy_PARSER_TOKEN_FUNCTION = 287, /* "function"  */
    holy_PARSER_TOKEN_NAME = 288,  /* "name"  */
    holy_PARSER_TOKEN_WORD = 289   /* "word"  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 18 "./holy-core/script/parser.y"

  struct holy_script_cmd *cmd;
  struct holy_script_arglist *arglist;
  struct holy_script_arg *arg;
  char *string;
  struct {
    unsigned offset;
    struct holy_script_mem *memory;
    struct holy_script *scripts;
  };

#line 110 "holy_script.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif




int holy_script_yyparse (struct holy_parser_param *state);


#endif /* !YY_HOLY_SCRIPT_YY_HOLY_SCRIPT_TAB_H_INCLUDED  */
