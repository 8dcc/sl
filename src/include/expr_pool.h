
#ifndef EXPR_POOL_H_
#define EXPR_POOL_H_ 1

#include "expr.h"

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
    NODE_NONE     = 0,
    NODE_FREE     = (1 << 0),
    NODE_GCMARKED = (1 << 1),
};

/*
 * Each node in the pool will either store a valid expression (if the node is
 * non-free) or a pointer to the next free node (therefore building a linked
 * list). For more information on the advantages of this method, along with a
 * simpler implementation, see my pool allocation article, linked above.
 *
 * We also need a `flags' member to store, for example, whether specific node is
 * free or should be garbage-collected.
 */
typedef struct PoolNode {
    union {
        Expr expr;
        struct PoolNode* next;
    } val;

    enum EPoolNodeFlags flags;
} PoolNode;

/*
 * Linked list of pointers, used to store the start of the arrays inside a pool.
 *
 * We need to store them as a linked list, since there can be an arbitrary
 * number of them, one for each call to `pool_expand' plus the initial one from
 * `pool_new'. New pointers will be prepended to the linked list.
 */
typedef struct LinkedPtr {
    struct LinkedPtr* next;
    PoolNode* ptr;
} LinkedPtr;

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
    LinkedPtr* array_starts;
} ExprPool;

/*----------------------------------------------------------------------------*/
/* Globals */

/*
 * Global expression pool. Declared public so the garbage collector can access
 * it directly.
 */
extern ExprPool* g_expr_pool;

/*----------------------------------------------------------------------------*/
/* Functions */

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
 * pool.
 *
 * If the expression argument is NULL, the function ignores it but does not
 * fail. If the pool is closed, however, an assertion will fail.
 */
void pool_free(Expr* e);

#endif /* EXPR_POOL_H_ */
