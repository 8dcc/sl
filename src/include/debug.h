
#ifndef DEBUG_H_
#define DEBUG_H_ 1

#include <stdbool.h>
#include <stdio.h> /* FILE */

struct Env;  /* env.h */
struct Expr; /* expr.h */

/*
 * Is the function `e' being traced in the `*debug-trace*' Lisp variable?
 */
bool debug_is_traced_function(const struct Env* env, const struct Expr* e);

/*
 * Print the opening and closing of a function trace.
 *
 * The `func' expression should be the un-evaluated function (usually a symbol);
 * the `args' pointer should be a linked list with the evaluated arguments, or
 * NULL; and the `applied' argument should be a pointer to the evaluated result
 * of the procedure call.
 */
void debug_trace_print_pre(FILE* fp, const struct Expr* func,
                           const struct Expr* args);
void debug_trace_print_post(FILE* fp, const struct Expr* e);

#endif /* DEBUG_H_ */
