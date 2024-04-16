
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

static PrimitiveFuncPair non_eval_primitives_list[] = {
    { "quote", prim_quote },    //

    { NULL, NULL }, /* Marks the end */
};

static PrimitiveFuncPair primitives_list[] = {
    { "define", prim_define },    //
    { "+", prim_add },            //
    { "-", prim_sub },            //
    { "*", prim_mul },            //
    { "/", prim_div },            //

    { NULL, NULL }, /* Marks the end */
};

PrimitiveFuncPair* non_eval_primitives = non_eval_primitives_list;
PrimitiveFuncPair* primitives          = primitives_list;

/*----------------------------------------------------------------------------*/
/* Primitives that should not have their parameters evaluated by the caller */

Expr* prim_quote(Expr* e) {
    /* We have to use `expr_clone_recur' since `expr_clone' does not clone
     * children. */
    Expr* cloned      = expr_clone_recur(e);
    cloned->is_quoted = true;
    return cloned;
}

/*----------------------------------------------------------------------------*/
/* Primitives that should have their parameters evaluated by the caller */

Expr* prim_define(Expr* e) {
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

        env_bind(global_env, sym, val);
        last_bound = val;
    }

    return (last_bound == NULL) ? NULL : expr_clone_recur(last_bound);
}

Expr* prim_add(Expr* e) {
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

Expr* prim_sub(Expr* e) {
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

Expr* prim_mul(Expr* e) {
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

Expr* prim_div(Expr* e) {
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
