
#ifndef LAMBDA_H_
#define LAMBDA_H_ 1

#include <stddef.h>

struct Expr; /* expr.h */
struct Env;  /* env.h */

typedef struct LambdaCtx LambdaCtx;

struct LambdaCtx {
    /* Environment for binding the formal arguments to the actual values when
     * calling the lambda. */
    struct Env* env;

    /* Mandatory formal arguments */
    char** formals;
    size_t formals_num;

    /* Non-mandatory arguments will be placed on a list bound to a symbol after
     * the "&rest" keyword. */
    char* formal_rest;

    /* List of expressions to be evaluated in order when calling the lambda */
    Expr* body;
};

/*
 * Allocate and initialize a new `LambdaCtx' structure using the specified
 * formal arguments and the specified list of body expressions.
 */
LambdaCtx* lambda_ctx_new(const struct Expr* formals, const struct Expr* body);

/*
 * Copy the specified `LambdaCtx' structure into an allocated copy, and return
 * it.
 */
LambdaCtx* lambda_ctx_clone(const LambdaCtx* ctx);

/*
 * Free all members of a `LambdaCtx' structure, and the structure itself.
 */
void lambda_ctx_free(LambdaCtx* ctx);

/*
 * Print all the formal arguments of a `LambdaCtx' structure, just like they
 * would be written on a lambda declaration.
 */
void lambda_ctx_print_args(const LambdaCtx* ctx);

/*
 * Call the specified lambda `func' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* lambda_call(struct Env* env, struct Expr* func, struct Expr* args);

/*
 * Expand the specified `macro' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* macro_expand(Env* env, Expr* macro, Expr* args);

/*
 * Call the specified `macro' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* macro_call(Env* env, Expr* func, Expr* args);

#endif /* LAMBDA_H_ */
