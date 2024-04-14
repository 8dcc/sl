
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/primitives.h"

static PrimitiveFuncPair non_eval_primitives_list[] = {
    { "quote", prim_quote },    //
    { NULL, NULL },             // Marks the end
};

static PrimitiveFuncPair primitives_list[] = {
    { "+", prim_add },    //
    { "-", prim_sub },    //
    { "*", prim_mul },    //
    { "/", prim_div },    //
    { NULL, NULL },       // Marks the end
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

Expr* prim_add(Expr* e) {
    SL_ASSERT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
                  arg->type);

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
        SL_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
                  arg->type);

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
        SL_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
                  arg->type);

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
        SL_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
                  arg->type);
        SL_ASSERT(arg->val.n != 0, "Trying to divide by zero.");

        total /= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
