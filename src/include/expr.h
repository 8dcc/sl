
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
    /* Environment for binding the formal arguments to the actual values when
     * calling the lambda. */
    struct Env* env;

    /* Formal arguments. Mandatory go first, then optional, and finally a single
     * one used for the rest. */
    char** formals;
    size_t formals_mandatory;
    size_t formals_optional;
    bool formals_rest;

    /* List of expressions to be evaluated in order when calling the lambda */
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

/* Allocate a new empty expression of the specified type */
Expr* expr_new(enum EExprType type);

/* Free expression and adjacent expressions, along with their children */
void expr_free(Expr* root);

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks */
Expr* expr_clone(const Expr* e);

/* Same as `expr_clone', but also clones children recursivelly */
Expr* expr_clone_recur(const Expr* e);

/* Call `expr_clone_recur' on a linked list of expressions, starting at `e' */
Expr* expr_clone_list(const Expr* e);

/* Print formatted expression */
void expr_print(const Expr* root);

/* Print formatted expression and a newline  */
void expr_println(const Expr* e);

/* Print expression in detailed tree format */
void expr_print_debug(const Expr* e);

/* Calculate number of elements in a linked list of Expr structures */
size_t expr_list_len(const Expr* e);

#endif /* EXPR_H_ */
