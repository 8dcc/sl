
#ifndef ENV_H_
#define ENV_H_ 1

#include "expr.h"

/* Linked list of `Env' structures will associate a symbol to its value */
typedef struct Env {
    char* sym;
    Expr* val;
    struct Env* next;
} Env;

/*----------------------------------------------------------------------------*/

/* Associate the expression `val' to the symbol `sym' in environment `env' */
void env_add(Env** env, const char* sym, const Expr* val);

/* Get a copy of the expression associated to the symbol `sym' in environment
 * `env'. */
Expr* env_get(Env* env, const char* sym);

/* Initialize environment with basic symbols like "nil". Note that C primitives
 * are handled separately, so they are not added to the environment. */
void env_init(Env** env);

/* Free a linked list of Env structures, along with all its allocated memory */
void env_free(Env* env);

#endif /* ENV_H_ */
