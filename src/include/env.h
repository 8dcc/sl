
#ifndef ENV_H_
#define ENV_H_ 1

#include <stdbool.h>
#include <stdio.h> /* FILE */

struct Expr; /* expr.h */

typedef struct Env Env;
typedef struct EnvBinding EnvBinding;

/*
 * Possible flags for each `EnvBinding' structure. They can be OR'd together.
 */
enum EEnvBindingFlag {
    ENV_FLAG_NONE  = 0x0,
    ENV_FLAG_CONST = 0x1,
};

/*
 * An `EnvBinding' structure is used to bind a symbol to its expression, with
 * some specified flags from the `EEnvBindingFlag' enum.
 */
struct EnvBinding {
    char* sym;
    struct Expr* val;
    enum EEnvBindingFlag flags;
};

/*
 * An environment is simply an array of `EnvBinding' structures, and a parent
 * environment.
 */
struct Env {
    Env* parent;
    size_t size;
    EnvBinding* bindings;
};

/*----------------------------------------------------------------------------*/
/* Global constants */

extern const struct Expr* nil;
extern const struct Expr* tru;

/*----------------------------------------------------------------------------*/

/*
 * Allocate and return an empty `Env' structure without specifying the parent.
 */
Env* env_new(void);

/*
 * Initialize environment with the default symbols like "nil" and primitives.
 */
void env_init_defaults(Env* env);

/*
 * Clone a linked list of Env structures into new allocated memory.
 */
Env* env_clone(Env* env);

/*
 * Free all elements of an Env structure, and the structure itself.
 */
void env_free(Env* env);

/*
 * Bind the symbol `sym' to the expression `val' in environment `env', with the
 * specified `flags'.
 */
bool env_bind(Env* env, const char* sym, const struct Expr* val,
              enum EEnvBindingFlag flags);

/*
 * Bind the symbol `sym' to the expression `val' in the top-most parent of
 * environment `env', with the specified `flags'.
 */
bool env_bind_global(Env* env, const char* sym, const struct Expr* val,
                     enum EEnvBindingFlag flags);

/*
 * Get a copy of the expression associated to the symbol `sym' in environment
 * `env', or in parent environments. The returned copy must be freed by the
 * caller.
 *
 * Returns NULL if the expression is not found.
 */
struct Expr* env_get(const Env* env, const char* sym);

/*
 * Print environment in Lisp list format.
 */
void env_print(FILE* fp, const Env* env);

#endif /* ENV_H_ */
