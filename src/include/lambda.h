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

#ifndef LAMBDA_H_
#define LAMBDA_H_ 1

#include <stddef.h>
#include <stdio.h> /* FILE */

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

/*----------------------------------------------------------------------------*/

/*
 * Are two `LambdaCtx' structures equal? Uses `strcmp' and `expr_list_equal'.
 */
bool lambda_ctx_equal(const LambdaCtx* a, const LambdaCtx* b);

/*----------------------------------------------------------------------------*/

/*
 * Print all the formal arguments of a `LambdaCtx' structure, just like they
 * would be written on a lambda declaration.
 */
void lambda_ctx_print_args(FILE* fp, const LambdaCtx* ctx);

/*----------------------------------------------------------------------------*/

/*
 * Call the specified lambda `func' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* lambda_call(struct Env* env, struct Expr* func, struct Expr* args);

/*
 * Expand the specified `macro' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* macro_expand(struct Env* env, struct Expr* macro, struct Expr* args);

/*
 * Call the specified `macro' in the specified environment `env' with the
 * specified arguments `args'.
 */
Expr* macro_call(struct Env* env, struct Expr* func, struct Expr* args);

#endif /* LAMBDA_H_ */
