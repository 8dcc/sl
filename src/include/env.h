
#ifndef ENV_H_
#define ENV_H_ 1

struct Expr; /* expr.h */

typedef struct Env Env;

/* List of `Env' structures will associate a symbol to its value */
struct Env {
    Env* parent;
    size_t size;
    char** symbols;
    struct Expr** values;
};

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
 * Bind the expression `val' to the symbol `sym' in environment `env'.
 */
void env_bind(Env* env, const char* sym, const struct Expr* val);

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
void env_print(Env* env);

#endif /* ENV_H_ */
