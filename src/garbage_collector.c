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
#include "include/garbage_collector.h"
#include "include/error.h"

/*
 * Mark an environment and its contents as currently in use.
 */
static inline void gc_mark_env(Env* env) {
    /*
     * We assume that, if an environment is marked as "used", all of its
     * expressions have been marked with 'gc_mark_expr' too, just like
     * 'gc_mark_env_contents' does. In other words, we assume that an
     * environment (and its contents) can't be "partially" marked.
     */
    if (env->is_used)
        return;

    env->is_used = true;
    gc_mark_env_contents(env);
}

/*
 * Mark an environment, its contents and all parent environments until there
 * is no parent left.
 */
static inline void gc_mark_env_and_parents(Env* env) {
    for (; env != NULL; env = env->parent)
            gc_mark_env(env);
}

/*----------------------------------------------------------------------------*/

void gc_unmark_all(void) {
    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next) {
        for (size_t i = 0; i < a->arr_sz; i++) {
            PoolItem* pool_item = &a->arr[i];
            pool_item_flag_unset(pool_item, POOL_FLAG_GCMARKED);

            /*
             * If the current expression uses a 'LambdaCtx', unmark its
             * environment and their parents.
             */
            if (!pool_item_is_free(pool_item)) {
                const Expr* e = &pool_item->val.expr;

                /*
                 * TODO: This check is repeated in 'gc_collect', we should
                 * abstract it.
                 */
                if (e->type == EXPR_LAMBDA || e->type == EXPR_MACRO)
                    for (Env* env = e->val.lambda->env; env != NULL;
                         env      = env->parent)
                        env->is_used = false;
            }
        }
    }
}

void gc_mark_env_contents(Env* env) {
    SL_ASSERT(env != NULL);

    for (size_t i = 0; i < env->size; i++)
        gc_mark_expr(env->bindings[i].val);
}

void gc_mark_expr(Expr* e) {
    SL_ASSERT(e != NULL);

    PoolItem* pool_item = pool_item_from_expr(e);
    if (pool_item_is_gcmarked(pool_item))
        return;
    pool_item_flag_set(pool_item, POOL_FLAG_GCMARKED);

    switch (e->type) {
        case EXPR_PAIR:
            gc_mark_expr(CAR(e));
            gc_mark_expr(CDR(e));
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            /*
             * Mark the environment of the lambda (along with all parents) and
             * its body.
             */
            gc_mark_env_and_parents(e->val.lambda->env);
            gc_mark_expr(e->val.lambda->body);
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
}

void gc_collect(void) {
    /*
     * Iterate the list of array starts, then iterate the arrays themselves.
     */
    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next) {
        PoolItem* cur_arr = a->arr;
        for (size_t i = 0; i < a->arr_sz; i++) {
            const PoolItem* pool_item = &cur_arr[i];

            /*
             * Current expression is either marked, or already free. Ignore.
             */
            if (pool_item_is_gcmarked(pool_item) ||
                pool_item_is_free(pool_item))
                continue;

            /*
             * Current expression is an unmarked lambda, but its environment is
             * still in use. Keep both.
             */
            const Expr* e = &pool_item->val.expr;
            if ((e->type == EXPR_LAMBDA || e->type == EXPR_MACRO) &&
                e->val.lambda->env->is_used)
                continue;

            pool_free(&cur_arr[i].val.expr);
        }
    }
}
