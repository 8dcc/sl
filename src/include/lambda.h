
#ifndef LAMBDA_H_
#define LAMBDA_H_ 1

#include <stddef.h>

struct Expr; /* expr.h */
struct Env;  /* env.h */

typedef struct LambdaCtx LambdaCtx;

struct LambdaCtx {
    struct Env* env;
    char** formals;
    size_t formals_num;
    struct Expr* body;
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
 * Call the specified lambda `func' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* lambda_call(struct Env* env, struct Expr* func, struct Expr* args);

#endif /* LAMBDA_H_ */
