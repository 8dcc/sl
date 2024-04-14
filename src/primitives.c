
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/primitives.h"

/* If COND is not true, show error and return NULL */
#define PRIM_ASSERT(COND, ...) \
    if (!(COND)) {             \
        ERR(__VA_ARGS__);      \
        return NULL;           \
    }

Expr* prim_add(Expr* e) {
    PRIM_ASSERT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        PRIM_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
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
    PRIM_ASSERT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        PRIM_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
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
    PRIM_ASSERT(e != NULL, "Missing arguments.");

    double total = 1;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        PRIM_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
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
    PRIM_ASSERT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        PRIM_ASSERT(arg->type == EXPR_CONST, "Unexpected argument of type: %d",
                    arg->type);
        PRIM_ASSERT(arg->val.n != 0, "Trying to divide by zero.");

        total /= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
