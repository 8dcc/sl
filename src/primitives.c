
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

#define EXPECT_ARG_NUM(EXPR, NUM)                            \
    SL_EXPECT(expr_list_len(EXPR) == NUM,                    \
              "Expected exactly %d arguments, got %d.", NUM, \
              expr_list_len(EXPR))

#define EXPECT_TYPE(EXPR, TYPE)                              \
    SL_EXPECT((EXPR)->type == TYPE,                          \
              "Expected expression of type '%s', got '%s'.", \
              exprtype2str(TYPE), exprtype2str((EXPR)->type))

/*----------------------------------------------------------------------------*/
/* Special Form primitives, their arguments are not evaluated normally by the
 * caller. See SICP Chapter 4.1.1 */

Expr* prim_quote(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next == NULL,
              "The special form `quote' expects exactly 1 argument.");

    /* This `prim' function is useful for binding it to `quote' in the
     * environment. */
    return expr_clone_recur(e);
}

Expr* prim_define(Env* env, Expr* e) {
    /*
     * The `define' function binds the even arguments to the odd arguments.
     * Therefore, it expects an even number of arguments.
     *
     * Returns an evaluated copy of the last bound expression.
     */
    Expr* last_bound = NULL;

    /* See note at the bottom */
    SL_ON_ERR(return last_bound);

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_EXPECT(arg->next != NULL,
                  "Got an odd number of arguments, ignoring last.");

        /* Odd argument: Symbol */
        EXPECT_TYPE(arg, EXPR_SYMBOL);
        const char* sym = arg->val.s;

        /* Even argument: Expression */
        arg = arg->next;

        /* We evaluate the expression before binding. Invalid expressions are
         * not defined. */
        Expr* evaluated = eval(env, arg);
        if (evaluated == NULL)
            continue;

        /* Bind a copy of the evaluated expression to the current environment */
        env_bind(env, sym, evaluated);

        /* The `last_bound' variable holds either NULL or the last returned
         * expression by `eval'. Before overwriting the copy returned by `eval',
         * free it. */
        expr_free(last_bound);
        last_bound = evaluated;
    }

    /* Last bound holds either NULL or the last valid expression returned by
     * `eval'. Since eval returns a new expression, we can return it safely. */
    return last_bound;
}

Expr* prim_lambda(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next != NULL,
              "The special form `lambda' expects at least 2 arguments: Formals "
              "and body.");
    EXPECT_TYPE(e, EXPR_PARENT);

    const Expr* formals = e;
    const Expr* body    = e->next;

    /*
     * Allocate and initialize a new `LambdaCtx' structure using the formals and
     * the body expressions we received. Store that context structure in the
     * expression we will return.
     */
    Expr* ret       = expr_new(EXPR_LAMBDA);
    ret->val.lambda = lambda_ctx_new(formals, body);

    return ret;
}

Expr* prim_if(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) == 3,
              "The special form `if' expects exactly 3 arguments: Predicate, "
              "consequent and alternative.");

    /* Evaluate the predicate (first argument) */
    Expr* evaluated_predicate = eval(env, e);

    /* If the predicate is false (nil), the expression to be evaluated is the
     * "alternative" (third argument) , otherwise, evaluate the "consequent"
     * (second argument). */
    Expr* result = expr_is_nil(evaluated_predicate) ? e->next->next : e->next;

    /* Make sure we free the expression allocated by `eval' */
    expr_free(evaluated_predicate);

    /* Since `if' is a special form, we need to evaluate the result */
    return eval(env, result);
}

/*----------------------------------------------------------------------------*/
/* General primitives */

Expr* prim_eval(Env* env, Expr* e) {
    return eval(env, e);
}

Expr* prim_apply(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);
    SL_EXPECT(e->type == EXPR_PRIM || e->type == EXPR_LAMBDA,
              "Expected a function as the first argument, got '%s'.",
              exprtype2str(e->type));
    EXPECT_TYPE(e->next, EXPR_PARENT);

    return apply(env, e, e->next->val.children);
}

Expr* prim_begin(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Expected at least 1 expression.");

    /*
     * In Scheme, begin is technically a special form because when making a
     * call, the arguments are not required to be evaluated in order. In this
     * Lisp, however, they are.
     */
    while (e->next != NULL)
        e = e->next;

    return expr_clone_recur(e);
}

/*----------------------------------------------------------------------------*/
/* List-related primitives */

Expr* prim_cons(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);

    /*
     * The `cons' implementation is a bit different for now.
     *
     *   (cons x y) =!=> (x . y)
     *   (cons x y) ===> (x y)
     *
     * Maybe we could add a EXPR_CONS type.
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_clone_recur(e);

    /* Append the cdr to the car */
    ret->val.children->next = expr_clone_recur(e->next);

    return ret;
}

Expr* prim_car(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * (car '()) ===> nil
     */
    if (expr_is_nil(e))
        return expr_clone(e);

    /*
     * (car '(a b c))     ===> a
     * (car '((a b) y z)) ===> (a b)
     */
    return expr_clone_recur(e->val.children);
}

Expr* prim_cdr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    Expr* ret = expr_new(EXPR_PARENT);

    if (e->val.children == NULL || e->val.children->next == NULL) {
        /*
         * (cdr '())  ===> nil
         * (cdr '(a)) ===> nil
         */
        ret->val.children = NULL;
    } else {
        /*
         * (cdr '(a b c))     ===> (b c)
         * (cdr '((a b) y z)) ===> (y z)
         */
        ret->val.children = expr_clone_list(e->val.children->next);
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/* Arithmetic primitives */

Expr* prim_add(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        total += arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    /*
     * If there is only one argument, negate. Otherwise subtract in order.
     *   (- 5)     ===> -5
     *   (- 9 5 1) ===> 3
     */
    if (e->next == NULL) {
        total = -total;
    } else {
        for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
            EXPECT_TYPE(arg, EXPR_CONST);
            total -= arg->val.n;
        }
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_mul(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = 1;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        total *= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

Expr* prim_div(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total /= arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

/*----------------------------------------------------------------------------*/
/* Logical primitives */

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

    /* Get the true or false expressions from the environment */
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
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

    /* Get the true or false expressions from the environment */
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
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

    /* Get the true or false expressions from the environment */
    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}
