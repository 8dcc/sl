
#ifndef ENV_H_
#define ENV_H_ 1

struct Expr; /* expr.h */

typedef struct Env Env;

/* Linked list of `Env' structures will associate a symbol to its value */
struct Env {
    char* sym;
    struct Expr* val;
    Env* next;
};

/*----------------------------------------------------------------------------*/

/* Bind the expression `val' to the symbol `sym' in environment `env' */
Env* env_bind(Env* env, const char* sym, const struct Expr* val);

/* Get a copy of the expression associated to the symbol `sym' in environment
 * `env'. */
struct Expr* env_get(Env* env, const char* sym);

/* Initialize environment with basic symbols like "nil". Note that C primitives
 * are handled separately, so they are not added to the environment. */
void env_init(Env** env);

/* Free a linked list of Env structures, along with all its allocated memory */
void env_free(Env* env);

/* Print environment in S-expression format */
void env_print(Env* env);

#endif /* ENV_H_ */
