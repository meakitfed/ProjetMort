
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "tp.h"
#include "code.h"
#include "tp_y.h"

extern char* strdup(const char *);
extern ScopeP env;
extern LClasseP lclasse;
FILE* output;
extern int verbose;

/*
 * -----------------GENERATION DE CODE-----------------
 *
 */


/* Fonctions basiques servant à la génération de code
 * pour la machine virtuelle
 */
void NOP() {fprintf(output, "NOP\n");}

/*ERR str, START, STOP*/
void PUSHI(int i) {fprintf(output, "PUSHI %d\n", i);}
void PUSHS(char *c) {fprintf(output, "PUSHS %s\n", c);}
void PUSHG(int a) {fprintf(output, "PUSHG %d\n", a);}
void PUSHL(int a) {fprintf(output, "PUSHL %d\n", a);}
void PUSHA(char* a) {fprintf(output, "PUSHA %s\n", a);}

void STOREL(int x) {fprintf(output, "STOREL %d\n", x);}
void STOREG(int x) {fprintf(output, "STOREG %d\n", x);}
void PUSHN(int x) {fprintf(output, "PUSHN %d\n", x);}
void POPN(int x) {fprintf(output, "POPN %d\n", x);}
void DUPN(int x) {fprintf(output, "DUPN %d\n", x);}
void SWAP() {fprintf(output, "SWAP\n");}
void EQUAL() {fprintf(output, "EQUAL\n");}
void NOT() {fprintf(output, "NOT\n");}
void JUMP(char* label) {fprintf(output, "JUMP %s\n", label);}
void JZ(char* label) {fprintf(output, "JZ %s\n", label);}
void CALL() {fprintf(output, "CALL\n");}
void CRETURN() {fprintf(output, "RETURN\n");}
void CADD() {fprintf(output, "ADD\n");}
void CMUL() {fprintf(output, "MUL\n");}
void CSUB() {fprintf(output, "SUB\n");}
void CDIV() {fprintf(output, "DIV\n");}
void CINF() {fprintf(output, "INF\n");}
void CINFEQ() {fprintf(output, "INFEQ\n");}
void CSUP() {fprintf(output, "SUP\n");}
void CSUPEQ() {fprintf(output, "SUPEQ\n");}
void WRITEI() {fprintf(output, "WRITEI\n");}

/*STR*/
void WRITES() {fprintf(output, "WRITES\n");}
void CCONCAT() {fprintf(output, "CONCAT\n");}
void STORE(int x) {fprintf(output, "STORE %d\n", x);}
void LOAD(int x) {fprintf(output, "LOAD %d\n", x);}
void ALLOC(int x) {fprintf(output, "ALLOC %d\n", x);}

void LABEL(char* c) {
	fprintf(output, "%s : \t", c);}

void NEWLABEL(char* c) {
	LABEL(c);
	NOP();}

int compteurAdresse = 0; /* se mefier de lui*/
LInstanceP linstances = NIL(LInstance); /* a mettre a jour au moment des declarations dans code.c ah bah nan ça se fait pas dans l'oredre  ??!?N?*/

bool dansClasse =FALSE;
char *affected = NULL; /*garde en memoire le nom de la variable pour mettre les new dedans */

void afficherListeInstances()
{
    LInstanceP cur = linstances;

    puts("");
    while (cur != NULL)
    {
        printf("addr : %d, %s : %s\n", adresse(cur->instance->nom), cur->instance->nom, cur->instance->type->nom);
        cur = cur->next;
    }
}


InstanceP makeInstance(VarDeclP var)
{
	InstanceP instance = NEW(1, Instance);
	instance->nom = var->nom;
	instance ->type = var->type;
	instance -> adresse = compteurAdresse;
	compteurAdresse++;
	return instance; 	
}

InstanceP makeInstance2(char *nom,ClasseP type)
{
    InstanceP instance = NEW(1, Instance);
    instance->nom = nom;
    instance ->type = type;
    instance -> adresse = compteurAdresse;
    compteurAdresse++;

    return instance;
}


InstanceP getInstFromName(char *name)
{
	LInstanceP cur = linstances;

	while (linstances != NIL(LInstance))
	{
		if (!strcmp(cur->instance->nom, name)) return cur->instance;
		cur = cur->next;
	}
	return NIL(Instance);
}

void addInstance(InstanceP ins)
{
	LInstanceP cur = linstances;

	LInstanceP new = NEW(1, LInstance);
	new->instance = ins;
	new->next = NULL;

	if(cur == NULL)
	{
		linstances = new;
	}
	else
	{
		new->next = linstances;
		linstances = new;
	}
}


void depilerInstance()
{
    /*LOL on fait pas de free() */

    linstances = linstances->next;
}

/* Retourne l'adresse d'une variable contenue dans l'environnement
 * de variables. 
 * Sera utile pour faire PUSHG Id.adresse() par exemple.
 */
int adresse(char *id)
{
    bool trouve = FALSE;
    int i,j;
    i=j=0;
    LInstanceP cur = linstances;
    while (cur != NULL)
    {
        if(!trouve && !strcmp(cur->instance->nom, id))
        {
            trouve = TRUE;
            i=j;
        }
        cur = cur->next;
        j++;
    }

    return j-1-i;
}


/* ajoute une variable dans env et verifie son affection opt ainsi que son type */
bool addVarEnvSansNum(VarDeclP var, ClasseP classe)
{
    bool check = TRUE;

    if(var != NIL(VarDecl))
    {
        if(strcmp(var->nom, "this") != 0 && strcmp(var->nom, "super") && strcmp(var->nom, "result"))
        {
            ScopeP newEnv = NEW(1, Scope);

            LVarDeclP tmp = NEW(1, LVarDecl);
            tmp->var = var;
            tmp->next = NIL(LVarDecl);

            check = setEnvironnementType(tmp, classe, NIL(Methode));

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
        else
        {
            fprintf(stderr, "Erreur de declaration de variable\n");
            fprintf(stderr, "\t> la variable %s est un id reserve\n\n", var->nom);
        }
    }

    return check;
}

/*
 * Retourne la méthode correspondant à un nom
 * à partir de l'environnement de sa classe
 */
MethodeP getMethodeFromName(ClasseP classe, char *nom)
{
	LMethodeP temp = classe->lmethodes;
	while(temp != NULL)
	{
		if(strcmp(nom, temp->methode->nom) == 0)
		{
			return temp->methode;
		}
		temp = temp->next;
	}
	printf("Erreur methode : methode %s  introuvable.\n", nom);
	return NULL;
}


/* Retourne la variable correspondant à un nom
 * à partir de l'environnement de variables 
 */
VarDeclP getVarDeclFromName(char *nom)                      /*TODO : ne marche pas, ScopeP env est vide*/
{
	/*ScopeP env : env de variables*/
	LVarDeclP temp = env->env;

	while(temp != NULL)
	{
		if(strcmp(nom, temp->var->nom) == 0)
		{
			return temp->var;
		}
		temp = temp->next;
	}
	printf("Erreur environnement : variable %s introuvable.\n", nom);
	return NULL;
}


/* Retourne le décalage d'une variable par rapport
 * au fond de pile de sa classe
 */
int getOffset(ClasseP classe, char *idNom)
{
	/*TODO : classe extends*/
	int offset = 0;

	LVarDeclP temp = classe->lchamps;
	printf("PRINT VAR DECL :: \n");
	printVarDecl(temp);
	printf("Var de la classe %s : \n", classe->nom);

	while(temp != NULL)
	{
		printf(">%s\n", temp->var->nom);
		if(strcmp(temp->var->nom, idNom) == 0)
		{
			printf("Offset = %d\n",offset);
			return offset;
		}
		else
		{
			offset += 1;		/* TODO bizarre : renvoie 0 pour la dernière variable déclarée.*/
		}						/*ça veut dire que c'est la variable qui est à 0 du fond de pile. C'est pas le cas si ?*/
		temp = temp->next;	/*est égal à NULL s'il n'existe qu'un élément dans la liste*/
	}	
	printf("Erreur offset.\n");
	return -1;
}

/*CodeConstructeur*/ 
void codeConstructeurVersionStructure(ClasseP classe)	
{
	int taille = getTailleListeVarDecl(classe->lchamps) + getTailleListeMethode(classe->lmethodes);
	ALLOC(taille);     
	codeLDeclChampStructure(classe->lchamps);
	LMethodeP lmethodes = classe->lmethodes;
	while(lmethodes != NULL) {
		codeDeclMethode(lmethodes->methode);
	}
    /* en fait il faut pas faire ça je crois j'ai rien compris hahahahahahahahaha
    if (affected != NULL)
    {
        puts("SALUTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT JE SUIS UN un constructeur");

        addInstance(makeInstance2(affected, classe));
    } */
}  

/*CodeConstructeur*/ 
void codeConstructeur(TreeP arbre)
{
	/*fprintf(output, "Instanciation de la classe %s :\n", getChild(arbre, 0)->u.str);*/
	ClasseP classe = getClassePointer(getChild(arbre, 0)->u.str); 
	TreeP lexpressions = getChild(arbre, 1);
	int taille = getTailleListeVarDecl(classe->lparametres); /*TODO : getTailleChamps*/

	TreeP tmp = lexpressions;

	ALLOC(taille);  
	int i = 0;      
	for (i = 0;  i< taille; i++) {
		if(tmp != NIL(Tree)){
			DUPN(1);
			codeExpr(getChild(tmp, 0));
			STORE(i);
			tmp = getChild(tmp,1);  
			i++;
		}   
	}
}   


/*CodeConstructeurVersionStructure*/ 
void CodeConstructeurVersionStructure(MethodeP methode)
{
	/*fprintf(output, "Instanciation de la classe %s :\n", getChild(arbre, 0)->u.str);*/
	LVarDeclP lexpressions = methode->lparametres;
	int taille = getTailleListeVarDecl(lexpressions);

	TreeP tmp = methode->bloc;

	ALLOC(taille);  
	int i = 0;      
	for (i = 0;  i< taille; i++) {
		if(tmp != NIL(Tree)){
			DUPN(1);
			codeInstr(getChild(tmp, 0));
			codeLInstr(getChild(tmp, 1));
			STORE(i);
			tmp = getChild(tmp,1);  
			i++;
		}   
	}
} 

int tailleListeArbre(TreeP t)
{
    TreeP cur = t;
    int i =1;

    while(cur->nbChildren == 2)
    {
        i++;
        t = getChild(cur, 1);
    }

    return i;
}

/* 
 * Génère le code d'une Liste de déclarations
 */
void codeLDeclChampStructure(LVarDeclP lChamps)		
{
	LVarDeclP lchamps = lChamps;
	while(lchamps != NULL) {
		codeDeclChampStructure(lchamps->var);
		lchamps = lchamps->next;
	}
}

/*
 * Génère le code des déclarations
 */
void codeDeclChampStructure(VarDeclP Champ)		   
{                                           
	if(verbose) fprintf(output,"--Var %s : ", Champ->nom);
	if(verbose) printf("Ajout de la variable %s à l'environnement...\n", Champ->nom);
	addVarEnvSansNum(Champ, NIL(Classe));	/*TODO? Environnement de classe*/
    
    /*puts("SALUTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTTT JE SUIS UNe structure");*/

	addInstance(makeInstance(Champ));

	if(Champ->exprOpt != NULL)
	{
		codeExpr(Champ->exprOpt);  /*instanciation, affectation...*/

	}
}

/*Génère le code d'une objet*/
void codeObj(ClasseP classe)
{
	/*printf(">Generation du code d'un objet : %s\n", classe->nom);*/
	if(classe->lparametres != NULL || classe->superClasse != NULL || classe->constructeur != NULL) {
		printf("Erreur code Objet\n");
	}
	else {
		LMethodeP lmethodes = classe->lmethodes;
		while(lmethodes != NULL)
		{
			codeDeclMethode(lmethodes->methode);
			lmethodes = lmethodes->next; 
		}
		LVarDeclP lchamps = classe->lchamps;
		codeLDeclChampStructure(lchamps);  
	}
}


/*
 * Génère le code des expressions
 */
void codeExpr(TreeP tree)
{
	switch(tree->op) {

		case Cste:
			PUSHI(tree->u.val);
			printf("PUSHI\n");

		break;

		case Chaine:
		case Classname:
			PUSHS(tree->u.str);
			printf("PUSHS\n");
		break;
	
		case SELEXPR:
			codeSelec(tree);
			printf("SELEXPR\n"); 
			break;

		case Id:
			if(dansClasse)
			{
				DUPN(1);
				PUSHL(adresse(tree->u.str));	/*PUSHL par rapport à fp*/
			}
			else
			{
				DUPN(1);
				PUSHG(adresse(tree->u.str));	/*PUSHG par rapport à gp*/
			}
			break;

		/*instanciation/constructeur*/
		case EINST: 
			printf("Instanciation : EINST\n"); 

			codeConstructeur(tree);
			break;

		/*ExprOperateur*/
		case ADD :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CADD();
			printf("ADD\n");
			break;

		case SUB :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CSUB();
			printf("SUB\n");
			break;

		/*on traite seulement le cas du moins unaire, le plus unaire reviendrait à ne rien faire*/
		case USUB :
			printf("unarySUB\n");
			PUSHI(0);   
			codeExpr(getChild(tree, 0));
			CSUB();             /*on fait 0 moins l'expression */
			break;

		case MUL :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CMUL();
			printf("MUL\n");
			break;
		case DIV :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			printf("DIV\n");
			CDIV();
			break;
		case CONCAT:
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CCONCAT();
			printf("CONCAT\n");
			break;

		/*Relop*/        
		case NE:
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			EQUAL();
			NOT();
			printf("NE\n");
			break;
		case EQ :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			EQUAL();
			printf("EQ\n");
			break;
 
		case INF :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CINF();
			printf("INF\n");
			break;

		case INFE :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CINFEQ();
			printf("INFE\n");
			break;

		case SUP :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CSUP();
			printf("SUB\n");
			break;

		case SUPE :
			codeExpr(getChild(tree, 0));
			codeExpr(getChild(tree, 1));
			CSUPEQ();
			printf("SUPE\n");
			break;
		
		case EENVOI:
			codeEnvoi(tree);
			printf("EENVOI\n");
			break;

		default: 
			if(tree->nbChildren != 0)
			{
				codeExpr(getChild(tree, 0));
			}
			fprintf(output, "#ERREUR : Expression inconnue : %d!\n", tree->op);
			break;
		}
}

/* 
 * Génère le code d'une liste d'instructions.
 */
void codeLInstr(TreeP tree)
{
	if(tree->op == LINSTR)	
	{
		if(tree->nbChildren == 2)
		{
			codeInstr(getChild(tree, 0));
			codeLInstr(getChild(tree, 1));
		}
		else
		{
			codeInstr(tree);  
		}
	}
	else if((tree->op == YEXPR) | (tree->op == EAFF)) 
	{
		codeInstr(tree);
	}
	else
	{
		codeExpr(tree);
	}

}


/* Renvoie une étiquette toujours différente, en fonction
 * du type indiqué en paramètre.
 * Permet d'obtenir if3 ou else10 par exemple.
 */
char *makeLabel(char *type)
{
	static char buf[30];
	static int cpt;

	sprintf(buf, "%s%d", type, cpt++);        /*incrementer l'étiquette statique*/

	return strdup(buf);
}


/* 
 * Génère le code d'une instruction ITE.
 */
void codeITE(TreeP tree)
{
	/*Création des étiquettes*/
	char *labelElse = makeLabel("else");
	char *labelEndIf = makeLabel("endif");

	/*ajout de la condition du if*/
	codeExpr(getChild(tree, 0));

	/*JZ vers le bloc else si la condition ci-dessus est fausse*/
	fprintf(output, "JZ %s\n", labelElse);

	/*Corps du if*/
	codeInstr(getChild(tree, 1));

	/*Si on effectue le code du corps du if, on saute le corps du when*/
	fprintf(output, "JUMP %s\n", labelEndIf);

	/*Etiquette du else*/
	fprintf(output, "%s:\t NOP\n", labelElse);
 
	/*Corps du else optionnel*/
	TreeP childElse = getChild(tree, 2);
	if(childElse)
	{
		codeInstr(getChild(tree, 2));
	}
	else
	{
	   fprintf(output, "\n");
	}

	/*Etiquette de sortie de la boucle*/
	fprintf(output, "%s:\t NOP\n", labelEndIf);
}


/* 
 * Génère le code d'une instruction.
 */
void codeInstr(TreeP tree)
{

	switch (tree->op) {
	case YEXPR:                     		/* Expr;*/
		codeExpr(getChild(tree, 0));   
	   	POPN(1);							/*POPN car on ne garde pas l'expression en mémoire*/	
	   	printf("YEXPR\n");
		break;

	/*atteignable à travers Bloc ;*/
	/*Generation du code d'un bloc*/
	case YCONT:                 			/*Contenu := LDeclChamp IS LInstr */

		if(verbose) fprintf(output, "--########DEBUG : Bloc de type LDeclChamp IS LInstr--\n");
		/*LDeclChamp = list variables locales*/
		codeLDeclChamp(getChild(tree, 0));
		codeLInstr(getChild(tree, 1));
		printf("YCONT\n");
		break;

	/*atteignable à travers Bloc ;*/
	/*Generation du code d'un bloc*/
	case LINSTR:        					/*liste d'instr*/
		codeLInstr(tree);
		printf("LINSTR\n");
		break;
	
	case YRETURN:                           /*$$ = makeTree(YRETURN, 0); */
		CRETURN();
		printf("YRETURN\n");
		break;
	
	case EAFF:								/*Selection := Expr*/
		codeAff(tree);
		printf("EAFF\n");
		break;

	case YITE:								/*Bloc if then else*/
		codeITE(tree);
		break;

	default: 
		fprintf(output, "#ERREUR : Cas Instruction inconnu : %d! (son contenu est : %s)\n", tree->op, tree->u.str);
		break;
	}   

}

/*Génère le code des fonctions print() et println()*/
bool codePrint(TreeP expr, TreeP methodeC)
{
	if(expr->op == Cste)
	{
		if(verbose) fprintf(output, "\n--%s affiche la constante suivante :\n", getChild(methodeC,0)->u.str);
		codeExpr(expr);
		if (!strcmp(getChild(methodeC,0)->u.str,"print")) {				/*écriture simple de la chaine en début de pile*/
			WRITES();
			return TRUE;
		}
		else if (!strcmp(getChild(methodeC,0)->u.str,"println")) {		/*écriture de la chaine et saut de ligne*/
			WRITES();
			PUSHS("\"\\n\"");											/*équivaut à PUSHS "\n" */
			WRITES();
			return TRUE;
		}
	}
	else if((expr->op == Id) && (strcmp(getChild(methodeC,0)->u.str, "toString") == 0))				
	{
		if(verbose) fprintf(output, "\n--%s affiche l'id suivant : %s\n", getChild(methodeC,0)->u.str, expr->u.str);
		
		PUSHL(adresse(expr->u.str));
		PUSHA("toString");
		CALL();
		return TRUE;
	}

	else
	{
		if(verbose) fprintf(output, "\n--%s affiche la chaine suivante :\n", getChild(methodeC,0)->u.str);
		codeExpr(expr);
		if (!strcmp(getChild(methodeC,0)->u.str,"print")) {             /*écriture simple de la chaine en début de pile*/
			WRITES();
			return TRUE;                        
		}
		else if (!strcmp(getChild(methodeC,0)->u.str,"println")) {      /*écriture de la chaine et saut de ligne*/
			WRITES();
			PUSHS("\"\\n\"");                                           /*équivaut à PUSHS "\n" */
			WRITES();
			return TRUE;
		}
	}
	return FALSE;
}



/*Génère le code d'un envoi*/ 
void codeEnvoi(TreeP tree)					/*Envoi: Expr '.' MethodeC */
{
	
	/*if(verbose) fprintf(output, "--#######DEBUG : IL Y AURA UN ENVOI vers %s\n", getChild(tree, 0)->u.str);*/
	TreeP Expr = getChild(tree, 0);										
	TreeP MethodeC = getChild(tree, 1);
	int tailleParam = 0;

	bool printOK = FALSE;
	printOK = codePrint(Expr, MethodeC);			/*Si la méthode est print ou println, on génère son code*/

	if(printOK == FALSE)							/*Si la méthode n'est pas un print ou println*/
	{
		/*On exécuter le code de l'expression*/											
		codeExpr(Expr);
		if(Expr->op != EENVOI)
		{
			/*On récupère la classe correspondant à l'expression*/
			VarDeclP tempExpr = getVarDeclFromName(Expr->u.str);
			
			if(tempExpr)
			{
				char *type = tempExpr->type->nom;
				ClasseP classeEnvoi = getClassePointer(type);

				/*On récupère la structure correspondant à MethodeC*/
				MethodeP tempMethode = getMethodeFromName(classeEnvoi, getChild(MethodeC,0)->u.str);

				if(tempMethode)
				{
					LVarDeclP tempParam = tempMethode->lparametres;
					if(verbose) fprintf(output, "--Envoi : méthode %s de la classe %s\n", tempMethode->nom, classeEnvoi->nom);

					tailleParam = 0; 
					while(tempParam != NULL) 
					{
						PUSHG(tailleParam);  								
						tailleParam++;
						tempParam = tempParam->next;
					}

					/*Appel (statique...) de la méthode*/
					/*	char *adresseMethode = tempMethode->nom;
					if(verbose) fprintf(output, "\n--Appel de la methode %s.\n", adresseMethode);
					PUSHA(adresseMethode);
					CALL();*/

					/*Appel dynamique de la méthode*/
					char *nomMethode = tempMethode->nom;
					if(verbose) fprintf(output, "\n--Appel de la methode %s.\n", nomMethode);

					LMethodeP methodeCourante = classeEnvoi->lmethodes;
					int numMethode = 1;
					while(methodeCourante != NULL)
					{
						if(strcmp(methodeCourante->methode->nom, nomMethode) == 0 )
						{
							break;
						}
						numMethode++;
						methodeCourante = methodeCourante->next;
					}
					fprintf(output, "PUSHA call%d\n", numMethode);	/*appel de l'invocation correspondant à la position de la méthode*/
					CALL();
				
					/*On dépile le nombre de paramètres empilés*/
					POPN(tailleParam);
				}
				else{printf("Erreur envoi : methode introuvable.\n");}

		
			}
			else{printf("Erreur envoi : variable introuvable.\n");}	
		}
	}
}

/*
 * Génère le code d'une sélection
 */
void codeSelec(TreeP tree)		                 /*Selection: Expr '.' Id*/		/*Pb du id dans la production Selection?*/
{                                                /*Voir l'exemple de paire.entiers*/

	if(verbose) fprintf(output, "--#######DEBUG : IL Y AURA UNE SELECTION ICI;\n");

	/*Voir le type de retour de l'expression
	puis chercher le id dans la classe correspondante*/

	/*cas où Expr est un ident*/
	TreeP Expr = getChild(tree,0);
	TreeP Ident = getChild(tree,1);

	/*cas de Select ::= Id . Id*/
	if(Expr->op == Id)
	{
		VarDeclP temp = getVarDeclFromName(Expr->u.str);
		if(temp)
		{
			char *type = temp->type->nom;
			ClasseP classeType = getClassePointer(type);
			
			if(dansClasse)
			{
				PUSHL(adresse(tree->u.str));	/*PUSHL par rapport à fp*/
			}
			else
			{
				PUSHG(adresse(tree->u.str));	/*PUSHG par rapport à gp*/
			}	

			int offset = getOffset(classeType,Ident->u.str);
			LOAD(offset);
		}
	}

	/*cas de Select ::= expr . Id*/
	else 
	{
		codeExpr(getChild(tree, 0));
		ClasseP classe = getInstFromName(getChild(tree,1)->u.str)->type;
		LOAD(getOffset(classe, getChild(tree,1)->u.str));
	}

}


/*Code d'une affectation x := y*/
void codeAff(TreeP tree)    									
{
	/*membre gauche de l'affectation gauche := droit*/
	TreeP gauche = getChild(tree, 0);
    affected = NULL;
    /*if (gauche->nbChildren == 0)  la selection est un id seul donc on va ajouter une nouvelle instance dans la pile
    {
        affected = gauche->u.str;
    }*/

	/*membre droit de l'affectation*/
	TreeP droit = getChild(tree, 1);

	/*pour l'instant on gère les affectations de constante et d'id*/

	if(gauche->op == Id)
	{
		if(droit->op == Cste)
		{
			if(verbose) fprintf(output, "--#######DEBUG : Affectation : constante %d\n", droit->u.val);
			codeExpr(droit);
			DUPN(1);
			STOREG(adresse(gauche->u.str));
		}
		else
		{
			if(verbose) fprintf(output, "--#######DEBUG : Affectation : ident %s\n", droit->u.str);
			codeExpr(droit);
			DUPN(1);
			STOREG(adresse(gauche->u.str));
		}

	}
	else if (gauche->op == SELEXPR) {

		TreeP Expr = getChild(gauche, 0);
		/*TreeP Ident = getChild(gauche, 1);*/

		if (Expr->op == SELEXPR) {
			
			if(dansClasse)
			{
				PUSHL(adresse(tree->u.str));	/*PUSHL par rapport à fp*/
			}
			else
			{
				PUSHG(adresse(tree->u.str));	/*PUSHG par rapport à gp*/
			}

			codeExpr(Expr);

			/*char* t = getSymbole(Expr->u.str)->Expr->type;		TODO

			ClassP class = figureClass(t); cimer 			TODOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

			offset = getOffset(class,Id->u.str);
		   
			STORE(offset);*/
		}

	}
}

/*
 * Code d'un bloc objet situé dans les objets isolés et dans les classes
 */
void codeBlocObj(TreeP tree)        							/*BlocObj: '{' LDeclChampMethodeOpt '}' */
{
	if(tree != NULL)
	{
		if(tree->nbChildren == 2)  
		{
			codeBlocObj(getChild(tree, 0));                     /*LDeclChampMethode: LDeclChampMethode DeclChampMethode*/
			codeDeclChampMethode(getChild(tree, 1));            
		}
		else if(tree->nbChildren == 1)
		{
			codeDeclChampMethode(getChild(tree, 0));            /* LDeclChampMethode: DeclChampMethode*/
		}
	}
}


/*
* fait les instr du bloc puis retire les decls du bloc de la pile
*/
void codeBloc(TreeP bloc)
{
    bool destruction = FALSE;
    int nbDecls;
    if (bloc->nbChildren == 2) /* cas ou on peut trouver des decls dans le bloc */
    {
        destruction = TRUE;
        nbDecls = tailleListeArbre(getChild(bloc, 0));
    }
    codeInstr(bloc);

    if(destruction)
    {
        int i;
        for (i=0 ; i<nbDecls ; i++) depilerInstance();
    }
}


/*
 * Permet d'accéder au code d'une liste de déclaration
 */
void codeDeclChampMethode(TreeP tree)
{
	
	if(tree->op == YDECLC) /*DeclChamp*/
	{
		printf("DECL CHAMP\n");
		codeDeclChamp(tree);
	}

/*	else if(tree->op == DMETHODE) DeclMethode pas besoin de gérer ce cas, 
									vu qu'on met les méthodes dans des structures
	{
	}
	*/
}


/* 
 * Génère le code d'une Liste de déclarations
 */
void codeLDeclChamp(TreeP tree)		
{
	if(tree->nbChildren == 2)				/*LDeclChamp: DeclChamp LDeclChamp */
	{
		codeDeclChamp(getChild(tree, 0));
		codeLDeclChamp(getChild(tree, 1));
	}
	else 									/*LDecleChamp: DeclChamp*/
	{	
		codeDeclChamp(tree);  
	}
}


/*
 * Génère le code des déclarations
 */
void codeDeclChamp(TreeP tree)		        /*DeclChamp: VAR Id ':' TypeC ValVar ';'*/
{                                           /*ValVar: AFF Expr*/
	if(verbose) printf("Ajout de la variable %s à l'environnement...\n", tree->u.var->nom);
	
	/*ajouter la nouvelle instance à la liste*/
	addVarEnvSansNum(tree->u.var, NIL(Classe));	
	addInstance(makeInstance(tree->u.var));

	if(tree->u.var->exprOpt != NULL)
	{
		codeExpr(tree->u.var->exprOpt);  /*instanciation, affectation...*/

	}

}

/*Methodes d'une classe*/
void codeDeclMethode(MethodeP methode)	/*voir l'exemple du subint/addint*/
{									

	TreeP tree = methode->bloc;
	int tailleParam = 0;
	int i = 0;
	if(tree != NULL)
	{   
		if(verbose) fprintf(output,"\n\n--Declaration de la methode %s de type de retour %s.\n", methode->nom, methode->typeDeRetour->nom);
		fprintf(output, "%s: \t", methode->nom);

		/*Empilement des paramètres de la méthode*/
		tailleParam = getTailleListeVarDecl(methode->lparametres);
		i = tailleParam;
		while(i > 0)
		{
			PUSHL(-i);
			LOAD(0);
			i--;
		}

		/*Génération du code du bloc de la fonction*/
		if(strcmp(methode->typeDeRetour->nom, "Void") == 0)
		{
			if(tree->op == LINSTR) codeLInstr(tree);    /*si les instructions sont sous forme de liste*/
			else codeInstr(tree);                   /*s'il existe qu'une seule instruction*/
			
		}
		else
		{
			 if(verbose) fprintf(output,"\n--Methode avec un type de retour.\n"); 
			 codeLInstr(tree);											
		}

		if(verbose) fprintf(output, "--stocke le resultat a son emplacement\n");
		PUSHL(-(tailleParam+1)); 
		SWAP(); 
		STORE(0);
		CRETURN();
	}
}


/*
 * Génère le code d'une classe
 */
void codeClasse(ClasseP classe)			
{
	if(verbose) fprintf(output, "--Generation du code d'une classe : %s\n", classe->nom);

	LMethodeP liste = classe->lmethodes;

	while(liste != NULL)
	{
		codeDeclMethode(liste->methode);
		liste = liste->next;
	}
}


/*
 * Génère le code d'une liste de classes
 */
void codeLClasse()
{
	if(verbose) printf(">Generation du code d'une liste de classe...\n");

	/*Variable globale LClasse*/
	LClasseP liste = lclasse;

	/*Parcours l'environnement de classe*/
	while(liste != NIL(LClasse))
	{
		codeClasse(liste->classe);
		liste = liste->next;
	}
}


/*
 * Retourne la taille d'une liste de methodes
 */
int getTailleListeMethode(LMethodeP liste)
{
    int i = 0;
    if(liste != NIL(LMethode))
    {
        LMethodeP tmp = liste;
        while(tmp != NIL(LMethode))
        {
            i++;
            tmp = tmp->next;
        }
    }
    return i;
}


/*
 * Génère la table virtuelle du programme.
 */
void codeTV()
{
	int nbMethodes = 0;
	int cptStore = 0;
	if(verbose) printf(">Generation de la table virtuelle...\n");

	fprintf(output, "--------Initialisation des tables virtuelles.--------\n");
	
	fprintf(output, "init:\t NOP\n");

	/*Variable globale LClasse*/
	LClasseP liste = lclasse;

	/*Parcours l'environnement de classe*/
	while(liste != NIL(LClasse))
	{
		if((strcmp(liste->classe->nom, "Integer") != 0) && (strcmp(liste->classe->nom, "String") != 0) && (strcmp(liste->classe->nom, "Void") != 0))
		{
			cptStore = 0;
			if(verbose) fprintf(output, "\n--Classe %s\n",liste->classe->nom);

			LMethodeP lmethodes = liste->classe->lmethodes;
			nbMethodes = getTailleListeMethode(lmethodes);

			ALLOC(nbMethodes);

			while(lmethodes != NIL(LMethode))
			{
				PUSHA(lmethodes->methode->nom);
				STORE(cptStore);
				DUPN(1);

				cptStore++;
				lmethodes = lmethodes->next;
			}
		}
		liste = liste->next;
	}
	JUMP("main");
	fprintf(output, "\n");

	fprintf(output, "--------Initialisation des invocations.--------\n\n");

	/*Variable globale LClasse*/
	liste = lclasse;

	nbMethodes = 1;

	/*Parcours l'environnement de classe*/
	while(liste != NIL(LClasse))
	{
		/*on cherche le plus grand nombre de méthodes d'une classe*/
		LMethodeP lmethodes = liste->classe->lmethodes;

		if(getTailleListeMethode(lmethodes) > nbMethodes)
		{
			nbMethodes = getTailleListeMethode(lmethodes);
		}	
		liste = liste->next;
	}

	/*generation des methodes call*/
	int cptCall = 1;
	while (cptCall <= nbMethodes)
	{
		fprintf(output, "--invoquer methode numero %d du receveur\n", cptCall);
		fprintf(output, "call%d: \tPUSHL -1\n", cptCall);
		DUPN(1);
		LOAD(0);
		LOAD(cptCall-1);
		CALL();
		CRETURN();
		cptCall++;
	}
}


/*
 * Génération du code de la méthode toString pour la classe Integer
 */
void codeToString()
{
	fprintf(output, "\n\n--Methode toString\n");

	fprintf(output, "toString:\t PUSHL -2\n");
	fprintf(output, "STR\n");
	fprintf(output, "STOREL -1\n");
	fprintf(output, "POPN 1\n");
	fprintf(output, "RETURN\n");
}


/*
 * Fonction principale de génération de code
 */
void genCode(TreeP LClass, TreeP Bloc)
{

	output = fopen("genCode", "w+");

	if (output != NULL) {

		dansClasse = TRUE;
		/*Jump vers la table virtuelle*/
		fprintf(output, "JUMP init\t --initialiser la table virtuelle\n\n");

		/*Generation du code des méthodes des classes 
		 à partir de la variable globale LClasse*/
		codeLClasse();

		/*Generation de code du bloc principal*/
		dansClasse = FALSE;

		fprintf(output, "--------------BLOC PRINCIPAL--------------");
		fprintf(output, "\n\n\nmain: \tSTART\n");
		codeInstr(Bloc);
		fprintf(output, "STOP\n\n\n");
		fprintf(output, "--------------FIN DU BLOC PRINCIPAL--------------\n\n");

		dansClasse = TRUE;
		/*Generation de la table virtuelle*/
		codeTV();

		/*Generation de la methode toString*/
		codeToString();

		/*saut de ligne nécessaire à la fin du programme, 
		sans quoi il y a une erreur*/
		fprintf(output, "\n");

        afficherListeInstances();

       /* puts("----------TESTS--------------");
        printf("adresse de p1 : %d\n", adresse("p1"));
        printf("adresse de p2 : %d\n", adresse("p2"));*/

		fclose(output);
	}
}
