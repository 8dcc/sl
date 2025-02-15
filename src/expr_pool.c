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
 * The pool is simply an array of 'PoolItem' structures. This 'PoolItem'
 * structure is a bit different from the one used in the blog article, since it
 * contains the union (with the "next free" pointer and the expression itself),
 * but also a 'flags' member, used for garbage collection. See the enum and
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
#define VALGRIND_CREATE_MEMPOOL(a, b, c) ((void)0)
#define VALGRIND_DESTROY_MEMPOOL(a)      ((void)0)
#define VALGRIND_MEMPOOL_ALLOC(a, b, c)  ((void)0)
#define VALGRIND_MEMPOOL_FREE(a, b)      ((void)0)
#define VALGRIND_MAKE_MEM_DEFINED(a, b)  ((void)0)
#define VALGRIND_MAKE_MEM_NOACCESS(a, b) ((void)0)
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
 * Free all previously-allocated members of an expression when necessary.
 * Doesn't free other expressions, just members that were allocated using
 * 'mem_alloc' or similar. Doesn't free the 'Expr' structure itself.
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
            lambdactx_free(e->val.lambda);
            break;

        case EXPR_UNKNOWN:
        case EXPR_NUM_INT:
        case EXPR_NUM_FLT:
        case EXPR_PRIM:
        case EXPR_PAIR:
            break;
    }
}

/*----------------------------------------------------------------------------*/
/* Public wrappers */

enum EPoolItemFlags pool_item_flags(PoolItem* pool_item) {
    VALGRIND_MAKE_MEM_DEFINED(&pool_item->flags, sizeof(enum EPoolItemFlags));
    const enum EPoolItemFlags result = pool_item->flags;
    VALGRIND_MAKE_MEM_NOACCESS(&pool_item->flags, sizeof(enum EPoolItemFlags));
    return result;
}

void pool_item_flag_set(PoolItem* pool_item, enum EPoolItemFlags flag) {
    VALGRIND_MAKE_MEM_DEFINED(&pool_item->flags, sizeof(enum EPoolItemFlags));
    pool_item->flags |= flag;
    VALGRIND_MAKE_MEM_NOACCESS(&pool_item->flags, sizeof(enum EPoolItemFlags));
}

void pool_item_flag_unset(PoolItem* pool_item, enum EPoolItemFlags flag) {
    VALGRIND_MAKE_MEM_DEFINED(&pool_item->flags, sizeof(enum EPoolItemFlags));
    pool_item->flags &= ~flag;
    VALGRIND_MAKE_MEM_NOACCESS(&pool_item->flags, sizeof(enum EPoolItemFlags));
}

/*----------------------------------------------------------------------------*/
/* Public pool-related functions */

bool pool_init(size_t pool_sz) {
    SL_ASSERT(g_expr_pool == NULL);

    g_expr_pool   = mem_alloc(sizeof(ExprPool));
    PoolItem* arr = g_expr_pool->free_items =
      mem_alloc(pool_sz * sizeof(PoolItem));

    for (size_t i = 0; i < pool_sz - 1; i++) {
        arr[i].val.next = &arr[i + 1];
        arr[i].flags    = POOL_FLAG_FREE;
    }
    arr[pool_sz - 1].val.next = NULL;
    arr[pool_sz - 1].flags    = POOL_FLAG_FREE;

    g_expr_pool->array_starts         = mem_alloc(sizeof(ArrayStart));
    g_expr_pool->array_starts->next   = NULL;
    g_expr_pool->array_starts->arr    = arr;
    g_expr_pool->array_starts->arr_sz = pool_sz;

    VALGRIND_MAKE_MEM_NOACCESS(arr, pool_sz * sizeof(PoolItem));
    VALGRIND_CREATE_MEMPOOL(g_expr_pool, sizeof(enum EPoolItemFlags), 0);

    return true;
}

bool pool_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL && extra_sz > 0);

    ArrayStart* array_start = mem_alloc(sizeof(ArrayStart));
    PoolItem* extra_arr     = mem_alloc(extra_sz * sizeof(PoolItem));

    /* Link the new free items together */
    for (size_t i = 0; i < extra_sz - 1; i++) {
        extra_arr[i].val.next = &extra_arr[i + 1];
        extra_arr[i].flags    = POOL_FLAG_FREE;
    }

    /* Prepend the new item array to the linked list of free items */
    extra_arr[extra_sz - 1].val.next = g_expr_pool->free_items;
    extra_arr[extra_sz - 1].flags    = POOL_FLAG_FREE;
    g_expr_pool->free_items          = extra_arr;

    /* Prepend to the linked list of array starts */
    array_start->arr          = extra_arr;
    array_start->arr_sz       = extra_sz;
    array_start->next         = g_expr_pool->array_starts;
    g_expr_pool->array_starts = array_start;

    VALGRIND_MAKE_MEM_NOACCESS(extra_arr, extra_sz * sizeof(PoolItem));

    return true;
}

void pool_close(void) {
    if (g_expr_pool == NULL)
        return;

    /*
     * First, free the members of all expressions in each array, because one
     * might reference another from a separate array.
     */
    ArrayStart* array_start;
    for (array_start = g_expr_pool->array_starts; array_start != NULL;
         array_start = array_start->next) {
        VALGRIND_MAKE_MEM_DEFINED(array_start->arr,
                                  array_start->arr_sz * sizeof(PoolItem));

        for (size_t i = 0; i < array_start->arr_sz; i++)
            if (!pool_item_is_free(&array_start->arr[i]))
                pool_free(&array_start->arr[i].val.expr);
    }

    /*
     * Then we can actually free the expression arrays, along with the
     * 'ArrayStart' structures themselves.
     */
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
/* Public functions for pool items */

Expr* pool_alloc(void) {
    SL_ASSERT(g_expr_pool != NULL);

    if (g_expr_pool->free_items == NULL)
        return NULL;
    VALGRIND_MAKE_MEM_DEFINED(g_expr_pool->free_items, sizeof(PoolItem*));

    PoolItem* result        = g_expr_pool->free_items;
    g_expr_pool->free_items = g_expr_pool->free_items->val.next;

    SL_ASSERT(pool_item_is_free(result));
    pool_item_flag_unset(result, POOL_FLAG_FREE);

    VALGRIND_MEMPOOL_ALLOC(g_expr_pool, &result->val.expr, sizeof(Expr));
    VALGRIND_MAKE_MEM_NOACCESS(g_expr_pool->free_items, sizeof(PoolItem*));

    return &result->val.expr;
}

Expr* pool_alloc_or_expand(size_t extra_sz) {
    SL_ASSERT(g_expr_pool != NULL);
    if (g_expr_pool->free_items == NULL && !pool_expand(extra_sz))
        return NULL;

    return pool_alloc();
}

void pool_free(Expr* e) {
    SL_ASSERT(g_expr_pool != NULL);
    if (e == NULL)
        return;

    PoolItem* pool_item = pool_item_from_expr(e);

    /*
     * Avoid double-frees.
     */
    SL_ASSERT(!pool_item_is_free(pool_item));
    pool_item_flag_set(pool_item, POOL_FLAG_FREE);

    /*
     * Before freeing the expression we have to free its heap members. They are
     * currently allocated using the functions in 'memory.c', not with a pool.
     * Note that this function doesn't try to free any 'Expr' at all.
     */
    free_heap_expr_members(e);

    pool_item->val.next     = g_expr_pool->free_items;
    g_expr_pool->free_items = pool_item;

    VALGRIND_MEMPOOL_FREE(g_expr_pool, e);
}

/*----------------------------------------------------------------------------*/

void pool_print_stats(FILE* fp) {
    size_t total_free = 0, total_items = 0, total_arrays = 0;

    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next) {
        size_t num_free = 0;
        for (size_t i = 0; i < a->arr_sz; i++)
            if (pool_item_is_free(&a->arr[i]))
                num_free++;

        fprintf(fp,
                "Array %zu: %zu/%zu free.\n",
                total_arrays,
                num_free,
                a->arr_sz);

        total_items += a->arr_sz;
        total_free += num_free;
        total_arrays++;
    }

    fprintf(fp,
            "Total: %zu/%zu free in %zu arrays.\n",
            total_free,
            total_items,
            total_arrays);
}

void pool_dump(FILE* fp) {
    size_t array_count = 0;

    for (ArrayStart* a = g_expr_pool->array_starts; a != NULL; a = a->next) {
        for (size_t i = 0; i < a->arr_sz; i++) {
            const enum EPoolItemFlags flags = pool_item_flags(&a->arr[i]);
            fprintf(fp,
                    "[%p] [%zu,%3zu] [F: %X] ",
                    &a->arr[i],
                    array_count,
                    i,
                    flags);

            if ((flags & POOL_FLAG_FREE) == 0) {
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
}
