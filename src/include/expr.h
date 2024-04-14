
#ifndef EXPR_H_
#define EXPR_H_ 1

#include <stdint.h>

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

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks. */
Expr* expr_clone(const Expr* e);

/* Same as `expr_clone', but also clones children recursivelly. */
Expr* expr_clone_recur(const Expr* e);

#endif /* EXPR_H_ */
