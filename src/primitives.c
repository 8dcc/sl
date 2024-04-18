
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/env.h"
#include "include/primitives.h"

#define EXPECT_TYPE(EXPR, TYPE)                              \
    SL_ASSERT((EXPR)->type == TYPE,                          \
              "Expected expression of type '%s', got '%s'.", \
              exprtype2str(TYPE), exprtype2str((EXPR)->type));

/*----------------------------------------------------------------------------*/
/* Special Form primitives. See SICP Chapter 4.1.1 */

Expr* prim_quote(Env* env, Expr* e) {
    UNUSED(env);

    /* We have to use `expr_clone_recur' since `expr_clone' does not clone
     * children. This function is useful for binding it to `quote' in the
     * environment. */
    return expr_clone_recur(e);
}

/*----------------------------------------------------------------------------*/
/* Primitives that should have their parameters evaluated by the caller */

Expr* prim_define(Env* env, Expr* e) {
    /* The `define' function will bind the even arguments to the odd arguments.
     * It expects an even number of arguments.
     *
     * Returns the last bound expression. */
    const Expr* last_bound = NULL;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_ASSERT(arg->next != NULL, "Got an odd number of arguments.");

        /* Odd argument: Symbol */
        EXPECT_TYPE(arg, EXPR_SYMBOL);
        const char* sym = arg->val.s;

        /* Even argument: Expression */
        arg             = arg->next;
        const Expr* val = arg;

        env_bind(env, sym, val);
        last_bound = val;
    }

    return (last_bound == NULL) ? NULL : expr_clone_recur(last_bound);
}

Expr* prim_add(Env* env, Expr* e) {
    UNUSED(env);
    SL_ASSERT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total += arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    UNUSED(env);
    SL_ASSERT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total -= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_mul(Env* env, Expr* e) {
    UNUSED(env);
    SL_ASSERT(e != NULL, "Missing arguments.");

    double total = 1;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total *= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_div(Env* env, Expr* e) {
    UNUSED(env);
    SL_ASSERT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_ASSERT(arg->val.n != 0, "Trying to divide by zero.");

        total /= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
