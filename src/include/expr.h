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

#include "lisp_types.h" /* LispInt, LispFlt, GenericNum */
#include "error.h"      /* SL_FATAL() */

struct Env;       /* env.h */
struct LambdaCtx; /* lambda.h */

/*----------------------------------------------------------------------------*/
/* Types and enums */

/*
 * Pointer to a Lisp primitive; that is, a C function that operates with
 * expressions and that can be called from Lisp.
 */
typedef struct Expr* (*PrimitiveFuncPtr)(struct Env*, struct Expr*);

/*
 * Possible expression types. They are mutually exclusive (i.e. an expression
 * can only have one type at a time), but we still use distinct bits for
 * checking multiple expression types at once. For example, we can easily check
 * if an expression is a number with:
 *
 *     (e->type & (EXPR_NUM_INT | EXPR_NUM_FLT)) != 0
 *
 * See also 'EXPR_NUM_GENERIC', defined above.
 */
enum EExprType {
    EXPR_UNKNOWN = 0,
    EXPR_NUM_INT = (1 << 0),
    EXPR_NUM_FLT = (1 << 1),
    EXPR_ERR     = (1 << 2),
    EXPR_SYMBOL  = (1 << 3),
    EXPR_STRING  = (1 << 4),
    EXPR_PAIR    = (1 << 5),
    EXPR_PRIM    = (1 << 6),
    EXPR_LAMBDA  = (1 << 7),
    EXPR_MACRO   = (1 << 8),
};

/*
 * Expression type whose value has the same C type as 'GenericNum'.
 *
 * Expressions with 'EXPR_NUM_GENERIC' as the type are, for example, returned by
 * arithmetic functions when their parameters don't share a common type
 * (e.g. when calling '+' with an Integer and a Float).
 */
SL_ASSERT_TYPES(GenericNum, LispFlt);
#define EXPR_NUM_GENERIC EXPR_NUM_FLT

/*
 * Structure used to represent a pair of expressions.
 *
 * TODO: Link article about cons pairs.
 * TODO: We could store indexes in the pool instead of pointers to save memory.
 */
typedef struct ExprPair ExprPair;
struct ExprPair {
    struct Expr* car;
    struct Expr* cdr;
};

/*
 * The main expression type. This will be used to hold basically all data in our
 * Lisp.
 *
 * The 'type' member will determine what member we should access in the 'val'
 * union. Some types use the same union member (e.g. EXPR_STRING and
 * EXPR_SYMBOL). See the enum above for more information.
 *
 * Note that the expressions whose value is allocated (e.g. EXPR_STRING,
 * EXPR_LAMBDA, etc.) should own a unique pointer that is not being used by any
 * other expression. Therefore, we should be able to modify or free these
 * pointers without affecting other expressions.
 */
typedef struct Expr Expr;
struct Expr {
    enum EExprType type;
    union {
        LispInt n;
        LispFlt f;
        char* s;
        ExprPair pair;
        PrimitiveFuncPtr prim;
        struct LambdaCtx* lambda;
    } val;
};

/*----------------------------------------------------------------------------*/
/* Callable macros */

/* Expression predicates */
/* TODO: Grep for "->type", replace with these macros */
#define EXPR_ERR_P(E)    ((E)->type == EXPR_ERR)
#define EXPR_INT_P(E)    ((E)->type == EXPR_NUM_INT)
#define EXPR_FLT_P(E)    ((E)->type == EXPR_NUM_FLT)
#define EXPR_SYM_P(E)    ((E)->type == EXPR_SYMBOL)
#define EXPR_STR_P(E)    ((E)->type == EXPR_STRING)
#define EXPR_PAIR_P(E)   ((E)->type == EXPR_PAIR)
#define EXPR_PRIM_P(E)   ((E)->type == EXPR_PRIM)
#define EXPR_LAMBDA_P(E) ((E)->type == EXPR_LAMBDA)
#define EXPR_MACRO_P(E)  ((E)->type == EXPR_MACRO)

#define EXPR_NUMBER_P(E) (EXPR_INT_P(E) || EXPR_FLT_P(E))
#define EXPR_APPLICABLE_P(E)                                                   \
    (EXPR_PRIM_P(E) || EXPR_LAMBDA_P(E) || EXPR_MACRO_P(E))

/*
 * List-related macros. Make sure you check the expression type before calling
 * them.
 */
#define CAR(E)  ((E)->val.pair.car)
#define CDR(E)  ((E)->val.pair.cdr)
#define CADR(E) (CAR(CDR(E)))
#define CDDR(E) (CDR(CDR(E)))

/*----------------------------------------------------------------------------*/
/* Functions for creating expressions */

/*
 * Allocate and initialize a new empty expression of the specified type.
 * Wrapper for 'pool_alloc_or_expand'.
 */
Expr* expr_new(enum EExprType type);

/*
 * Clone the specified 'Expr' structure into an allocated copy, and return it.
 *
 * In the case of pairs, it copies the references, doesn't clone recursively. To
 * clone recursively, use 'expr_clone_recur'.
 */
Expr* expr_clone(const Expr* e);

/*
 * Same as 'expr_clone', but also clones pairs recursivelly.
 *
 * TODO: Rename to 'expr_tree_clone', update comments where this function is
 * mentioned.
 */
Expr* expr_clone_recur(const Expr* e);

/*----------------------------------------------------------------------------*/
/* Predicates for expressions */

/*
 * Is the specified expression an empty list? Note that the empty list is also
 * used to represent false in functions that return predicates.
 */
bool expr_is_nil(const Expr* e);

/*
 * Return true if 'a' and 'b' have the same effective value.
 */
bool expr_equal(const Expr* a, const Expr* b);

/*
 * Return true if 'a' is lesser/greater than 'b'.
 */
bool expr_lt(const Expr* a, const Expr* b);
bool expr_gt(const Expr* a, const Expr* b);

/*----------------------------------------------------------------------------*/
/* Functions for Lisp lists */

/*
 * Is the specified expression a proper Lisp list? A proper list can be either
 * `nil', or one or more chained cons pairs whose last CDR is 'nil'.
 */
bool expr_is_proper_list(const Expr* e);

/*
 * Count the number of elements the specified list.
 */
size_t expr_list_len(const Expr* list);

/*
 * Return a pointer to the N-th element of the specified list. The returned
 * expression is the N-th "car", not the N-th "pair".
 *
 * The 'n' argument is supposed to be one-indexed and smaller than the size of
 * the list (according to 'expr_list_len'), or an assertion will fail.
 */
Expr* expr_list_nth(const Expr* list, size_t n);

/*
 * Does the specified list contain the specified expression? The check is
 * performed using 'expr_equal'.
 *
 * TODO: Swap argument order, just like Scheme's `member' functions.
 * TODO: Add 'expr_member' function that returns the reference, like Scheme's
 * `member'. Make 'expr_is_member' inline for checking if it returned NULL or
 * not.
 */
bool expr_is_member(const Expr* list, const Expr* e);

/*
 * Is the specified list homogeneous? In other words, do all elements share the
 * same type?
 */
bool expr_list_is_homogeneous(const Expr* list);

/*
 * Does the specified list contain at least one expression with the specified
 * type?
 */
bool expr_list_has_type(const Expr* list, enum EExprType type);

/*
 * Does the specified list contain ONLY expressions with numeric types? Uses the
 * 'EXPR_NUMBER_P' macro, defined above.
 */
bool expr_list_has_only_numbers(const Expr* list);

/*
 * Does the specified list contain ONLY proper lists? Uses the
 * 'expr_is_proper_list' function.
 */
bool expr_list_has_only_lists(const Expr* list);

/*
 * Does the specified linked list contain ONLY expressions with the specified
 * type?
 */
static inline bool expr_list_has_only_type(const Expr* list,
                                           enum EExprType type) {
    return expr_list_is_homogeneous(list) && CAR(list)->type == type;
}

/*----------------------------------------------------------------------------*/
/* Expression functions for I/O */

/*
 * Print an expression in a human-friendly form.
 */
bool expr_print(FILE* fp, const Expr* e);

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
        case EXPR_PAIR:    return "Pair";
        case EXPR_PRIM:    return "Primitive";
        case EXPR_LAMBDA:  return "Lambda";
        case EXPR_MACRO:   return "Macro";
    }
    /* clang-format on */

    __builtin_unreachable();
}

/*----------------------------------------------------------------------------*/
/* Numerical functions */

/*
 * Set the value and type of an expression of type EXPR_NUM_GENERIC.
 */
static inline void expr_set_generic_num(Expr* e, GenericNum num) {
    SL_ASSERT_TYPES(GenericNum, LispFlt);
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

/*
 * Negate the value of an expression depending on its type.
 */
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

#endif /* EXPR_H_ */
