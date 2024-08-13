
#include <stdbool.h>
#include <stdio.h>
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

    /* We have to use `expr_clone_recur' since `expr_clone' does not clone
     * children. This function is useful for binding it to `quote' in the
     * environment. */
    return expr_clone_recur(e);
}

/*----------------------------------------------------------------------------*/
/* Primitives that should have their parameters evaluated by the caller */

Expr* prim_define(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);

    /* The `define' function will bind the even arguments to the odd arguments.
     * It expects an even number of arguments.
     *
     * Returns the last bound expression. */
    const Expr* last_bound = NULL;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_EXPECT(arg->next != NULL, "Got an odd number of arguments.");

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
    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
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
        Expr* ret = malloc(sizeof(Expr));
        SL_ASSERT_ALLOC(ret);
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
    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
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

    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
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

    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
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

    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
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

    Expr* ret = malloc(sizeof(Expr));
    SL_ASSERT_ALLOC(ret);
    ret->type  = EXPR_CONST;
    ret->val.n = total;
    ret->next  = NULL;
    return ret;
}
