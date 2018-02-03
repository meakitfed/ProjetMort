/* attention: NEW est defini dans tp.h Utilisez un autre nom de token */
%token IS CLASS VAR EXTENDS DEF OVERRIDE RETURN IF THEN ELSE AFF NEWC ADD SUB MUL DIV CONCAT OBJECT
%token<S> Id Classname Chaine
%token<I> Cste
%token<C> RelOp


%nonassoc RelOp
%left ADD SUB 
%left MUL DIV
%left CONCAT
%nonassoc UNARY
%nonassoc '.'


%type <pT> 	ValVar MethodeC
			TypeCOpt Expr LExprOpt LExpr
			ExprOperateur Instr LInstrOpt 
			LInstr Bloc BlocOpt Contenu
			Envoi Selection Prog
			LDeclChamp DeclChamp ExtendsOpt
			LDeclChampMethodeOpt LClassOpt
			LParamOpt Class ObjetIsole 	LDeclChampMethode
			DeclChampMethode BlocObj Param  LParam
			OverrideOpt DeclMethode

%{
#include <stdio.h>
#include "tp.h"
#include "tp_y.h"
#include "code.h"

extern int yylex();
extern void yyerror(const char *); /*const necessaire pour eviter les warning de string literals*/
%}


%%

Prog : LClassOpt Bloc 														{ $$ = makeTree(YPROG, 2, $1, $2);  /*afficherProgramme($$, FALSE);*/ compile($1, $2);/* genCode($1, $2);*/}
;


Class: ObjetIsole															{ $$ = $1; } 
| CLASS Classname '(' LParamOpt ')' ExtendsOpt BlocOpt IS BlocObj			{ $$ =  makeTree(YCLASS, 5, makeLeafStr(Classname, $2), $4, $6, $7, $9); }
;


ObjetIsole : OBJECT Classname IS BlocObj									{ $$ =  makeTree(YOBJ, 2, makeLeafStr(Classname, $2), $4); }
;


LClassOpt: Class LClassOpt 													{ $$ = makeTree(YLCLASS, 2, $1, $2); }
| 																			{ $$ = NIL(Tree); }
;


LParamOpt: LParam 															{ $$ = $1; }
|				  															{ $$ = NIL(Tree); }
;


LParam: Param ',' LParam 													{ $$ = makeTree(YLPARAM, 2, $1, $3); }
| Param            	     													{ $$ = $1; }
;

 
Param: VAR Id ':' Classname ValVar 											{ $$ = makeLeafLVar(YPARAM, makeVarDecl($2, $4, $5)); }
| Id ':' Classname ValVar 													{ $$ = makeLeafLVar(YPARAM, makeVarDecl($1, $3, $4)); }
;


ValVar: AFF Expr 															{ $$ = $2; }
| 																			{ $$ = NIL(Tree); }
;


LDeclChamp: DeclChamp LDeclChamp 											{ $$ = makeTree(LDECLC, 2, $1, $2); }
| DeclChamp 																{ $$ = $1; }
;


DeclChamp: VAR Id ':' Classname ValVar ';' 									{ $$ = makeLeafLVar(YDECLC, makeVarDecl($2, $4, $5)); }
;


DeclMethode : OverrideOpt DEF Id '(' LParamOpt ')' ':' Classname AFF Expr 	{ $$ = makeTree(DMETHODEL, 5, $1, makeLeafStr(Id, $3), $5, makeLeafStr(Classname, $8), $10); }
| OverrideOpt DEF Id '(' LParamOpt ')' TypeCOpt IS Bloc 			  		{ $$ = makeTree(DMETHODE, 5, $1, makeLeafStr(Id, $3), $5, $7, $9); }
;


TypeCOpt: ':' Classname														{ $$ = makeLeafStr(Classname, $2); }
| 																			{ $$ = NIL(Tree); }
;
	

OverrideOpt: OVERRIDE 														{ $$ = makeLeafStr(OVERRIDE, "TRUE"); }
|     				  														{ $$ = makeLeafStr(OVERRIDE, "FALSE"); }
;


ExtendsOpt: EXTENDS Classname '(' LExprOpt ')'  							{ $$ = makeTree(YEXT, 2, makeLeafStr(Classname, $2), $4); }
|																			{ $$ = NIL(Tree); }
;


Expr: Cste 																	{ $$ = makeLeafInt(Cste, $1); }
| Chaine																	{ $$ = makeLeafStr(Chaine, $1); }
| '(' Expr ')'  															{ $$ = $2; }
| '(' Classname Id ')' 														{ $$ = makeTree(ECAST, 2, makeLeafStr(Classname, $2), makeLeafStr(Id, $3)); }	
| Selection    																{ $$ = $1; }
| NEWC Classname '(' LExprOpt ')' 											{ $$ = makeTree(EINST, 2, makeLeafStr(Classname, $2), $4); }    
| Envoi  																	{ $$ = $1; }
| ExprOperateur 															{ $$ = $1; }
| Classname																	{ $$ = makeLeafStr(Classname, $1); }
;


LExprOpt: LExpr 															{ $$ = $1; }
| 																			{ $$ = NIL(Tree); }
;


LExpr: Expr ',' LExpr 														{ $$ = makeTree(YLEXPR, 2, $1, $3); }							
| Expr 			  															{ $$ = $1;  }
;


ExprOperateur: Expr ADD Expr 												{ $$ = makeTree(ADD, 2, $1, $3); }
| Expr SUB Expr 															{ $$ = makeTree(SUB, 2, $1, $3); }
| Expr MUL Expr 															{ $$ = makeTree(MUL, 2, $1, $3); }
| Expr DIV Expr  															{ $$ = makeTree(DIV, 2, $1, $3); }
| SUB Expr %prec UNARY 														{ $$ = makeTree(USUB, 2, makeLeafInt(Cste, 0), $2); }
| ADD Expr %prec UNARY 														{ $$ = makeTree(ADD, 2, makeLeafInt(Cste, 0), $2); }      
| Expr CONCAT Expr 															{ $$ = makeTree(CONCAT, 2, $1, $3); }
| Expr RelOp Expr 															{ $$ = makeTree($2, 2, $1, $3); }
;


Instr : Expr ';' 															{ $$ = makeTree(YEXPR, 1, $1); }
| Bloc  																	{ $$ = $1; }
| RETURN ';' 																{ $$ = makeTree(YRETURN, 0); }
| Selection AFF Expr ';' 													{ $$ = makeTree(EAFF, 2, $1, $3); }
| IF Expr THEN Instr ELSE Instr 											{ $$ = makeTree(YITE, 3, $2, $4, $6); }
;


LInstrOpt: LInstr 															{ $$= $1; }
|  			 																{ $$ = NIL(Tree); }
;


LInstr : Instr LInstr 														{ $$ = makeTree(LINSTR, 2, $1, $2); }
| Instr		 																{ $$ = $1; }
;


BlocOpt: Bloc 																{ $$ =$1; }
| 																			{ $$ = NIL(Tree); }
;


Bloc : '{' Contenu '}'    													{ $$ = $2; }
;


BlocObj: '{' LDeclChampMethodeOpt '}' 										{ $$ = $2;	}
;


LDeclChampMethodeOpt: LDeclChampMethode 									{ $$ = $1;}
| 																			{ $$ = NIL(Tree); }
;


LDeclChampMethode: LDeclChampMethode DeclChampMethode 						{ $$ = makeTree(LDECLMETH, 2, $1, $2); }
| DeclChampMethode 															{ $$ = $1; }
;


DeclChampMethode: DeclChamp 												{ $$ = $1; }
| DeclMethode 																{ $$ = $1; }
;


Contenu : LInstrOpt  														{ $$ = $1; }
| LDeclChamp IS LInstr 														{ $$ = makeTree(YCONT, 2, $1, $3); }
;


Envoi: Expr '.' MethodeC 													{ $$ = makeTree(EENVOI, 2, $1, $3); }
;


MethodeC: Id '(' LExprOpt ')' 												{ $$ = makeTree(METHOD, 2, makeLeafStr(Id, $1), $3); }
;
	

Selection: Expr '.' Id 														{ $$ = makeTree(SELEXPR, 2, $1, makeLeafStr(Id, $3)); }
| Id 				   														{ $$ = makeLeafStr(Id, $1); }
;