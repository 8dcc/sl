#ifndef PARSER_H_
#define PARSER_H_ 1

#include <stdint.h>

enum EExprType {
    EXPR_ERR,
    EXPR_NIL, /* End of list. Expr.next will be NULL */
    EXPR_PARENT,
    EXPR_SYMBOL,
    EXPR_CONST,
};

typedef struct Expr {
    enum EExprType type;
    union {
        double n;
        char* s;
        struct Expr* children;
    } val;
    struct Expr* next;
} Expr;

/*----------------------------------------------------------------------------*/

/* Convert array of tokens into a expression tree */
Expr* parse(Token* tokens);

/* Print expression in tree format */
void expr_print(Expr* root);

/* Free expression and adjacent expressions with their children */
void expr_free(Expr* root);

#endif /* PARSER_H_ */
