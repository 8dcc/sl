#ifndef PARSER_H_
#define PARSER_H_ 1

#include <stdint.h>

#include "lexer.h"

enum EExprType {
    EXPR_ERR, /* Unused for now */
    EXPR_CONST,
    EXPR_SYMBOL,
    EXPR_PARENT,
};

typedef struct Expr {
    enum EExprType type;
    union {
        double n;
        char* s;
        struct Expr* children;
    } val;

    /* True if this expression was quoted */
    bool is_quoted;

    /* Next expression in the linked list */
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
