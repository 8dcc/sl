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
 * this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <stddef.h>
#include <stdbool.h>

#include "include/env.h"
#include "include/expr.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/error.h"

/*
 * Structure representing an array of pointers, used for keeping track of
 * registered objects.
 *
 * No need to declare it in the header, since it's only used from the source.
 */
typedef struct {
    void** arr;
    size_t sz;
    size_t pos;
} PtrArray;

/*
 * Global array of pointers containing all registered items that could be
 * garbage-collected.
 */
static PtrArray gc_registered;

/*----------------------------------------------------------------------------*/

static bool is_ptr_in_expr(void* ptr, Expr* expr);

static bool is_ptr_in_lambdactx(void* ptr, LambdaCtx* ctx) {
    /* NOTE: We ignore `ctx->env'. */

    if (ptr == ctx->formals || ptr == ctx->formal_rest)
        return true;

    for (size_t i = 0; i < ctx->formals_num; i++)
        if (ptr == ctx->formals[i])
            return true;

    if (is_ptr_in_expr(ptr, ctx->body))
        return true;

    return false;
}

/*
 * Check if the specified pointer appears anywhere in an expression tree. Does
 * not check if the pointer is the expression itself.
 */
static bool is_ptr_in_expr(void* ptr, Expr* e) {
    switch (e->type) {
        case EXPR_PARENT:
            return ptr == e->val.children ||
                   is_ptr_in_expr(ptr, e->val.children);

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            return ptr == e->val.s;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            return ptr == e->val.lambda ||
                   is_ptr_in_lambdactx(ptr, e->val.lambda);

        case EXPR_UNKNOWN:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_PRIM:
            break;
    }

    return false;
}

/*
 * Check if the specified pointer appears anywhere in the specified environment.
 */
static bool is_ptr_in_env(void* ptr, Env* env) {
    for (size_t i = 0; i < env->size; i++) {
        if (ptr == env->bindings[i].sym || ptr == env->bindings[i].val ||
            is_ptr_in_expr(ptr, env->bindings[i].val))
            return true;
    }

    return false;
}

/*----------------------------------------------------------------------------*/

void gc_init(void) {
    gc_registered.pos = 0;
    gc_registered.sz  = 100; /* Arbitrary size */
    gc_registered.arr = mem_alloc(gc_registered.sz * sizeof(void*));
}

void gc_register(void* ptr) {
    /*
     * If the `gc_registered' array is too small, duplicate its size and
     * reallocate it. Therefore, its capacity grows exponencially.
     */
    if (gc_registered.pos >= gc_registered.sz) {
        gc_registered.sz *= 2;
        mem_realloc(gc_registered.arr, gc_registered.sz * sizeof(void*));
    }

    gc_registered.arr[gc_registered.pos++] = ptr;
}

void gc_collect_env(Env* env) {
    /*
     * This function assumes that `env' is the only environment in use. See
     * comment in the header for more details.
     */
    SL_ASSERT(env->parent == NULL);

    for (size_t i = 0; i < gc_registered.pos; i++) {
        if (is_ptr_in_env(gc_registered.arr[i], env))
            continue;

        /* TODO: Free pointer, remove from `gc_registered' */
    }
}
