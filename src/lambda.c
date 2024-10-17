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

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"

/* Count and validate the number of formal arguments in a list */
static bool count_formals(const Expr* list, size_t* mandatory, bool* has_rest) {
    SL_ON_ERR(return false);

    /* Initialize output variables */
    *mandatory = 0;
    *has_rest  = false;

    for (const Expr* cur = list; cur != NULL; cur = cur->next) {
        SL_EXPECT(cur->type == EXPR_SYMBOL,
                  "Invalid formal argument expected type 'Symbol', got '%s'.",
                  exprtype2str(cur->type));

        /* Only a single formal can appear after "&rest" */
        if (strcmp(cur->val.s, "&rest") == 0) {
            SL_EXPECT(cur->next != NULL && cur->next->next == NULL,
                      "Expected exactly 1 formal after `&rest' keyword.");
            *has_rest = true;
            break;
        }

        *mandatory += 1;
    }

    return true;
}

LambdaCtx* lambda_ctx_new(const Expr* formals, const Expr* body) {
    SL_ASSERT(formals->type == EXPR_PARENT);

    /* Count and validate the formal arguments */
    size_t mandatory;
    bool has_rest;
    if (!count_formals(formals->val.children, &mandatory, &has_rest))
        return NULL;

    /*
     * Create a new LambdaCtx structure that will contain:
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
    LambdaCtx* ret   = sl_safe_malloc(sizeof(LambdaCtx));
    ret->env         = env_new();
    ret->formals_num = mandatory;
    ret->formals     = sl_safe_malloc(mandatory * sizeof(char*));
    ret->formal_rest = NULL;
    ret->body        = expr_list_clone(body);

    /*
     * For each formal argument we counted above, store the symbol as a C string
     * in the array we just allocated. Note that we already verified that all of
     * the formals are symbols when counting them in `count_formals'.
     */
    const Expr* cur_formal = formals->val.children;
    for (size_t i = 0; i < mandatory; i++) {
        ret->formals[i] = strdup(cur_formal->val.s);
        cur_formal      = cur_formal->next;
    }

    if (has_rest) {
        /* Skip "&rest" and save the symbol after it on the context */
        cur_formal       = cur_formal->next;
        ret->formal_rest = strdup(cur_formal->val.s);
    }

    return ret;
}

LambdaCtx* lambda_ctx_clone(const LambdaCtx* ctx) {
    /* Allocate a new LambdaCtx structure */
    LambdaCtx* ret = sl_safe_malloc(sizeof(LambdaCtx));

    /* Copy the environment and the list of body expressions */
    ret->env  = env_clone(ctx->env);
    ret->body = expr_list_clone(ctx->body);

    /* Allocate a new string array for the mandatory formals, and copy them */
    ret->formals_num = ctx->formals_num;
    ret->formals     = sl_safe_malloc(ret->formals_num * sizeof(char*));
    for (size_t i = 0; i < ret->formals_num; i++)
        ret->formals[i] = sl_safe_strdup(ctx->formals[i]);

    /* If it had a "&rest" formal, copy it */
    ret->formal_rest =
      (ctx->formal_rest == NULL) ? NULL : sl_safe_strdup(ctx->formal_rest);

    return ret;
}

void lambda_ctx_free(LambdaCtx* ctx) {
    /* First, free the environment and the body of the lambda */
    env_free(ctx->env);
    expr_list_free(ctx->body);

    /* Free each formal argument string, and the array itself */
    for (size_t i = 0; i < ctx->formals_num; i++)
        free(ctx->formals[i]);
    free(ctx->formals);

    /* Free the "&rest" formal */
    free(ctx->formal_rest);

    /* And finally, the LambdaCtx structure itself */
    free(ctx);
}

/*----------------------------------------------------------------------------*/

bool lambda_ctx_equal(const LambdaCtx* a, const LambdaCtx* b) {
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

void lambda_ctx_print_args(FILE* fp, const LambdaCtx* ctx) {
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

static Expr* lambda_ctx_eval_body(Env* env, LambdaCtx* ctx, Expr* args) {
    SL_ON_ERR(return NULL);

    /* Count the number of arguments that we received */
    const size_t arg_num = expr_list_len(args);

    /* Make sure the number of arguments that we got is what we expected */
    SL_EXPECT(ctx->formal_rest != NULL || arg_num == ctx->formals_num,
              "Invalid number of arguments. Expected %d, got %d.",
              ctx->formals_num, arg_num);

    /*
     * In the lambda's environment, bind each mandatory formal argument to its
     * corresponding argument value.
     */
    Expr* cur_arg = args;
    for (size_t i = 0; i < ctx->formals_num && cur_arg != NULL; i++) {
        if (!env_bind(ctx->env, ctx->formals[i], cur_arg, ENV_FLAG_NONE))
            return NULL;

        cur_arg = cur_arg->next;
    }

    /* If the lambda has a "&rest" formal, bind it */
    if (ctx->formal_rest != NULL) {
        Expr* rest_list         = expr_new(EXPR_PARENT);
        rest_list->val.children = cur_arg;

        if (!env_bind(ctx->env, ctx->formal_rest, rest_list, ENV_FLAG_NONE))
            return NULL;

        rest_list->val.children = NULL;
        expr_free(rest_list);
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
        expr_free(last_evaluated);
        last_evaluated = eval(ctx->env, cur);
        if (last_evaluated == NULL)
            return NULL;
    }

    return last_evaluated;
}

Expr* lambda_call(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(func->type == EXPR_LAMBDA);
    return lambda_ctx_eval_body(env, func->val.lambda, args);
}

Expr* macro_expand(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(func->type == EXPR_MACRO);
    return lambda_ctx_eval_body(env, func->val.lambda, args);
}

Expr* macro_call(Env* env, Expr* func, Expr* args) {
    Expr* expansion = macro_expand(env, func, args);
    if (expansion == NULL)
        return NULL;

    /* Calling a macro is just evaluation its macro exansion */
    Expr* evaluated = eval(env, expansion);
    expr_free(expansion);

    return evaluated;
}
