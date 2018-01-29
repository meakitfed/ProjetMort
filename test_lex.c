/*
 * Un petit programme de demonstration qui n'utilise que l'analyse lexicale.
 * Permet principalement de tester la correction de l'analyseur lexical et de
 * l'interface entre celui-ci et le programme qui l'appelle.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"


#define TABSIZE 1000
extern char *strdup(const char *);

/* Fonction appelee par le programme principal pour obtenir l'unite lexicale
 * suivante. Elle est produite par Flex (fichier tp_l.c)
 */
extern int yylex (void);

/* La chaine de caractere en cours de lecture : definie et mise a jour
 * dans tp_l.c
 */
extern char *yytext;

/* Le numero de ligne courant : defini et mis a jour dans tp_l.c */
extern int yylineno;

/* Pour chaque identificateur, memorise la ligne de sa premiere apparition */
typedef struct {
  int line;
  char *id;
} IdentS, *IdentP;

/* (future) table des identificateurs avec leur numero de ligne */
IdentP idents = NIL(IdentS);

/* Variable pour interfacer flex avec le programme qui l'appelle, notamment
 * pour transmettre de l'information, en plus du type d'unite reconnue.
 * Le type YYSTYPE est defini dans tp.h.
 */
YYSTYPE yylval;

bool verbose = FALSE;

/* Compteurs */
int nbIdent = 0;

void setError(int code) {
  /* presente  juste pour des raisons de compatibilite */
}



/* Recherche un identificateur dans la table */
IdentP getIdent(char * id) {
  int i;
  for (i = 0; i < nbIdent; i++) {
    if (!strcmp(id, idents[i].id)) return(&idents[i]);
  }
  return NIL(IdentS);
}

/* Cree un identificateur */
IdentP makeIdent(int line, char *id) {
  IdentP ident = getIdent(id);
  if (!ident) {
    ident = &idents[nbIdent++];
    ident->line = line;
    /* Si on a duplique la chaine dans la partie Flex, il n'y a pas de raison
     * de le faire a nouveau ici.
     * Comme ca risque de dependre de ce que les uns et les autres font dans
     * tp.l, on prefere le (re-)faire ici.
     */
    ident->id = strdup(id);
  }
  return(ident);
}

/* format d'appel */
void help() {
  fprintf(stderr, "Appel: tp [-h] [-v] programme.txt\n");
}

/* Appel:
 *   tp [-option]* programme.txt
 * Les options doivent apparaitre avant le nom du fichier du programme.
 * Options: -[vV] -[hH?]
 */
int main(int argc, char **argv) {
  idents = NEW(TABSIZE, IdentS); /* liste des identificateurs */
  int fi;
  int token;
  int i;

  for(i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'v': case 'V':
        verbose = TRUE; continue;
      case '?': case 'h': case 'H':
  help();
  exit(1);
      default:
  fprintf(stderr, "Option inconnue: %c\n", argv[i][1]);
  exit(1);
      }
    } else break;
  }

  if (i == argc) {
    fprintf(stderr, "Erreur: Fichier programme manquant\n");
    help();
    exit(1);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "Erreur: fichier inaccessible %s\n", argv[i-1]);
    help();
    exit(1);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  while (1) {
    token = yylex();
    switch (token) {

    case 0: /* EOF */
      printf("Fin de l'analyse lexicale\n");
      if (verbose) printf("Liste des identificateurs avec leur premiere occurrence:\n");
      for (i = 0; i < nbIdent; i++) {
        if (verbose) printf("ligne %d : %s\n", idents[i].line, idents[i].id);
      }
      printf("Nombre d'identificateurs: %4d\n", nbIdent);
      return 0;

    case Cste:
      /* ici on suppose qu'on a recupere la valeur de la constante, pas sa
       * representation sous forme de chaine de caracteres.
       */
      if (verbose) printf("Constante:\t\t%d\n", yylval.I);
      break;

    case OBJECT:
      if (verbose) printf("object \n"); 
      break;


    case VAR:
      if (verbose) printf("Var \n"); 
      break;


    case CLASS:
      if (verbose) printf("class \n"); 
      break;

    case IS:
      if (verbose) printf("Is \n"); 
      break;

    case DEF:
      if (verbose) printf("Def \n"); 
      break;

    case EXTENDS :
      if (verbose) printf("Extends \n");
      break;
    case OVERRIDE:
      if (verbose) printf("Override \n");
      break;

    case NEWC:
      if (verbose) printf("Newc \n");
      break;

    case RETURN:
      if (verbose) printf("Return \n");
      break;

      /*Classes predefinies*/
    case INTC:
      if (verbose) printf("Intc \n");
      break;
    case STRINGC:
      if (verbose) printf("Stringc \n");
      break;


    case Id:
      makeIdent(yylineno, yylval.S);
      if (verbose) printf("Identificateur:\t\t%s\n", yylval.S);
      break;

    case Classname:
      makeIdent(yylineno, yylval.S);
      if (verbose) printf("Classname:\t\t%s\n", yylval.S);
      break;


    case VOIDC:
      if (verbose) printf("Voidc \n");
      break;

  
    case AFF:
      if (verbose) printf("Affectation \n"); break;

    case IF:
      if (verbose) printf("IF \n"); break;
    case THEN:
      if (verbose) printf("THEN \n"); break;
    case ELSE:
      if (verbose) printf("ELSE \n"); break;


/*symboles*/
    case '{':
    case '}':
    case '[':
    case ']':
    case '(':
    case ')':
    case ':':
    case '.':
    case ',':
    case '\\':
    case ';':
      if (verbose) printf("Symbole:\t%s\n",  yytext);
      break;


    case ADD:
      if (verbose) printf("addition\n"); break;
    case SUB:
      if (verbose) printf("soustraction\n"); break;
    case MUL:
     if (verbose) printf("multiplication\n"); break;
    case DIV:
     if (verbose) printf("division\n"); break;
    case CONCAT:
     if (verbose) printf("concatenation\n"); break;


    case RelOp:
      /* inutilement complique ici, mais sert a illustrer la difference
       * entre le token et l'infirmation supplementaire qu'on peut associer
       * via la valeur.
       */
      if (verbose) { 
        printf("Operateur de comparaison:\t%s ", yytext);
        switch(yylval.C) {
          case EQ: printf("egalite\n"); break;
          case NE: printf("non egalite\n"); break;
          case INF: printf("inferieur a\n"); break;
          case SUP: printf("superieur a\n"); break;
          default: printf("operateur inconnu de code: %d\n", yylval.C);
        }
      }
      break;


    default:
      printf("############Token non reconnu:\t\"%d\" on ligne %d\n", token, yylineno);
    }
  }
}