%{
#include "parser.h"
#undef YY_INPUT
#define YY_INPUT(b,r,s) readInputForLexer(b,&r,s)
%}
%option noyywrap
%% 
"=="    { return EQ; }
"!="    { return NE; }
"<"     { return LT; }
">"     { return GT; }
">="    { return GE; }
"<="    { return LE; }
"not"     { return NOT; }
"("     { return LPAREN; }
")"     { return RPAREN; }
"or"    { return OR; }
"and"   { return AND; }
"contains" { return CONTAINS; }
"endswith" { return ENDSWITH; }
"path" { return PATH; }
"path" { return PATHONLY; }
"isdir" { return ISDIR; }
"code" { return CODE; }
"htmlmime" { return HTMLMIME; }
"magicmime" { return MAGICMIME; }
"size" { return SIZE; }
"body" { return BODY; }
","    { return COMMA; }
"in"   { return IN; }

L?\"(\\.|[^\\"])*\" { yylval.string=strdup(yytext); return(STRING_LITERAL); }
[0-9]+   { yylval.number = atoi(yytext); return NUMBER_LITERAL; }
