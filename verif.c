#include <string.h>
#include <stdio.h>
#include "tp.h"
#include "tp_y.h"

extern char *strdup(const char*);
extern void setError(int code);

extern LClasseP lclasse;
extern ScopeP env;
extern int nbErreur;

/* extern int yylineno; */


/* verifie l'extistance d'une classe */
bool checkClassDefine(char* nom)
{
    if(nom != NULL)
    {
        LClasseP tmp = lclasse;

        while(tmp != NIL(LClasse))
        {
            if(tmp->classe != NIL(Classe))
            {
                if(strcmp(tmp->classe->nom, nom) == 0)
                {
                    return TRUE;
                }
            }
            
            tmp = tmp->next;
        }
    }
   
    fprintf(stderr, "Erreur checkClassDefine :\n");
    fprintf(stderr, "\t> la classe %s est indefini\n\n", nom);
    nbErreur++;
    return FALSE;
}


/* Verifie la portee d'une variable */
bool checkPortee(LVarDeclP lvar, char* nom)
{
    if(nom != NULL)
    {
        while(lvar != NIL(LVarDecl))
        {
            if(strcmp(lvar->var->nom, nom) == 0)
            {
                return TRUE;
            }
            lvar= lvar->next;
        }

        fprintf(stderr, "Erreur checkPortee :\n");
        fprintf(stderr, "\t> la variable %s est indefini\n\n", nom);
        nbErreur++;

        return FALSE;
    }

    fprintf(stderr, "Erreur checkPortee :\n");
    fprintf(stderr, "\t> la variable est indefini (nom = NIL)\n\n");
    nbErreur++;

    return FALSE;
}


/* verification contextuelle d'un bloc */
bool checkBlocMain(TreeP bloc, ClasseP classe)
{
    int* i = NEW(1, int);
    *i = 0;

    bool check = TRUE;

    if(bloc != NIL(Tree))
    {
        LVarDeclP tmp = NIL(LVarDecl);

        switch(bloc->op)
        {
            case YCONT :
              tmp = makeLParam(getChild(bloc, 0), i);

              if(tmp != NIL(LVarDecl))
                check = addEnv(tmp, classe) && check;

              check = checkBlocMain(getChild(bloc, 1), classe) && check;
              removeEnv(*i);
              break;
            
            case LINSTR :
              check = checkBlocMain(getChild(bloc, 0), classe) && check;
              check = checkBlocMain(getChild(bloc, 1), classe) && check;
              break;

            case YITE :
              check = checkExpr(getChild(bloc, 0), classe) && check;  
              check = checkBlocMain(getChild(bloc, 1), classe) && check;
              check = checkBlocMain(getChild(bloc, 2), classe) && check;
              break;

            case EAFF :
              check = checkBlocMain(getChild(bloc, 0), classe) && check;
              check = checkExpr(getChild(bloc, 1), classe) && check;
              break;

            case SELEXPR :
              check = checkSelection(bloc) && check;
              break;

            case Id :
              check = checkPortee(env->env, bloc->u.str) && check;  
              break;  

            case YEXPR :
              check = checkExpr(getChild(bloc, 0), classe) && check;  
              break;

            case YRETURN :
              break;

            default :
              fprintf(stderr, "Erreur etiquette dans checkBlocMain :\n");
              fprintf(stderr, "\t> etiquette %d inconnu\n\n", bloc->op);
              check = FALSE;
              break;
        }
    }

    free(i);
    return check;
}


/* verifie la validite d'une expression */
/* TODO : ne fait que la portee, faut faire le reste */
bool checkExpr(TreeP tree, ClasseP classe)
{
    bool check = TRUE;

    if(tree != NIL(Tree))
    {
        char* nom;

        ClasseP typeG;
        ClasseP typeD;

        switch (tree->op)
        {
            case Id :
                nom = tree->u.str;
                /* TODO : verifier OK ou KO */
                if(strcmp(nom, "this") != 0 && strcmp(nom, "super") && strcmp(nom, "result"))
                    check = checkPortee(env->env, nom) && check;
                break;

            case Classname :
                nom = tree->u.str;
                check = checkClassDefine(nom) && check;
                break;

            case Chaine :
            case Cste :
                break;
        
            case ADD :
                /* TODO : verif avec le truc unaire */
            case SUB :
            case MUL :
            case DIV :    
            case EQ :
            case NE :
            case SUP :
            case SUPE :
            case INF :
            case INFE :

                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkExpr(getChild(tree, 1), classe) && check;

                typeG = getType(getChild(tree, 0), classe);
                typeD = getType(getChild(tree, 1), classe);

                if(typeG != NIL(Classe) && typeD != NIL(Classe))
                {
                    if(!(strcmp(typeG->nom, "Integer") == 0))
                    {
                        fprintf(stderr, "Erreur verification d'une expression [op arithmetique] :\n");
                        fprintf(stderr, "\t> le fils gauche %s n'est pas un Integer\n\n", typeG->nom);
                        nbErreur++;
                        /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                        check = FALSE;             
                    }

                    if((!strcmp(typeD->nom, "Integer") == 0))
                    {
                        fprintf(stderr, "Erreur verification d'une expression [op arithmetique] :\n");
                        fprintf(stderr, "\t> le fils droit %s n'est pas un Integer\n\n", typeD->nom);
                        nbErreur++;
                        /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                        check = FALSE;             
                    }
                }
                else
                {
                    fprintf(stderr, "Erreur verification d'une expression [op arithmetique] :\n");
                    fprintf(stderr, "\t> type inconnu\n\n");
                    nbErreur++;
                    /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                    check = FALSE;
                }
                
                break;

            case USUB :
                /* TODO unaire */
                check = checkExpr(getChild(tree, 1), classe) && check; 
                break;

            case CONCAT :

                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkExpr(getChild(tree, 1), classe) && check;

                typeG = getType(getChild(tree, 0), classe);
                typeD = getType(getChild(tree, 1), classe);

                if(typeG != NIL(Classe) && typeD != NIL(Classe))
                {
                    if(!(strcmp(typeG->nom, "String") == 0))
                    {
                        fprintf(stderr, "Erreur verification d'une expression [concatenation] :\n");
                        fprintf(stderr, "\t> le fils gauche %s n'est pas un String\n\n", typeG->nom);
                        nbErreur++;
                        /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                        check = FALSE;             
                    }

                    if((!strcmp(typeD->nom, "String") == 0))
                    {
                        fprintf(stderr, "Erreur verification d'une expression [concatenation] :\n");
                        fprintf(stderr, "\t> le fils droit %s n'est pas un String\n\n", typeD->nom);
                        nbErreur++;
                        /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                        check = FALSE;             
                    }
                }
                else
                {
                    fprintf(stderr, "Erreur verification d'une expression [op arithmetique] :\n");
                    fprintf(stderr, "\t> type inconnu\n\n");
                    nbErreur++;
                    /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                    check = FALSE;
                }
                

                break;

            case ECAST :

                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkExpr(getChild(tree, 1), classe) && check;
                check = checkCast(getClassePointer(getChild(tree, 1)->u.str), getChild(tree, 0)->u.str) && check; 
                break;

            case EINST :

                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkExpr(getChild(tree, 1), classe) && check;
                break;

            case SELEXPR :      

                check = checkSelection(tree) && check;
                break;

            case EENVOI :

                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkMethodes(getType(getChild(tree, 0), classe), getChild(getChild(tree, 1), 0)->u.str) && check;
                break;

            case YLEXPR : 
                check = checkExpr(getChild(tree, 0), classe) && check;
                check = checkExpr(getChild(tree, 1), classe) && check;
                break; 

            default :
                fprintf(stderr, "Erreur etiquette dans checkExpr :\n");
                fprintf(stderr, "\t> etiquette %d inconnu\n\n", tree->op);
                check = FALSE;
                break;
        }
    }

    return check;
}


/* verifie la validite d'une selection */
bool checkSelection(TreeP selection)
{
    /* TODO */
    return TRUE;
}



/* retourne le nom du type d'une expression correcte ou NIL */
ClasseP getType(TreeP expr, ClasseP classe)
{
    ClasseP type = NIL(Classe);

    if(expr != NIL(Tree))
    {
        ClasseP typeG;
        ClasseP typeD;

        char *nom;
        LVarDeclP tmp = NIL(LVarDecl);

        switch (expr->op)
        {
            case Id :
                if(strcmp(expr->u.str, "this") == 0)
                {
                    if(classe != NIL(Classe))
                        type = classe;
                    else
                    {
                        fprintf(stderr, "Erreur de type :\n");
                        fprintf(stderr, "\t> this est indefini\n\n");
                    }
                }
                else
                {
                    type = getTypeId(expr->u.str);
                }
                break;

            case Classname :
                type = getClassePointer(expr->u.str);
                break;

            case Chaine :
                type = getClassePointer("String");
                break;

            case Cste :
                type = getClassePointer("Integer");
                break;
        
            case ADD :
                /* TODO : verif avec le truc unaire */
            case SUB :
            case MUL :
            case DIV :            
            case EQ :
            case NE :
            case SUP :
            case SUPE :
            case INF :
            case INFE :

                typeG = getType(getChild(expr, 0), classe);
                typeD = getType(getChild(expr, 1), classe);

                if(typeG != NIL(Classe) && typeD != NIL(Classe))
                {
                    if((strcmp(typeG->nom, typeD->nom) == 0) && (strcmp(typeG->nom, "Integer") == 0))
                    {
                        type = typeG;           
                    }
                    else
                    {
                        type = NIL(Classe);
                    }
                }
                else
                {
                    type = NIL(Classe);
                }
                
                break;

            case USUB :
                /* TODO unaire*/
                type = getType(getChild(expr, 1), classe);; 
                break;

            case CONCAT : 

                typeG = getType(getChild(expr, 0), classe);
                typeD = getType(getChild(expr, 1), classe);

                if(typeG != NIL(Classe) && typeD != NIL(Classe))
                {
                    if((strcmp(typeG->nom, typeD->nom) == 0) && (strcmp(typeG->nom, "String") == 0))
                    {
                        type = typeG;           
                    }
                    else
                    {
                        type = NIL(Classe);
                    }
                }
                else
                {
                    type = NIL(Classe);
                }

                break;

            case ECAST :

                typeG = getType(getChild(expr, 0), classe);
                typeD = getType(getChild(expr, 1), classe);

                if(typeG != NIL(Classe) && typeD != NIL(Classe))
                {
                    if((strcmp(typeG->nom, typeD->nom) == 0))
                    {
                        type = typeG;           
                    }
                    else
                    {
                        type = NIL(Classe);
                    }
                }
                else
                {
                    type = NIL(Classe);
                }

                break;

            case EINST :

                type = getType(getChild(expr, 0), classe);
                break;

            case SELEXPR :      

                if(expr->nbChildren == 2)
                {
                    typeG = getType(getChild(expr, 0), classe);
                    if(typeG != NIL(Classe))
                    {
                        nom = getChild(expr, 1)->u.str;
                        tmp = typeG->lchamps;
                        if(tmp != NIL(LVarDecl))
                        {
                            while(tmp != NIL(LVarDecl))
                            {
                                if(strcmp(tmp->var->nom, nom) == 0)
                                {
                                    type = tmp->var->type;
                                    break;
                                }

                                tmp = tmp->next;
                            }
                        }
                        else
                        {
                            fprintf(stderr, "Erreur de selection dans getType :\n");
                            fprintf(stderr, "\t> la liste de champs de la classe %s est NIL\n\n", typeG->nom);
                            type = NIL(Classe);
                        }
                    }
                    else
                    {
                        fprintf(stderr, "Erreur de selection dans getType :\n");
                        fprintf(stderr, "\t> la classe est indifini\n\n");
                        type = NIL(Classe);
                    }
                }
                else
                {
                    type = getType(getChild(expr, 0), classe);
                }
                break;

            case EENVOI :

                typeG = getType(getChild(expr, 0), classe);
                
                if(typeG != NIL(Classe))
                {
                    MethodeP methode = getMethodePointer(typeG, getChild(getChild(expr, 1), 0)->u.str);
                    if(methode != NIL(Methode))
                    {
                        type = methode->typeDeRetour;
                    }
                    else
                    {
                        type = NIL(Classe);
                    }
                }
                else
                {
                    type = NIL(Classe);
                }

                break;

            default :
                fprintf(stderr, "Erreur etiquette dans getType :\n");
                fprintf(stderr, "\t> etiquette %d inconnu\n\n", expr->op);
                type = NIL(Classe);
                break;
        }
    }

    return type;
}


/* retourne le type d'un id */
ClasseP getTypeId(char* nom)
{
    if(env != NIL(Scope))
    {
        LVarDeclP tmp = env->env;
        while(tmp != NIL(LVarDecl))
        {
            if(strcmp(nom, tmp->var->nom) == 0)
            {
                return tmp->var->type;
            }
            tmp = tmp->next;
        }
    }
    return NIL(Classe);
}  


/* verification du type des variables et de l'affectation */
bool setEnvironnementType(LVarDeclP var, ClasseP classe)               
{
    bool check = TRUE;
    LVarDeclP tmp = var;
    while(tmp != NIL(LVarDecl))
    {
        if(tmp->var->type != NIL(Classe))
        {
            ClasseP type = getClassePointer(tmp->var->type->nom);
            if(type != NIL(Classe))
            {
                tmp->var->type = type;

                if(tmp->var->exprOpt != NIL(Tree))
                {
                    check = checkAff(tmp->var, tmp->var->exprOpt, classe);
                }
            }
            else
            {
                fprintf(stderr, "Erreur de verification de type :\n");
                fprintf(stderr, "\t> le type %s est inconnu\n\n", tmp->var->type->nom);
                nbErreur++;
                check = FALSE;
            }
        }
        else /* 99% sur que ce cas est impossible */
        {
            fprintf(stderr, "Erreur de verification de type :\n");
            fprintf(stderr, "\t> le type de %s = NIL\n\n", tmp->var->nom);
            nbErreur++;
            check = FALSE;
        }
        

        tmp = tmp->next;
    }

    return check;
}


/* verification du bloc contenant les classes et les objets */
bool checkBlocClasse(TreeP tree, ClasseP classe)
{
    int* i = NEW(1, int);
    *i = 0;

    bool check = TRUE;

    if(tree != NIL(Tree))
    {
        LVarDeclP tmp = NIL(LVarDecl);
        ClasseP superClasse = NIL(Classe);

        switch(tree->op)
        {
            case YLCLASS :
                check = checkBlocClasse(getChild(tree, 0), classe) && check;
                check = checkBlocClasse(getChild(tree, 1), classe) && check;
                break;

            case YCLASS :
                classe = getClassePointer(getChild(tree, 0)->u.str);

                /* TODO : 
                * verifier le nom, pour ne pas avoir deux fois la meme variable dans le meme bloc
                */

                TreeP extends = getChild(tree, 2);
                if(extends != NIL(Tree))
                {
                    superClasse = getClassePointer(getChild(extends, 0)->u.str);
                    if(superClasse != NIL(Classe))
                    {
                        tmp = NIL(LVarDecl);
                        tmp = envHerite(superClasse);
                        check = addEnv(tmp, classe) && check;
                    }
                    else
                    {
                        fprintf(stderr, "Erreur d'heritage :\n");
                        fprintf(stderr, "\t> la classe %s est inconnu\n\n", getChild(extends, 0)->u.str);
                        nbErreur++;
                    }
                }


                tmp = classe->lparametres;

                if(tmp != NIL(LVarDecl))
                    check = addEnv(tmp, classe) && check;
            
                TreeP constructeur = getChild(tree, 3);
                check = checkBlocMain(constructeur, classe) && check;

                int tailleLParam = getTailleListeVarDecl(tmp);
                removeEnv(tailleLParam);
    

                /* TODO : 
                * verifier le nom, pour ne pas avoir deux fois la meme variable dans le meme bloc
                */

                tmp = classe->lchamps;
                if(tmp != NIL(LVarDecl))
                    check = addEnv(tmp, classe) && check;

                check = checkBlocClasse(getChild(tree, 4), classe) && check; 
                
                removeEnv(env->taille);
                break;

            case YOBJ :
                classe = getClassePointer(getChild(tree, 0)->u.str);

                /* TODO : 
                * verifier le nom, pour ne pas avoir deux fois la meme variable dans le meme bloc
                */

                tmp = classe->lchamps;
                if(tmp != NIL(LVarDecl))
                    check = addEnv(tmp, classe) && check;

                check = checkBlocClasse(getChild(tree, 1), classe) && check; 
                removeEnv(env->taille);
                break;

            case LDECLMETH :
                check = checkBlocClasse(getChild(tree, 0), classe) && check;
                check = checkBlocClasse(getChild(tree, 1), classe) && check;
                break; 

            case YDECLC :
                break;

            case DMETHODEL :

                /* TODO : mettre dans cette partie la verification d'une declaration d'une methode 
                * reste encore a mettre : override correct (attention entre classe et objet), nom correct (pas de surcharge) 
                */

                tmp = makeLParam(getChild(tree, 2), i);
                if(tmp != NIL(LVarDecl))
                    check = addEnv(tmp, classe) && check;

                check = checkClassDefine(getChild(tree, 3)->u.str) && check;

                check = checkExpr(getChild(tree, 4), classe) && check;

                removeEnv(*i);
                break;

            case DMETHODE :

                /* TODO : mettre dans cette partie la verification d'une declaration d'une methode 
                * reste encore a mettre : override correct (attention entre classe et objet), nom correct (pas de surcharge) 
                */
                tmp = makeLParam(getChild(tree, 2), i);
                if(tmp != NIL(LVarDecl))
                    check = addEnv(tmp, classe) && check;

                TreeP type = getChild(tree, 3);
                if(type != NIL(Tree))
                    check = checkClassDefine(type->u.str) && check;

                check = checkBlocMain(getChild(tree, 4), classe) && check;

                removeEnv(*i);
                break;

            default :
                fprintf(stderr, "Erreur etiquette dans checkBlocClasse :\n");
                fprintf(stderr, "\t> etiquette %d inconnu\n\n", tree->op);
                check = FALSE;
                break;
        }
    }

    free(i);
    return check;
}


/* renvoie l'environnement de la super classe */
LVarDeclP envHerite(ClasseP classeMere)
{
    LVarDeclP env = NIL(LVarDecl);

    env = addVarDecl(classeMere->lchamps, env);

    /*
    if(classeMere->superClasse != NIL(Classe))
    {
        env = addVarDecl(envHerite(classeMere->superClasse), env);
    }
    */

    return env;
}


/* ajoute dans env une liste de variable var */
bool addEnv(LVarDeclP var, ClasseP classe)
{
    bool check = TRUE;
    
    if(var != NIL(LVarDecl))
    {
        LVarDeclP tmp = var;
        while(tmp != NIL(LVarDecl))
        {
            check = addVarEnv(tmp->var, classe) & check;
            tmp = tmp->next;
        }
    }

    return check;
}


/* ajoute une variable dans env */
bool addVarEnv(VarDeclP var, ClasseP classe)
{
    bool check = TRUE;

    if(var != NIL(VarDecl))
    {
        ScopeP newEnv = NEW(1, Scope);

        LVarDeclP tmp = NEW(1, LVarDecl);
        tmp->var = var;
        tmp->next = NIL(LVarDecl);

        check = setEnvironnementType(tmp, classe);

        if(env->env == NIL(LVarDecl))
        {
            env->env = tmp;
            env->taille = env->taille + 1;     
        }
        else
        {
            newEnv->env = tmp;
            newEnv->taille = env->taille + 1;
  
            newEnv->env->next = env->env;

            env = newEnv;
        }
    }

    return check;
}


/* retire n variables de l'env */
void removeEnv(int n)
{
    if(env->taille < n)
    {
        fprintf(stderr, "Erreur removeEnv\n");
        return;
    }

    if(n > 0)
    {
        int i;
        LVarDeclP tmp = env->env;
        for(i = 0; i < n; i++)
        {
            tmp = tmp->next;
        }

        env->env = tmp;
        env->taille = env->taille - n;
    }
}


/* verifie une affectation */
bool checkAff(VarDeclP var, TreeP expr, ClasseP classe)
{
    bool check = checkExpr(expr, classe);

    if(check)
    {
        ClasseP tmp = getType(expr, classe);
        if(tmp != NIL(Classe))
        {
            return (var != NIL(VarDecl) && strcmp(var->type->nom, tmp->nom) == 0 && check);
        }

        fprintf(stderr, "Erreur verification affectation\n");
        nbErreur++;
    }
    
    /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
    return FALSE;
}


/* retourne la taille d'une lsite de varDecl */
int getTailleListeVarDecl(LVarDeclP liste)
{
    int i = 0;
    if(liste != NIL(LVarDecl))
    {
        LVarDeclP tmp = liste;
        while(tmp != NIL(LVarDecl))
        {
            i++;
            tmp = tmp->next;
        }
    }

    return i;
}

















































bool checkArguments(LParamP larg1, LParamP larg2)
{
    bool retour = TRUE;
    while(larg1 != NIL(LParam) && larg2 != NIL(LParam))
    {
        if(larg1->var->type != NIL(Classe) && larg2->var->type != NIL(Classe) && strcmp(larg1->var->type->nom, larg2->var->type->nom) == 0)
        {
            larg1 = larg1->next;
            larg2 = larg2->next;
        }
        else
        {
            retour = FALSE;
        }
    }
    if(larg1 != NIL(LParam) || larg2 != NIL(LParam)) retour = FALSE;
    return retour;
}

/*Renvoie true si pas de probleme d'override*/
bool checkOverrideMethode(ClasseP classe, char* nom, LParamP larg, bool isOverride)
{
    if(classe != NIL(Classe))
    {
        LMethodeP tmpMethodes = classe->lmethodes;
        while(tmpMethodes != NIL(LMethode))
        {
            if(strcmp(tmpMethodes->methode->nom,nom) == 0 )
            {
                if(checkArguments(tmpMethodes->methode->lparametres,larg))
                {
                    if(!isOverride)
                    {
                        printf("| Erreur Override | Rajouter un override dans la fonction %s de ",nom );
                        nbErreur++;
                    }
                    return TRUE;
                }
                else
                {
                    if(!isOverride)
                    {
                        printf("surcharge \n");
                        nbErreur++;
                    }
                    else
                    {
                        printf("| Erreur Override | Les paramètres de la méthode %s sont différents dans la classe %s et ",nom,classe->nom);
                        nbErreur++;                      
                    }
                    return FALSE;
                }
            }
            tmpMethodes = tmpMethodes->next;
        }
        if(classe->superClasse != NIL(Classe))
        {
            return checkOverrideMethode(classe->superClasse, nom, larg, isOverride);
        }
        else
        {
            if(isOverride)
            {
                printf("| Erreur Override | La méthode %s n'a pas été trouvée dans les Super Classes de ", nom);
                nbErreur++;
                return FALSE;
            }
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
    
}

bool checkOverrideLClasse(LClasseP lclasse)
{
    bool b = TRUE;
    LClasseP tmpClasses = lclasse;
    while(tmpClasses != NIL(LClasse))
    {
            LMethodeP tmpMethodes = tmpClasses->classe->lmethodes;
            while(tmpMethodes != NIL(LMethode))
            {
                if(tmpMethodes->methode->override && !checkOverrideMethode(tmpClasses->classe->superClasse, tmpMethodes->methode->nom, tmpMethodes->methode->lparametres,tmpMethodes->methode->override))
                {
                    if(tmpClasses->classe->superClasse == NIL(Classe)) printf("| Erreur Override | La classe mère n'existe pas alors qu'il y a un override dans ");
                    printf("la classe %s \n\n", tmpClasses->classe->nom);
                    b =  FALSE;
                }
                if(!tmpMethodes->methode->override && checkOverrideMethode(tmpClasses->classe->superClasse, tmpMethodes->methode->nom, tmpMethodes->methode->lparametres,tmpMethodes->methode->override))
                {
                    printf("la classe %s \n\n", tmpClasses->classe->nom);
                    b = FALSE;
                }
                tmpMethodes = tmpMethodes->next;
            }
            tmpClasses = tmpClasses->next;
    }
    return b;
}

/*Check s'il y a un doublon dans les classes*/
bool checkDoublonClasse(LClasseP lclasse)
{
    if(lclasse != NIL(LClasse))
    {
        LClasseP tmpClasses = lclasse;
        LClasseP tmp= tmpClasses;
        tmpClasses = tmpClasses->next;
        while(tmpClasses != NIL(LClasse))
        {
           if(strcmp(tmpClasses->classe->nom,tmp->classe->nom)==0)     
           {
                printf("Doublon de classe concernant la classe de nom %s\n", tmp->classe->nom);
                nbErreur++;
                return FALSE;
           }
           tmpClasses = tmpClasses->next;
        }
        return checkDoublonClasse(tmpClasses);
    }
    else
    {
        return TRUE;
    }
}

/*Check s'il y a une boucle dans l'héritage des classes*/
bool checkBoucleHeritage(LClasseP lclasse)
{
    LClasseP tmpClasses = lclasse;
    while(tmpClasses != NIL(LClasse))
    {
        ClasseP tmpSuper= tmpClasses->classe->superClasse;
        while(tmpSuper != NIL(Classe))
        {
            if(strcmp(tmpClasses->classe->nom, tmpSuper->nom) == 0)
            {
                printf("Erreur, Boucle d'héritage concernant la classe %s\n", tmpClasses->classe->nom);
                nbErreur++;
                return FALSE;
            }
            tmpSuper = tmpSuper->superClasse;
        }
        tmpClasses = tmpClasses->next;
    }
    return TRUE;
}

/*devrait marche, à tester avec les blocs*/
bool checkCast(ClasseP classe, char* nom)
{
    if(classe != NIL(Classe))
    {
        if(strcmp(classe->nom, nom) == 0)
        {
            return TRUE;
        }
        else
        {
            if(classe->superClasse != NULL)
            {
                return checkCast(classe->superClasse,nom);
            }
            else
            {
                fprintf(stderr, "Erreur verification d'un cast :\n");
                fprintf(stderr, "\t> %s ne peut pas etre cast en %s\n\n", classe->nom, nom);
                nbErreur++;
                /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
                return FALSE;
            }
        }
    }
    else
    {
        fprintf(stderr, "Erreur verification d'un cast :\n");
        fprintf(stderr, "\t> classe indefini\n\n");
        nbErreur++;

        return FALSE;
    }
}

/*Fonction utile pour un envoi : on regarde si la méthode existe dans la classe de l'objet*/
bool checkMethodes(ClasseP classe, char* nom)
{
    bool check = FALSE;

    if(classe != NIL(Classe))
    {
        LMethodeP tmp = classe->lmethodes;
        while(tmp != NIL(LMethode))
        {
            if(strcmp(tmp->methode->nom, nom) == 0)
            {
                check = TRUE;
                break;
            }
            tmp = tmp->next;
        }

        if(classe->superClasse != NIL(Classe) && check == FALSE)
        {
            check = checkMethodes(classe->superClasse, nom);
        }

        if(check == FALSE)
        {
            fprintf(stderr, "Erreur verification methode :\n");
            fprintf(stderr, "\t> %s n'est pas une methode de %s\n\n", nom, classe->nom);
            nbErreur++;
            /* fprintf(stderr, "erreur ligne : %d\n", yylineno); */
        }

        /* TODO : ajouter la verif des parametres */
    }
    else
    {
        fprintf(stderr, "Erreur verification methode :\n");
        fprintf(stderr, "\t> le type de l'envoi est indefini\n\n");
        nbErreur++;
    }
        
    return check;
}



































/*METHODES UTILES POUR LES VERIFICATIONS CONTEXTUELLES : YASSINE AMRAOUI
Yassine AMRAOUI : j'ai cree ces methodes qui servent à l'analyse contextuelle
Et cf. les TODO apres les methodes qui sont des idees de verifications supplementaires
Recuperation de la classe (ClasseP) portant le nom passe en parametre*/
/**ClasseP getClasse(ClasseP lclasse, char *nom)
{
    if(nom!= NULL) {
        ClasseP buffer = lclasse;
        while(buffer!=NULL)
        {
            if(strcmp(nom, buffer->nom)==0) return buffer;
            buffer=buffer->next;
        }
        printf("\nClasse %s non existante...", nom);
        setError(CONTEXT_ERROR);
    }
    return NIL(Classe);
}*/

/*Renvoie true si pas de probleme d'override*/
/*bool checkOverrideMethode(ClasseP classe, char* nom)
{
    if(classe != NIL(Classe))
    {
        LMethodeP tmpMethodes = classe->lmethodes;
        while(tmpMethodes != NIL(LMethode))
        {
            if(strcmp(tmpMethodes->methode->nom,nom) == 0 )
            {
                return TRUE;
            }
            tmpMethodes = tmpMethodes->next;
        }
        if(classe->superClasse != NIL(Classe))
        {
            return checkOverrideMethode(classe->superClasse, nom);
        }
        else
        {
            return FALSE;
        }
    }
    return FALSE;
}
bool checkOverrideLClasse(LClasseP lclasse)
{ 
    LClasseP tmpClasses = lclasse;
    while(tmpClasses != NIL(LClasse))
    {
        LMethodeP tmpMethodes = lclasse->classe->lmethodes;
        while(tmpMethodes != NIL(LMethode))
        {
            if(tmpMethodes->methode ==  NIL(Methode)) printf("fzebfzeufeybfeyb\n");
            printf("override : %d de %s de la classe%s\n", tmpMethodes->methode->override, tmpMethodes->methode->nom,lclasse->classe->nom);
            if(tmpMethodes->methode->override)
            {
                if(!checkOverrideMethode(lclasse->classe->superClasse, tmpMethodes->methode->nom))
                {
                    return FALSE;
                }
                
            }
            printf("next\n");
            tmpMethodes = tmpMethodes->next;
        } 
        lclasse = lclasse->next;
    }
    return TRUE;
}*/
    /*if(lclasse != NULL) 
    {
        bool booleen = checkOverride(lclasse->next); */
        /*Appel recursif pour parcourir l'ensemble de la liste
        Maintenant, on check les override classe par classe (depuis la fin de lclasse par recursivite)
        Si la classe n'a pas de classe mere, override interdit : on va checker*/

       /* if(lclasse->classe->mereOpt == NULL) {
            MethodeP lmethodestemp = lclasse->classe->lmethodes; 
            while(lmethodestemp != NULL) {
                if(lmethodestemp->methode->override) {printf("\nErreur Override : la classe %s n'a pas de classe mere\n",lclasse->classe->nom); retour = FALSE;}
                lmethodestemp = lmethodestemp->next;
            }
        }
        
        else {
            MethodeP lmethodestemp = lclasse->classe->lmethodes;
            while(lmethodestemp != NULL) {
                if(lmethodestemp->methode->override && !checkSuper(lclasse->classe->mereOpt, lmethodestemp)) {printf("Erreur Override : la methode %s de la classe %s n'est pas presente dans la classe mere %s\n", lmethodestemp->methode, lclasse->classe->nom, lclasse->classe->mereOpt->nom); retour = FALSE;}
                lmethodestemp = lmethodestemp->next;
            }
        }
        return retour;  
    }
    return TRUE;   
}*/
/*Renvoie true si la methode passe en parametre est trouvee dans la classe mere*/
/*bool checkSuper(ClasseP lclasse, MethodeP lmethode) { 
    if(lclasse != NULL) {
        bool retour = checkSuper(lclasse->mereOpt, lmethode);  *//*Appel recursif pour parcourir l'ensemble de la liste    
        *//*if(checkMethodeInClasse(lmethode, lclasse)) return TRUE;    
        else return retour;
    }
    return FALSE;
}*/
/*Renvoie true si la méthode passée en paramètre est dans la classe*/
/*bool checkMethodeInClasse(MethodeP lmethode, ClasseP lclasse) { 
    bool retour = FALSE;
    MethodeP methodebuffer = lclasse->lmethodes->methode;
    
    while(methodebuffer != NULL) {
        if(strcmp(lmethode->methode->nom, methodebuffer->nom) == 0) {
            if(lmethode->typeDeRetour != NULL && methodebuffer->typeDeRetour != NULL && strcmp(lmethode->typeDeRetour, methodebuffer->typeDeRetour) == 0) {
                if(checkArguments(lmethode->args, methodebuffer->args)) retour = TRUE;
            }
            else if(lmethode->typeDeRetour == NULL && methodebuffer->typeDeRetour == NULL) {
                if(checkArguments(lmethode->larg, methodebuffer->larg)) retour = TRUE;
            }
        }
        methodebuffer = methodebuffer->next;
    }
    return retour;
}*/
    /*
Renvoie true si même arguments*/





/*bool checkArguments(LParamP larg1, LPAram larg2) {
    bool retour = TRUE;
    while(arg1 != NULL && arg2 != NULL) {
        if(arg1->attibut->type != NULL && arg2->attribut->type != NULL && strcmp(arg1->attribut->type, arg2->attribut->type) != 0) retour = FALSE;
        arg1 = arg1->next;
        arg2 = arg2->next;
    }
    if(arg1 != NULL || arg2 != NULL) retour = FALSE;
    return retour;
}    */
/*
TODO : methodes permettant d'update les listes : par exemple, completer l'attribut mereOpt, les types
TODO : methode permettant de verifier s'il existe plusieurs classes avec le même nom
TODO : methode permettant de voir si un Id est dans l'environnement
*/
/*Yassine AMRAOUI : ce que j'ai ameliore et ajoute
Verifier si une méthode est bien definie
*/
/*bool checkMethodes(ClasseP classe, char* nom)
{
    while(classe->lmethodes != NULL)
    {
        if(strcmp(classe->lmethodes->methode->nom,nom)==0)
        {
            return TRUE;
        }
        else
        {
            classe->lmethodes = classe->lmethodes->next;
        }
    }
    if(classe->mereOpt != NULL)
    {
        return checkMethodes(classe->mereOpt, nom);
    }
    else
    {
        return FALSE;
    }
}*/
/*Verifie l'heritage d'une classe*/
/*bool checkHeritageClass(ClasseP classe, char* nom)
{
    if(strcmp(classe->nom,nom)==0)
    {
        return TRUE; 
    }
    else
    {
        if(classe->mereOpt != NULL)
        {
            return checkHeritageClass(classe->mereOpt,nom);
        }
        else
        {
            return FALSE;
        }
    }
}
*/