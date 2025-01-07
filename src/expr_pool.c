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
 * This source implements a simple pool allocator for expressions, along with
 * garbage collection.
 *
 * Brief explanation of the pool allocator:
 * ----------------------------------------
 *
 * The pool allocator is based on my 'libpool' project, so see that for more
 * detailed comments <https://github.com/8dcc/libpool>, along with my blog
 * article <https://8dcc.github.io/programming/pool-allocator.html>.
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
#include "include/memory.h"
#include "include/error.h"

/*----------------------------------------------------------------------------*/

ExprPool* g_expr_pool = NULL;

/*----------------------------------------------------------------------------*/

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

    g_expr_pool->array_starts       = mem_alloc(sizeof(LinkedPtr));
    g_expr_pool->array_starts->next = NULL;
    g_expr_pool->array_starts->ptr  = arr;

    return true;
}

bool pool_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL && extra_sz > 0);

    LinkedPtr* array_start = mem_alloc(sizeof(LinkedPtr));
    PoolNode* extra_arr    = mem_alloc(extra_sz * sizeof(PoolNode));

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
    array_start->ptr          = extra_arr;
    array_start->next         = g_expr_pool->array_starts;
    g_expr_pool->array_starts = array_start;

    return true;
}

void pool_close(void) {
    if (g_expr_pool == NULL)
        return;

    LinkedPtr* linkedptr = g_expr_pool->array_starts;
    while (linkedptr != NULL) {
        LinkedPtr* next = linkedptr->next;
        free(linkedptr->ptr);
        free(linkedptr);
        linkedptr = next;
    }

    free(g_expr_pool);
    g_expr_pool = NULL;
}

/*----------------------------------------------------------------------------*/

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
     * We are able to cast an `Expr' pointer to a `PoolNode' one because the
     * expression is stored (inside a union) in the first member of `PoolNode'.
     */
    PoolNode* node         = (PoolNode*)e;
    node->val.next         = g_expr_pool->free_node;
    g_expr_pool->free_node = node;
}
