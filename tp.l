Chiffre [0-9]
Entier {Chiffre}+
Lower	[a-z]
Upper	[A-Z]
Lettre	[a-zA-Z]
LC 	({Lettre}|{Chiffre})
Delim   [ \b\t\n\r]
Symbole [\():;,.{}]
BS      \\.
PasBSG [^\\"]
Guill		"\""
Star  		"*"
NotStar		[^*]
NotStarPar      [^*/]

%{
#include <string.h>
#include <strings.h>
#include "tp.h"
#include "tp_y.h"

extern char *strdup(const char *);
extern YYSTYPE yylval;
extern int yylineno;
extern int yylex();

int keywordOrId() { 
  if (! strcmp(yytext, "is")) return IS;
  if (! strcmp(yytext, "class")) return CLASS;
  if (! strcmp(yytext, "var")) return VAR;
  if (! strcmp(yytext, "extends")) return EXTENDS;
  if (! strcmp(yytext, "def")) return DEF;
  if (! strcmp(yytext, "override")) return OVERRIDE;
  if (! strcmp(yytext, "if")) return IF;
  if (! strcmp(yytext, "then")) return THEN;
  if (! strcmp(yytext, "else")) return ELSE;
  if (! strcmp(yytext, "new")) return NEWC;
  if (! strcmp(yytext, "return")) return RETURN;
  if (! strcmp(yytext, "object")) return OBJECT;

  yylval.S = strdup(yytext);
  return(Id);
}


int IdClasse() {
	yylval.S = strdup(yytext);
	return(Classname);
}


void lexError(char c) {
  fprintf(stderr, "Error: Skipping invalid character '%c' on line %d\n",
          c, yylineno);
  setError(LEXICAL_ERROR);
}


%}


%%
{Entier} 	{ yylval.I = atoi(yytext); return(Cste); }
{Delim}        { }
{Symbole}	{ return(yytext[0]); }

":="		{ return(AFF); }
"="		{ yylval.C = EQ; return(RelOp); }
"<>"		{ yylval.C = NE; return(RelOp); }
"<="		{ yylval.C = INFE; return(RelOp); }
">="		{ yylval.C = SUPE; return(RelOp); }
">"		{ yylval.C = SUP; return(RelOp); }
"<"		{ yylval.C = INF; return(RelOp); }

"+"    { return(ADD); }
"-"    { return(SUB); }
"*"    { return(MUL); }
"/"    { return(DIV); }
"&"    { return(CONCAT); }

"/*"({NotStar}|({Star}+{NotStarPar}))*{Star}*"*/"  { }
{Lower}{LC}*   { return keywordOrId(); }
{Upper}{LC}*   { return IdClasse(); }
{Guill}({BS}|{PasBSG})*{Guill}   {yylval.S = strdup(yytext); return Chaine;}

. 		         { lexError(yytext[0]); }


%%