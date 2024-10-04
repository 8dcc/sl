
#include <stddef.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/util.h"
#include "include/primitives.h"

Expr* prim_equal(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A == B == ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_equal(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_lt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A < B < ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_lt(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(tru) : expr_clone(nil);
}

Expr* prim_gt(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2, "Expected at least 2 arguments.");

    bool result = true;

    /* (A > B > ...) */
    for (Expr* arg = e; arg->next != NULL; arg = arg->next) {
        if (!expr_gt(arg, arg->next)) {
            result = false;
            break;
        }
    }

    return (result) ? expr_clone(tru) : expr_clone(nil);
}
