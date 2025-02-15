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

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/eval.h"

/*
 * Count and validate the number of formal arguments in a list. Returns
 * LAMBDACTX_ERR_NONE on success.
 */
static enum ELambdaCtxErr count_formals(const Expr* list, size_t* mandatory,
                                        bool* has_rest) {
    SL_ASSERT(expr_is_proper_list(list));

    /* Initialize output variables */
    *mandatory = 0;
    *has_rest  = false;

    for (; !expr_is_nil(list); list = CDR(list)) {
        const Expr* cur = CAR(list);
        if (!EXPR_SYMBOL_P(cur))
            return LAMBDACTX_ERR_FORMALTYPE;

        /*
         * Only a single formal can appear after "&rest", so we expect:
         *
         *   ("&rest" . ("SYMBOL" . nil))
         *    ^(list.car)           ^(list.cdr.cdr)
         *   ^(list)     ^(list.cdr.car)
         *              ^(list.cdr)
         */
        if (strcmp(cur->val.s, "&rest") == 0) {
            if (expr_is_nil(CDR(list)) || !expr_is_nil(CDDR(list)))
                return LAMBDACTX_ERR_NOREST;

            *has_rest = true;
            break;
        }

        *mandatory += 1;
    }

    return LAMBDACTX_ERR_NONE;
}

/*----------------------------------------------------------------------------*/

LambdaCtx* lambdactx_new(void) {
    LambdaCtx* ret   = mem_alloc(sizeof(LambdaCtx));
    ret->env         = NULL;
    ret->formals_num = 0;
    ret->formals     = NULL;
    ret->formal_rest = NULL;
    ret->body        = NULL;
    return ret;
}

enum ELambdaCtxErr lambdactx_init(Env* env, LambdaCtx* ctx, const Expr* formals,
                                  const Expr* body) {
    SL_ASSERT(expr_is_proper_list(formals));
    SL_ASSERT(expr_is_proper_list(body));

    /* Count and validate the formal arguments */
    size_t mandatory;
    bool has_rest;
    const enum ELambdaCtxErr formal_err =
      count_formals(formals, &mandatory, &has_rest);
    if (formal_err != LAMBDACTX_ERR_NONE)
        return formal_err;

    /*
     * Initialize the lambda's environment. This environment will be used to:
     *
     *   1. Bind the formal argument symbols to the argument values, when the
     *      lambda is called.
     *   2. If more symbols are bound inside this function call, the current
     *      lambda's environment will be used (restricting the scope of nested
     *      functions, for example).
     *
     * When the lambda is created, no symbols are bound in their
     * environment. However, we need to set the parent environment when the
     * lambda is created (instead of when called), so it's able to create a
     * closure with the caller:
     *
     *     (lambda (a)
     *       (lambda (b)  ; Inner lambda needs to access 'a' later.
     *         (+ a b)))
     *
     *
     * There is another problem: whenever we free a lambda, we are also freeing
     * its environment. Since the lifetime of the parent environment might be
     * shorter than the one of the lambda we are creating, we should create a
     * clone of the parent environment instead.
     *
     * Since a lambda can't access the environment where it was called,
     * something like this is not valid, because 'b' was not defined when the
     * lambda was created:
     *
     *     (define func
     *       (lambda (a)
     *         (+ a b)))
     *     (let ((b 2))
     *       (func 3)) ; Error.
     *
     * This last detail is the difference between "dynamic" and "lexical"
     * binding. This Lisp uses lexical binding.
     */
    ctx->env         = env_new();
    ctx->env->parent = env;

    /*
     * Apart from the environment, the lambda needs to store:
     *
     *   - A string array for the formal arguments of the function, the first
     *     argument of `lambda'.
     *   - The body of the function, consisting the rest of the arguments of
     *     `lambda'.
     *
     * Note that since `lambda' is a special form, and it is handled differently
     * in `eval', we know that the Lisp arguments were not evaluated implicitly.
     */
    ctx->formals_num = mandatory;
    ctx->formals     = mem_alloc(mandatory * sizeof(char*));
    ctx->formal_rest = NULL;
    /*
     * TODO: Should we clone the expressions, or store the original references?
     */
    ctx->body = expr_clone_tree(body);

    /*
     * For each formal argument we counted above, store the symbol as a C string
     * in the array we just allocated. Note that we already verified that all of
     * the formals are symbols when counting them in 'count_formals'.
     */
    const Expr* cur_formal = formals;
    for (size_t i = 0; i < mandatory; i++) {
        ctx->formals[i] = mem_strdup(CAR(cur_formal)->val.s);
        cur_formal      = CDR(cur_formal);
    }

    /*
     * Save the symbol after the "&rest" keyword (i.e. the current `cadr') in
     * the context.
     */
    if (has_rest)
        ctx->formal_rest = mem_strdup(CADR(cur_formal)->val.s);

    return LAMBDACTX_ERR_NONE;
}

LambdaCtx* lambdactx_clone(const LambdaCtx* ctx) {
    /* Allocate a new LambdaCtx structure */
    LambdaCtx* ret = mem_alloc(sizeof(LambdaCtx));

    /* Copy the environment and the list of body expressions */
    ret->env = env_clone(ctx->env);
    /*
     * TODO: Should we clone the expressions, or store the original references?
     */
    ret->body = expr_clone_tree(ctx->body);

    /* Allocate a new string array for the mandatory formals, and copy them */
    ret->formals_num = ctx->formals_num;
    ret->formals     = mem_alloc(ret->formals_num * sizeof(char*));
    for (size_t i = 0; i < ret->formals_num; i++)
        ret->formals[i] = mem_strdup(ctx->formals[i]);

    /* If it had a "&rest" formal, copy it */
    ret->formal_rest =
      (ctx->formal_rest == NULL) ? NULL : mem_strdup(ctx->formal_rest);

    return ret;
}

void lambdactx_free(LambdaCtx* ctx) {
    /*
     * 1. Free the lambda environment. We assume that if the environment was in
     *    use somewhere else, the 'LambdaCtx' wouldn't have been freed.  Either
     *    way, expressions in that environment are not freed, so they can still
     *    be used somewhere else.
     * 2. Free each formal argument string, and the pointer to the array itself.
     * 3. Free the "&rest" formal string. This might be NULL, but it's a valid
     *    value for 'free'.
     * 4. Finally, free the 'LambdaCtx' structure itself.
     *
     * Note how we don't free the body, since those expressions might be in use
     * somewhere else, and they will be garbage-collected if necessary.
     */
    env_free(ctx->env);

    for (size_t i = 0; i < ctx->formals_num; i++)
        free(ctx->formals[i]);
    free(ctx->formals);

    free(ctx->formal_rest);
    free(ctx);
}

/*----------------------------------------------------------------------------*/

bool lambdactx_equal(const LambdaCtx* a, const LambdaCtx* b) {
    if (a->formals_num != b->formals_num)
        return false;

    for (size_t i = 0; i < a->formals_num; i++)
        if (strcmp(a->formals[i], b->formals[i]) != 0)
            return false;

    if (a->formal_rest != b->formal_rest &&
        (a->formal_rest == NULL || b->formal_rest == NULL ||
         strcmp(a->formal_rest, b->formal_rest) != 0))
        return false;

    if (!expr_equal(a->body, b->body))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

void lambdactx_print_args(FILE* fp, const LambdaCtx* ctx) {
    /* Position in the 'ctx->formals' array, shared across all argument types */
    size_t formals_pos = 0;

    fputc('(', fp);

    /* Print mandatory arguments */
    for (size_t i = 0; i < ctx->formals_num; i++) {
        if (formals_pos > 0)
            fputc(' ', fp);
        fprintf(fp, "%s", ctx->formals[formals_pos++]);
    }

    /* There can only be one argument after "&rest" */
    if (ctx->formal_rest) {
        if (formals_pos > 0)
            fputc(' ', fp);
        fprintf(fp, "&rest %s", ctx->formal_rest);
    }

    fputc(')', fp);
}

/*----------------------------------------------------------------------------*/

static Expr* lambdactx_eval_body(LambdaCtx* ctx, Expr* args) {
    SL_ASSERT(expr_is_proper_list(args));

    /* Count the number of arguments that we received */
    const size_t arg_num = expr_list_len(args);

    /* Make sure the number of arguments that we got is what we expected */
    SL_EXPECT(ctx->formal_rest != NULL || arg_num == ctx->formals_num,
              "Invalid number of arguments. Expected %zu, got %zu.",
              ctx->formals_num,
              arg_num);

    /*
     * In the lambda's environment, bind each mandatory formal argument to its
     * corresponding argument value.
     */
    const Expr* rem_args = args;
    for (size_t i = 0; i < ctx->formals_num && !expr_is_nil(rem_args); i++) {
        const enum EEnvErr code =
          env_bind(ctx->env, ctx->formals[i], CAR(rem_args), ENV_FLAG_NONE);
        SL_EXPECT(code == ENV_ERR_NONE,
                  "Could not bind symbol `%s': %s",
                  ctx->formals[i],
                  env_strerror(code));

        rem_args = CDR(rem_args);
    }

    /* If the lambda has a "&rest" formal, bind it */
    if (ctx->formal_rest != NULL) {
        Expr* rest_list = expr_clone_tree(rem_args);
        const enum EEnvErr code =
          env_bind(ctx->env, ctx->formal_rest, rest_list, ENV_FLAG_NONE);
        SL_EXPECT(code == ENV_ERR_NONE,
                  "Could not bind symbol `%s': %s",
                  ctx->formal_rest,
                  env_strerror(code));
    }

    /*
     * Evaluate each expression in the body of the lambda, using its environment
     * with the bound the formal arguments. Return the last evaluated
     * expression.
     */
    Expr* last_evaluated = NULL;
    for (Expr* exprs = ctx->body; !expr_is_nil(exprs); exprs = CDR(exprs)) {
        last_evaluated = eval(ctx->env, CAR(exprs));
        if (EXPR_ERR_P(last_evaluated))
            break;
    }

    return last_evaluated;
}

Expr* lambda_call(Env* env, Expr* func, Expr* args) {
    SL_UNUSED(env);
    SL_ASSERT(EXPR_LAMBDA_P(func));
    return lambdactx_eval_body(func->val.lambda, args);
}

Expr* macro_expand(Env* env, Expr* func, Expr* args) {
    SL_UNUSED(env);
    SL_ASSERT(EXPR_MACRO_P(func));
    return lambdactx_eval_body(func->val.lambda, args);
}

Expr* macro_call(Env* env, Expr* func, Expr* args) {
    Expr* expansion = macro_expand(env, func, args);
    if (EXPR_ERR_P(expansion))
        return expansion;

    /* Calling a macro is just evaluation its macro exansion */
    return eval(env, expansion);
}
