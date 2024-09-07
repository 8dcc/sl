
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

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
    SL_EXPECT(expr_list_len(e) == 1,
              "The special form `quote' expects exactly 1 argument.");

    /*
     * The special form `quote' simply returns the expression it receives,
     * effectively delaying its evaluation.
     */
    return expr_clone_recur(e);
}

Expr* prim_define(Env* env, Expr* e) {
    /*
     * The `define' function binds the even arguments to the odd arguments.
     * Therefore, it expects an even number of arguments.
     *
     * Since it's a special form, the arguments we received in `e' are not
     * evaluated. Before binding each even argument, we evaluate it. We don't
     * bind it if there is an error in the evaluation.
     *
     * Returns an evaluated copy of the last bound expression.
     */
    Expr* last_bound = NULL;
    SL_ON_ERR(return last_bound);

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        SL_EXPECT(arg->next != NULL,
                  "Got an odd number of arguments, ignoring last.");

        /* Odd argument: Symbol */
        EXPECT_TYPE(arg, EXPR_SYMBOL);
        const char* sym = arg->val.s;

        /* Even argument: Expression */
        arg = arg->next;

        Expr* evaluated = eval(env, arg);
        if (evaluated == NULL)
            continue;

        env_bind(env, sym, evaluated);

        expr_free(last_bound);
        last_bound = evaluated;
    }

    /*
     * Last bound holds either NULL or the last valid expression returned by
     * `eval'. Since `eval' returns a new expression, we can return it safely.
     */
    return last_bound;
}

Expr* prim_lambda(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2,
              "The special form `lambda' expects at least 2 arguments: Formals "
              "and body.");
    EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * Allocate and initialize a new `LambdaCtx' structure using the formals and
     * the body expressions we received. Store that context structure in the
     * expression we will return.
     */
    Expr* ret = expr_new(EXPR_LAMBDA);

    const Expr* formals = e;
    const Expr* body    = e->next;
    ret->val.lambda     = lambda_ctx_new(formals, body);

    return ret;
}

Expr* prim_macro(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2,
              "The special form `macro' expects at least 2 arguments: Formals "
              "and body.");
    EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * The `macro' and `lambda' primitives are identical, but the type of the
     * returned expression changes.
     */
    Expr* ret = expr_new(EXPR_MACRO);

    const Expr* formals = e;
    const Expr* body    = e->next;
    ret->val.lambda     = lambda_ctx_new(formals, body);

    return ret;
}

Expr* prim_begin(Env* env, Expr* e) {
    /*
     * In Scheme, `begin' is a special form for various reasons. When making a
     * call, the arguments are not required to be evaluated in order, when using
     * `begin', they are. The fact that it has to evaluate the expressions is
     * helpful when combined with something like `apply' and a quoted
     * expression:
     *
     *   ;; Arguments not evaluated because it's a special form. Returns 7.
     *   (begin
     *     (+ 1 2)
     *     (+ 3 4))
     *
     *   ;; Arguments not evaluated because the list is quoted. Also returns 7.
     *   (apply begin
     *          '((+ 1 2)
     *            (+ 3 4)))
     */
    Expr* last_evaluated = NULL;
    for (Expr* cur = e; cur != NULL; cur = cur->next) {
        expr_free(last_evaluated);
        last_evaluated = eval(env, cur);
    }

    return last_evaluated;
}

Expr* prim_if(Env* env, Expr* e) {
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) == 3,
              "The special form `if' expects exactly 3 arguments: Predicate, "
              "consequent and alternative.");

    /*
     * First, evaluate the predicate (first argument). If the predicate is false
     * (nil), the expression to be evaluated is the "alternative" (third
     * argument); otherwise, evaluate the "consequent" (second argument).
     */
    Expr* evaluated_predicate = eval(env, e);
    Expr* result = expr_is_nil(evaluated_predicate) ? e->next->next : e->next;
    expr_free(evaluated_predicate);
    return eval(env, result);
}

Expr* prim_or(Env* env, Expr* e) {
    /*
     * The `or' function does not have to be a primitive, it can be built with
     * `if' and macros. In any case, we can't use normal evaluation rules
     * because not all arguments of `or' are always evaluated. As soon as one of
     * them is true, we stop evaluating the arguments and return that one. The
     * same is true for `prim_and', but we stop as soon as one of them is `nil'
     * (false).
     */
    Expr* result = NULL;
    for (Expr* cur = e; cur != NULL; cur = cur->next) {
        expr_free(result);
        result = eval(env, cur);
        if (!expr_is_nil(result))
            break;
    }

    return (result == NULL) ? env_get(env, "nil") : result;
}

Expr* prim_and(Env* env, Expr* e) {
    /*
     * For more information, see comment in `prim_or'.
     *
     * Also note that we are returning `tru' if we didn't receive any arguments.
     * This is the standard behavior in Scheme.
     */
    Expr* result = NULL;
    for (Expr* cur = e; cur != NULL; cur = cur->next) {
        expr_free(result);
        result = eval(env, cur);
        if (expr_is_nil(result))
            break;
    }

    return (result == NULL) ? env_get(env, "tru") : result;
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

Expr* prim_macroexpand(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    /* This is similar to how `eval' handles EXPR_PARENT expressions. */
    Expr* func = e->val.children;
    SL_EXPECT(func != NULL, "The macro call must have at least one element.");

    /* Save unevaluated args before evaluating the function */
    Expr* args = func->next;

    func = eval(env, func);
    if (func == NULL)
        return NULL;

    Expr* expanded = macro_expand(env, func, args);
    expr_free(func);

    return expanded;
}

/*----------------------------------------------------------------------------*/
/* List-related primitives */

Expr* prim_list(Env* env, Expr* e) {
    SL_UNUSED(env);

    /*
     * (list)          ===> nil
     * (list 'a 'b 'c) ===> (a b c)
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_clone_list(e);

    return ret;
}

Expr* prim_cons(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);
    EXPECT_TYPE(e->next, EXPR_PARENT);

    /*
     * (cons a nil)    ===> (a)
     * (cons a '(b c)) ===> (a b c)
     */
    Expr* ret         = expr_new(EXPR_PARENT);
    ret->val.children = expr_clone_recur(e);

    if (e->next->val.children != NULL)
        ret->val.children->next = expr_clone_list(e->next->val.children);

    return ret;
}

Expr* prim_car(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * (car '())          ===> nil
     * (car '(a b c))     ===> a
     * (car '((a b) y z)) ===> (a b)
     */
    if (expr_is_nil(e))
        return expr_clone(e);

    return expr_clone_recur(e->val.children);
}

Expr* prim_cdr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_PARENT);

    /*
     * (cdr '())          ===> nil
     * (cdr '(a))         ===> nil
     * (cdr '(a b c))     ===> (b c)
     * (cdr '((a b) y z)) ===> (y z)
     */
    Expr* ret = expr_new(EXPR_PARENT);

    if (e->val.children == NULL || e->val.children->next == NULL)
        ret->val.children = NULL;
    else
        ret->val.children = expr_clone_list(e->val.children->next);

    return ret;
}

Expr* prim_append(Env* env, Expr* e) {
    SL_UNUSED(env);

    /*
     * (append)                   ===> nil
     * (append nil)               ===> nil
     * (append '(a b) ... '(y z)) ===> (a b ... y z)
     */
    Expr* ret = expr_new(EXPR_PARENT);
    if (e == NULL) {
        ret->val.children = NULL;
        return ret;
    }

    SL_ON_ERR(return ret);

    Expr dummy_copy;
    dummy_copy.next = NULL;
    Expr* cur_copy  = &dummy_copy;

    for (Expr* arg = e; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_PARENT);

        if (arg->val.children == NULL)
            continue;

        cur_copy->next = expr_clone_list(arg->val.children);
        while (cur_copy->next != NULL)
            cur_copy = cur_copy->next;
    }

    ret->val.children = dummy_copy.next;
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

Expr* prim_mod(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    double total = e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");

        /*
         * Allow decimal and negative inputs when calculating the modulus.
         * See: https://8dcc.github.io/programming/fmod.html
         */
        total = fmod(total, arg->val.n);
        if (arg->val.n < 0 ? total > 0 : total < 0)
            total += arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = total;
    return ret;
}

/*----------------------------------------------------------------------------*/
/* Bit-wise primitives */

Expr* prim_bit_and(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    uint64_t total = (uint64_t)e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total &= (uint64_t)arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)total;
    return ret;
}

Expr* prim_bit_or(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    uint64_t total = (uint64_t)e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total |= (uint64_t)arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)total;
    return ret;
}

Expr* prim_bit_xor(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(e != NULL, "Missing arguments.");

    uint64_t total = (uint64_t)e->val.n;
    for (Expr* arg = e->next; arg != NULL; arg = arg->next) {
        EXPECT_TYPE(arg, EXPR_CONST);
        SL_EXPECT(arg->val.n != 0, "Trying to divide by zero.");
        total ^= (uint64_t)arg->val.n;
    }

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)total;
    return ret;
}

Expr* prim_bit_not(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 1);
    EXPECT_TYPE(e, EXPR_CONST);

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)(~((uint64_t)e->val.n));
    return ret;
}

Expr* prim_shr(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);
    EXPECT_TYPE(e, EXPR_CONST);
    EXPECT_TYPE(e->next, EXPR_CONST);

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)((uint64_t)e->val.n >> (uint64_t)e->next->val.n);
    return ret;
}

Expr* prim_shl(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    EXPECT_ARG_NUM(e, 2);
    EXPECT_TYPE(e, EXPR_CONST);
    EXPECT_TYPE(e->next, EXPR_CONST);

    Expr* ret  = expr_new(EXPR_CONST);
    ret->val.n = (double)((uint64_t)e->val.n << (uint64_t)e->next->val.n);
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

    return (result) ? env_get(env, "tru") : env_get(env, "nil");
}
