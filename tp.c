#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "tp.h"
#include "tp_y.h"
#include <assert.h>

extern int yyparse();
extern int yylineno;

/* Peut servir a controloer le niveau de 'verbosite'.
 * Par defaut, n'imprime que le resultat et les messages d'erreur
 */
bool verbose = TRUE;

/* Peut servir a controler la generation de code. Par defaut, on produit le code
 * On pourrait avoir un flag similaire pour s'arreter avant les verifications
 * contextuelles (certaines ou toutes...)
 */
bool noCode = FALSE;

/* Pour controler la pose de points d'arret ou pas dans le code produit */
bool debug = FALSE;

/* code d'erreur a retourner */
int errorCode = NO_ERROR; /* defini dans tp.h */

FILE *out; /* fichier de sortie pour le code engendre */


/*---------------------------VARIABLE GLOBALE---------------------------*/

LClasseP lclasse = NIL(LClasse);

ScopeP env = NIL(Scope);
int nbErreur = 0;



int main(int argc, char **argv) {
  int fi;
  int i, res;

  out = stdout;
  for(i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch (argv[i][1]) {
      case 'd': case 'D':
  debug = TRUE; continue;
      case 'v': case 'V':
  verbose = TRUE; continue;
      case 'e': case 'E':
  noCode = TRUE; continue;
      case '?': case 'h': case 'H':
  fprintf(stderr, "Appel: tp -v -e -d -o file.out programme.txt\n");
  exit(USAGE_ERROR);
      case'o':
    if ((out= fopen(argv[++i], "w")) == NULL) {
      fprintf(stderr, "erreur: Cannot open %s\n", argv[i]);
      exit(USAGE_ERROR);
    }
  break;
      default:
  fprintf(stderr, "Option inconnue: %c\n", argv[i][1]);
  exit(USAGE_ERROR);
      }
    } else break;
  }

  if (i == argc) {
    fprintf(stderr, "Fichier programme manquant\n");
    exit(USAGE_ERROR);
  }

  if ((fi = open(argv[i++], O_RDONLY)) == -1) {
    fprintf(stderr, "erreur: Cannot open %s\n", argv[i-1]);
    exit(USAGE_ERROR);
  }

  /* redirige l'entree standard sur le fichier... */
  close(0); dup(fi); close(fi);

  res = yyparse();
  /* si yyparse renvoie 0, le programme en entree etait syntaxiquement correct.
   * Le plus simple est que les verifications contextuelles et la generation
   * de copde soient lancees par les actions associees a la regle de grammaire
   * pour l'axiome. Dans ce cas, quand yyparse renvoie sa valeur on n'a plus
   * rien a faire, sauf fermer les fichiers qui doivent l'etre.
   * Si yyparse renvoie autre chose que 0 c'est que le programme avait une
   * erreur lexicale ou syntaxique
   */
  if (out != NIL(FILE) && out != stdout) fclose(out);
  return res ? SYNTAX_ERROR : errorCode;
}


void setError(int code) {
  errorCode = code;
  if (code != NO_ERROR) {
    noCode = TRUE;
    /* la ligne suivante peut servir a "planter" volontairement le programme
     * des qu'une de vos fonctions detectent une erreur et appelle setError.
     * Si vous executez le rpogramme sous le debuggeur vous aurez donc la main
     * et pourrez examiner la pile d'execution.
     */
    abort(); 
  }
}


/* yyerror:  fonction importee par Bison et a fournir explicitement. Elle
 * est appelee quand Bison detecte une erreur syntaxique.
 * Ici on se contente d'un message a minima.
 */
void yyerror(char *ignore) {
  printf("erreur de syntaxe: Ligne %d\n", yylineno);
  setError(SYNTAX_ERROR);
}



/* ****** Fonctions pour la construction d'AST   ********************
 *
 * Ajoutez vos propres constructeurs, si besoin
 *
 */

/* Tronc commun pour la construction d'arbre. Normalement on ne l'appelle
 * pas directement. Elle ne fait qu'allouer, sans remplir les champs
 */
TreeP makeNode(int nbChildren, short op) {
  TreeP tree = NEW(1, Tree);
  tree->op = op;
  tree->nbChildren = nbChildren;
  tree->u.children = nbChildren > 0 ? NEW(nbChildren, TreeP) : NIL(TreeP);
  return(tree);
}


/* Construction d'un arbre a nbChildren branches, passees en parametres.
 * Pour comprendre en detail (si necessaire), regardez un tutorial sur
 * comment on passe un nombre variable d'arguments à une fonction et
 * comment on recupere chacun de ces arguments.
 * cf va_list, va_start, va_arg et va_end.
 * makeTree prend donc toujours au moins 2 arguments
 */
TreeP makeTree(short op, int nbChildren, ...) {
  va_list args;
  int i;
  TreeP tree = makeNode(nbChildren, op);
  va_start(args, nbChildren);
  for (i = 0; i < nbChildren; i++) {
    tree->u.children[i] = va_arg(args, TreeP);
  }
  va_end(args);
  return(tree);
}


/* Retourne le rank-ieme fils d'un arbre (de 0 a n-1) */
TreeP getChild(TreeP tree, int rank) {
  if (tree->nbChildren < rank -1) {
    fprintf(stderr, "Incorrect rank in getChild: %d\n", rank);
    abort(); /* plante le programme en cas de rang incorrect */
  }
  return tree->u.children[rank];
}


void setChild(TreeP tree, int rank, TreeP arg) {
  if (tree->nbChildren < rank -1) {
    fprintf(stderr, "Incorrect rank in getChild: %d\n", rank);
    abort(); /* plante le programme en cas de rang incorrect */
  }
  tree->u.children[rank] = arg;
}


/* Constructeur de feuille dont la valeur est une chaine de caracteres */
TreeP makeLeafStr(short op, char *str) {
  TreeP tree = makeNode(0, op);
  tree->u.str = str;
  return tree;
}


/* Constructeur de feuille dont la valeur est un entier */
TreeP makeLeafInt(short op, int val) {
  TreeP tree = makeNode(0, op);
  tree->u.val = val;
  return(tree);
}


/* Constructeur de feuille dont la valeur est une declaration */
TreeP makeLeafLVar(short op, VarDeclP var) {
  TreeP tree = makeNode(0, op);
  tree->u.var = var;
  return(tree);
}


/*-------------------------MAKEPERSO-------------------------*/

/* Creer une variable à partir des elements de la grammaire
*  Cette fonction est appele dans la grammaire pour creer une feuille LVar */
VarDeclP makeVarDecl(char *nom, char *type, TreeP exprOpt)
{
    VarDeclP newVar = NEW(1, VarDecl);

    newVar->nom = nom;
    newVar->exprOpt = exprOpt;

    ClasseP classeType = getClassePointer(type);
    if(classeType != NIL(Classe))
    {
        newVar->type = classeType;
    }
    else
    {
        classeType = makeClasse(type);
        newVar->type = classeType;
    }

    return newVar;
}


/* Creer une classe sans aucun element initialise sauf le nom */
ClasseP makeClasse(char *nom)
{
  ClasseP classe = NEW(1, Classe);
  
  classe->nom = nom;

  classe->lparametres = NEW(1, LVarDecl);
  classe->lparametres->var = NEW(1, VarDecl);
  classe->lparametres->var->nom = "NIL";
  classe->lparametres->var->type = NIL(Classe);
  classe->lparametres->var->exprOpt = NIL(Tree);
  classe->lparametres->next = NIL(LVarDecl);

  classe->superClasse = NIL(Classe);

  classe->constructeur = NIL(Methode);

  classe->lchamps = NEW(1, LVarDecl);
  classe->lchamps->var = NEW(1, VarDecl);
  classe->lchamps->var->nom = "NIL";
  classe->lchamps->var->type = NIL(Classe);
  classe->lchamps->var->exprOpt = NIL(Tree);
  classe->lchamps->next = NIL(LVarDecl);
 
  classe->lmethodes = NIL(LMethode);

  return classe;
}


/* Creer un objet isole sans aucun element initialise sauf le nom */
ClasseP makeObjet(char *nom)
{
  ClasseP objet = NEW(1, Classe);
  
  objet->nom = nom;

  objet->lparametres = NEW(1, LVarDecl);
  objet->lparametres->var = NIL(VarDecl);

  objet->superClasse = NIL(Classe);

  objet->constructeur = NIL(Methode);

  objet->lchamps = NEW(1, LVarDecl);
  objet->lchamps->var = NEW(1, VarDecl);
  objet->lchamps->var->nom = "NIL";
  objet->lchamps->var->type = NIL(Classe);
  objet->lchamps->var->exprOpt = NIL(Tree);
  objet->lchamps->next = NIL(LVarDecl);
 
  objet->lmethodes = NIL(LMethode);

  return objet;
}


/* Creer une methode a partir d'un arbre */
MethodeP makeMethode(TreeP declMethode, ClasseP classe)
{
    int *i = NEW(1, int);
    *i = 0;
    MethodeP newMethode = NEW(1, Methode);

    /* liste de parametres */
    TreeP arbreParamMeth = getChild(declMethode, 2);
    LParamP lparametres = makeLParam(arbreParamMeth, i);
    newMethode->lparametres = lparametres;

    /* pointeur vers le type de retour */
    char *typeDeRetour = "Void";
    if(declMethode->op == DMETHODEL)
    {
        typeDeRetour = getChild(declMethode, 3)->u.str;
    }
    else
    {
        TreeP typeOpt = getChild(declMethode, 3);
        if(typeOpt != NIL(Tree))
            typeDeRetour = typeOpt->u.str;
    }

    ClasseP classeType = getClassePointer(typeDeRetour);
    if(classeType != NIL(Classe))
    {
        newMethode->typeDeRetour = classeType;
    }
    else
    {
        classeType = makeClasse(typeDeRetour);
        newMethode->typeDeRetour = classeType;
    }

    /* override */
    newMethode->override = (strcmp(getChild(declMethode, 0)->u.str, "TRUE") == 0) ? TRUE : FALSE;

    /* nom de la methode */
    newMethode->nom = getChild(declMethode, 1)->u.str;

    /* classe d'appartenance */
    newMethode->classeAppartenance = classe;

    /* expression optionnelle */
    newMethode->bloc = getChild(declMethode, 4);

    return newMethode;
}


/* retourne une liste de parametres a partir d'un arbre */
LVarDeclP makeLParam(TreeP arbreLParam, int *i)
{
  LParamP lparam = NIL(LVarDecl);

  if(arbreLParam != NIL(Tree))
  {

      while(arbreLParam->op == YLPARAM || arbreLParam->op == LDECLC)
      {
          *i = *i + 1; 
          ParamP tmp = getChild(arbreLParam, 0)->u.var;

          LParamP tmpListe = NEW(1, LParam);
          tmpListe->var = tmp;
          tmpListe->next = NIL(LParam);

          lparam = addVarDecl(tmpListe, lparam);

          arbreLParam = getChild(arbreLParam, 1);
      }

      *i = *i + 1;           
      ParamP tmp = arbreLParam->u.var;

      LParamP tmpListe = NEW(1,LParam);
      tmpListe->var = tmp;
      tmpListe->next = NIL(LParam);

      lparam = addVarDecl(tmpListe, lparam);
  }

  return lparam;
}


/* retourne une liste des champs d'un objet ou d'une classe */
LChampP makeChampsBlocObj(TreeP blocObj)
{
    LVarDeclP lChamps = NIL(LVarDecl);

    if(blocObj != NIL(Tree))
    {
        TreeP arbreChampMethode = blocObj;

        while(arbreChampMethode->op == LDECLMETH)
        {
            TreeP declChampMethode = getChild(arbreChampMethode, 1);

            if(declChampMethode->op == YDECLC)
            {
                VarDeclP tmp = declChampMethode->u.var;
                LVarDeclP tmpListe = NEW(1,LVarDecl);
              tmpListe->var = tmp;
              tmpListe->next = NIL(LVarDecl);
                lChamps = addVarDecl(tmpListe, lChamps);
            }

            arbreChampMethode = getChild(arbreChampMethode, 0);
        }

        if(arbreChampMethode->op == YDECLC)
        {
            VarDeclP tmp = arbreChampMethode->u.var;
            LVarDeclP tmpListe = NEW(1,LVarDecl);
            tmpListe->var = tmp;
            tmpListe->next = NIL(LVarDecl);
            lChamps = addVarDecl(tmpListe, lChamps);
        }
    }

    return lChamps;
}


/* retourne une liste des methodes d'une classe ou d'un objet */
LMethodeP makeMethodeBlocObj(TreeP blocObj, ClasseP classe)
{
    LMethodeP lMethodes = NIL(LMethode);

    if(blocObj != NIL(Tree))
    {
        TreeP arbreChampMethode = blocObj;

        while(arbreChampMethode->op == LDECLMETH)
        {
            TreeP declChampMethode = getChild(arbreChampMethode, 1);

            if(declChampMethode->op != YDECLC)
            {
                MethodeP tmp = makeMethode(declChampMethode, classe);
                lMethodes = addMethode(tmp, lMethodes);
            }

            arbreChampMethode = getChild(arbreChampMethode, 0);
        }


        if(arbreChampMethode->op != YDECLC)
        {
            MethodeP tmp = makeMethode(arbreChampMethode, classe);
            lMethodes = addMethode(tmp, lMethodes);
        }
    }

    return lMethodes;
}


/*--------------------------GET POINTEUR---------------------------*/


/* recupere le pointeur vers une classe nommee nom ou NIL */
ClasseP getClassePointer(char *nom)
{
  LClasseP cur = lclasse;
  while (cur != NIL(LClasse))
  {
    if (strcmp(cur->classe->nom, nom) == 0)
    {
      return cur->classe;
    }

    cur = cur->next;
  }
  return NIL(Classe);
}


/* recupere le pointeur vers une methode nomme nom ou NIL */
MethodeP getMethodePointer(ClasseP classe, char* nom)
{
    LMethodeP tmp = classe->lmethodes;
    while(tmp != NIL(LMethode))
    {
        if(strcmp(tmp->methode->nom,nom)==0)
        {
            return tmp->methode;
        }
        else
        {
            tmp = tmp->next;
        }
    }
    if(classe->superClasse != NIL(Classe))
    {
        return getMethodePointer(classe->superClasse, nom);
    }
    else
    {
        return NIL(Methode);
    }
}

/*--------------------------ADD TO---------------------------*/


/* ajoute une classe dans la liste de classe globale */
void addClasse(ClasseP classe)
{
    LClasseP newClasse = NEW(1, LClasse);

    newClasse->classe = classe;
    newClasse->next = NIL(LClasse);  
    if(lclasse == NIL(LClasse))
    {
        lclasse = newClasse;      
    }
    else
    {
        LClasseP current = lclasse;
        while (TRUE) { 
            if(current->next == NULL)
            {
                current->next = newClasse;
                break; 
            }
            current = current->next;
        }
    }
}


/*  ajoute dans une liste une variable var */
LVarDeclP addVarDecl(LVarDeclP var, LVarDeclP liste)
{
    if(var != NIL(LVarDecl))
    {
        if(liste != NIL(LVarDecl))
        {
            LVarDeclP tmp = liste;

            while(tmp->next != NIL(LVarDecl))
            {
                tmp = tmp->next;
            }
            tmp->next = var;
        }
        else
        {
            liste = var;
        }
    }

    return liste;
}


/* ajoute dans une liste une methode methode */
LMethodeP addMethode(MethodeP methode, LMethodeP liste)
{
    LMethodeP newListe = NEW(1, LMethode);
    newListe->methode = methode;
    newListe->next = liste;

    return newListe;
}


/* ajoute un constructeur à la classe */
void addConstructeur(TreeP blocOpt, ClasseP classe)
{
  if(classe != NIL(Classe))
  {
      MethodeP constr = NEW(1, Methode);
      constr->override = FALSE;
      constr->nom = classe->nom;
      constr->lparametres = classe->lparametres;    /* TODO : OK OU KO */
      constr->typeDeRetour = classe;
      constr->classeAppartenance = classe;
      constr->bloc = blocOpt;
      
      classe->constructeur = constr;
  }
}


/*--------------------------STOCKAGE DES CLASSES ET DES OBJETS---------------------------*/


/* Creation des types primitifs Integer, String et Void */
void makeClassesPrimitives()
{
  ClasseP integer = makeClasse("Integer");
  ClasseP string = makeClasse("String");
  ClasseP voidC = makeClasse("Void"); 

  TreeP exprOpt = NIL(Tree);

  /*Initialisation d'Integer*/
  ParamP paramInt = makeVarDecl("val", "Integer", exprOpt);
  LParamP paramListeInt = NEW(1, LParam);
  paramListeInt->var = paramInt;
  paramListeInt->next = NIL(LParam);
  integer->lparametres = paramListeInt;

  /*Constructeur du type Integer*/
  addConstructeur(exprOpt, integer);

  /*methode toString*/
  MethodeP toString = NEW(1, Methode);
  toString->override = FALSE;
  toString->nom = "toString";
  toString->typeDeRetour = string;
  toString->classeAppartenance = integer;
  toString->bloc = NIL(Tree);

  integer->lmethodes = addMethode(toString, integer->lmethodes);


  integer->superClasse = NIL(Classe);
  integer->lchamps = NIL(LVarDecl);
  
  
  /*Initialisation de String*/
  ParamP paramStr = makeVarDecl("str", "String", exprOpt);
  LParamP paramListeStr = NEW(1, LParam);
  paramListeStr->var = paramStr;
  paramListeStr->next = NIL(LParam);
  string->lparametres = paramListeStr;

  /*Constructeur du type String*/
  addConstructeur(exprOpt, string);

  /*methode print*/
  MethodeP print = NEW(1, Methode);
  print->override = FALSE;
  print->nom = "print";
  print->typeDeRetour = string;
  print->classeAppartenance = string;
  print->bloc = NIL(Tree);

  /*methode println*/
  MethodeP println = NEW(1, Methode);
  println->override = FALSE;
  println->nom = "println";
  println->typeDeRetour = string;
  println->classeAppartenance = string;
  println->bloc = NIL(Tree);

  string->lmethodes = addMethode(print, string->lmethodes);
  string->lmethodes = addMethode(println, string->lmethodes);
  string->superClasse = NIL(Classe);
  string->lchamps = NIL(LVarDecl);

  addClasse(integer);
  /* permet de mettre a jour le type du parametre, pour qu'il pointe vers la structure ajouter dans lclasse */ 
  setEnvironnementType(paramListeInt, integer);
  addClasse(string);
  /* idem pour le parametre d'un string */
  setEnvironnementType(paramListeStr, string);
  addClasse(voidC);
}


/* Permet de mettre a jour les classes et les objets */
void initClasse(TreeP arbreLClasse)
{
    ClasseP bufferClasse = NIL(Classe);
    ClasseP bufferObj = NIL(Classe);
    TreeP arbreCourant = arbreLClasse;
    int *i = NEW(1, int);
    *i = 0;

    while(arbreCourant != NIL(Tree))
    {
        TreeP arbreClasse = getChild(arbreCourant, 0);
        if(arbreClasse->op == YCLASS)
        { 
            /* recupere le pointeur vers la classe correspondante stocke dans lclasse */ 
            bufferClasse = getClassePointer(getChild(arbreClasse, 0)->u.str); 

            /* met a jour le pointeur de la superClasse vers la classe associe dans lclasse */ 
            TreeP arbreExtendOpt = getChild(getChild(arbreCourant, 0), 2);
            if(arbreExtendOpt != NIL(Tree))
            {
                ClasseP tmp = getClassePointer(getChild(arbreExtendOpt, 0)->u.str);
                bufferClasse->superClasse = tmp;

                /* TODO : regrouper checkBlocClasse et init classe ? */
                /* cette verif est faite dans checkBlocClasse */
                /*
                if(tmp != NIL(Classe))
                {
                    bufferClasse->superClasse = tmp;
                }
                else
                {
                    fprintf(stderr, "Erreur d'extends\n");
                    fprintf(stderr, "\t> la classe %s n'existe pas\n\n", getChild(arbreExtendOpt, 0)->u.str);
                    nbErreur++;
                }
                */
            }
        
            /* stockage des parametres de la classe */
            TreeP arbreLParam = getChild(arbreClasse, 1);
            bufferClasse->lparametres = makeLParam(arbreLParam,i);

            /* stockage des champs de la classe */
            TreeP arbreBlocObj = getChild(arbreClasse, 4); 
            LChampP lchamps = makeChampsBlocObj(arbreBlocObj);
            bufferClasse->lchamps = lchamps; 

            /* ajoute les champs statiques declares par le constructeur */
            if(arbreLParam != NIL(Tree))
            {
              while(arbreLParam->op == YLPARAM)
              {
                if(getChild(arbreLParam,0)->op == VYPARAM)
                {
                  LParamP tmpListe = NEW(1,LParam);
                  tmpListe->var = getChild(arbreLParam,0)->u.var;
                  tmpListe->next = NIL(LParam);
                  bufferClasse->lchamps = addVarDecl(tmpListe, bufferClasse->lchamps);  
                }
                arbreLParam = getChild(arbreLParam,1);
              }
              if(arbreLParam->op == VYPARAM)
              {
                LParamP tmpListe = NEW(1,LParam);
                tmpListe->var = arbreLParam->u.var;
                tmpListe->next = NIL(LParam);
                bufferClasse->lchamps = addVarDecl(tmpListe, bufferClasse->lchamps);  
              }        
            }

            /* stockage des methodes de la classe */
            LMethodeP lmethodes = makeMethodeBlocObj(arbreBlocObj, bufferClasse);
            bufferClasse->lmethodes = lmethodes;
            
            /* creation du constructeur de la classe */
            TreeP arbreBlocOpt = getChild(arbreClasse, 3);
            addConstructeur(arbreBlocOpt, bufferClasse);
        }
        else
        {
            /* recupere le pointeur vers l'objet correspondant stocke dans lclasse */ 
            bufferObj = getClassePointer(getChild(arbreClasse, 0)->u.str);

            /* stockage des champs de l'objet */
            TreeP arbreBlocObj = getChild(arbreClasse, 1); 
            LChampP lchamps = makeChampsBlocObj(arbreBlocObj);
            bufferObj->lchamps = lchamps;

            /* stockage des methodes de la classe */
            LMethodeP lmethodes = makeMethodeBlocObj(arbreBlocObj, bufferClasse);
            bufferObj->lmethodes = lmethodes;
        }

        arbreCourant = getChild(arbreCourant, 1);
    }
}


/* set la variable globale lclasse */
void stockerClasse(TreeP arbreLClasse, bool verbose)
{
    TreeP courant = arbreLClasse;

    /* ajoute les classes primitives */
    makeClassesPrimitives();

    /* initialise la liste lclasse */
    while(courant != NIL(Tree))
    {
        TreeP arbreClasse = getChild(courant, 0);
        if(arbreClasse->op == YCLASS)
        {
            ClasseP tmp = makeClasse(getChild(arbreClasse, 0)->u.str); 
            addClasse(tmp);
            courant = getChild(courant, 1);
        }
        else
        {
            ClasseP tmp = makeObjet(getChild(arbreClasse, 0)->u.str);
            addClasse(tmp);
            courant = getChild(courant, 1);
        }
    }
    
    /* met a jour la liste lclasse */
    initClasse(arbreLClasse);

    if(verbose)
    {
        printf("----------------------------LES CLASSES && OBJETS----------------------------\n");
        printLClasse();
        printf("\n");
    }
}


/* initialise l'environnement */
void initEnv()
{
    env = NEW(1, Scope);
    env->env = NIL(LVarDecl);
    env->taille = 0;
}


/*---------------------------VERIFICATION CONTEXTUELLE----------------------------*/


/* verification contextuelle du programme */
bool verifContextProg(TreeP arbreLClasse, TreeP main)
{
    bool check = TRUE;

    check = verifContextLClasse(arbreLClasse) && check;
    check = verifContextMain(main) && check;

    if(nbErreur > 0)
        fprintf(stderr, "Votre programme a %d erreur(s)\n\n", nbErreur);
    else
        printf("\nC'est good\n\n");

    return check;
}


/* verification contextuelle du main */
bool verifContextMain(TreeP main)
{
    bool check = TRUE;

    check = checkBlocMain(main, NIL(Classe)) && check;

    return check;
}


/* verification contextuelle des classes et des objets */
bool verifContextLClasse(TreeP arbreLClasse)
{
    bool check = TRUE;

    /* check = checkBoucleHeritage(lclasse) && check;
    check = checkDoublonClasse(lclasse) && check; */

    check = checkBlocClasse(arbreLClasse, NIL(Classe)) && check;

    return check;
}


/*---------------------------C'EST GOOD----------------------------*/


/* fonction principale : stocke les donnes, fait la verif contextuelle et la generation de code */
void compile(TreeP arbreLClasse, TreeP main, bool verbose)
{
    initEnv();

    if(arbreLClasse != NIL(Tree))
    {
        stockerClasse(arbreLClasse, verbose);
    }

    verifContextProg(arbreLClasse, main); 

    /* genCode($1, $2); */
}



/*---------------------------AFFICHAGE----------------------------*/


/* affiche une liste de variables */
void printVarDecl(LVarDeclP lvar)
{
    LVarDeclP tmp = lvar; 
    while(tmp != NIL(LVarDecl))
    {
          if(tmp->var->type != NIL(Classe))
              printf("\t%s : %s\n", tmp->var->nom, tmp->var->type->nom);
          tmp = tmp->next;
    }
}


/* affiche une classe ou un objet */
void printClasse(ClasseP classe)
{
    if(classe->constructeur != NIL(Methode))
    {
        printf("#####################################\n");
        printf("Classe : %s\n", classe->nom);
        if(classe->superClasse != NIL(Classe))
        {
            printf("\nSuperClasse : %s\n", classe->superClasse->nom);
        }
        else
        {
            printf("\nSuperClasse : NIL\n");
        }
        printf("\n");
        printf("Parametres :\n");
        printVarDecl(classe->lparametres); 
        printf("\n");
        printf("Constructeur :\n");
        printMethode(classe->constructeur); 
        printf("\n");
        printf("Champs :\n");
        printVarDecl(classe->lchamps);
        printf("\n");
        printf("Methodes :\n\n");
        printLMethode(classe->lmethodes);
        printf("\n");
    }
    else
    {
        printf("#####################################\n");
        printf("Objet : %s\n", classe->nom);
        printf("\n");
        printf("Champs :\n");
        printVarDecl(classe->lchamps);
        printf("\n");
        printf("Methodes :\n\n");
        printLMethode(classe->lmethodes);
        printf("\n");
    }
    
}


/* affiche la liste de classe et d'objet */
void printLClasse()
{
    LClasseP tmp = lclasse;
    while(tmp != NIL(LClasse))
    {
        if(tmp->classe != NIL(Classe))
            printClasse(tmp->classe);
        tmp = tmp->next;
    }
}


/* affiche une methode */
void printMethode(MethodeP methode)
{
    if(methode != NIL(Methode))
    {
        printf("-------Methode %s-------\n", methode->nom);
        char *override = (methode->override == TRUE) ? "TRUE" : "FALSE";
        printf("Override : %s\n", override);
        printf("Parametres :\n");
        printVarDecl(methode->lparametres);
        if(methode->typeDeRetour != NIL(Classe))
            printf("Type de retour : %s\n", methode->typeDeRetour->nom);
        else
            fprintf(stderr, "Type de retour : indefini\n"); 
        if(methode->classeAppartenance != NIL(Classe))
            printf("Classe d'appartenance : %s\n", methode->classeAppartenance->nom);
        else
            printf("Classe d'appertenance : indefini");               
        printf("\n");
    }
}


/* affiche une liste de methode */
void printLMethode(LMethodeP lmethode)
{
    LMethodeP tmp = lmethode;
    while(tmp != NIL(LMethode))
    {
          MethodeP methode = tmp->methode;
          printMethode(methode);
          printf("\n");
          tmp = tmp->next;
    }
}


/* affiche l'environnement */
void printScope()
{
    printf("taille : %d\n", env->taille);
    printf("Variable accessible :\n");
    printVarDecl(env->env);
}


/* affiche l'arbre syntaxique */
void afficherProgramme(TreeP tree, bool verbose) 
{
    if(tree != NIL(Tree)) 
    {
        switch (tree->op) 
        {

        case YPROG :
          if(verbose) printf("PROG\n");
          afficherProgramme(tree->u.children[0], verbose);
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YCONT :
          if(verbose) printf("YCONT\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" IS ");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case LDECLC :
          if(verbose) printf("LDECLC\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf("\n");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YDECLC :
          if(verbose) printf("YDECLC\n");
          LVarDeclP tmpListe = NEW(1,LVarDecl);
          tmpListe->var = tree->u.var;
          tmpListe->next = NIL(LVarDecl);
          printVarDecl(tmpListe);
          break;

        case LINSTR :
          if(verbose) printf("LINSTR\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf("\n");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YOBJ :
          if(verbose) printf("YOBJ\n");
          printf("OBJECT ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" IS ");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YLCLASS :
          if(verbose) printf("YLCLASS\n");  
          afficherProgramme(tree->u.children[0], verbose);
          printf("\n");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YCLASS :
          if(verbose) printf("YCLASS\n");
          printf("CLASS ");
          afficherProgramme(tree->u.children[0], verbose);
          printf("( ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          afficherProgramme(tree->u.children[2], verbose);
          afficherProgramme(tree->u.children[3], verbose);
          printf(" IS ");
          afficherProgramme(tree->u.children[4], verbose);
          break;

        case Chaine :
          if(verbose) printf("Chaine\n");
          printf("%s", tree->u.str);
          break;

        case YITE :
          if(verbose) printf("ITE\n");
          printf("IF ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" THEN ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" ELSE ");
          afficherProgramme(tree->u.children[2], verbose);
          break;

        case EAFF :
          if(verbose) printf("EAFF\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" := ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(";");
          break;

        case ECAST :
          if(verbose) printf("CAST\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case YEXPR :                                                            
          if(verbose) printf("EXPR\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf(";");
          break;

        case EINST :
          if(verbose) printf("EINST\n");
          printf("NEW ");
          afficherProgramme(tree->u.children[0], verbose);
          printf("( ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case SELEXPR :
          if(verbose) printf("SELEXPR\n");
          if(tree->nbChildren == 2)
          {
            afficherProgramme(tree->u.children[0], verbose);
            printf(".");
            afficherProgramme(tree->u.children[1], verbose);
          }
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case Id :
          if(verbose) printf("Id\n");
          printf("%s", tree->u.str);  
          break;

        case Cste :
          if(verbose) printf("Cste\n");
          printf("%d", tree->u.val);  
          break;

        case Classname :
          if(verbose) printf("Classname\n");
          printf("%s", tree->u.str);  
          break;

        case YEXT :
          if(verbose) printf("YEXT\n");
          printf("EXTENDS ");
          afficherProgramme(tree->u.children[0], verbose);
          printf("( ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case YLEXPR :
          if(verbose) printf("YLEXPR\n");
          printf("(");
          afficherProgramme(tree->u.children[0], verbose);
          printf(", ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(")");
          break;

        case EENVOI :
          if(verbose) printf("EENVOI\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf(".");  
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case METHOD :
          if(verbose) printf("METHOD\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf("( ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case LDECLMETH :
          if(verbose) printf("LDECLMETH\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf("\n");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case DMETHODE :
          if(verbose) printf("DMETHODE\n");
          afficherProgramme(tree->u.children[0], verbose); 
          printf(" DEF ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" ( ");
          afficherProgramme(tree->u.children[2], verbose); 
          printf(" ) ");
          afficherProgramme(tree->u.children[3], verbose);
          printf(" IS ");
          afficherProgramme(tree->u.children[4], verbose);
          break;

        case DMETHODEL :
          if(verbose) printf("DMETHODEL\n");
          afficherProgramme(tree->u.children[0], verbose); 
          printf(" DEF ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" ( ");
          afficherProgramme(tree->u.children[2], verbose); 
          printf(") :");
          afficherProgramme(tree->u.children[3], verbose);
          printf(" := ");
          afficherProgramme(tree->u.children[4], verbose);
          break;

        case YLPARAM :
          if(verbose) printf("YLPARAM\n");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" , ");
          afficherProgramme(tree->u.children[1], verbose);
          break;

        case YPARAM :
          if(verbose) printf("YPARAM\n");
          LVarDeclP tmpListeBis = NEW(1,LVarDecl);
          tmpListeBis->var = tree->u.var;
          tmpListeBis->next = NIL(LVarDecl);
          printVarDecl(tmpListeBis);
          break;

        case ADD :
          if(verbose) printf("ADD\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" + ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case SUB :
          if(verbose) printf("SUB\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" - ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case MUL :
          if(verbose) printf("MUL\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" * ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case DIV :
          if(verbose) printf("DIV\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" / ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case CONCAT :
          if(verbose) printf("CONCAT\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" & ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case EQ :
          if(verbose) printf("EQ\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" ) = ( ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case NE :
          if(verbose) printf("NE\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" <> ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case INF :
          if(verbose) printf("INF\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" < ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case INFE :
          if(verbose) printf("INFE\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" <= ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case SUP :
          if(verbose) printf("SUP\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" > ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case SUPE :
          if(verbose) printf("SUPE\n");
          printf("( ");
          afficherProgramme(tree->u.children[0], verbose);
          printf(" >= ");
          afficherProgramme(tree->u.children[1], verbose);
          printf(" )");
          break;

        case OVERRIDE :
          if(verbose) printf("OVERRIDE\n");
          if(strcmp(tree->u.str, "TRUE") == 0)
            printf("OVERRIDE ");
          break;

        default :
          printf("Bonjour Monsieur Voisin, si ce message s'affiche, c'est qu'il existe un cas inconnu dans la construction de l'arbre. Veuillez nous en excuser. Cordialement.");
          break;
        }
    }
}