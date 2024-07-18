#ifndef PARSER_H_
#define PARSER_H_ 1

struct Token; /* lexer.h */
struct Expr;  /* expr.h */

/* Convert array of tokens into a expression tree */
struct Expr* parse(struct Token* tokens);

#endif /* PARSER_H_ */
