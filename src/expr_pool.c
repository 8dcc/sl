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

#if defined(SL_NO_POOL_VALGRIND)
#define VALGRIND_CREATE_MEMPOOL(a, b, c)
#define VALGRIND_DESTROY_MEMPOOL(a)
#define VALGRIND_MEMPOOL_ALLOC(a, b, c)
#define VALGRIND_MEMPOOL_FREE(a, b)
#define VALGRIND_MAKE_MEM_DEFINED(a, b)
#define VALGRIND_MAKE_MEM_NOACCESS(a, b)
#else
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#endif

/*----------------------------------------------------------------------------*/
/* Globals */

ExprPool* g_expr_pool = NULL;

/*----------------------------------------------------------------------------*/
/* Static functions */

/*
 * Is the specified node flagged as free?
 */
static inline bool pool_node_is_free(PoolNode* node) {
    return (pool_node_flags(node) & NODE_FLAG_FREE) != 0;
}

/*
 * Free all previously-allocated members of an expression when necessary.
 * Doesn't free other expressions, just members that were allocated using
 * 'mem_alloc' or similar. Doesn't free the `Expr' structure itself.
 */
static void free_heap_expr_members(Expr* e) {
    SL_ASSERT(e != NULL);

    switch (e->type) {
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
        case EXPR_PARENT:
            break;
    }
}

/*----------------------------------------------------------------------------*/
/* Public wrappers */

enum EPoolNodeFlags pool_node_flags(PoolNode* node) {
    VALGRIND_MAKE_MEM_DEFINED(&node->flags, sizeof(enum EPoolNodeFlags));
    const enum EPoolNodeFlags result = node->flags;
    VALGRIND_MAKE_MEM_NOACCESS(&node->flags, sizeof(enum EPoolNodeFlags));
    return result;
}

void pool_node_flag_set(PoolNode* node, enum EPoolNodeFlags flag) {
    VALGRIND_MAKE_MEM_DEFINED(&node->flags, sizeof(enum EPoolNodeFlags));
    node->flags |= flag;
    VALGRIND_MAKE_MEM_NOACCESS(&node->flags, sizeof(enum EPoolNodeFlags));
}

void pool_node_flag_unset(PoolNode* node, enum EPoolNodeFlags flag) {
    VALGRIND_MAKE_MEM_DEFINED(&node->flags, sizeof(enum EPoolNodeFlags));
    node->flags &= ~flag;
    VALGRIND_MAKE_MEM_NOACCESS(&node->flags, sizeof(enum EPoolNodeFlags));
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
        arr[i].flags    = NODE_FLAG_FREE;
    }
    arr[pool_sz - 1].val.next = NULL;
    arr[pool_sz - 1].flags    = NODE_FLAG_FREE;

    g_expr_pool->array_starts         = mem_alloc(sizeof(ArrayStart));
    g_expr_pool->array_starts->next   = NULL;
    g_expr_pool->array_starts->arr    = arr;
    g_expr_pool->array_starts->arr_sz = pool_sz;

    VALGRIND_MAKE_MEM_NOACCESS(arr, pool_sz * sizeof(PoolNode));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool->array_starts, sizeof(ArrayStart));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool, sizeof(ExprPool));
    VALGRIND_CREATE_MEMPOOL(g_expr_pool, sizeof(enum EPoolNodeFlags), 0);

    return true;
}

bool pool_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL && extra_sz > 0);
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));

    ArrayStart* array_start = mem_alloc(sizeof(ArrayStart));
    PoolNode* extra_arr     = mem_alloc(extra_sz * sizeof(PoolNode));

    /* Link the new free nodes together */
    for (size_t i = 0; i < extra_sz - 1; i++) {
        extra_arr[i].val.next = &extra_arr[i + 1];
        extra_arr[i].flags    = NODE_FLAG_FREE;
    }

    /* Prepend the new node array to the linked list of free nodes */
    extra_arr[extra_sz - 1].val.next = g_expr_pool->free_node;
    extra_arr[extra_sz - 1].flags    = NODE_FLAG_FREE;
    g_expr_pool->free_node           = extra_arr;

    /* Prepend to the linked list of array starts */
    array_start->arr          = extra_arr;
    array_start->arr_sz       = extra_sz;
    array_start->next         = g_expr_pool->array_starts;
    g_expr_pool->array_starts = array_start;

    VALGRIND_MAKE_MEM_NOACCESS(extra_arr, extra_sz * sizeof(PoolNode));
    VALGRIND_MAKE_MEM_NOACCESS(array_start, sizeof(ArrayStart));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool, sizeof(ExprPool));

    return true;
}

void pool_close(void) {
    if (g_expr_pool == NULL)
        return;

    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));

    /*
     * First, free the members of all expressions in each array, because one
     * might reference another from a separate array.
     */
    ArrayStart* array_start;
    for (array_start = g_expr_pool->array_starts; array_start != NULL;
         array_start = array_start->next) {
        VALGRIND_MAKE_MEM_DEFINED(array_start, sizeof(ArrayStart));
        VALGRIND_MAKE_MEM_DEFINED(array_start->arr,
                                  array_start->arr_sz * sizeof(PoolNode));

        for (size_t i = 0; i < array_start->arr_sz; i++)
            if (!pool_node_is_free(&array_start->arr[i]))
                pool_free(&array_start->arr[i].val.expr);
    }

    /*
     * Then we can actually free the expression arrays, along with the
     * 'ArrayStart' structures themselves.
     */
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));
    for (array_start = g_expr_pool->array_starts; array_start != NULL;) {
        ArrayStart* next = array_start->next;
        free(array_start->arr);
        free(array_start);
        array_start = next;
    }

    /*
     * And the 'ExprPool' structure.
     */
    VALGRIND_DESTROY_MEMPOOL(g_expr_pool);
    free(g_expr_pool);
    g_expr_pool = NULL;
}

/*----------------------------------------------------------------------------*/
/* Public node-related functions */

Expr* pool_alloc(void) {
    SL_ASSERT(g_expr_pool != NULL);
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));

    if (g_expr_pool->free_node == NULL)
        return NULL;
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool->free_node, sizeof(PoolNode*));

    PoolNode* result       = g_expr_pool->free_node;
    g_expr_pool->free_node = g_expr_pool->free_node->val.next;

    SL_ASSERT(pool_node_is_free(result));
    pool_node_flag_unset(result, NODE_FLAG_FREE);

    VALGRIND_MEMPOOL_ALLOC(g_expr_pool, &result->val.expr, sizeof(Expr));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool->free_node, sizeof(PoolNode*));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool, sizeof(ExprPool));

    return &result->val.expr;
}

Expr* pool_alloc_or_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL);
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));
    if (g_expr_pool->free_node == NULL && !pool_expand(extra_sz))
        return NULL;

    return pool_alloc();
}

void pool_free(Expr* e) {
    SL_ASSERT(g_expr_pool != NULL);
    if (e == NULL)
        return;

    PoolNode* node = expr2node(e);

    /*
     * Avoid double-frees.
     */
    SL_ASSERT(!pool_node_is_free(node));
    pool_node_flag_set(node, NODE_FLAG_FREE);

    /*
     * Before freeing the expression we have to free its heap members. They are
     * currently allocated using the functions in 'memory.c', not with a pool.
     * Note that this function doesn't try to free any 'Expr' at all.
     */
    free_heap_expr_members(e);

    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool, sizeof(ExprPool));

    node->val.next         = g_expr_pool->free_node;
    g_expr_pool->free_node = node;

    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool, sizeof(ExprPool));
    VALGRIND_MEMPOOL_FREE(g_expr_pool, e);
}

/*----------------------------------------------------------------------------*/

void pool_print_stats(FILE* fp) {
    size_t total_free = 0, total_nodes = 0, total_arrays = 0;

    ArrayStart* a;
    POOL_FOREACH_ARRAYSTART(a) {
        size_t num_free = 0;
        for (size_t i = 0; i < a->arr_sz; i++)
            if (pool_node_is_free(&a->arr[i]))
                num_free++;

        fprintf(fp,
                "Array %zu: %zu/%zu free.\n",
                total_arrays,
                num_free,
                a->arr_sz);

        total_nodes += a->arr_sz;
        total_free += num_free;
        total_arrays++;
    }
    POOL_FOREACH_ARRAYSTART_END(a);

    fprintf(fp,
            "Total: %zu/%zu free in %zu arrays.\n",
            total_free,
            total_nodes,
            total_arrays);
}

void pool_dump(FILE* fp) {
    size_t array_count = 0;

    ArrayStart* a;
    POOL_FOREACH_ARRAYSTART(a) {
        for (size_t i = 0; i < a->arr_sz; i++) {
            const enum EPoolNodeFlags flags = pool_node_flags(&a->arr[i]);
            fprintf(fp,
                    "[%p] [%zu,%3zu] [F: %X] ",
                    &a->arr[i],
                    array_count,
                    i,
                    flags);
            if ((flags & NODE_FLAG_FREE) == 0) {
                expr_print(fp, &a->arr[i].val.expr);
                const enum EExprType type = a->arr[i].val.expr.type;
                if (type == EXPR_ERR || type == EXPR_SYMBOL ||
                    type == EXPR_STRING || type == EXPR_LAMBDA ||
                    type == EXPR_MACRO)
                    printf(" [%p]", a->arr[i].val.expr.val.s);
            } else {
                fprintf(fp, "<invalid>");
            }
            fputc('\n', fp);
        }

        array_count++;
    }
    POOL_FOREACH_ARRAYSTART_END(a);
}
