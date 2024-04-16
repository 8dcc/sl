
#ifndef EXPR_H_
#define EXPR_H_ 1

#include <stdbool.h>

typedef struct Env Env;
typedef struct Expr Expr;

typedef Expr* (*PrimitiveFuncPtr)(Env*, Expr*);

enum EExprType {
    EXPR_ERR, /* Unused for now */
    EXPR_CONST,
    EXPR_SYMBOL,
    EXPR_PARENT,
    EXPR_PRIM,
};

struct Expr {
    enum EExprType type;
    union {
        double n;
        char* s;
        Expr* children;
        PrimitiveFuncPtr f;
    } val;

    /* True if this expression was quoted */
    bool is_quoted;

    /* Next expression in the linked list */
    Expr* next;
};

/*----------------------------------------------------------------------------*/

static inline const char* exprtype2str(enum EExprType type) {
    /* clang-format off */
    switch (type) {
        default:
        case EXPR_ERR:    return "Error";
        case EXPR_CONST:  return "Number";
        case EXPR_SYMBOL: return "Symbol";
        case EXPR_PARENT: return "List";
        case EXPR_PRIM:   return "Primitive";
    }
    /* clang-format on */
}

/* Print expression in tree format */
void expr_print(Expr* root);

/* Free expression and adjacent expressions, along with their children */
void expr_free(Expr* root);

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks. */
Expr* expr_clone(const Expr* e);

/* Same as `expr_clone', but also clones children recursivelly. */
Expr* expr_clone_recur(const Expr* e);

#endif /* EXPR_H_ */
