/*
 * Copyright 2024 8dcc
 *
 * This file is part of SL.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * SL. If not, see <https://www.gnu.org/licenses/>.
 */
/*
 * Special Form primitives. Their arguments are not evaluated before applying.
 * See the "Special Forms" section of the SL manual, and Chapter 4.1.1 of SICP.
 */

#include <stddef.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"
#include "include/primitives.h"

/*
 * Is the specified list a call to a function with the specified name? In other
 * words, a list whose `car' is a symbol with the specified value.
 */
static inline bool is_call_to(const Expr* list, const char* func) {
    SL_ASSERT(expr_is_proper_list(list));
    return EXPR_SYMBOL_P(CAR(list)) && CAR(list)->val.s != NULL &&
           strcmp(CAR(list)->val.s, func) == 0;
}

/*
 * Evaluate the necessary parts of a single backquoted expression, and return
 * it.
 *
 * If the argument is a list, this function is a "selective" version of the
 * 'eval_list' function from 'eval.c'.
 */
static Expr* handle_backquote_arg(Env* env, Expr* arg) {
    /* Not a proper list, return unevaluated, just like `quote' */
    if (!expr_is_proper_list(arg))
        return arg;

    /*
     * If we reached this point, the expression is a proper list. Check if
     * it's a call to one of the special unquoting symbols.
     *
     * We can't splice directly outside of a list:
     *   `,@expr  =>  (` (,@ expr))  =>  <Error>
     *
     * We can unquote outside of a list. This conditional is useful since this
     * 'handle_backquote_arg' function calls itself recursively below.
     *   `,expr  =>  (` (, expr))  =>  (eval expr)
     */
    SL_EXPECT(!is_call_to(arg, ",@"), "Can't splice (,@) outside of a list.");
    if (is_call_to(arg, ",")) {
        SL_EXPECT(!expr_is_nil(CDR(arg)) && expr_is_nil(CDDR(arg)),
                  "Call to unquote (,) expected exactly one argument.");
        return eval(env, CADR(arg));
    }

    /*
     * If we reached this point, the backquoted expression is a normal list. We
     * handle each element recursively to allow calls to unquote from nested
     * lists. We will also handle valid calls to the splice function (,@) here.
     */
    Expr* result = g_nil;
    for (const Expr* list = arg; !expr_is_nil(list); list = CDR(list)) {
        Expr* cur = CAR(list);
        if (expr_is_proper_list(cur) && is_call_to(cur, ",@")) {
            /*
             * Calls to splice are handled when parsing a list:
             *
             *   (a b (,@ expr) c d)
             *
             * Is equivalent to:
             *
             *   (append '(a b) (eval expr) '(c d))
             *
             * Therefore, 'expr' must evaluate to a proper list.
             */
            SL_EXPECT(!expr_is_nil(CDR(cur)) && expr_is_nil(CDDR(cur)),
                      "Call to splice (,@) expected exactly one argument.");

            Expr* evaluated = eval(env, CADR(cur));
            if (EXPR_ERR_P(evaluated))
                return evaluated;
            SL_EXPECT(expr_is_proper_list(evaluated),
                      "Argument of splice (,@) did not evaluate to a proper "
                      "list. Use unquote (,) instead.");

            /*
             * Concatenate the list we got from the evaluation to the result.
             */
            result = expr_nconc(result, evaluated);
        } else {
            /*
             * The current element of the list is not a call to the splice
             * function, so we can handle it normally by calling ourselves
             * recursively.
             */
            Expr* handled = handle_backquote_arg(env, cur);
            if (EXPR_ERR_P(handled))
                return handled;

            Expr* pair = expr_new(EXPR_PAIR);
            CAR(pair)  = handled;
            CDR(pair)  = g_nil;
            result     = expr_nconc(result, pair);
        }
    }

    return result;
}

/*----------------------------------------------------------------------------*/

Expr* prim_quote(Env* env, Expr* args) {
    /*
     * The special form `quote' simply returns the expression it receives,
     * effectively delaying its evaluation.
     */
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);
    return CAR(args);
}

Expr* prim_backquote(Env* env, Expr* args) {
    /*
     * The special form `backquote' is similar to `quote', but allows selective
     * evaluation by wrapping an expression in (, ...). Note that `expr is
     * converted to (` expr) by the parser.
     */
    SL_UNUSED(env);
    SL_EXPECT_ARG_NUM(args, 1);
    return handle_backquote_arg(env, CAR(args));
}

Expr* prim_unquote(Env* env, Expr* args) {
    /*
     * The `unquote' function is only used inside 'handle_backquote_arg'. Note
     * that ,expr is converted to (, expr) by the parser.
     */
    SL_UNUSED(env);
    SL_UNUSED(args);
    return err("Invalid use of unquote (,) outside of backquote.");
}

Expr* prim_splice(Env* env, Expr* args) {
    /*
     * The `splice' function is only used inside 'handle_backquote_arg'. Note
     * that ,@expr is converted to (,@ expr) by the parser.
     */
    SL_UNUSED(env);
    SL_UNUSED(args);
    return err("Invalid use of splice (,@) outside of backquote.");
}

/*----------------------------------------------------------------------------*/

Expr* prim_define(Env* env, Expr* args) {
    /*
     * The `define' function binds a symbol (first argument) to a value (second
     * argument) in the current environment.
     *
     * Since it's a special form, we must evaluate the second argument. We don't
     * bind it if there is an error in the evaluation.
     *
     * Returns the evaluated expression.
     */
    SL_EXPECT_ARG_NUM(args, 2);

    Expr* sym = expr_list_nth(args, 1);
    Expr* val = expr_list_nth(args, 2);
    SL_EXPECT_TYPE(sym, EXPR_SYMBOL);

    Expr* evaluated = eval(env, val);
    if (EXPR_ERR_P(evaluated))
        return evaluated;

    const enum EEnvErr code =
      env_bind(env, sym->val.s, evaluated, ENV_FLAG_NONE);
    SL_EXPECT(code == ENV_ERR_NONE,
              "Could not bind symbol `%s': %s",
              sym->val.s,
              env_strerror(code));

    return evaluated;
}

Expr* prim_define_global(Env* env, Expr* args) {
    /*
     * The `define-global' function is just like `define', but it binds the
     * symbol to the value in the top-most environment.
     */
    SL_EXPECT_ARG_NUM(args, 2);

    Expr* sym = expr_list_nth(args, 1);
    Expr* val = expr_list_nth(args, 2);
    SL_EXPECT_TYPE(sym, EXPR_SYMBOL);

    Expr* evaluated = eval(env, val);
    if (EXPR_ERR_P(evaluated))
        return evaluated;

    const enum EEnvErr code =
      env_bind_global(env, sym->val.s, evaluated, ENV_FLAG_NONE);
    SL_EXPECT(code == ENV_ERR_NONE,
              "Could not bind global symbol `%s': %s",
              sym->val.s,
              env_strerror(code));

    return evaluated;
}

/*----------------------------------------------------------------------------*/

Expr* prim_lambda(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(args) >= 2,
              "The special form `lambda' expects at least 2 arguments: Formals "
              "and body.");

    const Expr* formals = CAR(args);
    SL_EXPECT_PROPER_LIST(formals);
    const Expr* body = CDR(args);
    SL_EXPECT_PROPER_LIST(body);

    /*
     * Allocate new 'LambdaCtx' structure. Try to initialize it using the
     * formals and the body expressions we received. Check for errors in the
     * lambda definition, and finally store that context structure in the actual
     * expression we will return.
     */
    LambdaCtx* ctx                      = lambdactx_new();
    const enum ELambdaCtxErr lambda_err = lambdactx_init(ctx, formals, body);
    if (lambda_err != LAMBDACTX_ERR_NONE) {
        lambdactx_free(ctx);
        return err("%s", lambdactx_strerror(lambda_err));
    }

    Expr* ret       = expr_new(EXPR_LAMBDA);
    ret->val.lambda = ctx;
    return ret;
}

Expr* prim_macro(Env* env, Expr* args) {
    SL_UNUSED(env);
    SL_EXPECT(expr_list_len(args) >= 2,
              "The special form `macro' expects at least 2 arguments: Formals "
              "and body.");

    const Expr* formals = CAR(args);
    SL_EXPECT_PROPER_LIST(formals);
    const Expr* body = CDR(args);
    SL_EXPECT_PROPER_LIST(body);

    LambdaCtx* ctx                      = lambdactx_new();
    const enum ELambdaCtxErr lambda_err = lambdactx_init(ctx, formals, body);
    if (lambda_err != LAMBDACTX_ERR_NONE) {
        lambdactx_free(ctx);
        return err("%s", lambdactx_strerror(lambda_err));
    }

    /*
     * The `macro' and `lambda' primitives are identical, but the type of the
     * returned expression changes.
     */
    Expr* ret       = expr_new(EXPR_MACRO);
    ret->val.lambda = ctx;
    return ret;
}

/*----------------------------------------------------------------------------*/

Expr* prim_begin(Env* env, Expr* args) {
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
    Expr* last_evaluated = g_nil;

    for (; !expr_is_nil(args); args = CDR(args)) {
        last_evaluated = eval(env, CAR(args));
        if (EXPR_ERR_P(last_evaluated))
            break;
    }

    return last_evaluated;
}

/*----------------------------------------------------------------------------*/

Expr* prim_if(Env* env, Expr* args) {
    SL_EXPECT(expr_list_len(args) == 3,
              "The special form `if' expects exactly 3 arguments: Predicate, "
              "consequent and alternative.");

    Expr* predicate   = expr_list_nth(args, 1);
    Expr* consequent  = expr_list_nth(args, 2);
    Expr* alternative = expr_list_nth(args, 3);

    /*
     * First, evaluate the predicate (first argument). If the predicate is false
     * (nil), the expression to be evaluated is the "alternative" (third
     * argument); otherwise, evaluate the "consequent" (second argument).
     */
    Expr* evaluated_predicate = eval(env, predicate);
    if (EXPR_ERR_P(evaluated_predicate))
        return evaluated_predicate;

    Expr* result = !expr_is_nil(evaluated_predicate) ? consequent : alternative;
    return eval(env, result);
}

Expr* prim_or(Env* env, Expr* args) {
    /*
     * The `or' function does not have to be a primitive, it can be built with
     * `if' and macros. In any case, we can't use normal evaluation rules
     * because not all arguments of `or' are always evaluated. As soon as one of
     * them is true, we stop evaluating the arguments and return that one. The
     * same is true for 'prim_and', but we stop as soon as one of them is `nil'
     * (false).
     */
    Expr* result = g_nil;

    for (; !expr_is_nil(args); args = CDR(args)) {
        result = eval(env, CAR(args));
        if (EXPR_ERR_P(result) || !expr_is_nil(result))
            break;
    }

    return result;
}

Expr* prim_and(Env* env, Expr* args) {
    /*
     * For more information, see comment in 'prim_or'.
     *
     * Also note that we are returning `tru' if we didn't receive any arguments.
     * This is the standard behavior in Scheme.
     */
    Expr* result = g_tru;

    for (; !expr_is_nil(args); args = CDR(args)) {
        result = eval(env, CAR(args));
        if (EXPR_ERR_P(result) || expr_is_nil(result))
            break;
    }

    return result;
}
