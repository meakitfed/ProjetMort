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
bool verbose = FALSE;

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

/* VARIABLE GLOBALE */
LClasseP lclasse = NIL(LClasse);
ObjetP lobjet = NIL(Objet); 
VarDeclP env = NIL(VarDecl);

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
    /*  abort(); */
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
TreeP makeLeafLVar(short op, VarDeclP lvar) {
  TreeP tree = makeNode(0, op);
  tree->u.lvar = lvar;
  return(tree);
}


/*-------------------------MAKEPERSO-------------------------*/

/* Creer une variable à partir des elements de la grammaire
*  Cette fonction est appele dans la grammaire pour creer une feuille LVar */
VarDeclP makeVarDecl(char *nom, TreeP type, TreeP exprOpt)
{
    VarDeclP newVar = NEW(1, VarDecl);

    newVar->nom = nom;
    newVar->exprOpt = exprOpt;
    newVar->next = NIL(VarDecl);

    if(type != NIL(Tree))
    {
        ClasseP classeType = getClassePointer(type->u.str);
        if(classeType != NIL(Classe))
        {
            newVar->type = classeType;
        }
        else
        {
            classeType = makeClasse(type->u.str);
            newVar->type = classeType;
        }
    }
    else
    {
        fprintf(stderr, "Probleme de generation d'arbre\n");
        ClasseP classeType = makeClasse("NIL");
        newVar->type = classeType;
    }

    return newVar;
}


/* Creer une classe sans aucun element initialise sauf le nom */
ClasseP makeClasse(char *nom)
{
  ClasseP classe = NEW(1, Classe);
  
  classe->nom = nom;

  classe->lparametres = NEW(1, VarDecl);
  classe->lparametres->nom = "NIL";
  classe->lparametres->type = NIL(Classe);
  classe->lparametres->exprOpt = NIL(Tree);
  classe->lparametres->next = NIL(VarDecl);

  classe->superClasse = NIL(Classe);

  /* classe->constructeur = NIL(Methode); */
  classe->constructeur = NIL(VarDecl);

  classe->lchamps = NEW(1, VarDecl);
  classe->lchamps->nom = "NIL";
  classe->lchamps->type = NIL(Classe);
  classe->lchamps->exprOpt = NIL(Tree);
  classe->lchamps->next = NIL(VarDecl);
 
  classe->lmethodes = NEW(1, LMethode);
  classe->lmethodes->methode = NIL(Methode);
  classe->lmethodes->next = NIL(LMethode);

  return classe;
}


/* Creer un objet isole sans aucun element initialise sauf le nom */
ObjetP makeObjet(char *nom)
{
  ObjetP objet = NEW(1, Objet);
  
  objet->nom = nom;

  objet->lchamps = NEW(1, VarDecl);
  objet->lchamps->nom = "NIL";
  objet->lchamps->type = NIL(Classe);
  objet->lchamps->exprOpt = NIL(Tree);
  objet->lchamps->next = NIL(VarDecl);
 
  objet->lmethodes = NEW(1, LMethode);
  objet->lmethodes->methode = NIL(Methode);
  objet->lmethodes->next = NIL(LMethode);

  objet->next = NIL(Objet);

  return objet;
}


/* Creer une methode a partir d'un arbre' */
MethodeP makeMethode(TreeP declMethode)
{
    MethodeP newMethode = NEW(1, Methode);

    /* liste de parametres */
    TreeP arbreParamMeth = getChild(declMethode, 2);
    ParamP lparametres = makeLParam(arbreParamMeth);
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

    /* expression optionnelle */
    newMethode->bloc = getChild(declMethode, 4);

    return newMethode;
}


/* Creer une liste de parametres a partir d'un arbre */
VarDeclP makeLParam(TreeP arbreLParam)
{
  ParamP lparam = NIL(VarDecl);

  if(arbreLParam != NIL(Tree))
  {
      lparam = NEW(1, VarDecl);

      while(arbreLParam->op == YLPARAM || arbreLParam->op == LDECLC)
      {
          ParamP tmp = getChild(arbreLParam, 0)->u.lvar;
          addVarDecl(tmp, lparam);

          arbreLParam = getChild(arbreLParam, 1);
      }
            
      ParamP tmp = arbreLParam->u.lvar;
      addVarDecl(tmp, lparam);
  }

  return lparam;
}


/* creer une liste des champs d'un objet ou d'une classe */
ChampP makeChampsBlocObj(TreeP blocObj)
{
    VarDeclP lChamps = NIL(VarDecl);

    if(blocObj != NIL(Tree))
    {
        lChamps = NEW(1, VarDecl);

        TreeP arbreChampMethode = blocObj;

        while(arbreChampMethode->op == LDECLMETH)
        {
            TreeP declChampMethode = getChild(arbreChampMethode, 1);

            if(declChampMethode->op == YDECLC)
            {
                VarDeclP tmp = declChampMethode->u.lvar;
                addVarDecl(tmp, lChamps);
            }

            arbreChampMethode = getChild(arbreChampMethode, 0);
        }

        if(arbreChampMethode->op == YDECLC)
        {
            VarDeclP tmp = arbreChampMethode->u.lvar;
            addVarDecl(tmp, lChamps);
        }
    }

    return lChamps;
}


/* creer une liste des methodes d'une classe ou d'un objet */
LMethodeP makeMethodeBlocObj(TreeP blocObj)
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
                MethodeP tmp = makeMethode(declChampMethode);
                lMethodes = addMethode(tmp, lMethodes);
            }

            arbreChampMethode = getChild(arbreChampMethode, 0);
        }


        if(arbreChampMethode->op != YDECLC)
        {
            MethodeP tmp = makeMethode(arbreChampMethode);
            lMethodes = addMethode(tmp, lMethodes);
        }
    }

    return lMethodes;
}


/* Creer une liste des variables d'un bloc */
VarDeclP makeVarBloc(TreeP bloc)
{
    VarDeclP var = NEW(1, VarDecl);

    if(bloc != NIL(Tree))
    {
        if(bloc->op == YCONT)
        {
            VarDeclP tmp = makeLParam(getChild(bloc, 0));
            addVarDecl(tmp, var);
            addVarDecl(makeVarBloc(getChild(bloc, 1)), var);
        }
        else /* arbreConstructeur->op == LINSTR */
        {
            /* TODO
            * stocker les variables declare dans une liste d'instructions
            * on depile quand les variables en fin de bloc ???
            */
        }
    }

    return var;
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


/* recupere le pointeur vers un objet nomme nom ou NIL */
ObjetP getObjetPointer(char *nom)
{
  ObjetP cur = lobjet;
  while (cur != NIL(Objet))
  {
    if (strcmp(cur->nom, nom) == 0)
    {
      return cur;
    }

    cur = cur->next;
  }

  return NIL(Objet);
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


/* ajoute un objet dans la liste d'objet globale */
void addObjet(ObjetP objet)
{
    if(lobjet == NIL(Objet))
    {
        lobjet = objet;      
    }
    else
    {
        ObjetP current = lobjet;
        while (TRUE) { 
            if(current->next == NULL)
            {
                current->next = objet;
                break; 
            }
            current = current->next;
        }
    }
}


/* ajoute dans env une variable var */
void addEnv(VarDeclP var)
{
    if(env == NIL(VarDecl))
    {
        env = var;      
    }
    else
    {
        VarDeclP current = env;
        while (TRUE) { 
            if(current->next == NULL)
            {
                current->next = var;
                break; 
            }
            current = current->next;
        }
    }
}



/* ajoute dans une liste une variable var */
void addVarDecl(VarDeclP var, VarDeclP liste)
{
    if(liste != NIL(VarDecl))
    {
        VarDeclP tmp = liste;

        while(tmp->next != NIL(VarDecl))
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


/* ajoute dans une liste une methode methode */
LMethodeP addMethode(MethodeP methode, LMethodeP liste)
{
    LMethodeP newListe = NEW(1, LMethode);
    newListe->methode = methode;
    newListe->next = liste;

    return newListe;
}



/*--------------------------STOCKAGE DES CLASSES ET DES OBJETS---------------------------*/


/*Creation des types primitifs Integer, String et Void*/
void makeClassesPrimitives()
{
  ClasseP integer = makeClasse("Integer");
  ClasseP string = makeClasse("String");
  ClasseP voidC = makeClasse("Void"); 

  /* printLClasse(); */

  TreeP exprOpt = NIL(Tree);
  TreeP type = NEW(1, Tree);

  /*Initialisation d'Integer*/

  type->u.str = "Integer";
  ParamP paramInt = makeVarDecl("val", type, exprOpt);

  /*Constructeur du type Integer*/
  /*
  MethodeP constrInt = NEW(1, Methode);
  constrInt->override = FALSE;
  constrInt->nom = "Integer";
  constrInt->lparametres = NIL(VarDecl);
  constrInt->typeDeRetour = integer;
  constrInt->bloc = NIL(Tree);

  integer->constructeur = constrInt;
  */

  /*methode toString*/
  MethodeP toString = NEW(1, Methode);
  toString->override = FALSE;
  toString->nom = "toString";
  toString->typeDeRetour = string;
  toString->bloc = NIL(Tree);

  integer->lparametres = paramInt;
  integer->superClasse = NIL(Classe);
  integer->lchamps = paramInt;
  integer->lmethodes->methode = toString;
  integer->lmethodes->next = NIL(LMethode);
  
  /*Initialisation de String*/

  type->u.str = "String";
  ParamP paramStr = makeVarDecl("str", type, exprOpt);

  /*Constructeur du type String*/
  /*
  MethodeP constrStr = NEW(1, Methode);
  constrStr->override = FALSE;
  constrStr->nom = "String";
  constrStr->lparametres = NIL(VarDecl);
  constrStr->typeDeRetour = string;
  constrStr->bloc = NIL(Tree);

  string->constructeur = constrStr;
  */

  string->lparametres = paramStr;
  string->superClasse = NIL(Classe);
  string->lchamps = paramStr;
  string->lmethodes = NIL(LMethode);

  /* TODO
  * methode print
  * methode println
  */

  addClasse(integer);
  addClasse(string);
  addClasse(voidC);
}


/* Permet de mettre a jour les classes et les objets */
void initClasse(TreeP arbreLClasse)
{
    ClasseP bufferClasse = NIL(Classe);
    ObjetP bufferObj = NIL(Objet);
    TreeP arbreCourant = arbreLClasse;

    while(arbreCourant != NIL(Tree))
    {
        TreeP arbreClasse = getChild(arbreCourant, 0);
        if(arbreClasse->op == YCLASS)
        { 
            bufferClasse = getClassePointer(getChild(arbreClasse, 0)->u.str); 

            TreeP arbreExtendOpt = getChild(getChild(arbreCourant, 0), 2);
            if(arbreExtendOpt != NIL(Tree))
                bufferClasse->superClasse = getClassePointer(getChild(arbreExtendOpt, 0)->u.str); 
        

            TreeP arbreLParam = getChild(arbreClasse, 1);
            ParamP lparam = makeLParam(arbreLParam);

            bufferClasse->lparametres = lparam;
            

            TreeP arbreBloc = getChild(arbreClasse, 4); 

            ChampP lchamps = makeChampsBlocObj(arbreBloc);
            LMethodeP lmethodes = makeMethodeBlocObj(arbreBloc);

            bufferClasse->lchamps = lchamps;
            bufferClasse->lmethodes = lmethodes;

            TreeP arbreConstructeur = getChild(arbreClasse, 3);

            /* TODO 
            * Verifier cette partie pour le constructeur
            */

            if(arbreConstructeur != NIL(Tree))
            {
                if(arbreConstructeur->op == YCONT)
                {
                    ChampP constructeur = makeLParam(getChild(arbreConstructeur, 0));
                    bufferClasse->constructeur = constructeur;

                    /* TODO 
                    * stocker les variables declare dans une liste d'instructions ??
                    */
                }
                else
                {
                    /* TODO 
                    * stocker les variables declare dans une liste d'instructions ??
                    */
                }
            }
        }
        else
        {
            bufferObj = getObjetPointer(getChild(arbreClasse, 0)->u.str);

            TreeP arbreBloc = getChild(arbreClasse, 1); 

            ChampP lchamps = makeChampsBlocObj(arbreBloc);
            LMethodeP lmethodes = makeMethodeBlocObj(arbreBloc);

            bufferObj->lchamps = lchamps;
            bufferObj->lmethodes = lmethodes;
        }

        arbreCourant = getChild(arbreCourant, 1);
    }
}


/* initialise les variables globales lclasse et lobjet */
void stockerClasse(TreeP arbreLClasse, bool verbose)
{
    TreeP courant = arbreLClasse;
    makeClassesPrimitives();

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
            ObjetP tmp = makeObjet(getChild(arbreClasse, 0)->u.str);
            addObjet(tmp);
            courant = getChild(courant, 1);
        }
    }
    
    initClasse(arbreLClasse);

    if(verbose)
    {
        printf("----------------------------LES CLASSES----------------------------\n");
        printLClasse();
        printf("\n");
        printf("----------------------------LES OBJETS----------------------------\n");
        printObjet();
        printf("\n");
    }
}


/* Permet de mettre a jour l'env */
void stockerEnv(TreeP arbreMain, bool verbose)
{
    VarDeclP tmp = makeVarBloc(arbreMain);
    addEnv(tmp);

    if(verbose)
    {
        printf("----------------------------ENVIRONNEMENT----------------------------\n");
        printf("Variables :\n");
        printVarDecl(env); 
        printf("\n");
    }
}


/*---------------------------C'EST GOOD----------------------------*/


/* fonction principale pour le stockage de donnees */
void compile(TreeP arbreLClasse, TreeP main)
{
    if(arbreLClasse != NIL(Tree))
    {
        stockerClasse(arbreLClasse, TRUE);
    }

    stockerEnv(main, TRUE);
}



/*---------------------------AFFICHAGE----------------------------*/


/* affiche une liste de variables */
void printVarDecl(VarDeclP lvar)
{
    VarDeclP tmp = lvar; 
    while(tmp != NIL(VarDecl))
    {
          if(tmp->type != NIL(Classe))
              printf("\t%s : %s\n", tmp->nom, tmp->type->nom);
          tmp = tmp->next;
    }
}


/* affiche une classe */
void printClasse(ClasseP classe)
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
    printVarDecl(classe->constructeur); 
    printf("\n");
    printf("Champs :\n");
    printVarDecl(classe->lchamps);
    printf("\n");
    printf("Methodes :\n\n");
    printLMethode(classe->lmethodes);
    printf("\n");
}


/* affiche une liste de classe */
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


/* affiche un objet */
void printObjet()
{
    ObjetP tmp = lobjet;
    while(tmp != NIL(Objet))
    {
        printf("#####################################\n");
        printf("Objet : %s\n", tmp->nom);
        printf("\n");
        printf("Champs :\n");
        printVarDecl(tmp->lchamps);
        printf("\n");
        printf("Methodes :\n\n");
        printLMethode(tmp->lmethodes);
        printf("\n");

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
            printf("Type de retour : indefini (void ?)\n");                 /* TODO */
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
          printVarDecl(tree->u.lvar);
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
          printVarDecl(tree->u.lvar);
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
