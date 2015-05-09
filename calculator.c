#include <locale.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	NUMBER = 'N', LOG = 'L',
	PLUS = '+', MINUS = '-',
	MUL = '*', DIV = '/',
	POW = '^', LP = '(',
	RP = ')', END = 'E'
} TokenType;

typedef struct _Token {
	TokenType type;
	double value;
	struct _Token* next;
} Token;

typedef enum {
	NODE_NUMBER = 'N', OLOG = 'L',
	OPLUS = '+', OMINUS = '-',
	OMUL = '*', ODIV = '/',
	OPOW = '^', UMINUS = 'M',
	ERROR = 'E'
} NodeType;

typedef struct _AST {
	NodeType type;
	double value;
	struct _AST* left;
	struct _AST* right;
} AST;

void freeList(Token* token) {
	while (token) {
		Token* next = token->next;
		free(token);
		token = next;
	}
}
void freeTree(AST* ast) {
	if(ast){
		if(ast->left)
			freeTree(ast->left);
		if(ast->right)
			freeTree(ast->right);
		free(ast);
	}
}

int parseToken(char** str, Token** token) {
	*token = (Token*)malloc(sizeof(Token));
	while (**str && (**str == ' ' || **str == '\n'))
		++(*str);
	switch (**str) {
	case '\0':
		free(*token);
		*token = NULL;
		return 1;

	case '+':
		(*token)->type = PLUS;
		++(*str);
		break;
	case '-':
		(*token)->type = MINUS;
		++(*str);
		break;
	case '*':
		(*token)->type = MUL;
		++(*str);
		break;
	case '/':
		(*token)->type = DIV;
		++(*str);
		break;
	case '^':
		(*token)->type = POW;
		++(*str);
		break;
	case '(':
		(*token)->type = LP;
		++(*str);
		break;
	case ')':
		(*token)->type = RP;
		++(*str);
		break;

	default: {
		char dec_point = localeconv()->decimal_point[0];
		if (**str == dec_point || (**str >= '0' && **str <= '9')) {
			(*token)->type = NUMBER;
			sscanf_s(*str, "%lf", &((*token)->value));
			do {
				++(*str);
			} while (**str == dec_point || (**str >= '0' && **str <= '9'));
		}
		else if (strncmp(*str, "log", 3) == 0) {
			(*token)->type = LOG;
			*str += 3;
		}
		else {
			free(*token);
			*token = NULL;
			return 0;
		}
		break;
	}
	}
	return 1;
}

Token* lexer(char** str) {
	Token* first;
	Token* current;
	parseToken(str, &first);
	current = first;
	while (current) {
		Token* next;
		if (!parseToken(str, &next)) {
			current->next = NULL;
			freeList(first);
			return NULL;
		}
		current->next = next;
		current = next;
	}
	return first;
}

void nextToken(Token* token){
	if(token->next){
		token->type = token->next->type;
		token->value = token->next->value;
		token->next = token->next->next;
	}
	else
		token->type = END;


}
AST* makeNode(NodeType type, AST* left, AST* right){
	AST* tmp;
	tmp = (AST*)malloc(sizeof(AST));
	tmp->type = type;
	tmp->value = 0;
	tmp->left = left;
	tmp->right = right;
	return tmp;
}
AST* makeNodeUnary(NodeType type, AST* left){
	AST* tmp;
	tmp = (AST*)malloc(sizeof(AST));
	tmp->type = type;
	tmp->value = 0;
	tmp->left = left;
	tmp->right = NULL;
	return tmp;
}
AST* makeNodeNumber(double value){
	AST* tmp;
	tmp = (AST*)malloc(sizeof(AST));
	tmp->type = NODE_NUMBER;
	tmp->value = value;
	tmp->left = NULL;
	tmp->right = NULL;
	return tmp;
}
AST* parseMul(Token* token);
AST* parsePow(Token* token);
AST* parseMul1(Token* token);
AST* parsePow1(Token* token);
AST* parseSum1(Token* token);
AST* parseTerm(Token* token);

AST* parseSum(Token* token) {
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	left = parseMul(token);
	right = parseSum1(token);
	return makeNode(OPLUS, left, right);
}
AST* parseSum1(Token* token) {
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	switch(token->type){
	case PLUS:
		nextToken(token);
		left = parseMul(token);
		right = parseSum1(token);
		return makeNode(OPLUS, left, right);
	case MINUS:
		nextToken(token);
		left = parseMul(token);
		right = parseSum1(token);
		return makeNode(OMINUS, left, right);
	}
	return makeNodeNumber(0);
}
AST* parseMul(Token* token){
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	left = parsePow(token);
	right = parseMul1(token);
	return makeNode(OMUL, left, right);
}
AST* parseMul1(Token* token) {	
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	switch(token->type){
	case MUL:
		nextToken(token);
		left = parsePow(token);
		right = parseMul1(token);
		return makeNode(OMUL, left, right);
	case DIV:
		nextToken(token);
		left = parsePow(token);
		right = parseMul1(token);
		return makeNode(ODIV, left, right);
	}
	return makeNodeNumber(1);
}
AST* parsePow(Token* token){
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	left = parseTerm(token);
	right = parsePow1(token);
	return makeNode(OPOW, left, right);
}
AST* parsePow1(Token* token){	
	AST* left;
	AST* right;
	left = (AST*)malloc(sizeof(AST));
	right = (AST*)malloc(sizeof(AST));
	if(token->type == POW){
		nextToken(token);
		left = parseTerm(token);
		right = parsePow1(token);
		return makeNode(OPOW, left, right);
	}
	return makeNodeNumber(1);
}
AST* parseTerm(Token* token){
	AST* tmp;
	double value;
	tmp = (AST*)malloc(sizeof(AST));	
	switch(token->type){
	case NUMBER:
		value = token->value;
		nextToken(token);
		return makeNodeNumber(value);
	case MINUS:
		nextToken(token);
		tmp = parseTerm(token);
		return makeNodeUnary(UMINUS, tmp);
	case LOG:
		nextToken(token);
		tmp = parseTerm(token);
		return makeNodeUnary(OLOG, tmp);
	case LP:
		nextToken(token);
		tmp = parseSum(token);
		if(token->type != RP)
			return makeNode(ERROR, NULL, NULL);
		nextToken(token);
		return tmp;
	default:
		nextToken(token);
		return makeNode(ERROR, NULL, NULL);
	}
}
double evaluate(AST* ast){
	if(ast)
		switch(ast->type){
			case NODE_NUMBER:
				return ast->value;
			case UMINUS:
				return -evaluate(ast->left);
			case OLOG:
				return log(evaluate(ast->left));
			case OPLUS:
				return evaluate(ast->left)+evaluate(ast->right);
			case OMINUS:
				return evaluate(ast->right)-evaluate(ast->left);
			case OMUL:
				return evaluate(ast->left)*evaluate(ast->right);
			case ODIV:
				return evaluate(ast->right)/evaluate(ast->left);
			case OPOW:
				return pow(evaluate(ast->left),evaluate(ast->right));
		}
}
int checkTree(AST* ast);
double eval(char* str, double* res) {
	AST* root;
	double result;
	Token* tokens = lexer(&str);
	if(tokens == NULL)
		return 0;
	root = parseSum(tokens);
	if(checkTree(root))
		return 0;
	result = evaluate(root);	
	freeTree(root);
	freeList(tokens);
	*res = result;
	return 1;
}

int checkTree(AST* ast){
	if(ast->type == ERROR)
			return 1;
		if((ast->type == OLOG) || (ast->type == UMINUS)){
			if(ast->left == NULL)
				return 1;
			else if(checkTree(ast->left))
				return 1;
		}
		else if(ast->type != NODE_NUMBER){
			if((ast->left == NULL)||(ast->right == NULL))
				return 1;
			else if((checkTree(ast->left))||(checkTree(ast->right)))
				return 1;
		}
		return 0;
}

int main() {
	char str[256];
	double result;
	setlocale(LC_ALL, "Russian");	
	while(1){
		printf("Введите выражение: \n(\"exit\" для выхода)\n");
		fgets(str, sizeof(str), stdin);
		if (strcmp(str, "exit\n") == 0) 
			break;
		if (eval(str, &result)) {
			printf("Результат: %lf\n", result);
		}
		else {
			puts("Некорректный ввод");
		}
	}	
}