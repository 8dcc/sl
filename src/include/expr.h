
#ifndef EXPR_H_
#define EXPR_H_ 1

#include <stdbool.h>

struct Env; /* env.h */

typedef struct Expr Expr;
typedef struct LambdaCtx LambdaCtx;

typedef Expr* (*PrimitiveFuncPtr)(struct Env*, Expr*);

enum EExprType {
    EXPR_ERR,
    EXPR_CONST,
    EXPR_SYMBOL,
    EXPR_PARENT,
    EXPR_PRIM,
    EXPR_LAMBDA,
};

struct LambdaCtx {
    struct Env* env;
    char** formals;
    size_t formals_num;
    Expr* body;
};

struct Expr {
    /* Type and value of the expression */
    enum EExprType type;
    union {
        double n;
        char* s;
        Expr* children;
        PrimitiveFuncPtr prim;
        LambdaCtx* lambda;
    } val;

    /* Next expression in the linked list */
    Expr* next;
};

/*----------------------------------------------------------------------------*/

static inline const char* exprtype2str(enum EExprType type) {
    /* clang-format off */
    switch (type) {
        case EXPR_ERR:    return "Error";
        case EXPR_CONST:  return "Number";
        case EXPR_SYMBOL: return "Symbol";
        case EXPR_PARENT: return "List";
        case EXPR_PRIM:   return "Primitive";
        case EXPR_LAMBDA: return "Lambda";
    }
    /* clang-format on */

    /* Should be unreachable. Compiler warns about unhandled cases in the
     * previous switch, since we are not using `default'. */
    return "???";
}

/* Print formatted expression */
void expr_print(Expr* root);

/* Print formatted expression and a newline  */
void expr_println(Expr* e);

/* Print expression in detailed tree format */
void expr_print_debug(Expr* e);

/* Free expression and adjacent expressions, along with their children */
void expr_free(Expr* root);

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks. */
Expr* expr_clone(const Expr* e);

/* Same as `expr_clone', but also clones children recursivelly. */
Expr* expr_clone_recur(const Expr* e);

#endif /* EXPR_H_ */
