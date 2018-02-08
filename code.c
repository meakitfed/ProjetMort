
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
void PUSHA(char* a) {fprintf(output, "PUSHA %s\n", a);}
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

int compteurAdresse = 0;
LInstanceP linstances = NIL(LInstance); /* a mettre a jour au moment des declarations dans code.c ah bah nan ça se fait pas dans l'oredre  ??!?N?*/

/* Retourne l'adresse d'une variable contenue dans l'environnement
 * de variables. 
 * Sera utile pour faire PUSHG Id.adresse() par exemple.
 */



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

int adresse(char *id) 
{

/*
	On va parcourir l'environnement de variables et prendre la première qui correspond,
	et return son adresse (créée arbitrairement lors de sa déclaration.
*/


/* Dans l'environnement de variables

	Il faut :
	Portée : Globale, Locale, Param
	Offset
	
	Il faut savoir gérer l'appel de fonction (x.f(), passer x en paramètres ?)

	Nombre d'attributs dans une classe afin de faire ALLOC 3 par ex dans le cas d'un new, alloc de mémoire
*/
  return 0;
}


/*retourne la méthode correspondant à un nom
 *à partir de l'environnement de sa classe
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
void codeConstructeur(TreeP arbre)		/*Le constructeur permet d'instancier les variables de la classe*/
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



/*Génère le code d'une objet*/
void codeObj(ClasseP classe)
{
	/*printf(">Generation du code d'un objet : %s\n", classe->nom);*/

	LMethodeP lmethodes = classe->lmethodes;

	while(lmethodes != NULL)
	{
		codeDeclMethode(lmethodes->methode);
		lmethodes = lmethodes->next; /*attention ça compilait pas ? */
	}


	LVarDeclP lchamps = classe->lchamps;
	codeDeclChamp(lchamps);  /*TODO */
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
			/*get adresse de la variable Id
			Comme ça :
			if (in_method) PUSHL_addr(tree->u.str); 
			else PUSHG_addr(tree->u.str)

			Il faut comprendre pourquoi on pushl dans une méthode et on pushg autre part ?
			pushl : push la valeur vers le pointeur fp (voir poly genCode)

			fp est le pointeur de la pile après qu'on ait fait START (affecte sp à fp)

			*/

			/*TODO !!!!!!!!!!!!*/
			fprintf(output, "PUSHG %s.adresse()\n", tree->u.str);

			break;

		/*instanciation/constructeur*/
		case EINST: 
			printf("Instanciation : EINST\n"); 

			codeConstructeur(tree);
			/*TODO*/     
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
	fprintf(output, "%s:\t", labelElse);
 
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
	fprintf(output, "%s:\tNOP\n", labelEndIf);
}


/* 
 * Génère le code d'une instruction.
 */
void codeInstr(TreeP tree)
{

	switch (tree->op) {
	case YEXPR:                     		/* Expr;*/

		codeExpr(getChild(tree, 0));   
		printf("YEXPR\n");
	   														/*TODO : il faut un POPN(1) ici; a quoi sert-il ?*/
		break;

	/*atteignable à travers Bloc ;*/
	/*Generation du code d'un bloc*/
	case YCONT:                 			/*Contenu := LDeclChamp IS LInstr */

		fprintf(output, "--########DEBUG : Bloc de type LDeclChamp IS LInstr--\n");
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
	else if(expr->op == Id)				/*TODO*/
	{


		if(verbose) fprintf(output, "\n--%s affiche l'id suivant :\n", getChild(methodeC,0)->u.str);
		
		/*PUSHL("tree.adresse()");*/	/*TODO*/
		PUSHA("toString");
		CALL();
		
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
	
	fprintf(output, "--IL Y AURA UN ENVOI vers %s d'operateur %d\n", getChild(tree, 0)->u.str,  getChild(tree, 0)->op);
	TreeP Expr = getChild(tree, 0);										
	TreeP MethodeC = getChild(tree, 1);
	int tailleParam = 0;

	bool printOK = FALSE;
	printOK = codePrint(Expr, MethodeC);			/*Si la méthode est print ou println, on génère son code*/

	if(printOK == FALSE)							/*Si la méthode n'est pas un print ou println*/
	{
		/*On exécuter le code de l'expression*/
		/*?TODO?? ????????????????????????????????????????????????????*/
		codeExpr(Expr);
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
				if(verbose) fprintf(output, "--Envoi : méthode %s de la class %s\n", tempMethode->nom, classeEnvoi->nom);

				tailleParam = 0; 
				while(tempParam != NULL) 
				{
					PUSHG(tailleParam);  /*empiler l'argument ?????????????????????????????????????????????????????*/ 
					tailleParam++;
					tempParam = tempParam->next;
				}

				/*Appel (statique...) de la méthode*/
				char *adresseMethode = tempMethode->nom;
				if(verbose) fprintf(output, "\n--Appel de la methode %s.\n", adresseMethode);
				PUSHA(adresseMethode);
				CALL();

				/*On dépile le nombre de paramètres empilés*/
				POPN(tailleParam);
			}
			else{printf("Erreur envoi : methode introuvable.\n");}

	
		}
		else{printf("Erreur envoi : variable introuvable.\n");}

	}

	/*Créer des méthodes avec des étiquettes uniques ?*/
	/*Question : comment marche l'empilement de l'envoi ?
	est-ce qu'on empile d'abord la partie gauche puis la partie droite ?*/
}

/*Appel de méthode : 
	class C is f(){}
	
	{var monC : C = new
	   monC.f
	}

	START
	PUSHN 1 -- allocation de ta variable monC 
	PUSHG 0
	PUSHA f1 (ça c'est en statique, dynamique plus compliqué que ça mais en principe c'est ça)
	
	CALL 
	POPN (&???)
	STOP

	--fonctions 
	f1: NOP            -methode f   
		corpsf1
		RETURN
	f2: skdfsd
		RETURN

 
	f546456: WRITES   -tostring
			RETURN
*/


/*Génère le code d'une sélection*/
void codeSelec(TreeP tree)		                 /*Selection: Expr '.' Id*/		/*Pb du id dans la production Selection?*/
{                                                /*Voir l'exemple de paire.entiers*/
	/*TODO*/
	/*sélections comment faire ? mettre le x de x.t en paramètres de f ? prof a dit ça*/
	if(verbose) fprintf(output, "--IL Y AURA UNE SELECTION ICI;\n");

	/*
	Voir le type de retour de l'expression
	puis chercher le id dans la classe correspondante*/

	/*cas où Expr est un ident*/
	TreeP Expr = getChild(tree,0);
	TreeP Ident = getChild(tree,1);

	/*cas de select ::= Id . Id*/
	if(Expr->op == Id)
	{
		VarDeclP temp = getVarDeclFromName(Expr->u.str);
		if(temp)
		{
					char *type = temp->type->nom;
			ClasseP classeType = getClassePointer(type);

			/*TODO : savoir si on est dans une méthode ou pas*/
						/*???if (in_method) PUSHL_addr(expr->u.str); else*/ 
						/*???PUSHG_addr(expr->u.str);*/
			fprintf(output,"PUSHG %s.adresse()\n", Expr->u.str);
			
			int offset = getOffset(classeType,Ident->u.str);
			LOAD(offset);
		}
	}

	/*cas de select ::= expr . Id*/
	else 
	{
		codeExpr(getChild(tree, 0));
		/*TODO trouver la classe de Expr :( parcourir la liste d'instances ? */
		ClasseP classe = getInstFromName(getChild(tree,1)->u.str)->type;
		LOAD(getOffset(classe, getChild(tree,1)->u.str));
	}

}


/*Code d'une affectation x := y*/
void codeAff(TreeP tree)    									/*TODO : il faudrait un boolean qui dise si on est dans une méthode ou pas*/
{
	/*membre gauche de l'affectation gauche := droit*/
	TreeP gauche = getChild(tree, 0);

	/*membre droit de l'affectation*/
	TreeP droit = getChild(tree, 1);

	/*pour l'instant on gère les affectations de constante et d'id*/

	if( getChild(tree, 0)->op == Id)
	{
		if(getChild(tree, 1)->op == Cste)
		{
			fprintf(output, "Affectation : constante %d\n", droit->u.val);
			codeExpr(droit);
			/*STOREG(gauche.getAdresse());*/
			fprintf(output,"STOREG %s.adresse()\n", gauche->u.str);
		}
		else
		{
			fprintf(output, "Affectation : ident %s\n", droit->u.str);
			codeExpr(droit);
			/*STOREG(gauche.getAdresse());*/
			fprintf(output,"STOREG %s.adresse()\n", gauche->u.str);
		}
	}

/*	if (gauche->op = id et droit->op = eaff)
		on fait constructeur(droit)
		on fait storeg gauche.getadresse()

	if(gauche->op = id et droit->op != eaff)
		on pushl ou pushg id
		on codeExpr(droit)	

	if gauche->op = selection 
		if(gauche.filsgauche = est une classe)	
*/
/*	if (selectorid->op == IDVAR) {

		if (expr->op == EALLOC) {

			codeExpr(expr);
			
			STOREG_addr(selectorid->u.str);
		}
		else {
			
			if (in_method) PUSHL_addr(selectorid->u.str); else PUSHG_addr(selectorid->u.str);
			codeExpr(expr);
			 il faut générer l'adresse de la variable locale ou du champ d'objet 
			
			 STORE(offset);
			 x = 0 si variable locale, x = ? offset du champ de classe à déterminer 
		}
	}

	else if (selectorid->op == EPOINT) {
		TreeP var = getChild(selectorid, 0);
		TreeP field = getChild(selectorid, 1);
		if (var->op == IDVAR) {
			
			if (in_method) PUSHL_addr(var->u.str); else PUSHG_addr(var->u.str);		JE TROUVE CA BIZARRE
			codeExpr(expr, depth+1);
			char* t = getSymbole(var->u.str)->var->type;
			ClassP class = figureClass(t);
			offset = getOffset(class,field->u.str);
		   
			STORE(offset);
		}
		else { c'est fait
		}
	}
*/
}

/*Code d'un bloc objet situé dans les objets isolés et dans les classes*/
void codeBlocObj(TreeP tree)        							/*BlocObj: '{' LDeclChampMethodeOpt '}' */
{

	if(tree != NULL)
	{
		if(tree->nbChildren == 2)  
		{
			/*ptet qu'on ne peut pas appeler codeBlocObj ???*/

			codeBlocObj(getChild(tree, 0));                     /*LDeclChampMethode: LDeclChampMethode DeclChampMethode*/
			codeDeclChampMethode(getChild(tree, 1));            
		}
		else if(tree->nbChildren == 1)
		{
			codeDeclChampMethode(getChild(tree, 0));            /* LDeclChampMethode: DeclChampMethode*/
		}
	}
}

void codeDeclChampMethode(TreeP tree)
{
	printf("Mkay %d\n", tree->op);
	if(tree->op == YDECLC) /*DeclChamp*/
	{
		printf("DECL CHAMP\n");
		codeDeclChamp(tree);
	}

	else if(tree->op == DMETHODE) /*DeclMethode pas besoin de gérer ce cas, 
									vu qu'on met les méthodes dans des structures*/
	{
	}

}


/*Liste de déclarations champ*/
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



	if(verbose) fprintf(output, "--DeclChamp.\n");

	fprintf(output,"--Var %s : ", tree->u.var->nom);

	if(verbose) printf("Ajout de la variable %s à l'environnement...\n", tree->u.var->nom);
	
	/*ajouter la nouvelle instance à la liste*/
	addVarEnv(tree->u.var, NIL(Classe));	/*TODO? Environnement de classe*/

	if(tree->u.var->exprOpt != NULL)
	{
		codeExpr(tree->u.var->exprOpt);  /*instanciation, affectation...*/

	}

	/*il faut STROREG id.adresse à la fin ?*/

  /*comment gérer le type d'une expression en terme de gén de code ?*/


  /*  TreeP valVar = getChild(tree, 2);
	if (valVar)
	{
		codeExpr(valVar);
	}
	fprintf(output, "\n");
	*/
/*  ajoutVariable(var) à l'environnement : initialiser l'adresse de la variable,
	afin de pouvoir la STOREG dans le futur
	
	déclaration ! comment ?
	
	histoire de constructeur ?

	sinon pour une alloc normale on a : 

	alloc 1
	dupn 1
	codeExpr(Valvar);
	store(0)
	*/
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
			if(tree->op == 11) codeLInstr(tree);    /*si les instructions sont sous forme de liste*/
			else codeInstr(tree);                   /*s'il existe qu'une seule instruction*/
			
		}
		else
		{
			 if(verbose) fprintf(output,"\n--Methode avec un type de retour.\n"); /*TODO : pas qqc de general. */
			 codeLInstr(tree);											/*On peut avoir une liste d'instr mais ma fct pour list instr est mal concue.*/
		}

		if(verbose) fprintf(output, "--stocke le resultat a son emplacement\n");
		PUSHL(-(tailleParam+1)); 
		SWAP(); 
		STORE(0);
		CRETURN();
	}

   
}

/*Génère le code d'une classe*/
void codeClasse(ClasseP classe)			
{
	if(verbose) printf(">Generation du code d'une classe : %s\n", classe->nom);

	LMethodeP liste = classe->lmethodes;

	while(liste != NULL)
	{
		codeDeclMethode(liste->methode);
		liste = liste->next;
	}
}


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

/* retourne la taille d'une liste de methodes */
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

void codeTV()
{
	int nbMethodes = 0;
	int cptStore = 0;
	if(verbose) printf(">Generation de la table virtuelle...\n");

	fprintf(output, "--------Initialiser les tables virtuelles.--------\n");
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

	/*
		-- allouer un objet de classe A
	ALLOC 1 -- a
	DUPN 1
	PUSHG 0 -> on met la table virtuelle dans l'objet a
	STORE 0*/

	/*dans le main
		-- invoquer la premiere methode sur a: m1
	PUSHL 0
	PUSHA call1
	CALL
	POPN 1
	-- invoquer la seconde methode sur a: m2
	PUSHL 0
	PUSHA call2
	CALL
	POPN 1


	*/
}


void genCode(TreeP LClass, TreeP Bloc)
{

	output = fopen("bailtest", "a+");

	if (output != NULL) {

		/*Generation du code des méthodes des classes 
		 à partir de la variable globale LClasse*/
		codeLClasse();

		/*Generation de code du bloc principal*/
		fprintf(output, "main: \tSTART\n");
		codeInstr(Bloc);
		fprintf(output, "STOP\n");


		/*saut de ligne nécessaire à la fin du programme, 
		sans quoi il y a une erreur*/
		fprintf(output, "\n");
		codeTV();

		fclose(output);
	}
}