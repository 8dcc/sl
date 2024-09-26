
#include <stddef.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

Expr* prim_eval(Env* env, Expr* e) {
    return eval(env, e);
}

Expr* prim_apply(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 2);
    SL_EXPECT(expr_is_applicable(e),
              "Expected a function or macro as the first argument, got '%s'.",
              exprtype2str(e->type));
    SL_EXPECT_TYPE(e->next, EXPR_PARENT);

    return apply(env, e, e->next->val.children);
}

Expr* prim_macroexpand(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * Similar to how function calls are evaluated in the `eval_function_call'
     * static function in "eval.c", but the arguments are never evaluated.
     *
     * Note that we expect a quoted expression, so neither the arguments nor the
     * macro should have been evaluated.
     */
    Expr* car = e->val.children;
    SL_EXPECT(car != NULL,
              "The supplied list must have at least one element: The macro.");

    /* If `eval' returns NULL, we don't have to print our own errors */
    Expr* func = eval(env, car);
    if (func == NULL)
        return NULL;
    SL_EXPECT_TYPE(func, EXPR_MACRO);

    Expr* args     = e->val.children->next;
    Expr* expanded = macro_expand(env, func, args);

    expr_free(func);
    return expanded;
}

Expr* prim_random(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT(expr_is_number(e), "Expected numeric argument.");

    /*
     * We return the same numeric type we received.
     *
     * For more information on the value ranges, see:
     * https://c-faq.com/lib/randrange.html
     */
    Expr* ret = expr_new(e->type);
    switch (e->type) {
        case EXPR_NUM_INT:
            ret->val.n = rand() / (RAND_MAX / e->val.n + 1);
            break;

        case EXPR_NUM_FLT:
            ret->val.f = (double)rand() / ((double)RAND_MAX + 1) * e->val.f;
            break;

        default:
            SL_FATAL("Unhandled numeric type.");
    }

    return ret;
}

Expr* prim_set_random_seed(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT_ARG_NUM(e, 1);
    SL_EXPECT_TYPE(e, EXPR_NUM_INT);

    srand(e->val.n);
    return env_get(env, "tru");
}
