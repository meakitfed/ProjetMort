#include <stdlib.h>

#define TRUE 1
#define FALSE 0

typedef unsigned char bool;

/* deux macros pour eviter de faire des malloc en se trompant de type : on
 * s'oblige a expliciter le type d'objet a allouer et on récupere une
 * de type pointeur sur le type alloue.
 * Exemple: Allouer 3 objets de la structure Tree (voir ci-dessous)
 * TreeP p := NEW(3, Tree);
 *
 * De meme, sobloiger à mettre un type au NIL ci-dessous permet souvent
 * d'eviter de confondre entre des pointeurs de type differents.
 */
#define NEW(howmany, type) (type *) calloc((unsigned) howmany, sizeof(type))
#define NIL(type) (type *) 0

/* Etiquettes additionnelles pour les arbres de syntaxe abstraite.
 * Certains tokens servent directement d'etiquette. Attention ici a ne pas
 * donner des valeurs identiques a celles des tokens.
 *
 * Il y a surement des choses a recuperer en plus de ce que vous avez
 * produit pour le tp.
 */
#define NE  1
#define EQ  2
#define INF 3
#define INFE  4
#define SUP 5
#define SUPE  6
#define YPROG 7 
#define YCONT 8
#define LDECLC 9
#define YDECLC 10
#define LINSTR 11
#define EAFF 12
#define YITE 13

#define USUB 14
#define YRETURN 15

#define ECAST 16
#define YSELEC 17
#define EINST 18
#define SELEXPR 21
#define YEXPR 23
#define YLEXPR 24
#define YEXT 25
#define EENVOI 26
#define METHOD 27
#define YLCLASS 28
#define YCLASS 29
#define LDECLMETH 30
#define DMETHODE 31
#define YLPARAM 32
#define YPARAM 33
#define YOBJ 34
#define DMETHODEL 35


/* Codes d'erreurs. Cette liste n'est pas obligatoire ni limitative */
#define NO_ERROR  0
#define USAGE_ERROR 1
#define LEXICAL_ERROR 2
#define SYNTAX_ERROR    3
#define CONTEXT_ERROR 40  /* default value for this stage */
#define DECL_ERROR  41  /* scope problem */
#define TYPE_ERROR  42  /* type checking problem */
#define OTHER_CONTEXTUAL_ERROR  43 /* replace by something more precise */
#define EVAL_ERROR  50  /* dubious when writing a compiler ! */
#define UNEXPECTED  10O



/* Adapt as needed. Currently it is simply a list of names ! */
typedef struct _varDecl {
  char *nom;
  struct _Classe *type;
  struct _Tree *exprOpt;
  struct _varDecl *next;
} Param, Champ, VarDecl, *ParamP, *ChampP, *VarDeclP;


/* la structure d'un arbre (noeud ou feuille) */
typedef struct _Tree {
  short op;         /* etiquette de l'operateur courant */
  short nbChildren; /* nombre de sous-arbres */
  union {
    char *str;      /* valeur de la feuille si op = Id ou STR */
    int val;        /* valeur de la feuille si op = Cste */
    VarDeclP lvar;  /* ne pas utiliser tant qu'on n'en a pas besoin :-) */
    struct _Tree **children; /* tableau des sous-arbres */
  } u;
} Tree, *TreeP;


typedef struct _Classe /*represente la meta classe*/
{
  char *nom;
  struct _varDecl *lparametres;
  struct _Classe* superClasse;
  /* struct _Methode *constructeur; */ 
  struct _varDecl *constructeur;							/* TODO Je suis pas sur de ca*/
  struct _varDecl *lchamps;
  struct _LMethode *lmethodes;
} Classe, *ClasseP;


typedef struct _LClasse
{
  struct _Classe *classe;
  struct _LClasse *next;
} LClasse, *LClasseP;


typedef struct _Objet
{
	char *nom;
	struct _varDecl *lchamps;
  	struct _LMethode *lmethodes;
  	struct _Objet *next;
} Objet, *ObjetP;


typedef struct _Methode
{
	bool override;
	char *nom;
	struct _varDecl *lparametres;
  	struct _Classe *typeDeRetour;
  	struct _Tree *bloc;
} Methode, *MethodeP;


typedef struct _LMethode
{
  struct _Methode* methode;
  struct _LMethode* next;
} LMethode, *LMethodeP;


typedef union 
{ 

  char *S;
  char C;
  int I;
  TreeP pT;

} YYSTYPE; 


/*------------------protype------------------*/

void setError(int code);
/* void yyerror(char *ignore); */

TreeP makeTree(short op, int nbChildren, ...);
TreeP makeNode(int nbChildren, short op);
TreeP makeLeafStr(short op, char *str);
TreeP makeLeafInt(short op, int val);
TreeP makeLeafLVar(short op, VarDeclP lvar);
TreeP getChild(TreeP tree, int rank);
void setChild(TreeP tree, int rank, TreeP arg);


/*------------------protype perso------------------*/

VarDeclP makeVarDecl(char *nom, TreeP type, TreeP exprOpt);
ClasseP makeClasse(char* nom);
ObjetP makeObjet(char *nom);
MethodeP makeMethode(TreeP declMethode);
VarDeclP makeLParam(TreeP arbreLParam);
ChampP makeChampsBlocObj(TreeP blocObj);
LMethodeP makeMethodeBlocObj(TreeP blocObj);
VarDeclP makeVarBloc(TreeP bloc);

ClasseP getClassePointer(char *nom);
ObjetP getObjetPointer(char *nom);

void addClasse(ClasseP classe);
void addObjet(ObjetP objet);
void addEnv(VarDeclP var);
void addVarDecl(VarDeclP var, VarDeclP liste);
LMethodeP addMethode(MethodeP methode, LMethodeP liste);

void makeClassesPrimitives();
void initClasse(TreeP arbreLClasse);
void stockerClasse(TreeP arbreLClasse, bool verbose);
void stockerEnv(TreeP arbreMain, bool verbose);

void compile(TreeP arbreLClasse, TreeP main);

void printVarDecl(VarDeclP lvar);
void printClasse(ClasseP classe);
void printLClasse();
void printObjet();
void printMethode(MethodeP methode);
void printLMethode(LMethodeP lmethode);
void afficherProgramme(TreeP tree, bool verbose);


#define YYSTYPE YYSTYPE
#define YYERROR_VERBOSE 1