
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "include/util.h"
#include "include/parser.h"
#include "include/eval.h"
#include "include/expr.h"
#include "include/env.h"
#include "include/primitives.h"

#define EXPECT_TYPE(EXPR, TYPE)                              \
    SL_EXPECT((EXPR)->type == TYPE,                          \
              "Expected expression of type '%s', got '%s'.", \
              exprtype2str(TYPE), exprtype2str((EXPR)->type));

/*----------------------------------------------------------------------------*/
/* Special Form primitives. See SICP Chapter 4.1.1 */

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
    SL_EXPECT(e != NULL && e->next != NULL && e->next->next == NULL,
              "The special form `lambda' expects exactly 2 arguments: Formals "
              "and body.");
    EXPECT_TYPE(e, EXPR_PARENT);

    /* Count the number of formal arguments, and verify that they are all
     * symbols. */
    size_t formals_num = 0;
    for (Expr* cur = e->val.children; cur != NULL; cur = cur->next) {
        if (cur->type != EXPR_SYMBOL) {
            ERR("Formal arguments of `lambda' must be of type 'Symbol', got "
                "'%s'.",
                exprtype2str(cur->type));
            return NULL;
        }

        formals_num++;
    }

    Expr* ret = sl_safe_malloc(sizeof(Expr));
    ret->type = EXPR_LAMBDA;
    ret->next = NULL;

    /*
     * Create a new LambdaCtx structure that will contain:
     *   - A new environment whose parent will be set when making the actual
     *     function call.
     *   - A string array for the formal arguments of the function, the first
     *     argument of `lambda'. It will be filled below.
     *   - The body of the function, the second argument of `lambda'.
     *
     * Note that since `lambda' is a special form, it's handled differently in
     * `eval' and its arguments won't be evaluated.
     */
    ret->val.lambda              = sl_safe_malloc(sizeof(LambdaCtx));
    ret->val.lambda->env         = env_new();
    ret->val.lambda->formals     = sl_safe_malloc(formals_num * sizeof(char*));
    ret->val.lambda->formals_num = formals_num;
    ret->val.lambda->body        = expr_clone_recur(e->next);

    Expr* cur_formal = e->val.children;
    for (size_t i = 0; i < formals_num; i++) {
        /* Store the symbol as a C string in the array we just allocated. Note
         * that we already verified that all of the formals are symbols when
         * counting the arguments. */
        ret->val.lambda->formals[i] = strdup(cur_formal->val.s);

        /* Go to the next formal argument */
        cur_formal = cur_formal->next;
    }

    return ret;
}

/*----------------------------------------------------------------------------*/
/* Primitives that should have their parameters evaluated by the caller */

Expr* prim_eval(Env* env, Expr* e) {
    return eval(env, e);
}

Expr* prim_apply(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Got empty expression.");

    /*
     * (define prim-apply (env e)
     *   (apply (car e) (cdr e)))
     */
    return apply(env, e, e->next);
}

Expr* prim_cons(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next != NULL && e->next->next == NULL,
              "Expected exactly 2 arguments.");

    /*
     * The `cons' implementation is a bit different for now.
     *
     *   (cons x y) =!=> (x . y)
     *   (cons x y) ===> (x y)
     *
     * Maybe we could add a EXPR_CONS type.
     */
    Expr* ret         = sl_safe_malloc(sizeof(Expr));
    ret->type         = EXPR_PARENT;
    ret->val.children = expr_clone_recur(e);
    ret->next         = NULL;

    /* Append the cdr to the car */
    ret->val.children->next = expr_clone_recur(e->next);

    return ret;
}

Expr* prim_car(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL && e->next == NULL, "Expected one argument.");
    SL_EXPECT(e->type == EXPR_PARENT, "Expected a list as first argument");

    /* (car '()) ===> nil */
    if (e->val.children == NULL)
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
    SL_EXPECT(e != NULL && e->next == NULL, "Expected one argument.");
    SL_EXPECT(e->type == EXPR_PARENT, "Expected a list as first argument");

    /*
     * (cdr '())  ===> nil
     * (cdr '(a)) ===> nil
     */
    if (e->val.children == NULL || e->val.children->next == NULL) {
        Expr* ret         = sl_safe_malloc(sizeof(Expr));
        ret->type         = EXPR_PARENT;
        ret->val.children = NULL;
        ret->next         = NULL;
        return ret;
    }

    /* For more information, see similar iteration in eval.c */
    Expr* cdr_start = NULL;
    Expr* cur_copy  = NULL;
    for (Expr* cur_item = e->val.children->next; cur_item != NULL;
         cur_item       = cur_item->next) {
        if (cur_copy == NULL) {
            cur_copy  = expr_clone_recur(cur_item);
            cdr_start = cur_copy;
        } else {
            cur_copy->next = expr_clone_recur(cur_item);
            cur_copy       = cur_copy->next;
        }
    }

    /*
     * (cdr '(a b c))     ===> (b c)
     * (cdr '((a b) y z)) ===> (y z)
     */
    Expr* ret         = sl_safe_malloc(sizeof(Expr));
    ret->type         = EXPR_PARENT;
    ret->val.children = cdr_start;
    ret->next         = NULL;

    return ret;
}

Expr* prim_add(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = 0;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total += arg->val.n;
    }

    Expr* ret  = sl_safe_malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}

Expr* prim_sub(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;

    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);

        total -= arg->val.n;
    }

    Expr* ret  = sl_safe_malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
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

    Expr* ret  = sl_safe_malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
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

    Expr* ret  = sl_safe_malloc(sizeof(Expr));
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
