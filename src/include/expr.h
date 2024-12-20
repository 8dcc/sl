/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EXPR_H_
#define EXPR_H_ 1

#include <stddef.h>
#include <stdbool.h>
#include <stdio.h> /* FILE, fputc() */

#include "error.h" /* SL_FATAL() */

struct Env;       /* env.h */
struct LambdaCtx; /* lambda.h */

/*----------------------------------------------------------------------------*/

/* Pointer to a primitive C function. */
typedef struct Expr* (*PrimitiveFuncPtr)(struct Env*, struct Expr*);

enum EExprType {
    EXPR_UNKNOWN = 0,
    EXPR_NUM_INT = (1 << 0),
    EXPR_NUM_FLT = (1 << 1),
    EXPR_ERR     = (1 << 2),
    EXPR_SYMBOL  = (1 << 3),
    EXPR_STRING  = (1 << 4),
    EXPR_PARENT  = (1 << 5),
    EXPR_PRIM    = (1 << 6),
    EXPR_LAMBDA  = (1 << 7),
    EXPR_MACRO   = (1 << 8),
};

typedef struct Expr Expr;
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

/* Expression predicates */
#define EXPRP_ERR(E)    ((E)->type == EXPR_ERR)
#define EXPRP_INT(E)    ((E)->type == EXPR_NUM_INT)
#define EXPRP_FLT(E)    ((E)->type == EXPR_NUM_FLT)
#define EXPRP_SYM(E)    ((E)->type == EXPR_SYMBOL)
#define EXPRP_STR(E)    ((E)->type == EXPR_STRING)
#define EXPRP_LST(E)    ((E)->type == EXPR_PARENT)
#define EXPRP_PRIM(E)   ((E)->type == EXPR_PRIM)
#define EXPRP_LAMBDA(E) ((E)->type == EXPR_LAMBDA)
#define EXPRP_MACRO(E)  ((E)->type == EXPR_MACRO)

#define EXPRP_NUMBER(E)     (EXPRP_INT(E) || EXPRP_FLT(E))
#define EXPRP_APPLICABLE(E) (EXPRP_PRIM(E) || EXPRP_LAMBDA(E) || EXPRP_MACRO(E))

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
 * Is the specified expression an empty list? Note that the empty list is also
 * used to represent false in functions that return predicates.
 */
bool expr_is_nil(const Expr* e);

/*
 * Check if two linked lists of `Expr' structures are identical in length and
 * content using `expr_equal'.
 */
bool expr_list_equal(const Expr* a, const Expr* b);

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
 * Count the number of elements in a linked list of `Expr' structures.
 */
size_t expr_list_len(const Expr* e);

/*
 * Is the expression `e' inside the linked list `lst'? Checks using
 * `expr_equal'.
 */
bool expr_list_is_member(const Expr* lst, const Expr* e);

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
 * Uses the `EXPRP_NUMBER' inline function.
 * See also the `expr_list_has_only_type' inline function below.
 */
bool expr_list_has_only_numbers(const Expr* e);

/*
 * Does the specified linked list contain ONLY expressions with the specified
 * type?
 */
static inline bool expr_list_has_only_type(const Expr* e, enum EExprType type) {
    return expr_list_is_homogeneous(e) && e->type == type;
}

/*----------------------------------------------------------------------------*/

/*
 * Print a linked list of expressions using `expr_print', wrapped in
 * parentheses.
 */
void expr_list_print(FILE* fp, const Expr* e);

/*
 * Print an expression in a human-friendly form.
 */
void expr_print(FILE* fp, const Expr* e);

/*
 * Print an expression in a way suitable for `read'.
 */
bool expr_write(FILE* fp, const Expr* e);

/*
 * Print a linked list of expressions in a tree form, for debugging.
 */
void expr_print_debug(FILE* fp, const Expr* e);

/*
 * Print an expression and a newline character ('\n').
 */
static inline void expr_println(FILE* fp, const Expr* e) {
    expr_print(fp, e);
    fputc('\n', fp);
}

/*
 * Return a string literal representing the specified expression type.
 */
static inline const char* exprtype2str(enum EExprType type) {
    /* clang-format off */
    switch (type) {
        case EXPR_UNKNOWN: return "Unknown";
        case EXPR_NUM_INT: return "Integer";
        case EXPR_NUM_FLT: return "Float";
        case EXPR_ERR:     return "Error";
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

/*----------------------------------------------------------------------------*/

/*
 * Generic numeric type, used by `expr_set_generic_num' and
 * `expr_get_generic_num'.
 */
typedef double GenericNum;
#define EXPR_NUM_GENERIC EXPR_NUM_FLT

/*
 * Set the value and type of an expression of type EXPR_NUM_GENERIC.
 */
static inline void expr_set_generic_num(Expr* e, GenericNum num) {
    /* NOTE: This should change if `GenericNum' changes. */
    e->val.f = num;
}

/*
 * Get the value of a numeric expression in a generic C type. The expression
 * should be a number according to `EXPRP_NUMBER'.
 */
static inline GenericNum expr_get_generic_num(const Expr* e) {
    switch (e->type) {
        case EXPR_NUM_INT:
            return (GenericNum)e->val.n;
        case EXPR_NUM_FLT:
            return (GenericNum)e->val.f;
        default:
            SL_FATAL("Unhandled numeric case (%s).", exprtype2str(e->type));
    }
}

static inline void expr_negate_num_val(Expr* e) {
    switch (e->type) {
        case EXPR_NUM_INT:
            e->val.n *= -1;
            break;
        case EXPR_NUM_FLT:
            e->val.f *= -1;
            break;
        default:
            SL_FATAL("Tried negating a non-numeric expression (%s).",
                     exprtype2str(e->type));
    }
}

/*----------------------------------------------------------------------------*/

#endif /* EXPR_H_ */
