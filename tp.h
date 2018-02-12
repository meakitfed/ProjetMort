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
#define YRETURN 14
#define USUB 15
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
#define VYPARAM 36

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
#define UNEXPECTED  100


typedef struct _Instance
{
	char *nom;
	struct _Classe *type;
	int adresse;
}Instance, *InstanceP;


typedef struct _LInstance
{
	struct _Instance *instance;
	struct _LInstance *next;
}LInstance, *LInstanceP;


/* Adapt as needed. Currently it is simply a list of names ! */
typedef struct _varDecl {
  char *nom;
  struct _Classe *type;
  struct _Tree *exprOpt;
  bool *isDefini;
} Param, Champ, VarDecl, *ParamP, *ChampP, *VarDeclP;

typedef struct _LVarDecl
{
    struct _varDecl *var;
    struct _LVarDecl *next;
}LVarDecl, *LVarDeclP, LParam, LChamp, *LParamP, *LChampP;

/* utile pour le check de portee */
typedef struct _Scope {
  int taille;
  struct _LVarDecl *env;
} Scope, *ScopeP;


/* la structure d'un arbre (noeud ou feuille) */
typedef struct _Tree {
  short op;         /* etiquette de l'operateur courant */
  short nbChildren; /* nombre de sous-arbres */
  union {
    char *str;      /* valeur de la feuille si op = Id ou STR */
    int val;        /* valeur de la feuille si op = Cste */
    VarDeclP var;  /* ne pas utiliser tant qu'on n'en a pas besoin :-) */
    struct _Tree **children; /* tableau des sous-arbres */
  } u;
} Tree, *TreeP;


typedef struct _Classe /*represente la meta classe*/
{
  char *nom;
  struct _LVarDecl *lparametres;
  struct _Classe* superClasse;
  struct _Methode *constructeur; 
  struct _LVarDecl *lchamps;
  struct _LMethode *lmethodes;
} Classe, *ClasseP;


typedef struct _LClasse
{
  struct _Classe *classe;
  struct _LClasse *next;
} LClasse, *LClasseP;


typedef struct _Methode
{
  bool override;
  char *nom;                                         
  struct _LVarDecl *lparametres;
  struct _Classe *typeDeRetour;
  struct _Classe *classeAppartenance;
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




/*------------------prototypes------------------*/

void setError(int code);

TreeP makeTree(short op, int nbChildren, ...);
TreeP makeNode(int nbChildren, short op);
TreeP makeLeafStr(short op, char *str);
TreeP makeLeafInt(short op, int val);
TreeP makeLeafLVar(short op, VarDeclP lvar);
TreeP getChild(TreeP tree, int rank);
void setChild(TreeP tree, int rank, TreeP arg);


/*------------------prototypes tp.c------------------*/

VarDeclP makeVarDecl(char *nom, char *type, TreeP exprOpt);
ClasseP makeClasse(char* nom);
ClasseP makeObjet(char *nom);
MethodeP makeMethode(TreeP declMethode, ClasseP classe);
LVarDeclP makeLParam(TreeP arbreLParam);
LVarDeclP makeLParamIsVar(TreeP arbreLParam);
LChampP makeChampsBlocObj(TreeP blocObj);
LMethodeP makeMethodeBlocObj(TreeP blocObj, ClasseP classe);
LVarDeclP dansLeBonOrdre(LVarDeclP liste);

ClasseP getClassePointer(char *nom);
MethodeP getMethodePointer(ClasseP classe, char* nom);

void addChamp(VarDeclP lchamps, ParamP champ);
LVarDeclP addLParamVar(LVarDeclP lchamps, TreeP arbreLParam);
void addClasse(ClasseP classe);
LVarDeclP addVarDecl(LVarDeclP var, LVarDeclP liste);
LMethodeP addMethode(MethodeP methode, LMethodeP liste);
void addConstructeur(TreeP blocOpt, ClasseP classe);

void makeClassesPrimitives();
void initClasse(TreeP arbreLClasse);
void stockerClasse(TreeP arbreLClasse, bool verbose);
void initEnv();

void compile(TreeP arbreLClasse, TreeP main, bool verbose);

void printVarDecl(LVarDeclP lvar);
void printClasse(ClasseP classe);
void printLClasse();
void printObjet();
void printMethode(MethodeP methode);
void printLMethode(LMethodeP lmethode);
void printScope();
void afficherProgramme(TreeP tree, bool verbose);

bool verifContextProg(TreeP arbreLClasse, TreeP main);
bool verifContextMain(TreeP main);
bool verifContextLClasse(TreeP arbreLClasse);


/*------------------prototypes verif.c------------------*/

bool checkDoublonClasse(LClasseP lclasse);
bool checkBoucleHeritage(LClasseP lclasse);

bool checkClassDefine(char* nom);
bool checkPortee(LVarDeclP lvar, char* nom);
bool checkBlocMain(TreeP bloc, ClasseP classe, MethodeP methode);

bool checkExpr(TreeP tree, ClasseP classe, MethodeP methode);
bool checkSelection(TreeP selection, ClasseP classe, MethodeP methode);

ClasseP getType(TreeP expr, ClasseP classe, MethodeP methode);
ClasseP getTypeId(char* nom);
ClasseP getTypeMethode(char* nom, ClasseP classe);

bool setEnvironnementType(LVarDeclP var, ClasseP classe, MethodeP methode);
bool checkBlocClasse(TreeP tree, ClasseP classe, MethodeP methode);

LVarDeclP envHerite(ClasseP classeMere);
bool addEnv(LVarDeclP var, ClasseP classe, int* i);
bool addVarEnv(VarDeclP var, ClasseP classe, int* i);
void removeEnv(int n);

bool checkAff(VarDeclP var, TreeP expr, ClasseP classe, MethodeP methode);
int getTailleListeVarDecl(LVarDeclP liste);
VarDeclP getVarSelection(TreeP Selection, ClasseP classe, MethodeP methode);
bool verifLParam(LVarDeclP lparam);
bool verifVarDeclDefinition(TreeP expr, ClasseP classe, MethodeP methode);

bool checkArguments(LParamP larg, LParamP largbis);
bool CheckArgumentOverride(LParamP nvlarg, LParamP larg);
bool checkOverrideMethode(ClasseP classe, char* nom, LParamP larg, bool isOverride, ClasseP typeDeRetour);
bool checkOverrideLClasse(LClasseP lclasse);
bool checkDoublonClasse(LClasseP lclasse);
bool checkBoucleHeritage(LClasseP lclasse);
bool checkCast(ClasseP classeCast, ClasseP typeId, ClasseP classe);
bool checkMethodes(ClasseP classe, char* nom, TreeP lparam, ClasseP dansclasse, MethodeP dansmethode);
bool checkHeritageClasse(ClasseP classe, char* nom);
bool checkDoublonMethodes(LMethodeP lmethode, char* nomClasse);
bool checkDoublonMethodesLClasse(LClasseP lclasse);

bool addVarEnvSansNum(VarDeclP var, ClasseP classe);

#define YYSTYPE YYSTYPE
#define YYERROR_VERBOSE 1