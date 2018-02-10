#ifndef YY_YY_CODE_H_INCLUDED
# define YY_YY_CODE_H_INCLUDED

#include <stdlib.h>





/*void generateur(TreeP tree);*/
void listeSymboles();

int adresse(char *id);

InstanceP makeInstance(VarDeclP var);
void addInstance(InstanceP ins);


void codeLInstr(TreeP tree);
void codeInstr(TreeP tree);
void codeEnvoi(TreeP tree);
void codeConstructeur(TreeP tree);

void codeExpr(TreeP tree);
void codeSelec(TreeP tree);
void codeAff();

void codeBlocObj(TreeP tree);
void codeDeclChampMethode(TreeP tree);
void codeLDeclChamp(TreeP tree);
void codeLDeclChampStructure(LVarDeclP lChamps);
void codeDeclChampStructure(VarDeclP Champ);
void codeObj(ClasseP classe);
void codeDeclChamp(TreeP tree);
void codeDeclMethode(MethodeP methode);
bool codePrint(TreeP expr, TreeP methodeC);

void codeClasse(ClasseP classe);
void genCode(TreeP LClasse, TreeP Bloc);
void codeITE(TreeP tree);

int getTailleListeMethode(LMethodeP liste);
void codeTV();
void codeToString();

VarDeclP getVarDeclFromName(char *name);
MethodeP getMethodeFromName(ClasseP classe, char *name);
int getOffset(ClasseP classe, char *idNom);
InstanceP getInstFromName(char *name);


/* Fonctions servant à la génération de code
 * pour la machine virtuelle
 */
void NOP();

/*ERR str, START, STOP*/
void PUSHI(int i);
void PUSHS(char *c);
void PUSHG(int a);
void PUSHL(int a);

/*PUSHSP, PUSHFP*/
void STOREL(int x);
void STOREG(int x);
void PUSHN(int x);
void POPN(int x);
void DUPN(int x);
void SWAP();
void EQUAL();
void NOT();
void JUMP(char* label);
void JZ(char* label);
void PUSHA(char* a);
void CALL();

void CRETURN();
void CADD();
void CMUL();
void CSUB();
void CDIV();
void CINF();
void CINFEQ();
void CSUP();
void CSUPEQ();
void WRITEI();
void WRITES();
void CCONCAT();
void STORE(int x);
void LOAD(int x);
void ALLOC(int x);












/*OK A PARTIR D'ICI C'EST DE L'EXPERIMENTAL.mp4*/



/**
 *La table des symboles est une structure de données 
 *qui est utilisée tout au long de la compilation. Son 
 *rôle est de mémoriser toutes les informations qu’on
 *peut associer à un identificateur : est-ce un 
 *identificateur de type, de variable, de constante, 
 *de sous-programme ? Si c’est une variable, est-elle
 *locale, globale ? Est-ce un paramètre ? On y stockera aussi
 *la taille en mots mémoire de l’objet référencé par
 *cet identificateur, son adresse de début (ou le méca-
 *nisme d’adressage associé) et toute autre information 
 *qui permettra d’engendrer du code plus facilement. 
 */


typedef struct _unSymbole{

  /*typeIdent type;*/
  
  TreeP tree;

  int adresse;    /*nécessaire ?*/

  struct _elemPile* suiv;
  
  int offset;     /*decalage par rapport a FP*/

} unSymbole, *unSymboleP;


/*contenu d'un elem de la pile
typedef struct _contenuPile{
  
} contenuPile, *contenuPileP;*/




/*conteneur de la pile de symboles*/
typedef struct _Pile{

  int taille;
  unSymboleP sommet;

} Pile, *PileP;



/*
table des symboles :
addSymbole(Ident x)
  declVar symboleX 
  symboleX->name = x.lexval
  symboleX->adresse = offset +1;
  taillePile += 1;
getAddre()
for taille pile 
  if adresse
    return
return adresse de x
*/


#endif /* !YY_YY_CODE_H_INCLUDED */
