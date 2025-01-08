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
#include "include/expr_pool.h"
#include "include/lambda.h"
#include "include/util.h"
#include "include/memory.h"
#include "include/garbage_collection.h"
#include "include/error.h"

/*----------------------------------------------------------------------------*/

/*
 * Mark for garbage collection the expressions in the body of the specified
 * lambda context.
 *
 * NOTE: We don't currently mark expressions in the lambda environment.
 */
static void mark_lambdactx(LambdaCtx* ctx) {
    SL_ASSERT(ctx != NULL);

    for (Expr* cur = ctx->body; cur != NULL; cur = cur->next)
        gc_mark_expr(cur);
}

/*----------------------------------------------------------------------------*/

void gc_unmark_all(void) {
    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next)
        for (size_t i = 0; i < a->arr_sz; i++)
            a->arr[i].flags &= ~NODE_FLAG_GCMARKED;
}

void gc_mark_env(Env* env) {
    SL_ASSERT(env != NULL);

    for (size_t i = 0; i < env->size; i++)
        gc_mark_expr(env->bindings[i].val);
}

void gc_mark_expr(Expr* e) {
    SL_ASSERT(e != NULL);

    switch (e->type) {
        case EXPR_PARENT:
            for (Expr* cur = e->val.children; cur != NULL; cur = cur->next)
                gc_mark_expr(cur);
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            mark_lambdactx(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
        case EXPR_PRIM:
            break;
    }

    PoolNode* node = expr2node(e);
    node->flags |= NODE_FLAG_GCMARKED;
}

void gc_collect(void) {
    /*
     * Iterate the list of array starts, then iterate the arrays themselves. We
     * collect (free) all expressions that are not marked (and not free).
     */
    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next) {
        PoolNode* cur_arr = a->arr;
        for (size_t i = 0; i < a->arr_sz; i++)
            if ((cur_arr[i].flags & (NODE_FLAG_GCMARKED | NODE_FLAG_FREE)) == 0)
                pool_free(&cur_arr[i].val.expr);
    }
}
