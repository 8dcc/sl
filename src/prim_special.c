
#include <stddef.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

/*
 * Special Form primitives. Their arguments are not evaluated before applying,
 * instead they are handled separately in `eval'.
 *
 * See the "Special Forms" section of the SL manual, and Chapter 4.1.1 of SICP.
 */

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
     * The `define' function binds a value (second argument) to a symbol (first
     * argument).
     *
     * Since it's a special form, we must evaluate the second argument. We don't
     * bind it if there is an error in the evaluation.
     *
     * Returns the evaluated expression.
     */
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) == 2,
              "The special form `define' expects exactly 2 arguments.");
    SL_EXPECT_TYPE(e, EXPR_SYMBOL);

    Expr* evaluated = eval(env, e->next);
    if (evaluated == NULL)
        return NULL;

    const bool success = env_bind(env, e->val.s, evaluated, ENV_FLAG_NONE);
    return (success) ? evaluated : NULL;
}

Expr* prim_define_global(Env* env, Expr* e) {
    /*
     * The `define' function binds a value (second argument) to a symbol (first
     * argument).
     *
     * Since it's a special form, we must evaluate the second argument. We don't
     * bind it if there is an error in the evaluation.
     *
     * Returns the evaluated expression.
     */
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) == 2,
              "The special form `define' expects exactly 2 arguments.");
    SL_EXPECT_TYPE(e, EXPR_SYMBOL);

    Expr* evaluated = eval(env, e->next);
    if (evaluated == NULL)
        return NULL;

    const bool success =
      env_bind_global(env, e->val.s, evaluated, ENV_FLAG_NONE);
    return (success) ? evaluated : NULL;
}

Expr* prim_lambda(Env* env, Expr* e) {
    SL_UNUSED(env);
    SL_ON_ERR(return NULL);
    SL_EXPECT(expr_list_len(e) >= 2,
              "The special form `lambda' expects at least 2 arguments: Formals "
              "and body.");
    SL_EXPECT_TYPE(e, EXPR_PARENT);

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
    SL_EXPECT_TYPE(e, EXPR_PARENT);

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
        if (last_evaluated == NULL)
            return NULL;
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
    if (evaluated_predicate == NULL)
        return NULL;
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
        if (result == NULL)
            return NULL;
        if (!expr_is_nil(result))
            break;
    }

    return (result == NULL) ? expr_clone(nil) : result;
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
        if (result == NULL)
            return NULL;
        if (expr_is_nil(result))
            break;
    }

    return (result == NULL) ? expr_clone(tru) : result;
}
