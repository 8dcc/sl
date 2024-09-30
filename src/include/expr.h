
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

/*
 * Allocate a new empty expression of the specified type. The returned pointer
 * should be freed by the caller using `expr_free'.
 */
Expr* expr_new(enum EExprType type);

/*
 * Free expression, along with their children, if any.
 *
 * Doesn't free adjacent expressions (i.e. ignores `e->next'). For freeing a
 * linked list of expressions, use `expr_list_free'.
 */
void expr_free(Expr* e);

/*
 * Free a linked list of `Expr' structures.
 */
void expr_list_free(Expr* e);

/*----------------------------------------------------------------------------*/

/*
 * Clone the specified `Expr' structure into an allocated copy, and return it.
 * The returned pointer must be freed by the caller using `expr_free'.
 *
 * In the case of lists, it doesn't clone children. To clone recursively, use
 * `expr_clone_recur'.
 *
 * It also doesn't clone adjacent expressions (i.e. `e->next' is ignored and
 * `returned->next' is NULL). To clone a list of `Expr' structures, use
 * `expr_list_clone'.
 */
Expr* expr_clone(const Expr* e);

/*
 * Same as `expr_clone', but also clones children recursivelly.
 */
Expr* expr_clone_recur(const Expr* e);

/*
 * Clones a linked list of `Expr' structures by calling `expr_clone_recur'.
 */
Expr* expr_list_clone(const Expr* e);

/*----------------------------------------------------------------------------*/

/*
 * Print expression in different formats.
 */
void expr_print(const Expr* e);
void expr_println(const Expr* e);
void expr_print_debug(const Expr* e);

/*----------------------------------------------------------------------------*/

/*
 * Count the number of elements in a linked list of `Expr' structures.
 */
size_t expr_list_len(const Expr* e);

/*
 * Check if two linked lists of `Expr' structures are identical in length and
 * content.
 */
bool expr_list_equal(const Expr* a, const Expr* b);

/*
 * Is the specified linked list homogeneous? In other words, do all elements
 * share the same type?
 */
bool expr_list_is_homogeneous(const Expr* e);

/*
 * Does the specified linked list contain at least one expression with the
 * specified type?
 */
bool expr_list_has_type(const Expr* e, enum EExprType type);

/*
 * Does the specified linked list contain ONLY expressions with numeric types?
 *
 * Uses the `expr_is_number' inline function.
 * See also the `expr_list_has_only_type' inline function below.
 */
bool expr_list_has_only_numbers(const Expr* e);

/*----------------------------------------------------------------------------*/

/*
 * Is the specified expression an empty list? Note that the empty list is also
 * used to represent false in functions that return predicates.
 */
bool expr_is_nil(const Expr* e);

/*
 * Return true if `a' and `b' have the same effective value.
 */
bool expr_equal(const Expr* a, const Expr* b);

/*
 * Return true if `a' is lesser/greater than `b'.
 */
bool expr_lt(const Expr* a, const Expr* b);
bool expr_gt(const Expr* a, const Expr* b);

/*----------------------------------------------------------------------------*/

/*
 * Return a string literal representing the specified expression type.
 */
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

/*
 * Check if the specified expression type is a number: Integer or float.
 */
static inline bool exprtype_is_number(enum EExprType type) {
    return type == EXPR_NUM_INT || type == EXPR_NUM_FLT;
}

/*
 * Check if the specified expression is a number. Wrapper for
 * `exprtype_is_number'.
 */
static inline bool expr_is_number(const Expr* e) {
    return exprtype_is_number(e->type);
}

/*
 * Check if the specified expression can be applied (i.e. called): Primitive,
 * lambda or macro.
 */
static inline bool expr_is_applicable(const Expr* e) {
    return e->type == EXPR_PRIM || e->type == EXPR_LAMBDA ||
           e->type == EXPR_MACRO;
}

/*
 * Does the specified linked list contain ONLY expressions with the specified
 * type?
 */
static inline bool expr_list_has_only_type(const Expr* e, enum EExprType type) {
    return expr_list_is_homogeneous(e) && e->type == type;
}

#endif /* EXPR_H_ */
