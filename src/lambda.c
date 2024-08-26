
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"

/* Sum the total number of formal arguments in a `LambdaCtx' structure. In other
 * words, the number of elements inside `ctx->formals[]'. */
static size_t sum_formals(const LambdaCtx* ctx) {
    return ctx->formals_mandatory + ctx->formals_optional +
           (ctx->formals_rest ? 1 : 0);
}

/* Count the number of mandatory and optional formal arguments in a list. */
static bool count_formals(const Expr* list, size_t* mandatory, size_t* optional,
                          bool* has_rest) {
    SL_ON_ERR(return false);

    /* Initialize output variables */
    *mandatory = 0;
    *optional  = 0;
    *has_rest  = false;

    /* Current stage when parsing the formal argument list */
    enum {
        READING_MANDATORY,
        READING_OPTIONAL,
        READING_REST,
    } state = READING_MANDATORY;

    for (const Expr* cur = list; cur != NULL; cur = cur->next) {
        SL_EXPECT(cur->type == EXPR_SYMBOL,
                  "Invalid formal argument expected type 'Symbol', got '%s'.",
                  exprtype2str(cur->type));

        if (strcmp(cur->val.s, "&optional") == 0) {
            /* Check if we should change state from mandatory to optional, and
             * continue reading. */
            SL_EXPECT(cur->next != NULL && state == READING_MANDATORY,
                      "Wrong usage of `&optional' keyword.");
            state = READING_OPTIONAL;
            continue;
        } else if (strcmp(cur->val.s, "&rest") == 0) {
            SL_EXPECT(cur->next != NULL && state != READING_REST,
                      "Wrong usage of `&rest' keyword.");
            state = READING_REST;
            continue;
        }

        switch (state) {
            case READING_MANDATORY:
                *mandatory += 1;
                break;
            case READING_OPTIONAL:
                *optional += 1;
                break;
            case READING_REST:
                /* Verify that the next argument to "&rest" is the last one. */
                SL_EXPECT(cur->next == NULL,
                          "Expected exactly 1 formal after `&rest' keyword.");
                *has_rest = true;
                break;
        }
    }

    return true;
}

/*----------------------------------------------------------------------------*/

LambdaCtx* lambda_ctx_new(const Expr* formals, const Expr* body) {
    SL_ASSERT(formals->type == EXPR_PARENT,
              "Expected list of formal arguments.");

    /* Count and validate the formal arguments */
    size_t mandatory;
    size_t optional;
    bool has_rest;
    if (!count_formals(formals->val.children, &mandatory, &optional, &has_rest))
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
    LambdaCtx* ret = sl_safe_malloc(sizeof(LambdaCtx));
    ret->env       = env_new();
    ret->body      = expr_clone_list(body);

    ret->formals_mandatory     = mandatory;
    ret->formals_optional      = optional;
    ret->formals_rest          = has_rest;
    const size_t total_formals = sum_formals(ret);

    ret->formals = sl_safe_malloc(total_formals * sizeof(char*));

    /*
     * For each formal argument we counted above, store the symbol as a C string
     * in the array we just allocated. Note that we already verified that all of
     * the formals are symbols when counting them in `count_formals'.
     */
    const Expr* cur_formal = formals->val.children;
    size_t formals_pos     = 0;
    for (size_t i = 0; i < mandatory; i++) {
        ret->formals[formals_pos++] = strdup(cur_formal->val.s);
        cur_formal                  = cur_formal->next;
    }

    if (optional > 0) {
        /* Skip "&optional" */
        cur_formal = cur_formal->next;

        for (size_t i = 0; i < optional; i++) {
            ret->formals[formals_pos++] = strdup(cur_formal->val.s);
            cur_formal                  = cur_formal->next;
        }
    }

    if (has_rest) {
        /* Skip "&rest" */
        cur_formal = cur_formal->next;

        /* There should only be one formal left after "&rest" */
        ret->formals[formals_pos] = strdup(cur_formal->val.s);
    }

    return ret;
}

LambdaCtx* lambda_ctx_clone(const LambdaCtx* ctx) {
    /* Allocate a new LambdaCtx structure */
    LambdaCtx* ret = sl_safe_malloc(sizeof(LambdaCtx));

    /* Copy the environment and the list of body expressions */
    ret->env  = env_clone(ctx->env);
    ret->body = expr_clone_list(ctx->body);

    /* Copy the number of formals of each type */
    ret->formals_mandatory = ctx->formals_mandatory;
    ret->formals_optional  = ctx->formals_optional;
    ret->formals_rest      = ctx->formals_rest;

    /* Get the total number of formal arguments */
    const size_t total_formals = sum_formals(ret);

    /* Allocate a new string array for the formals, and copy them */
    ret->formals = sl_safe_malloc(total_formals * sizeof(char*));
    for (size_t i = 0; i < total_formals; i++)
        ret->formals[i] = sl_safe_strdup(ctx->formals[i]);

    return ret;
}

void lambda_ctx_free(LambdaCtx* ctx) {
    /* First, free the environment and the body of the lambda */
    env_free(ctx->env);
    expr_free(ctx->body);

    /* Free each formal argument string, and the array itself */
    for (size_t i = 0; i < sum_formals(ctx); i++)
        free(ctx->formals[i]);
    free(ctx->formals);

    /* And finally, the LambdaCtx structure itself */
    free(ctx);
}

void lambda_ctx_print_args(const LambdaCtx* ctx) {
    /* Position in the `ctx->formals' array, shared across all argument types */
    size_t formals_pos = 0;

    putchar('(');

    /* Print mandatory arguments */
    for (size_t i = 0; i < ctx->formals_mandatory; i++) {
        if (formals_pos > 0)
            putchar(' ');
        printf("%s", ctx->formals[formals_pos++]);
    }

    /* Print optional arguments */
    if (ctx->formals_optional > 0) {
        if (formals_pos > 0)
            putchar(' ');
        printf("&optional ");

        for (size_t i = 0; i < ctx->formals_optional; i++) {
            if (i > 0)
                putchar(' ');
            printf("%s", ctx->formals[formals_pos++]);
        }
    }

    /* There can only be one argument after "&rest" */
    if (ctx->formals_rest) {
        if (formals_pos > 0)
            putchar(' ');
        printf("&rest %s", ctx->formals[formals_pos]);
    }

    putchar(')');
}

/*----------------------------------------------------------------------------*/

Expr* lambda_call(Env* env, Expr* func, Expr* args) {
    SL_ASSERT(func->type == EXPR_LAMBDA,
              "Expected function of type 'Lambda', got '%s'.",
              exprtype2str(func->type));

    SL_ON_ERR(return NULL);

    /* Count the number of arguments that we received */
    const size_t arg_num = expr_list_len(args);

    /* Make sure the number of arguments that we got is what we expected */
    /* TODO: Valid checks for optional/rest arguments. Set ommited optionals to
     * 'nil'. */
    SL_EXPECT(func->val.lambda->formals_mandatory == arg_num,
              "Invalid number of arguments. Expected %d, got %d.",
              func->val.lambda->formals_mandatory, arg_num);

    Expr* cur_arg = args;
    for (size_t i = 0; i < arg_num && cur_arg != NULL; i++) {
        /* In the lambda's environment, bind the i-th formal argument to its
         * corresponding argument value */
        env_bind(func->val.lambda->env, func->val.lambda->formals[i], cur_arg);

        /* Move to the next argument value */
        cur_arg = cur_arg->next;
    }

    /* Set the environment used when calling the lambda as the parent
     * environment of the lambda itself. It's important that we set this now,
     * and not when defining the lambda. */
    func->val.lambda->env->parent = env;

    /* Evaluate each expression in the body of the lambda, using its environment
     * with the bound the formal arguments. Return the last evaluated
     * expression. */
    Expr* last_evaluated = NULL;
    for (Expr* cur = func->val.lambda->body; cur != NULL; cur = cur->next) {
        expr_free(last_evaluated);
        last_evaluated = eval(func->val.lambda->env, cur);
    }

    return last_evaluated;
}
