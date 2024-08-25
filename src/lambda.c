
#include <stddef.h>

#include "include/expr.h"
#include "include/env.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/eval.h"

LambdaCtx* lambda_ctx_new(const Expr* formals, const Expr* body) {
    SL_ASSERT(formals->type == EXPR_PARENT,
              "Expected list of formal arguments.");

    /* Count the number of formal arguments, and verify that they are all
     * symbols. */
    size_t formals_num = 0;
    for (const Expr* cur = formals->val.children; cur != NULL;
         cur             = cur->next) {
        if (cur->type != EXPR_SYMBOL) {
            ERR("Formal arguments of `lambda' must be of type 'Symbol', got "
                "'%s'.",
                exprtype2str(cur->type));
            return NULL;
        }

        formals_num++;
    }

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
    ret->formals     = sl_safe_malloc(formals_num * sizeof(char*));
    ret->formals_num = formals_num;
    ret->body        = expr_clone_list(body);

    const Expr* cur_formal = formals->val.children;
    for (size_t i = 0; i < formals_num; i++) {
        /* Store the symbol as a C string in the array we just allocated. Note
         * that we already verified that all of the formals are symbols when
         * counting the arguments. */
        ret->formals[i] = sl_safe_strdup(cur_formal->val.s);

        /* Go to the next formal argument */
        cur_formal = cur_formal->next;
    }

    return ret;
}

LambdaCtx* lambda_ctx_clone(const LambdaCtx* ctx) {
    /* Allocate a new LambdaCtx structure */
    LambdaCtx* ret = sl_safe_malloc(sizeof(LambdaCtx));

    /* Copy the environment and the list of body expressions */
    ret->env  = env_clone(ctx->env);
    ret->body = expr_clone_list(ctx->body);

    /* Allocate a new string array for the formals, and copy them */
    ret->formals_num = ctx->formals_num;
    ret->formals     = sl_safe_malloc(ret->formals_num * sizeof(char*));
    for (size_t i = 0; i < ret->formals_num; i++)
        ret->formals[i] = sl_safe_strdup(ctx->formals[i]);

    return ret;
}

void lambda_ctx_free(LambdaCtx* ctx) {
    /* First, free the environment and the body of the lambda */
    env_free(ctx->env);
    expr_free(ctx->body);

    /* Free each formal argument string, and the array itself */
    for (size_t i = 0; i < ctx->formals_num; i++)
        free(ctx->formals[i]);
    free(ctx->formals);

    /* And finally, the LambdaCtx structure itself */
    free(ctx);
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
    /* TODO: Add optional arguments */
    SL_EXPECT(func->val.lambda->formals_num == arg_num,
              "Invalid number of arguments. Expected %d, got %d.",
              func->val.lambda->formals_num, arg_num);

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
