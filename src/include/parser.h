#ifndef PARSER_H_
#define PARSER_H_ 1

#include "lexer.h" /* Token */
#include "expr.h"  /* Expr */

/* Convert array of tokens into a expression tree */
Expr* parse(Token* tokens);

/* Print expression in tree format */
void expr_print(Expr* root);

/* Free expression and adjacent expressions with their children */
void expr_free(Expr* root);

#endif /* PARSER_H_ */
