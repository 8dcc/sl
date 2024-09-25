
#ifndef EXPR_H_
#define EXPR_H_ 1

#include <stddef.h>
#include <stdbool.h>

struct Env;       /* env.h */
struct LambdaCtx; /* lambda.h */

typedef struct Expr Expr;

typedef Expr* (*PrimitiveFuncPtr)(struct Env*, Expr*);

enum EExprType {
    EXPR_ERR,
    EXPR_NUM_INT,
    EXPR_NUM_FLT,
    EXPR_SYMBOL,
    EXPR_STRING,
    EXPR_PARENT,
    EXPR_PRIM,
    EXPR_LAMBDA,
    EXPR_MACRO,
};

struct Expr {
    /* Type and value of the expression */
    enum EExprType type;
    union {
        long long n;
        double f;
        char* s;
        Expr* children;
        PrimitiveFuncPtr prim;
        struct LambdaCtx* lambda;
    } val;

    /* Next expression in the linked list */
    Expr* next;
};

/*----------------------------------------------------------------------------*/

static inline const char* exprtype2str(enum EExprType type) {
    /* clang-format off */
    switch (type) {
        case EXPR_ERR:     return "Error";
        case EXPR_NUM_INT: return "Integer";
        case EXPR_NUM_FLT: return "Float";
        case EXPR_SYMBOL:  return "Symbol";
        case EXPR_STRING:  return "String";
        case EXPR_PARENT:  return "List";
        case EXPR_PRIM:    return "Primitive";
        case EXPR_LAMBDA:  return "Lambda";
        case EXPR_MACRO:   return "Macro";
    }
    /* clang-format on */

    __builtin_unreachable();
}

static inline bool expr_is_number(const Expr* e) {
    return e->type == EXPR_NUM_INT || e->type == EXPR_NUM_FLT;
}

static inline bool expr_is_applicable(const Expr* e) {
    return e->type == EXPR_PRIM || e->type == EXPR_LAMBDA ||
           e->type == EXPR_MACRO;
}

/*----------------------------------------------------------------------------*/

/* Allocate a new empty expression of the specified type */
Expr* expr_new(enum EExprType type);

/* Free expression and adjacent expressions, along with their children */
/* TODO: Make separate expr_list_free() function? */
void expr_free(Expr* e);

/*----------------------------------------------------------------------------*/

/* Allocate a new expr and copy `e'. Needed to avoid double frees and leaks */
Expr* expr_clone(const Expr* e);

/* Same as `expr_clone', but also clones children recursivelly */
Expr* expr_clone_recur(const Expr* e);

/* Call `expr_clone_recur' on a linked list of expressions, starting at `e' */
Expr* expr_list_clone(const Expr* e);

/*----------------------------------------------------------------------------*/

/* Print formatted expression */
void expr_print(const Expr* e);
void expr_println(const Expr* e);
void expr_print_debug(const Expr* e);

/*----------------------------------------------------------------------------*/

/* Calculate number of elements in a linked list of Expr structures */
size_t expr_list_len(const Expr* e);

/* Does the specified linked list contain an expression with the specified
 * type? */
bool expr_list_contains_type(const Expr* e, enum EExprType type);

/* Does the specified linked list ONLY contain expressions with the specified
 * type? */
bool expr_list_only_contains_type(const Expr* e, enum EExprType type);

/* Does the specified linked list contain only numeric types? */
bool expr_list_only_contains_numbers(const Expr* e);

/*----------------------------------------------------------------------------*/

/* Is the specified expression an empty list? */
bool expr_is_nil(const Expr* e);

/* Return true if `a' and `b' have the same effective value */
bool expr_equal(const Expr* a, const Expr* b);

/* Return true if `a' is lesser/greater than `b' */
bool expr_lt(const Expr* a, const Expr* b);
bool expr_gt(const Expr* a, const Expr* b);

#endif /* EXPR_H_ */
