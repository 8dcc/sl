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
 *
 * ============================================================================
 *
 * This source implements a simple pool allocator for expressions, meant to be
 * integrated with the garbage collector. The pool allocator is based on my
 * 'libpool' project, so see that for more detailed comments
 * <https://github.com/8dcc/libpool>, along with my blog article
 * <https://8dcc.github.io/programming/pool-allocator.html>.
 *
 * The pool is simply an array of `PoolNode' structures. This `PoolNode'
 * structure is a bit different from the one used in the blog article, since it
 * contains the union (with the "next free" pointer and the expression itself),
 * but also a `flags' member, used for garbage collection. See the enum and
 * structure definitions below for more information.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "include/expr_pool.h"
#include "include/lambda.h"
#include "include/memory.h"
#include "include/error.h"

/*----------------------------------------------------------------------------*/
/* Globals */

ExprPool* g_expr_pool = NULL;

/*----------------------------------------------------------------------------*/
/* Static functions */

/*
 * Free all previously-allocated members of an expression when necessary.
 * Doesn't free the `Expr' structure itself.
 */
static void free_expr_members(Expr* e) {
    SL_ASSERT(e != NULL);

    switch (e->type) {
        case EXPR_PARENT:
            expr_list_free(e->val.children);
            break;

        case EXPR_ERR:
        case EXPR_SYMBOL:
        case EXPR_STRING:
            free(e->val.s);
            break;

        case EXPR_LAMBDA:
        case EXPR_MACRO:
            lambda_ctx_free(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_PRIM:
            break;
    }
}

/*----------------------------------------------------------------------------*/
/* Public pool-related functions */

bool pool_init(size_t pool_sz) {
    SL_ASSERT(g_expr_pool == NULL);

    g_expr_pool   = mem_alloc(sizeof(ExprPool));
    PoolNode* arr = g_expr_pool->free_node =
      mem_alloc(pool_sz * sizeof(PoolNode));

    for (size_t i = 0; i < pool_sz - 1; i++) {
        arr[i].val.next = &arr[i + 1];
        arr[i].flags    = NODE_FREE;
    }
    arr[pool_sz - 1].val.next = NULL;
    arr[pool_sz - 1].flags    = NODE_FREE;

    g_expr_pool->array_starts         = mem_alloc(sizeof(ArrayStart));
    g_expr_pool->array_starts->next   = NULL;
    g_expr_pool->array_starts->arr    = arr;
    g_expr_pool->array_starts->arr_sz = pool_sz;

    return true;
}

bool pool_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL && extra_sz > 0);

    ArrayStart* array_start = mem_alloc(sizeof(ArrayStart));
    PoolNode* extra_arr     = mem_alloc(extra_sz * sizeof(PoolNode));

    /* Link the new free nodes together */
    for (size_t i = 0; i < extra_sz - 1; i++) {
        extra_arr[i].val.next = &extra_arr[i + 1];
        extra_arr[i].flags    = NODE_FREE;
    }

    /* Prepend the new node array to the linked list of free nodes */
    extra_arr[extra_sz - 1].val.next = g_expr_pool->free_node;
    extra_arr[extra_sz - 1].flags    = NODE_FREE;
    g_expr_pool->free_node           = extra_arr;

    /* Prepend to the linked list of array starts */
    array_start->arr          = extra_arr;
    array_start->arr_sz       = extra_sz;
    array_start->next         = g_expr_pool->array_starts;
    g_expr_pool->array_starts = array_start;

    return true;
}

void pool_close(void) {
    if (g_expr_pool == NULL)
        return;

    ArrayStart* array_start = g_expr_pool->array_starts;
    while (array_start != NULL) {
        ArrayStart* next = array_start->next;
        free(array_start->arr);
        free(array_start);
        array_start = next;
    }

    free(g_expr_pool);
    g_expr_pool = NULL;
}

/*----------------------------------------------------------------------------*/
/* Public node-related functions */

Expr* pool_alloc(void) {
    SL_ASSERT(g_expr_pool != NULL);
    if (g_expr_pool->free_node == NULL)
        return NULL;

    PoolNode* result       = g_expr_pool->free_node;
    g_expr_pool->free_node = g_expr_pool->free_node->val.next;

    result->flags &= ~NODE_FREE;
    return &result->val.expr;
}

Expr* pool_alloc_or_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL);
    if (g_expr_pool->free_node == NULL && !pool_expand(extra_sz))
        return NULL;

    return pool_alloc();
}

void pool_free(Expr* e) {
    SL_ASSERT(g_expr_pool != NULL);
    if (e == NULL)
        return;

    /*
     * Before freeing the expression we have to free all its members. They are
     * currently allocated using the functions in 'memory.c', not with a pool.
     */
    free_expr_members(e);

    PoolNode* node         = expr2node(e);
    node->val.next         = g_expr_pool->free_node;
    g_expr_pool->free_node = node;
}
