
#ifndef EXPR_POOL_H_
#define EXPR_POOL_H_ 1

#include <stddef.h>
#include <stdbool.h>

#include "expr.h"

#if defined(SL_NO_POOL_VALGRIND)
#define VALGRIND_MAKE_MEM_DEFINED(a, b)
#define VALGRIND_MAKE_MEM_NOACCESS(a, b)
#else
#include <valgrind/memcheck.h>
#endif

/*----------------------------------------------------------------------------*/
/* Macros */

/*
 * Base pool size. Arbitrary number used when initializing and expanding the
 * global pool.
 *
 * It doesn't make much sense to declare this macro in this header while also
 * accepting size parameters in some 'pool_*' functions. Still, since this value
 * was used from multiple sources, I think this is the best place.
 */
#define BASE_POOL_SZ 512

/*----------------------------------------------------------------------------*/
/* Enums and structures */

/*
 * Flags stored in each node of the expression pool. These flags are NOT
 * mutually exclusive, so they can be OR'd together.
 *
 * Brief explanation of each flag:
 *
 *   - The `FREE' flag is used to indicate that the current expression is
 *     free. This is not normally needed with pool allocators, since we directly
 *     look in the linked list. In this case, however, we will also iterate the
 *     whole pool when performing garbage collection, and we need to know which
 *     nodes are free without iterating the whole linked list each time
 *   - The `GCMARKED' flag is used to indicate that the current expression
 *     should not be freed by the garbage collector. This flag is set/cleared
 *     exclusively by the garbage collector.
 */
enum EPoolNodeFlags {
    NODE_FLAG_NONE     = 0,
    NODE_FLAG_FREE     = (1 << 0),
    NODE_FLAG_GCMARKED = (1 << 1),
};

/*
 * Each node in the pool will either store a valid expression (if the node is
 * non-free) or a pointer to the next free node (therefore building a linked
 * list). For more information on the advantages of this method, along with a
 * simpler implementation, see my pool allocation article, linked above.
 *
 * We also need a `flags' member to store whether a specific node is free, if it
 * should be garbage-collected, etc.
 */
typedef struct PoolNode {
    union {
        Expr expr;
        struct PoolNode* next;
    } val;

    enum EPoolNodeFlags flags;
} PoolNode;

/*
 * Structure used to store the start of each array inside a pool.
 *
 * We need to store them as a linked list, since there can be an arbitrary
 * number of them, one for each call to `pool_expand' plus the initial one from
 * `pool_new'. New pointers will be prepended to the linked list.
 *
 * We also need to store the size of the current array, since the garbage
 * collector will have to iterate over it.
 */
typedef struct ArrayStart {
    struct ArrayStart* next;
    PoolNode* arr;
    size_t arr_sz;
} ArrayStart;

/*
 * The actual pool structure, which contains a pointer to the first node, and
 * a pointer to the start of the linked list of free nodes.
 *
 * We need to store a list of array starts for freeing the actual `PoolNode'
 * arrays once the user is done with the pool.
 *
 * The user is able to allocate with O(1) time, because the `ExprPool.free_expr'
 * pointer always points to a free node without needing to iterate anything.
 */
typedef struct ExprPool {
    PoolNode* free_node;
    ArrayStart* array_starts;
} ExprPool;

/*----------------------------------------------------------------------------*/
/* Globals */

/*
 * Global expression pool. Declared public so the garbage collector can access
 * it directly.
 */
extern ExprPool* g_expr_pool;

/*----------------------------------------------------------------------------*/
/* Public functions */

/*
 * Wrappers for getting and setting node flags. Useful for setting the memory
 * access with valgrind, since they mark the node as 'DEFINED' temporarily,
 * before setting it back as 'NOACCESS'.
 *
 * NOTE: This means that if the node was previously marked as 'DEFINED', you
 * will need to set it again.
 */
enum EPoolNodeFlags pool_node_flags(PoolNode* node);
void pool_node_flag_set(PoolNode* node, enum EPoolNodeFlags flag);
void pool_node_flag_unset(PoolNode* node, enum EPoolNodeFlags flag);

/*
 * Allocate and initialize the global expression pool with the specified number
 * of expressions. True is returned on success, or false otherwise.
 *
 * The caller is responsible for initializing the pool only once (or an
 * assertion will fail).
 */
bool pool_init(size_t pool_sz);

/*
 * Expand the global expression pool, adding `extra_sz' free expressions.
 */
bool pool_expand(size_t extra_sz);

/*
 * Close the global expression pool, freeing all necessary data. All data in
 * the pool becomes unusable.
 *
 * If the pool is already closed, the function ignores it but does not fail.
 */
void pool_close(void);

/*
 * Retrieve a free expression from the global expression pool.
 *
 * The caller is responsible for ensuring that the pool was previously
 * initialized with `pool_init' (or an assertion will fail).
 */
Expr* pool_alloc(void);

/*
 * Like `pool_get_expr', but if there are no free nodes in the pool, try to
 * expand it by `extra_sz' nodes. If the pool can't be expanded (according to
 * `pool_expand'), NULL is returned.
 */
Expr* pool_alloc_or_expand(size_t extra_sz);

/*
 * Free an expression which was previously allocated from the global expression
 * pool, along with its members. The expression itself is just returned to the
 * pool, but the members are freed externally, so the pointers should not be in
 * use anywhere else.
 *
 * If the expression argument is NULL, the function ignores it but does not
 * fail. If the pool is closed, however, an assertion will fail.
 */
void pool_free(Expr* e);

/*
 * Print stats about the global expression pool to the specified file.
 */
void pool_print_stats(FILE* fp);

/*
 * Dump the contents of the global expression pool to the specified file.
 */
void pool_dump(FILE* fp);

/*----------------------------------------------------------------------------*/
/* Static functions */

/*
 * Return the pool node for the specified expression.
 *
 * We are able to cast an `Expr' pointer to a `PoolNode' one because the
 * expression is stored (inside a union) in the first member of `PoolNode'. This
 * might not always be the case, so it's useful to have this function.
 */
static inline PoolNode* expr2node(Expr* e) {
    return (PoolNode*)e;
}

#endif /* EXPR_POOL_H_ */
