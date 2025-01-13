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
    /* Initialize output variables */
    *mandatory = 0;
    *has_rest  = false;

    for (const Expr* cur = list; cur != NULL; cur = cur->next) {
        if (cur->type != EXPR_SYMBOL)
            return LAMBDACTX_ERR_FORMALTYPE;

        /* Only a single formal can appear after "&rest" */
        if (strcmp(cur->val.s, "&rest") == 0) {
            if (cur->next == NULL || cur->next->next != NULL)
                return LAMBDACTX_ERR_NOREST;
            *has_rest = true;
            break;
        }

        *mandatory += 1;
    }

    return LAMBDACTX_ERR_NONE;
}

/*----------------------------------------------------------------------------*/

const char* lambdactx_strerror(enum ELambdaCtxErr code) {
    const char* s;
    switch (code) {
        case LAMBDACTX_ERR_NONE:
            s = "No error.";
            break;
        case LAMBDACTX_ERR_FORMALTYPE:
            s = "Invalid type for formal argument. Expected 'Symbol'.";
            break;
        case LAMBDACTX_ERR_NOREST:
            s = "Exactly 1 formal must appear after `&rest' keyword.";
            break;
    }
    return s;
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

enum ELambdaCtxErr lambdactx_init(LambdaCtx* ctx, const Expr* formals,
                                   const Expr* body) {
    SL_ASSERT(formals->type == EXPR_PARENT);

    /* Count and validate the formal arguments */
    size_t mandatory;
    bool has_rest;
    const enum ELambdaCtxErr formal_err =
      count_formals(formals->val.children, &mandatory, &has_rest);
    if (formal_err != LAMBDACTX_ERR_NONE)
        return formal_err;

    /*
     * Initialize the `LambdaCtx' structure with:
     *   - A new environment whose parent will be set when making the actual
     *     function call.
     *   - A string array for the formal arguments of the function, the first
     *     argument of `lambda'. It will be filled below.
     *   - The body of the function, the rest of the arguments of `lambda'.
     *
     * Note that since `lambda' is a special form and they are handled
     * differently in `eval', we assume that the Lisp arguments were not
     * evaluated implicitly.
     */
    ctx->env         = env_new();
    ctx->formals_num = mandatory;
    ctx->formals     = mem_alloc(mandatory * sizeof(char*));
    ctx->formal_rest = NULL;
    /*
     * TODO: Should we clone the expressions, or store the original references?
     */
    ctx->body        = expr_list_clone(body);

    /*
     * For each formal argument we counted above, store the symbol as a C string
     * in the array we just allocated. Note that we already verified that all of
     * the formals are symbols when counting them in `count_formals'.
     */
    const Expr* cur_formal = formals->val.children;
    for (size_t i = 0; i < mandatory; i++) {
        ctx->formals[i] = strdup(cur_formal->val.s);
        cur_formal      = cur_formal->next;
    }

    if (has_rest) {
        /* Skip "&rest" and save the symbol after it on the context */
        cur_formal       = cur_formal->next;
        ctx->formal_rest = strdup(cur_formal->val.s);
    }

    return LAMBDACTX_ERR_NONE;
}

LambdaCtx* lambdactx_clone(const LambdaCtx* ctx) {
    /* Allocate a new LambdaCtx structure */
    LambdaCtx* ret = mem_alloc(sizeof(LambdaCtx));

    /* Copy the environment and the list of body expressions */
    ret->env  = env_clone(ctx->env);
    /*
     * TODO: Should we clone the expressions, or store the original references?
     */
    ret->body = expr_list_clone(ctx->body);

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
     * 1. Free the lambda environment, which shouldn't be in use anymore.
     *    Expressions in that environment are not freed, so they can still be
     *    used somewhere else.
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

    if (!expr_list_equal(a->body, b->body))
        return false;

    return true;
}

/*----------------------------------------------------------------------------*/

void lambdactx_print_args(FILE* fp, const LambdaCtx* ctx) {
    /* Position in the `ctx->formals' array, shared across all argument types */
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

static Expr* lambdactx_eval_body(Env* env, LambdaCtx* ctx, Expr* args) {
    /* Count the number of arguments that we received */
    const size_t arg_num = expr_list_len(args);

    /* Make sure the number of arguments that we got is what we expected */
    SL_EXPECT(ctx->formal_rest != NULL || arg_num == ctx->formals_num,
              "Invalid number of arguments. Expected %d, got %d.",
              ctx->formals_num,
              arg_num);

    /*
     * In the lambda's environment, bind each mandatory formal argument to its
     * corresponding argument value.
     */
    Expr* cur_arg = args;
    for (size_t i = 0; i < ctx->formals_num && cur_arg != NULL; i++) {
        const bool bound =
          env_bind(ctx->env, ctx->formals[i], cur_arg, ENV_FLAG_NONE);
        SL_EXPECT(bound, "Could not bind symbol `%s'.", ctx->formals[i]);

        cur_arg = cur_arg->next;
    }

    /* If the lambda has a "&rest" formal, bind it */
    if (ctx->formal_rest != NULL) {
        Expr* rest_list         = expr_new(EXPR_PARENT);
        rest_list->val.children = cur_arg;

        const bool bound =
          env_bind(ctx->env, ctx->formal_rest, rest_list, ENV_FLAG_NONE);
        SL_EXPECT(bound, "Could not bind symbol `%s'.", ctx->formal_rest);
    }

    /*
     * Set the environment used when calling the lambda as the parent
     * environment of the lambda itself. It's important that we set this now,
     * and not when defining the lambda.
     */
    ctx->env->parent = env;

    /*
     * Evaluate each expression in the body of the lambda, using its environment
     * with the bound the formal arguments. Return the last evaluated
     * expression.
     */
    Expr* last_evaluated = NULL;
    for (Expr* cur = ctx->body; cur != NULL; cur = cur->next) {
        last_evaluated = eval(ctx->env, cur);
        if (EXPRP_ERR(last_evaluated))
            break;
    }

    return last_evaluated;
}

Expr* lambda_call(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(func->type == EXPR_LAMBDA);
    return lambdactx_eval_body(env, func->val.lambda, args);
}

Expr* macro_expand(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(func->type == EXPR_MACRO);
    return lambdactx_eval_body(env, func->val.lambda, args);
}

Expr* macro_call(Env* env, Expr* func, Expr* args) {
    Expr* expansion = macro_expand(env, func, args);
    if (EXPRP_ERR(expansion))
        return expansion;

    /* Calling a macro is just evaluation its macro exansion */
    return eval(env, expansion);
}
