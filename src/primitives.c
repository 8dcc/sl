
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/primitives.h"

Expr* prim_add(Expr* e) {
    if (e == NULL) {
        ERR("Missing arguments.");
        return NULL;
    }

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        if (arg->type != EXPR_CONST) {
            ERR("Unexpected argument of type: %d", arg->type);
            return NULL;
        }

        total += arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_sub(Expr* e) {
    if (e == NULL) {
        ERR("Missing arguments.");
        return NULL;
    }

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        if (arg->type != EXPR_CONST) {
            ERR("Unexpected argument of type: %d", arg->type);
            return NULL;
        }

        total -= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_mul(Expr* e) {
    if (e == NULL) {
        ERR("Missing arguments.");
        return NULL;
    }

    double total = 1;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        if (arg->type != EXPR_CONST) {
            ERR("Unexpected argument of type: %d", arg->type);
            return NULL;
        }

        total *= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_div(Expr* e) {
    if (e == NULL) {
        ERR("Missing arguments.");
        return NULL;
    }

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        if (arg->type != EXPR_CONST) {
            ERR("Unexpected argument of type: %d", arg->type);
            return NULL;
        }

        if (arg->val.n == 0) {
            ERR("Trying to divide by zero.");
            return NULL;
        }

        total /= arg->val.n;
    }

    Expr* ret  = malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
