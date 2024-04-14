#ifndef PARSER_H_
#define PARSER_H_ 1

#include "lexer.h" /* Token */
#include "expr.h"  /* Expr */

/* Convert array of tokens into a expression tree */
Expr* parse(Token* tokens);

#endif /* PARSER_H_ */
